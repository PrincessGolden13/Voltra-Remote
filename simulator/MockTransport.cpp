#include "MockTransport.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
#include <cmath>
namespace voltra {
MockTransport::MockTransport() {
    const char* names[] = {"Green 2.0", "Yellow TW 2.0", "Pink TW 2.0", "Red 2.0 KG", "Black 1.x", "White OFF"};
    for (int i = 0; i < 6; ++i) {
        TrainingReport r;
        r.maxWeight = i == 3 ? 104 : i == 4 ? 200 : 230;
        r.unit = i == 3 ? WeightUnit::Kilograms : WeightUnit::Pounds;
        r.firmwareVersion = i == 4 ? "1.x" : "2.0";
        r.percentageSupported = r.modifierUnitsSelectable = i != 4;
        r.settings.magic.percent = r.settings.eccentric.percent = i != 4;
        r.settings.weight = i == 3 ? 23 : 50;
        r.magicSupported = {true,true,i != 4};
        r.eccentricSupported = r.autoLoadSupported = r.liveWeightSupported = true;
        r.twinKnown = true; r.twin = i == 1 || i == 2;
        r.twinPeerName = i == 1 ? names[2] : i == 2 ? names[1] : "";
        r.telemetryValid = true; r.repsStarted = false;
        r.settings.magic.value = i == 4 ? 10 : 50;
        r.settings.eccentric.value = i == 4 ? 5 : 25;
        devices_.push_back({i == 0 ? deviceId : std::string(deviceId)+'-'+std::to_string(i),names[i],i != 5,r});
    }
}
void MockTransport::previewModifier(int value) {
    auto& r=devices_[active_].state;
    r.settings.magic.value=std::clamp(value,0,modifierMaximum(r,r.settings,true));
    r.settings.eccentric.value=std::clamp(value,0,modifierMaximum(r,r.settings,false));
    sent_=false;
}
std::vector<DeviceEntry> MockTransport::catalog() const {
    std::vector<DeviceEntry> result;
    for(const auto& d : devices_) result.push_back({d.id,d.name,false});
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
    reported_ = {}; pendingWeight_.reset(); pendingLoaded_.reset(); pendingSettings_.reset(); pendingAuto_ = false;
    if (connection_) connection_(true);
    return TransportResult::Accepted;
}
void MockTransport::disconnect() {
    connected_ = subscribed_ = sent_ = false;
    pendingWeight_.reset(); pendingLoaded_.reset(); pendingSettings_.reset(); pendingAuto_ = false;
    reported_ = {}; heldSince_ = 0;
    if (connection_) connection_(false);
}
std::vector<std::string> MockTransport::discoverServices() {
    return connected_ ? std::vector<std::string>{VoltraGattProfile::service} : std::vector<std::string>{};
}
std::vector<Characteristic> MockTransport::discoverCharacteristics(const std::string& service) {
    if (!connected_ || service != VoltraGattProfile::service) return {};
    return {{VoltraGattProfile::notify, true, false}};
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
    r.twinKnown = profile != SimulatorProfile::TwinUnknown;
    r.moving = r.held = false; r.rep = 0; r.repsStarted = false; restSince_ = 0;
    r.force.reset(); r.setNumber.reset(); r.completedSets.reset(); r.lastSetReps.reset(); r.restSeconds.reset();
}
void MockTransport::poll(uint64_t now) {
    nowMs_ = now;
    if (!connected_ || !subscribed_ || !notification_) return;
    if (!devices_[active_].online) { disconnect(); return; }
    auto& state = devices_[active_].state;
    if (now >= dueMs_ && profile_ != SimulatorProfile::Stale) {
        if (pendingSettings_) { state.settings = *pendingSettings_; pendingSettings_.reset(); sent_ = false; }
        if (pendingLoaded_) {
            if (*pendingLoaded_ && !state.loaded) {
                state.setNumber = state.completedSets.value_or(0)+1; state.rep = 0; state.repsStarted = false;
                state.restSeconds.reset(); restSince_ = 0;
            } else if (!*pendingLoaded_ && state.loaded) {
                state.completedSets = state.setNumber.value_or(1); state.lastSetReps = int(state.rep);
                restSince_ = now; state.restSeconds = 0;
            }
            state.loaded = *pendingLoaded_; state.autoLoadArmed = false;
            if (!state.loaded) { state.moving = state.held = false; state.repsStarted = false; }
            pendingLoaded_.reset(); sent_ = false;
        }
        if (pendingAuto_) { state.autoLoadArmed = true; pendingAuto_ = false; sent_ = false; heldSince_ = 0; }
        // Synthetic device behavior, not a remote timer that issues a load.
        if (state.autoLoadArmed && state.held) {
            if (!heldSince_) heldSince_ = now;
            if (now-heldSince_ >= 3000) {
                state.loaded = true; state.autoLoadArmed = false; sent_ = false;
                state.setNumber = state.completedSets.value_or(0)+1; state.rep = 0; state.repsStarted = false;
                state.restSeconds.reset(); restSince_ = 0;
            }
        } else heldSince_ = 0;
    }
    if (sent_ && (profile_ == SimulatorProfile::Stale || now-lastMs_ < notificationIntervalMs)) return;
    if (sent_ && (pendingSettings_ || pendingLoaded_ || pendingAuto_)) return;
    lastMs_ = now; sent_ = true;
    if (state.loaded) { state.force = state.settings.weight; if (!state.setNumber) state.setNumber = 1; }
    else { state.force.reset(); if (restSince_) state.restSeconds = int((now-restSince_)/1000); }
    // Already-paired synthetic endpoints share a workout through one selected
    // session. This is not a second BLE connection or a duplicate command.
    if(active_ == 1 || active_ == 2) {
        auto& peer = devices_[active_ == 1 ? 2 : 1].state;
        const auto peerName = peer.twinPeerName;
        peer = state; peer.twinPeerName = peerName;
    }
    state.receivedAtMs = now; ++state.revision; reported_ = state;
    device_.weightLbs = uint16_t(std::lround(state.settings.weight * (state.unit == WeightUnit::Kilograms ? 2.2046226218 : 1.0)));
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
    r.ready = r.validated && profile_ != SimulatorProfile::Stale && !state.loaded && !state.autoLoadArmed;
    r.cableZero = r.ready; r.twinSafe = state.twinKnown;
    if (r.validated && r.twinSafe) {
        r.capabilities.weightTraining = Knowledge::Yes;
        r.capabilities.effectiveMaxWeightLbs = uint16_t(std::lround(state.maxWeight * (state.unit == WeightUnit::Kilograms ? 2.2046226218 : 1.0)));
        r.minWeightLbs = 0;
    }
    return r;
}
bool MockTransport::consumeIgnored() { const bool ignored = ignoreNext_; ignoreNext_ = false; return ignored; }
TransportResult MockTransport::setTraining(const TrainingSettings& s) {
    const auto r = trainingReport();
    if (!readiness().validated || !modifierUnitsAllowed(r,s) || !r.twinKnown || pendingLoaded_ || pendingAuto_ || r.autoLoadArmed ||
        s.weight < 0 || (r.loaded && s.weight == 0) || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && (!r.magicSupported[int(s.magicMode)] || s.magic.value > modifierMaximum(r, s, true))) ||
        (s.eccentric.enabled && (!r.eccentricSupported || s.eccentric.value > modifierMaximum(r, s, false))) ||
        envelopeHundredths(s) > r.maxWeight*100) return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    // New knob values replace the pending setting, not its delivery deadline.
    // Otherwise a continuous turn can starve reports and look like a disconnect.
    if (!pendingSettings_) dueMs_ = nowMs_ + confirmationDelayMs;
    pendingSettings_ = s; return TransportResult::Accepted;
}
TransportResult MockTransport::setWeight(uint16_t pounds) {
    const auto r = trainingReport(); auto s = r.settings;
    s.weight = int(std::lround(pounds / (r.unit == WeightUnit::Kilograms ? 2.2046226218 : 1.0)));
    return setTraining(s);
}
TransportResult MockTransport::setLoaded(bool loaded) {
    if (!connected_) return TransportResult::Disconnected;
    const auto r = readiness();
    if (loaded && (!r.ready || !r.cableZero || !r.twinSafe || pendingSettings_ || pendingLoaded_ || trainingReport().settings.weight < 1))
        return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    pendingSettings_.reset(); pendingAuto_ = false; pendingLoaded_ = loaded; dueMs_ = nowMs_ + confirmationDelayMs;
    return TransportResult::Accepted;
}
TransportResult MockTransport::triggerAutoLoad() {
    if (!readiness().ready || !readiness().twinSafe || !trainingReport().autoLoadSupported ||
        pendingSettings_ || pendingLoaded_ || trainingReport().settings.weight < 1) return TransportResult::Failed;
    if (consumeIgnored()) return TransportResult::Accepted;
    pendingAuto_ = true; dueMs_ = nowMs_ + confirmationDelayMs; return TransportResult::Accepted;
}
void MockTransport::toggleAvailability() { devices_[5].online = !devices_[5].online; }
void MockTransport::completeRep() {
    auto& r = devices_[active_].state;
    if (!connected_ || !r.loaded) return;
    ++r.rep; r.repsStarted = true; r.moving = true; r.held = false; sent_ = false;
}
void MockTransport::setHeld(bool held) {
    auto& r = devices_[active_].state;
    r.held = held; r.moving = !held; sent_ = false;
}
}
