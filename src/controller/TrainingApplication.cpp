#include "TrainingApplication.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
namespace voltra {
TrainingApplication::TrainingApplication(BleTransport& t, VoltraController& c, ControlSession& s, DeviceMemory* m)
    : transport_(t), controller_(c), session_(s), memory_(m) {
    if (memory_) recent_ = memory_->load();
    if (recent_.size() > 4) recent_.resize(4);
    for (auto& d : recent_) d.online = false;
}
void TrainingApplication::scan() {
    devices_ = recent_;
    for (auto& d : devices_) d.online = false;
    for (const auto& a : transport_.scan()) {
        if (std::find(a.services.begin(), a.services.end(), VoltraGattProfile::service) == a.services.end()) continue;
        auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == a.id; });
        if (it == devices_.end()) devices_.push_back({a.id, a.name, true});
        else { it->name = a.name; it->online = true; }
    }
    scannedAt_ = now_; scanned_ = true;
}
bool TrainingApplication::select(const DeviceId& id) {
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == id && d.online; });
    if (it == devices_.end()) return false; // An inactive tap has no side effects.
    const DeviceEntry chosen = *it;
    leave();
    if (!controller_.connect(chosen.id)) { message_ = "Connection unavailable"; scan(); return false; }
    selected_ = chosen.id; name_ = chosen.name;
    recent_.erase(std::remove_if(recent_.begin(), recent_.end(), [&](const auto& d) { return d.id == selected_; }), recent_.end());
    recent_.insert(recent_.begin(), chosen);
    if (recent_.size() > 4) recent_.resize(4);
    if (memory_ && !memory_->save(recent_)) message_ = "Could not save recent devices";
    scan();
    return true;
}
void TrainingApplication::leave() {
    if (!selected_.empty()) dropSettings_[selected_].enabled = false;
    controller_.disconnect(); selected_.clear(); name_.clear();
    pending_ = requestedLoad_ = false; weightIntent_.reset(); message_.clear(); resetDrops();
}
bool TrainingApplication::fresh() const {
    const auto r = report();
    return connected() && r.revision != 0 && now_ >= r.receivedAtMs && now_ - r.receivedAtMs <= 2000;
}
bool TrainingApplication::settingsAllowed() const {
    const auto r = report();
    return fresh() && r.twinKnown && session_.readiness().validated && r.maxWeight > 0 && !r.autoLoadArmed && !requestedLoad_;
}
void TrainingApplication::sent(TransportResult result) {
    if (result != TransportResult::Accepted) { message_ = "Request not accepted"; return; }
    pending_ = true; sentAt_ = now_; sentRevision_ = report().revision;
    message_ = "Awaiting device";
}
bool TrainingApplication::sendSettings(TrainingSettings s) {
    const auto r = report();
    if (!settingsAllowed() || s.weight < 0 || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && s.magic.value > modifierMaximum(r, s, true)) ||
        (s.eccentric.enabled && s.eccentric.value > modifierMaximum(r, s, false)) ||
        envelopeHundredths(s, r.percent) > r.maxWeight * 100) { message_ = "Outside device limits"; return false; }
    const auto result = session_.setTraining(s); sent(result);
    return result == TransportResult::Accepted;
}
void TrainingApplication::adjustWeight(int delta) {
    if (!settingsAllowed()) { message_ = "Device state unavailable"; return; }
    auto r = report();
    if (r.loaded && !r.liveWeightSupported) { message_ = "Unload to adjust"; return; }
    auto s = r.settings;
    s.weight = std::clamp(weightIntent_.value_or(s.weight) + delta, 0, r.maxWeight);
    if (r.loaded && s.weight == 0) { message_ = "Unload to select zero"; return; }
    // The original brief specifies reducing modifiers before capping the base.
    // Trim EO first, then magic; this is part of this explicit knob request.
    const int divisor = r.percent ? s.weight : 100;
    for (auto* m : {&s.eccentric, &s.magic}) {
        const int excess = envelopeHundredths(s, r.percent) - r.maxWeight*100;
        if (excess > 0 && m->enabled && divisor > 0)
            m->value = std::max(0, m->value - (excess+divisor-1)/divisor);
    }
    if (sendSettings(s)) weightIntent_ = s.weight;
}
void TrainingApplication::toggleWeight() {
    auto r = report();
    if (!fresh() || r.loaded || r.autoLoadArmed || requestedLoad_) {
        // Unload and cancel are not gated by load readiness or pending settings.
        resetDrops(); dropsCancelled_ = true; requestedLoad_ = false; weightIntent_.reset();
        sent(session_.setLoaded(false)); return;
    }
    const auto e = session_.readiness();
    if (pending_ || !settingsAllowed() || !e.ready || !e.cableZero || !e.twinSafe || r.settings.weight < 1) {
        message_ = r.settings.weight == 0 ? "Set at least 1 to load" : "Device not ready"; return;
    }
    const auto result = session_.setLoaded(true);
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
}
void TrainingApplication::autoLoad() {
    const auto r = report(); const auto e = session_.readiness();
    if (!settingsAllowed() || pending_ || r.loaded || !r.autoLoadSupported || !e.ready || !e.twinSafe || r.settings.weight < 1) {
        message_ = "Auto Load unavailable"; return;
    }
    const auto result = session_.triggerAutoLoad();
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
}
void TrainingApplication::toggleModifier(bool magic) {
    const auto r = report();
    if (pending_ || r.loaded || !(magic ? r.magicSupported[int(r.settings.magicMode)] : r.eccentricSupported)) return;
    auto s = r.settings; auto& m = magic ? s.magic : s.eccentric;
    m.enabled = !m.enabled;
    sendSettings(s);
}
void TrainingApplication::applyModifier(bool magic, MagicMode mode, int value) {
    const auto r = report();
    if (pending_ || r.loaded || !(magic ? r.magicSupported[int(mode)] : r.eccentricSupported)) {
        message_ = "Unload before applying"; return;
    }
    auto s = r.settings; auto& m = magic ? s.magic : s.eccentric;
    if (magic) s.magicMode = mode;
    m.enabled = true; m.value = value;
    sendSettings(s);
}
DropSettings TrainingApplication::drops() const {
    const auto it = dropSettings_.find(selected_);
    return it == dropSettings_.end() ? DropSettings{} : it->second;
}
void TrainingApplication::applyDrops(DropSettings d) {
    if (!fresh() || !report().liveWeightSupported || selected_.empty()) return;
    d.step = std::clamp(d.step, -report().maxWeight, 0);
    d.drops = std::clamp(d.drops, 0, 9); d.percent = std::clamp(d.percent, 0, 80);
    d.holdSeconds = std::clamp(d.holdSeconds, 1, 9);
    dropSettings_[selected_] = d; resetDrops(); message_.clear();
}
void TrainingApplication::toggleDrops() { auto d = drops(); d.enabled = !d.enabled; applyDrops(d); }
void TrainingApplication::resetDrops() {
    dropsCancelled_ = false;
    running_ = movementSeen_ = holdConsumed_ = wasLoaded_ = false;
    holdSince_ = telemetryRevision_ = 0; lastRep_ = 0; dropsCompleted_ = 0; dropTarget_.reset();
}
void TrainingApplication::runDrops(const TrainingReport& r) {
    if (!drops().enabled || !fresh() || !r.telemetryValid || !r.twinKnown) {
        if (running_) { dropSettings_[selected_].enabled = false; message_ = "Drops paused: state unavailable"; }
        running_ = false; holdSince_ = 0; return;
    }
    if (!r.loaded) { resetDrops(); return; }
    if (dropsCancelled_) return; // A failed unload must never restart automation.
    if (!wasLoaded_) {
        wasLoaded_ = running_ = true; lastRep_ = r.rep; telemetryRevision_ = r.revision;
        movementSeen_ = r.moving; return;
    }
    if (!running_ || r.revision == telemetryRevision_) return;
    telemetryRevision_ = r.revision;
    if (dropTarget_) {
        if (pending_) return;
        if (r.settings.weight != *dropTarget_) {
            running_ = false; dropSettings_[selected_].enabled = false;
            message_ = "Drop unconfirmed; stopped"; dropTarget_.reset(); return;
        }
        dropTarget_.reset(); ++dropsCompleted_;
    }
    const auto d = drops(); bool trigger = false;
    if (d.mode == DropMode::Stairs) {
        trigger = r.rep == lastRep_ + 1 && d.step < 0;
        if (r.rep < lastRep_ || r.rep > lastRep_ + 1) {
            running_ = false; dropSettings_[selected_].enabled = false; message_ = "Rep sequence changed; drops stopped";
        }
        lastRep_ = r.rep;
    } else {
        if (r.moving) { movementSeen_ = true; holdConsumed_ = false; holdSince_ = 0; }
        else if (r.held && movementSeen_ && !holdConsumed_) {
            if (!holdSince_) holdSince_ = r.receivedAtMs;
            trigger = r.receivedAtMs - holdSince_ >= uint64_t(d.holdSeconds)*1000 && dropsCompleted_ < d.drops && d.percent > 0;
            if (trigger) { holdConsumed_ = true; movementSeen_ = false; }
        } else holdSince_ = 0;
    }
    if (!trigger || !running_ || pending_ || r.settings.weight <= 1) return;
    auto s = r.settings;
    s.weight = std::max(1, d.mode == DropMode::Stairs ? s.weight + d.step : s.weight * (100-d.percent)/100);
    if (s.weight >= r.settings.weight) return;
    if (sendSettings(s)) dropTarget_ = s.weight;
    else { running_ = false; dropSettings_[selected_].enabled = false; }
}
void TrainingApplication::tick(uint64_t now) {
    now_ = now; controller_.tick(now);
    if (!scanned_ || now - scannedAt_ >= 5000) scan();
    if (!connected()) { pending_ = requestedLoad_ = false; weightIntent_.reset(); resetDrops(); return; }
    const auto r = report();
    // No retries or reconciliation writes: every new report wins, even a mismatch.
    if (pending_ && (r.revision > sentRevision_ || now - sentAt_ >= 3000)) {
        message_ = r.revision > sentRevision_ ? "" : "No confirmation";
        pending_ = requestedLoad_ = false; weightIntent_.reset();
    }
    runDrops(r);
}
}
