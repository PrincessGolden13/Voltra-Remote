#include "MockTransport.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
#include <cmath>
namespace voltra {
MockTransport::MockTransport() {
    const char* names[] = {"Green 2.0", "Yellow TW 2.0", "Pink TW 2.0", "Red 2.0 KG", "Black 1.x", "White OFF"};
    for (int i = 0; i < 6; ++i) {
        TrainingReport r;
        r.unit = i == 3 ? WeightUnit::Kilograms : WeightUnit::Pounds;
        r.firmwareVersion = i == 4 ? "1.8" : "2.0"; // Synthetic firmware identities.
        r.percentageSupported = r.modifierUnitsSelectable = i != 4;
        r.settings.magic.percent = r.settings.eccentric.percent = i != 4;
        r.settings.weight = i == 3 ? 46 : 50;
        r.magicSupported = {true,true,i != 4};
        r.eccentricSupported = r.autoLoadSupported = r.liveWeightSupported = true;
        r.liveModifierAmountsSupported = true;
        r.twinKnown = true; r.twin = i == 1 || i == 2;
        r.twinDetected=r.twin;r.twinStatusFresh=true;r.combinedWeightKnown=r.twin;
        r.twinLinkState=r.twin ? 0x12 : 0x00;
        // Explicit synthetic combined floor, not a physical Twin inference.
        r.minWeight = r.twin ? 10 : voltraISingleMinimum(r.unit);
        r.weightIncrement=r.twin && r.unit==WeightUnit::Pounds ? 2 : 1;
        r.twinPeerName = i == 1 ? names[2] : i == 2 ? names[1] : "";
        if(r.twin) {
            r.twinPeerId=std::string(deviceId)+'-'+std::to_string(i==1 ? 2 : 1);
            r.twinPeerBattery=75; // Explicit synthetic peer observation.
        }
        r.telemetryValid = true; r.repsStarted = false;
        r.settings.magic.value = i == 4 ? 10 : 50;
        r.settings.eccentric.value = i == 4 ? 5 : 25;
        WeightLimitEvidence limits;
        limits.model = "VOLTRA I"; limits.firmware = r.firmwareVersion;
        limits.twinKnown = true; limits.twin = r.twin;
        limits.combinedSemanticsKnown = r.twin;
        if (r.twin) {
            limits.peerFirmware = r.firmwareVersion;
            // User's October 24 Overdrive scenario; explicit synthetic reports,
            // not a general firmware fallback or a physical Twin routing claim.
            limits.single = {250, 226}; // 250 LB / 113 KG per endpoint.
            limits.combinedTwin = {500, 452}; // 500 LB / 226 KG effective pair.
        } else if (i == 0) limits.single = {230, 208};
        // Red and Black deliberately omit a max report and use the shared fallback.
        r.maxWeight = resolveWeightLimit(limits, r.unit).ticks;
        const auto role=i==1 ? CurrentTwinRole::Master : i==2 ? CurrentTwinRole::Follower : CurrentTwinRole::Unknown;
        devices_.push_back({i == 0 ? deviceId : std::string(deviceId)+'-'+std::to_string(i),names[i],i != 5,r,limits,role});
    }
}
void MockTransport::previewModifier(int value) {
    auto& r=devices_[active_].state;
    r.settings.magic.value=std::clamp(value,0,modifierMaximum(r,r.settings,true));
    r.settings.eccentric.value=std::clamp(value,0,modifierMaximum(r,r.settings,false));
    sent_=false;
}
void MockTransport::previewUnit(WeightUnit unit) {
    auto& device = devices_[active_]; auto& r = device.state;
    r.settings.weight = convertWeightUnits(r.settings.weight, r.unit, unit);
    for (auto* m : {&r.settings.magic, &r.settings.eccentric})
        if (!m->percent) m->value = convertWeightUnits(m->value, r.unit, unit);
    r.unit = unit; r.maxWeight = resolveWeightLimit(device.limits, unit).ticks;
    r.minWeight = r.twin ? (unit==WeightUnit::Pounds ? 10 : 8) : voltraISingleMinimum(unit);
    r.weightIncrement=r.twin && unit==WeightUnit::Pounds ? 2 : 1;
    r.settings.weight=weightTicksNearest(r,r.settings.weight);
    for(auto* m:{&r.settings.magic,&r.settings.eccentric})
        if(!m->percent) m->value=weightTicksNearest(r,m->value);
    sent_ = false;
}
void MockTransport::previewWeight(int ticks) {
    auto& r = devices_[active_].state;
    r.settings.weight = std::clamp(ticks,0,r.maxWeight);
    sent_ = false;
}
void MockTransport::previewAutoUnload(int seconds) {
    devices_[active_].state.autoUnloadHoldSeconds=seconds;
    sent_=false;
}
void MockTransport::previewLiveWeight(std::optional<int> pounds,uint64_t ageMs) {
    liveWeightPreview_=true;
    previewLiveWeightLbs_=pounds && *pounds>=0 && *pounds<=32767 ? pounds : std::optional<int>{};
    previewLiveWeightAgeMs_=std::min<uint64_t>(ageMs,2000);
    sent_=false;
}
std::vector<DeviceEntry> MockTransport::catalog() const {
    std::vector<DeviceEntry> result;
    for(size_t i=0;i<devices_.size();++i) {
        const auto& d=devices_[i];
        const bool paired=d.state.twin;
        const bool peerAvailable=paired && devices_[i==1 ? 2 : 1].online;
        result.push_back({d.id,d.name,false,paired ? d.state.twinPeerName : "",
            paired ? d.state.twinPeerId : DeviceId{},paired,peerAvailable});
    }
    return result;
}

std::vector<Advertisement> MockTransport::scan() {
    std::vector<Advertisement> result;
    for (const auto& d : devices_) if (d.online) result.push_back({d.id, d.name, {VoltraGattProfile::service}});
    return result;
}
TransportResult MockTransport::connect(const DeviceId& id) {
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == id && d.online; });
    if (it == devices_.end()) return TransportResult::Failed;
    active_ = size_t(it-devices_.begin()); connected_ = true; sent_ = subscribed_ = false;
    restSince_ = 0;
    liveWeightPreview_=false;previewLiveWeightLbs_.reset();previewLiveWeightAgeMs_=0;
    reported_ = {}; pendingWeight_.reset(); pendingLoaded_.reset(); pendingSettings_.reset(); pendingAuto_ = false;
    if (connection_) connection_(true);
    return TransportResult::Accepted;
}
void MockTransport::disconnect() {
    connected_ = subscribed_ = sent_ = false;
    pendingWeight_.reset(); pendingLoaded_.reset(); pendingSettings_.reset(); pendingAuto_ = false;
    reported_ = {}; heldSince_ = 0;
    liveWeightPreview_=false;previewLiveWeightLbs_.reset();previewLiveWeightAgeMs_=0;
    if (connection_) connection_(false);
}
std::vector<std::string> MockTransport::discoverServices() {
    return connected_ ? std::vector<std::string>{VoltraGattProfile::service} : std::vector<std::string>{};
}
std::vector<Characteristic> MockTransport::discoverCharacteristics(const std::string& service) {
    if (!connected_ || service != VoltraGattProfile::service) return {};
    return {{VoltraGattProfile::notify, true, false}, {VoltraGattProfile::write, false, true}};
}
TransportResult MockTransport::subscribe(const std::string& uuid) {
    if (!connected_) return TransportResult::Disconnected;
    if (uuid != VoltraGattProfile::notify) return TransportResult::Unsupported;
    subscribed_ = true; return TransportResult::Accepted;
}
void MockTransport::setProfile(SimulatorProfile profile) {
    profile_ = profile; sent_ = false; pendingSettings_.reset(); pendingLoaded_.reset(); pendingAuto_ = false;
    auto& r = devices_[active_].state;
    r.loaded = profile == SimulatorProfile::Loaded; r.autoLoadArmed = false;
    r.autoLoadPhase.reset();r.autoLoadCountdownMs.reset();r.autoLoadCountdownAtMs=0;
    r.twinKnown = profile != SimulatorProfile::TwinUnknown;
    r.twinDetected=r.twin || profile==SimulatorProfile::TwinUnknown;
    r.twinStatusFresh=r.twinKnown;r.combinedWeightKnown=r.twin && r.twinKnown;
    r.twinLinkState=r.twinKnown ? std::optional<int>(r.twin ? 0x12 : 0x00) : std::optional<int>{};
    r.currentTwinRole=CurrentTwinRole::Unknown;r.twinRoleAtMs=0;
    liveWeightPreview_=false;previewLiveWeightLbs_.reset();previewLiveWeightAgeMs_=0;
    r.moving = r.held = false; r.rep = 0; r.repsStarted = false; restSince_ = 0;
    r.force.reset(); r.setNumber.reset(); r.completedSets.reset(); r.lastSetReps.reset(); r.restSeconds.reset();
}
bool MockTransport::setReadOnlyProbe(bool enabled) {
    if(enabled==readOnlyProbe_) return true;
    if(!enabled && connected_) {
        const auto& r=reported_;
        const bool freshReport=r.revision && nowMs_>=r.receivedAtMs && nowMs_-r.receivedAtMs<=2000;
        const bool roleFresh=r.currentTwinRole==CurrentTwinRole::Master && nowMs_>=r.twinRoleAtMs &&
            nowMs_-r.twinRoleAtMs<=2000;
        const bool topologyFresh=r.twinStatusFresh && nowMs_>=r.twinAtMs && nowMs_-r.twinAtMs<=2000;
        if(!freshReport || !r.twinKnown || !currentTwinControlReady() ||
           (r.twin && (!topologyFresh || !roleFresh))) return false;
    }
    pendingWeight_.reset();pendingSettings_.reset();pendingLoaded_.reset();pendingAuto_=false;
    readOnlyProbe_=enabled;
    return true;
}
bool MockTransport::currentTwinControlReady() const {
    const auto& device=devices_[active_];const auto& state=device.state;
    // Preserve the ordinary standalone unload path even when its periodic
    // report is stale. Detected/unknown pairs never inherit that route.
    if(!state.twin && !state.twinDetected && profile_!=SimulatorProfile::TwinUnknown) return true;
    if((active_!=1 && active_!=2) || !state.twin || !state.twinKnown || !state.twinStatusFresh ||
       state.twinLinkState!=0x12 || device.role!=CurrentTwinRole::Master ||
       state.currentTwinRole!=CurrentTwinRole::Master || !state.twinRoleAtMs ||
       nowMs_<state.twinRoleAtMs || nowMs_-state.twinRoleAtMs>2000 ||
       nowMs_<state.twinAtMs || nowMs_-state.twinAtMs>2000) return false;
    const auto& peer=devices_[active_==1 ? 2 : 1];
    return state.twinPeerId==peer.id && peer.state.twinPeerId==device.id &&
        peer.state.twin && peer.role==CurrentTwinRole::Follower;
}
bool MockTransport::retainControlRoute() {
    if(readOnlyProbe_) return false;
    if(currentTwinControlReady()) return true;
    // The backend rechecks before every commit. An application's earlier
    // snapshot cannot authorize a write after this poll observes a role loss.
    pendingWeight_.reset();pendingSettings_.reset();pendingLoaded_.reset();pendingAuto_=false;
    readOnlyProbe_=true;
    return false;
}
void MockTransport::poll(uint64_t now) {
    nowMs_ = now;
    if (!connected_ || !subscribed_ || !notification_) return;
    if (!devices_[active_].online) { disconnect(); return; }
    auto& state = devices_[active_].state;
    if(!readOnlyProbe_) retainControlRoute();
    if (!readOnlyProbe_ && now >= dueMs_ && profile_ != SimulatorProfile::Stale) {
        if (pendingSettings_) { state.settings = *pendingSettings_; pendingSettings_.reset(); sent_ = false; }
        if (pendingLoaded_) {
            if (*pendingLoaded_ && !state.loaded) {
                state.setNumber = state.completedSets.value_or(0)+1; state.rep = 0; state.repsStarted = false;
                state.completedRep=0;state.repSet=state.setNumber;state.repRevision=0;state.repAtMs=0;
                state.restSeconds.reset(); restSince_ = 0;
            } else if (!*pendingLoaded_ && state.loaded && state.repsStarted.value_or(false)) {
                state.completedSets = state.setNumber.value_or(1); state.lastSetReps = int(state.rep);
                restSince_ = now; state.restSeconds = 0;
            }
            state.loaded = *pendingLoaded_; state.autoLoadArmed = false;
            state.autoLoadPhase.reset();state.autoLoadCountdownMs.reset();state.autoLoadCountdownAtMs=0;
            if (!state.loaded) { state.moving = state.held = false; state.repsStarted = false; }
            pendingLoaded_.reset(); sent_ = false;
        }
        if (pendingAuto_) { state.autoLoadArmed = true;state.autoLoadPhase=11; pendingAuto_ = false; sent_ = false; heldSince_ = 0; }
        // Synthetic device behavior, not a remote timer that issues a load.
        if (state.autoLoadArmed && state.held) {
            if (!heldSince_) heldSince_ = now;
            state.autoLoadPhase=12;state.autoLoadCountdownMs=int(3000-std::min<uint64_t>(3000,now-heldSince_));
            state.autoLoadCountdownAtMs=now;
            if (now-heldSince_ >= 3000) {
                state.loaded = true; state.autoLoadArmed = false; sent_ = false;
                state.autoLoadPhase=14;state.autoLoadCountdownMs.reset();state.autoLoadCountdownAtMs=0;
                state.setNumber = state.completedSets.value_or(0)+1; state.rep = 0; state.repsStarted = false;
                state.completedRep=0;state.repSet=state.setNumber;state.repRevision=0;state.repAtMs=0;
                state.restSeconds.reset(); restSince_ = 0;
            }
        } else {
            heldSince_ = 0;state.autoLoadCountdownMs.reset();state.autoLoadCountdownAtMs=0;
            if(state.autoLoadArmed) state.autoLoadPhase=11;
        }
    }
    if (sent_ && (profile_ == SimulatorProfile::Stale || now-lastMs_ < notificationIntervalMs)) return;
    if (sent_ && (pendingSettings_ || pendingLoaded_ || pendingAuto_)) return;
    lastMs_ = now; sent_ = true;
    if (state.loaded) { state.force = state.settings.weight; if (!state.setNumber) state.setNumber = 1; }
    else { state.force.reset(); if (restSince_) state.restSeconds = int((now-restSince_)/1000); }
    state.readbackKnown=state.loadStateKnown=true;state.readbackAtMs=now;
    state.twinDetected=state.twin || profile_==SimulatorProfile::TwinUnknown;
    state.twinStatusFresh=state.twinKnown;state.combinedWeightKnown=state.twin && state.twinKnown;
    state.twinAtMs=now;
    state.currentTwinRole=state.twin && state.twinKnown ? devices_[active_].role : CurrentTwinRole::Unknown;
    state.twinRoleAtMs=state.currentTwinRole!=CurrentTwinRole::Unknown ? now : 0;
    // Synthetic device-side report only. The UI must use this independently of
    // the selected base; explicit previews can differ, disappear or be stale.
    if(liveWeightPreview_) {
        state.liveWeightLbs=now>=previewLiveWeightAgeMs_ ? previewLiveWeightLbs_ : std::optional<int>{};
        state.liveWeightAtMs=now>=previewLiveWeightAgeMs_ ? now-previewLiveWeightAgeMs_ : 0;
    } else {
        state.liveWeightLbs=state.force ? convertWeightUnits(*state.force,state.unit,WeightUnit::Pounds) : 0;
        state.liveWeightAtMs=now;
    }
    // Synthetic target report follows the explicitly injected preview (or the
    // mock's configured target). The physical backend never computes this field.
    state.liveTargetTenthsLb=state.liveWeightLbs ? std::optional<int>(*state.liveWeightLbs*10) : std::optional<int>{};
    state.liveTargetAtMs=state.liveWeightAtMs;
    // Already-paired synthetic endpoints share a workout through one selected
    // session. This is not a second BLE connection or a duplicate command.
    if(!readOnlyProbe_ && (active_ == 1 || active_ == 2)) {
        auto& peer = devices_[active_ == 1 ? 2 : 1].state;
        const auto peerName = peer.twinPeerName;
        const auto peerId = peer.twinPeerId;
        peer = state; peer.twinPeerName = peerName;peer.twinPeerId=peerId;
        peer.currentTwinRole=state.twinKnown ? devices_[active_==1 ? 2 : 1].role : CurrentTwinRole::Unknown;
        peer.twinRoleAtMs=peer.currentTwinRole!=CurrentTwinRole::Unknown ? now : 0;
    }
    state.motionAtMs=now;++state.motionRevision;
    state.repActive=state.loaded && (state.moving || state.held);
    state.receivedAtMs = now; ++state.revision; reported_ = state;
    device_.weightLbs = uint16_t(std::lround(state.settings.weight / double(ticksPerUnit(state.unit)) * (state.unit == WeightUnit::Kilograms ? 2.2046226218 : 1.0)));
    device_.loaded = state.loaded;
    const auto bytes = device_.notification(profile_);
    constexpr size_t fragmentBytes = 7;
    for (size_t start = 0; start < bytes.size(); start += fragmentBytes) {
        const auto end = std::min(bytes.size(), start+fragmentBytes);
        notification_(VoltraGattProfile::notify, Bytes(bytes.begin()+start, bytes.begin()+end));
    }
}
SessionReadiness MockTransport::readiness() const {
    SessionReadiness r; const auto& state = devices_[active_].state;
    r.validated = connected_ && subscribed_ && sent_;
    r.ready = !readOnlyProbe_ && currentTwinControlReady() && r.validated && profile_ != SimulatorProfile::Stale && !state.loaded && !state.autoLoadArmed;
    r.cableZero = r.ready; r.twinSafe = state.twinKnown && currentTwinControlReady();
    if (r.validated && r.twinSafe) {
        r.capabilities.weightTraining = Knowledge::Yes;
        r.capabilities.effectiveMaxWeightLbs = uint16_t(resolveWeightLimit(devices_[active_].limits,WeightUnit::Pounds).ticks);
        r.minWeightLbs = state.twin ? 10 : 5;
    }
    return r;
}
bool MockTransport::consumeIgnored() { const bool ignored = ignoreNext_; ignoreNext_ = false; return ignored; }
TransportResult MockTransport::setTraining(const TrainingSettings& s) {
    if(!retainControlRoute()) return TransportResult::Unsupported;
    const auto r = trainingReport();
    if(r.loaded && (s.magicMode!=r.settings.magicMode || s.magic.percent!=r.settings.magic.percent || s.eccentric.percent!=r.settings.eccentric.percent))
        return TransportResult::Unsupported;
    if (!readiness().validated || !modifierUnitsAllowed(r,s) || !r.twinKnown || pendingLoaded_ || pendingAuto_ || r.autoLoadArmed ||
        r.minWeight < 1 || s.weight < r.minWeight || !weightOnIncrement(r,s.weight) || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && !s.magic.percent && !weightOnIncrement(r,s.magic.value)) ||
        (s.eccentric.enabled && !s.eccentric.percent && !weightOnIncrement(r,s.eccentric.value)) ||
        (s.magic.enabled && (s.magic.value < 1 || !r.magicSupported[int(s.magicMode)] || s.magic.value > modifierMaximum(r, s, true))) ||
        (s.eccentric.enabled && (s.eccentric.value < 1 || !r.eccentricSupported || s.eccentric.value > modifierMaximum(r, s, false))) ||
        envelopeHundredths(s) > r.maxWeight*100) return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    // New knob values replace the pending setting, not its delivery deadline.
    // Otherwise a continuous turn can starve reports and look like a disconnect.
    if (!pendingSettings_) dueMs_ = nowMs_ + confirmationDelayMs;
    pendingSettings_ = s; return TransportResult::Accepted;
}
TransportResult MockTransport::setWeight(uint16_t pounds) {
    const auto r = trainingReport(); auto s = r.settings;
    s.weight = int(std::lround(pounds * ticksPerUnit(r.unit) / (r.unit == WeightUnit::Kilograms ? 2.2046226218 : 1.0)));
    return setTraining(s);
}
TransportResult MockTransport::setLiveWeight(int ticks) {
    if(!retainControlRoute()) return TransportResult::Unsupported;
    const auto r=trainingReport();
    if(!r.loaded || pendingSettings_ || pendingLoaded_ || pendingAuto_) return TransportResult::Unsupported;
    auto s=r.settings;s.weight=ticks;return setTraining(s);
}
TransportResult MockTransport::setLoaded(bool loaded) {
    if(!retainControlRoute()) return TransportResult::Unsupported;
    if (!connected_) return TransportResult::Disconnected;
    const auto r = readiness();
    if (loaded && (!r.ready || !r.cableZero || !r.twinSafe || pendingSettings_ || pendingLoaded_ ||
        trainingReport().minWeight < 1 || trainingReport().settings.weight < trainingReport().minWeight))
        return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    pendingSettings_.reset(); pendingAuto_ = false; pendingLoaded_ = loaded; dueMs_ = nowMs_ + confirmationDelayMs;
    return TransportResult::Accepted;
}
TransportResult MockTransport::triggerAutoLoad() {
    if(!retainControlRoute()) return TransportResult::Unsupported;
    if (!readiness().ready || !readiness().twinSafe || !trainingReport().autoLoadSupported ||
        pendingSettings_ || pendingLoaded_ || trainingReport().minWeight < 1 ||
        trainingReport().settings.weight < trainingReport().minWeight) return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    pendingAuto_ = true; dueMs_ = nowMs_ + confirmationDelayMs; return TransportResult::Accepted;
}
void MockTransport::toggleAvailability() { devices_[5].online = !devices_[5].online; }
void MockTransport::completeRep() {
    auto& r = devices_[active_].state;
    if (!connected_ || !r.loaded) return;
    ++r.rep; r.repsStarted = true; r.moving = true; r.held = false; sent_ = false;
    r.completedRep=r.rep;r.repSet=r.setNumber;r.repAtMs=nowMs_;++r.repRevision;
}
void MockTransport::setHeld(bool held) {
    auto& r = devices_[active_].state;
    r.held = held; r.moving = !held; sent_ = false;
}
}
