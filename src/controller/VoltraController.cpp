#include "VoltraController.h"
#include "protocol/VoltraGattProfile.h"
#include "protocol/Parameters.h"
#include <algorithm>
namespace voltra {
VoltraController::VoltraController(BleTransport& t, ControllerPolicy p, ControlSession* controls)
    : transport_(t), policy_(p), controls_(controls) {
    transport_.onConnection([this](bool connected) {
        linkUp_ = connected;
        if (connected) return; // Validate GATT on the application loop first.
        const bool failed = connecting_;
        connecting_ = false;
        if (controls_) controls_->connectionChanged(false);
        confirmed_ = {}; requested_ = {}; parser_.reset();
        confirmed_.connected = connected;
        pendingLoad_.reset();
        feedback_ = failed ? "Bluetooth may be in use. Disconnect other app and retry; check VOLTRA is nearby." : "Disconnected";
    });
    transport_.onNotification([this](const std::string& characteristic, const Bytes& bytes) {
        if (controls_) controls_->notification(characteristic,bytes);
        if (!confirmed_.connected || characteristic != VoltraGattProfile::notify) return;
        for (const auto& frame : parser_.feed(bytes)) {
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
bool VoltraController::connect(const DeviceId& id) {
    disconnect();
    connecting_ = true;feedback_ = "Connecting...";
    if (transport_.connect(id) != TransportResult::Accepted) { connecting_=false;feedback_="Connection unavailable";return false; }
    return linkUp_ ? finishConnection() : true;
}
bool VoltraController::finishConnection() {
    const auto services = transport_.discoverServices();
    if (std::find(services.begin(), services.end(), VoltraGattProfile::service) == services.end()) {
        disconnect(); feedback_="VOLTRA service unavailable";return false;
    }
    const auto chars = transport_.discoverCharacteristics(VoltraGattProfile::service);
    const auto found = std::find_if(chars.begin(), chars.end(), [](const Characteristic& c) {
        return c.uuid == VoltraGattProfile::notify && c.canNotify;
    });
    const auto writable = std::find_if(chars.begin(), chars.end(), [](const Characteristic& c) {
        return c.uuid == VoltraGattProfile::write && c.canWrite;
    });
    if (found == chars.end() || writable == chars.end() || transport_.subscribe(found->uuid) != TransportResult::Accepted) {
        disconnect(); feedback_="VOLTRA subscription failed";return false;
    }
    // The pinned SDK and ESP32 reference receive replies on both channels.
    // This is a profile decision, not a generic subscribe-to-everything policy.
    if (writable->canNotify && transport_.subscribe(writable->uuid) != TransportResult::Accepted) {
        disconnect();feedback_="VOLTRA response subscription failed";return false;
    }
    const auto telemetry=std::find_if(chars.begin(),chars.end(),[](const Characteristic& c) {
        return c.uuid==VoltraGattProfile::telemetry && c.canNotify;
    });
    if(telemetry!=chars.end() && transport_.subscribe(telemetry->uuid)!=TransportResult::Accepted) {
        disconnect();feedback_="VOLTRA telemetry subscription failed";return false;
    }
    // Subscription is not handshake success. The protocol session owns bootstrap
    // and application acceptance; neither step may replay a load/settings intent.
    connecting_=false;confirmed_.connected=true;
    if (controls_) controls_->connectionChanged(true);
    feedback_="Connected / reading VOLTRA";
    return true;
}
void VoltraController::disconnect() {
    connecting_=false;linkUp_=false;
    if (controls_) controls_->connectionChanged(false);
    transport_.disconnect(); confirmed_ = {}; requested_ = {}; parser_.reset();
    pendingLoad_.reset();
}
void VoltraController::tick(uint64_t now) {
    now_ = now;
    transport_.poll(now);
    // Discovery can block: bootstrap must start on a new loop with a fresh clock.
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
