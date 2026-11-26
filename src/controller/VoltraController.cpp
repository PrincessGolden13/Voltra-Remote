#include "VoltraController.h"
#include "protocol/VoltraGattProfile.h"
#include "protocol/Parameters.h"
#include "diagnostics/EventLog.h"
#include <algorithm>
namespace voltra {
namespace {
std::string connectionFailureText(BleConnectionFailure failure,const char* fallback) {
    if(failure.stage==BleConnectionStage::Teardown && failure.timedOut)
        return "Bluetooth cleanup timed out. Restart remote and try again";
    const char* stage=nullptr;
    switch(failure.stage) {
    case BleConnectionStage::Setup:stage="setup";break;
    case BleConnectionStage::Teardown:stage="previous connection cleanup";break;
    case BleConnectionStage::Radio:stage="radio connection";break;
    case BleConnectionStage::Services:stage="service discovery";break;
    case BleConnectionStage::Characteristics:stage="characteristic discovery";break;
    case BleConnectionStage::Subscription:stage="notification subscription";break;
    case BleConnectionStage::Link:stage="link";break;
    case BleConnectionStage::Notifications:stage="notification queue";break;
    default:return fallback;
    }
    return std::string("Bluetooth ")+stage+(failure.timedOut ? " timed out" : " failed")+
        (failure.reason ? " (code "+std::to_string(failure.reason)+")" : "");
}
}
VoltraController::VoltraController(BleTransport& t, ControllerPolicy p, ControlSession* controls)
    : transport_(t), policy_(p), controls_(controls) {
    transport_.onConnection([this](bool connected) {
        linkUp_ = connected;
        if (connected) return; // Validate GATT on the application loop first.
        const bool failed = connecting_;
        connecting_ = false;
        if(!disconnecting_ && confirmed_.connected && controls_)
            endedSessionSnapshot_=controls_->diagnosticState().substr(0,800);
        if (controls_) controls_->connectionChanged(false);
        confirmed_ = {}; requested_ = {}; responseSubscribed_={};
        for(auto& parser:responseParsers_) parser.reset();
        confirmed_.connected = connected;
        pendingLoad_.reset();
        feedback_ = disconnecting_ ? "Disconnected" : connectionFailureText(transport_.connectionFailure(),
            failed ? "Bluetooth connection failed; try again" : "Disconnected");
    });
    transport_.onNotification([this](const std::string& characteristic, const Bytes& bytes) {
        if (controls_) controls_->notification(characteristic,bytes);
        const auto response=std::find(VoltraGattProfile::responses.begin(),VoltraGattProfile::responses.end(),characteristic);
        if(!confirmed_.connected || response==VoltraGattProfile::responses.end()) return;
        const auto channel=size_t(response-VoltraGattProfile::responses.begin());
        if(!responseSubscribed_[channel]) return;
        for (const auto& frame : responseParsers_[channel].feed(bytes)) {
            if (!applyAsyncParameters(frame, now_, confirmed_) || now_ <= sentAt_) continue;
            if (requested_.status == CommandStatus::Pending && confirmed_.weightLbs.receivedAtMs == now_ &&
                confirmed_.weightLbs.value == requested_.weightLbs) {
                requested_.status = CommandStatus::Confirmed;
                feedback_ = "Weight confirmed - tap weight to load";
            }
            if (pendingLoad_ && confirmed_.fitnessModeRaw.receivedAtMs == now_ &&
                loadState() == (*pendingLoad_ ? LoadState::Loaded : LoadState::Unloaded)) {
                feedback_ = *pendingLoad_ ? "" : "Turn wheel to adjust";
                pendingLoad_.reset();
            }
        }
    });
}
VoltraController::~VoltraController() {
    transport_.onConnection({}); transport_.onNotification({});
}
bool VoltraController::connect(const DeviceId& id, BleConnectionPolicy policy) {
    disconnect();
    endedSessionSnapshot_.clear();
    connecting_ = true;feedback_ = "Connecting...";
    if (transport_.connect(id,policy) != TransportResult::Accepted) { connecting_=false;feedback_=connectionFailureText(transport_.connectionFailure(),"Connection unavailable");return false; }
    return linkUp_ ? finishConnection() : true;
}
bool VoltraController::finishConnection() {
    if (!transport_.discoverService(VoltraGattProfile::service)) {
        return failConnection("VOLTRA service unavailable");
    }
    const auto writable=transport_.discoverCharacteristic(VoltraGattProfile::service,VoltraGattProfile::write);
    if(!writable || !writable->canWrite) return failConnection("VOLTRA write channel unavailable");
    if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
    std::array<const char*,VoltraGattProfile::responses.size()> responseStatus{};
    bool responseAvailable=false,responseReady=false;
    for(size_t i=0;i<VoltraGattProfile::responses.size();++i) {
        const auto* uuid=VoltraGattProfile::responses[i];
        const auto channel=std::string(uuid)==VoltraGattProfile::write ? writable :
            transport_.discoverCharacteristic(VoltraGattProfile::service,uuid);
        if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
        if(!channel) {responseStatus[i]="absent";continue;}
        if(!channel->canNotify) {responseStatus[i]="not-notifiable";continue;}
        responseAvailable=true;
        responseSubscribed_[i]=transport_.subscribe(channel->uuid)==TransportResult::Accepted;
        responseStatus[i]=responseSubscribed_[i] ? "subscribed" : "failed";
        if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
        responseReady=responseReady || responseSubscribed_[i];
    }
    diagnostic(std::string("VOLTRA response routes command=")+responseStatus[0]+" transport="+responseStatus[1]);
    if(!responseReady) return failConnection(responseAvailable ? "VOLTRA response subscription failed" : "VOLTRA response channel unavailable");
    // Official/Android captures subscribe response channels before application
    // frames. Send hello immediately after that quorum, before optional stream
    // setup; the protocol hook cannot accept the session or send a control.
    if(controls_) {
        if(!controls_->prepareConnection()) return failConnection("VOLTRA hello failed");
        diagnostic("VOLTRA hello prepared after response subscription");
    }
    const auto telemetry=transport_.discoverCharacteristic(VoltraGattProfile::service,VoltraGattProfile::telemetry);
    if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
    if(telemetry && telemetry->canNotify) {
        const bool subscribed=transport_.subscribe(telemetry->uuid)==TransportResult::Accepted;
        if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
        // A failed optional stream cannot invalidate a working response route.
        // It still cannot stand in for authentication or fresh machine state.
        if(!subscribed) diagnostic("VOLTRA optional telemetry subscription unavailable");
    }
    transport_.clearConnectionFailure();
    if(!transport_.linkAlive()) return failConnection("Bluetooth connection ended");
    // Subscription is not handshake success. The protocol session owns bootstrap
    // and application acceptance; neither step may replay a load/settings intent.
    connecting_=false;confirmed_.connected=true;
    if (controls_) controls_->connectionChanged(true);
    feedback_="Connected / reading VOLTRA";
    return true;
}
bool VoltraController::failConnection(const char* fallback) {
    const auto failure=connectionFailureText(transport_.connectionFailure(),fallback);
    disconnect();feedback_=failure;return false;
}
void VoltraController::disconnect() {
    disconnecting_=true;
    connecting_=false;linkUp_=false;
    if (controls_) controls_->connectionChanged(false);
    transport_.disconnect(); confirmed_ = {}; requested_ = {}; responseSubscribed_={};
    for(auto& parser:responseParsers_) parser.reset();
    pendingLoad_.reset();
    disconnecting_=false;
}
void VoltraController::tick(uint64_t now) {
    now_ = now;
    transport_.poll(now);
    // Discovery can block: continue bootstrap timing on a fresh loop after the
    // initial hello, rather than backdating deadlines to the discovery tick.
    if (connecting_ && linkUp_) {finishConnection();return;}
    if (controls_) controls_->tick(now);
    if (controls_ && confirmed_.connected) {
        const auto evidence = controls_->readiness();
        confirmed_.capabilities = evidence.capabilities;
        confirmed_.handshakeValid = evidence.validated;
    }
    if (controlPending() && now_ - sentAt_ > commandTimeoutMs) {
        if (requested_.status == CommandStatus::Pending) requested_.status = CommandStatus::Timeout;
        pendingLoad_.reset();
        feedback_ = "No confirmation - request timed out";
    }
}
bool VoltraController::weightFresh() const {
    return confirmed_.connected && confirmed_.weightLbs.fresh(now_, policy_.staleAfterMs);
}
LoadState VoltraController::loadState() const {
    if (!confirmed_.connected || !confirmed_.fitnessModeRaw.fresh(now_, policy_.staleAfterMs) ||
        !confirmed_.workoutStateRaw.fresh(now_, policy_.staleAfterMs) ||
        confirmed_.workoutStateRaw.value != 1) return LoadState::Unknown;
    if (confirmed_.fitnessModeRaw.value == 4) return LoadState::Unloaded;
    if (confirmed_.fitnessModeRaw.value == 5) return LoadState::Loaded;
    return LoadState::Unknown;
}
void VoltraController::adjustDraft(int delta) {
    // No known maximum/capability -> do not offer adjustment. No guessed default.
    const auto& cap = confirmed_.capabilities;
    if (!controls_ || !weightFresh() || cap.weightTraining != Knowledge::Yes || !cap.effectiveMaxWeightLbs) {
        report("Controls unavailable - state not ready"); return;
    }
    if (loadState() != LoadState::Unloaded || pendingLoad_) {
        report("Unload before adjusting weight"); return;
    }
    const auto current = requested_.weightLbs.value_or(*confirmed_.weightLbs.value);
    const auto value = std::clamp<int64_t>(int64_t(current) + delta,
        controls_->readiness().minWeightLbs, *cap.effectiveMaxWeightLbs);
    requested_.weightLbs = uint16_t(value);
    requested_.status = CommandStatus::Draft;
    submitDraft();
}
const char* VoltraController::requestLoad() {
    if (!controls_) return report("Physical loading is not enabled");
    const auto evidence = controls_->readiness();
    if (!weightFresh() || loadState() != LoadState::Unloaded || !evidence.validated ||
        !evidence.ready || !evidence.cableZero || !evidence.twinSafe ||
        evidence.capabilities.weightTraining != Knowledge::Yes ||
        !evidence.capabilities.effectiveMaxWeightLbs ||
        *confirmed_.weightLbs.value > *evidence.capabilities.effectiveMaxWeightLbs ||
        *confirmed_.weightLbs.value < evidence.minWeightLbs || controlPending() ||
        requested_.status == CommandStatus::Timeout || requested_.status == CommandStatus::Failed)
        return report("Cannot load - check state or pending request");
    if (controls_->setLoaded(true) != TransportResult::Accepted) return report("Load request rejected");
    pendingLoad_ = true; sentAt_ = now_;
    return report("Loading requested - awaiting confirmation");
}
const char* VoltraController::requestUnload() {
    if (!controls_ || !confirmed_.connected) return report("No connection - cannot unload");
    if (controls_->setLoaded(false) != TransportResult::Accepted) return report("Unload request could not be sent");
    requested_ = {}; pendingLoad_ = false; sentAt_ = now_;
    return report("Unloading - awaiting confirmation");
}
const char* VoltraController::submitDraft() {
    if (!controls_ || !requested_.weightLbs || !weightFresh() || loadState() != LoadState::Unloaded || pendingLoad_)
        return report("Parameter change unavailable");
    if (controls_->setWeight(*requested_.weightLbs) != TransportResult::Accepted) {
        requested_.status = CommandStatus::Failed; return report("Weight request rejected");
    }
    requested_.status = CommandStatus::Pending; sentAt_ = now_;
    return report("Weight requested - awaiting confirmation");
}
}
