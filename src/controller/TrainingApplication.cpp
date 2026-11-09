#include "TrainingApplication.h"
#include "diagnostics/EventLog.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
namespace voltra {
namespace {
constexpr uint64_t discoveryIntervalMs=5000;
// Three scan intervals tolerate missed radio packets; never machine readiness.
constexpr uint64_t discoveryGraceMs=15000;
constexpr size_t maximumDiscoveredDevices=64;
constexpr uint64_t motionFreshMs=500;
constexpr uint64_t restPauseBarDelayMs=250; // User-requested visual threshold.
const char* restPauseLimitError="RP pause must be shorter than Auto Unload";
}
TrainingApplication::TrainingApplication(BleTransport& t, VoltraController& c, ControlSession& s, DeviceMemory* m, std::vector<DeviceEntry> knownDevices, bool automaticSelection)
    : transport_(t), controller_(c), session_(s), memory_(m), knownDevices_(std::move(knownDevices)), automaticSelection_(automaticSelection) {
    if (memory_) recent_ = memory_->load();
    for (auto& d : recent_) d.online = false;
}
bool TrainingApplication::remembered(const DeviceId& id) const {
    return std::any_of(recent_.begin(), recent_.end(), [&](const auto& d) { return d.id == id; });
}
std::optional<int> TrainingApplication::restSeconds() const {
    if(!fresh()) return {};
    if(dropProgress_.active && report().setActive!=false) return {};
    if(!dropProgress_.visible) if(const auto deviceTime=report().restSeconds) return deviceTime;
    if(!restSince_ || now_<*restSince_) return {};
    return int(std::min<uint64_t>((now_-*restSince_)/1000,359999));
}
TrainingApplication::DisplayCounters TrainingApplication::displayCounters() const {
    const auto r=report();
    if(!fresh() || !r.telemetryValid) return {};
    if(!dropProgress_.visible) return {r.setNumber,int(r.rep),r.completedSets,r.lastSetReps};
    if(!dropProgress_.valid) return {};
    const int set=dropProgress_.completedSets+(dropProgress_.active ? 1 : 0);
    const bool resting=dropProgress_.active && r.setActive==false;
    return {set,dropProgress_.reps,
        resting ? std::optional<int>(set) : dropProgress_.completedSets>0 ? std::optional<int>(dropProgress_.completedSets) : std::nullopt,
        resting ? std::optional<int>(dropProgress_.reps) : dropProgress_.completedSets>0 ? std::optional<int>(dropProgress_.lastReps) : std::nullopt};
}
bool TrainingApplication::forget(const DeviceId& id) {
    automaticSelection_ = false;
    auto remaining = recent_;
    remaining.erase(std::remove_if(remaining.begin(), remaining.end(), [&](const auto& d) { return d.id == id; }), remaining.end());
    if (memory_ && !memory_->save(remaining)) { message_ = "Could not remove saved device"; return false; }
    recent_ = std::move(remaining);
    dropSettings_.erase(id);
    if (selected_ == id) leave();
    message_.clear();
    scan();
    return true;
}
bool TrainingApplication::tryAutomaticSelection() {
    if (maintenance_ || !automaticSelection_ || !scanned_ || scanPending_ || connected() || connecting()) return false;
    const auto count = std::count_if(devices_.begin(), devices_.end(), [](const auto& d) { return d.online; });
    if (!count) return false; // Keep discovering when the remote starts first.
    automaticSelection_ = false; // One startup attempt; never fight manual navigation.
    if (count != 1) return false;
    const auto it = std::find_if(devices_.begin(), devices_.end(), [](const auto& d) { return d.online; });
    return select(it->id);
}
void TrainingApplication::scan() {
    if (maintenance_ || discoveryPaused_ || connected() || connecting() || scanPending_) return;
    const auto found=transport_.scan();
    scanPending_=transport_.scanInProgress();
    if (!scanPending_) refreshDiscovery(found);
    scannedAt_ = now_; scanned_ = true;
}
void TrainingApplication::refreshDiscovery(const std::vector<Advertisement>& found) {
    for(auto it=seenDevices_.begin();it!=seenDevices_.end();) {
        if(now_-it->second.at>=discoveryGraceMs) it=seenDevices_.erase(it);else ++it;
    }
    for (const auto& a : found) {
        if (std::find(a.services.begin(), a.services.end(), VoltraGattProfile::service) == a.services.end()) continue;
        if(seenDevices_.size()>=maximumDiscoveredDevices && !seenDevices_.count(a.id)) continue;
        auto& seen=seenDevices_[a.id];seen.device.id=a.id;seen.device.online=true;seen.at=now_;
        if(!a.name.empty()) seen.device.name=a.name;
        if(seen.device.name.empty()) seen.device.name="VOLTRA";
    }
    devices_ = knownDevices_;
    for(const auto& d : recent_) if(std::none_of(devices_.begin(),devices_.end(),[&](const auto& known){return known.id == d.id;})) devices_.push_back(d);
    for (auto& d : devices_) d.online = false;
    for (const auto& pair : seenDevices_) {
        const auto& a=pair.second.device;
        auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == a.id; });
        if (it == devices_.end()) devices_.push_back({a.id, a.name.empty() ? "VOLTRA" : a.name, true, {}});
        else { if(!a.name.empty()) it->name = a.name; it->online = true; }
    }
}
bool TrainingApplication::select(const DeviceId& id) {
    if(maintenance_) return false;
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == id && d.online; });
    if (it == devices_.end()) return false; // An inactive tap has no side effects.
    const DeviceEntry chosen = *it;
    leave();
    if (!controller_.connect(chosen.id)) { message_ = controller_.feedback(); scan(); return false; }
    selected_ = chosen.id; name_ = chosen.name;
    if(connecting()) message_="Connecting to "+name_+"...";
    connectingDevice_ = chosen;
    if (session_.accepted()) rememberConnection();
    return true;
}
void TrainingApplication::rememberConnection() {
    if (!connectingDevice_) return;
    const auto chosen = *connectingDevice_;
    connectingDevice_.reset();
    if (remembered(selected_) || recent_.size() < maximumRememberedDevices) {
        auto saved = recent_;
        saved.erase(std::remove_if(saved.begin(), saved.end(), [&](const auto& d) { return d.id == selected_; }), saved.end());
        saved.insert(saved.begin(), chosen);
        if (memory_ && !memory_->save(saved)) message_ = "Could not save device";
        else recent_ = std::move(saved);
    } else message_ = "Device memory full";
    scan();
}
void TrainingApplication::leave(bool preserveFailure) {
    const std::string failure = preserveFailure ? (!message_.empty() ? message_ : session_.status().empty() ? "Connection lost" : session_.status()) : "";
    automaticSelection_ = false;
    scanned_ = scanPending_ = false;
    connectingDevice_.reset();
    presetIntent_.reset(); observedLoaded_ = false;
    restSince_.reset();restSetRevision_=0;
    dropProgress_={};
    confirmedDropWeight_.reset();
    dropStart_.reset();dropRestore_.reset();
    if (!selected_.empty()) dropSettings_[selected_].enabled = false;
    controller_.disconnect(); selected_.clear(); name_.clear();
    pending_ = requestedLoad_ = false; weightIntent_.reset(); settingsIntent_.reset(); message_ = failure; resetDrops();
}
bool TrainingApplication::fresh() const {
    const auto r = report();
    return connected() && r.revision != 0 && now_ >= r.receivedAtMs && now_ - r.receivedAtMs <= 2000;
}
bool TrainingApplication::settingsAllowed() const {
    const auto r = report();
    return fresh() && r.twinKnown && session_.readiness().validated &&
        r.minWeight > 0 && r.maxWeight >= r.minWeight && !r.autoLoadArmed && !requestedLoad_;
}
void TrainingApplication::sent(TransportResult result) {
    if (result != TransportResult::Accepted) { message_ = session_.status().empty() ? "Request not accepted" : session_.status(); return; }
    pending_ = true; sentAt_ = now_; sentRevision_ = report().revision;
    message_.clear(); // Brief requests do not flash a warning on every input.
}
bool TrainingApplication::sendSettings(TrainingSettings s) {
    const auto r = report();
    if (!settingsAllowed() || !modifierUnitsAllowed(r,s) || s.weight < r.minWeight || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && s.magic.value == 0) || (s.eccentric.enabled && s.eccentric.value == 0) ||
        (s.magic.enabled && s.magic.value > modifierMaximum(r, s, true)) ||
        (s.eccentric.enabled && s.eccentric.value > modifierMaximum(r, s, false)) ||
        envelopeHundredths(s) > r.maxWeight * 100) { message_ = "Outside device limits"; return false; }
    const auto result = session_.setTraining(s); sent(result);
    if (result == TransportResult::Accepted) settingsIntent_ = s;
    return result == TransportResult::Accepted;
}
void TrainingApplication::adjustWeight(int delta) {
    if(dropRestore_) {message_="Restoring starting weight; please wait";return;}
    if (presetIntent_) { message_ = "Awaiting preset confirmation"; return; }
    if (!settingsAllowed()) { message_ = "Device state unavailable"; return; }
    auto r = report();
    if (r.loaded && !r.liveWeightSupported) { message_ = "Unload to adjust"; return; }
    dropStart_.reset(); // A subsequent manual base choice supersedes restoration.
    if(running_ || dropTarget_ || dropsExhausted_) stopDrops("Drops stopped for manual adjustment");
    auto s = settingsIntent_.value_or(r.settings);
    const int ceiling=r.loaded ? baseMaximum(r) : r.maxWeight;
    if(ceiling<r.minWeight) {message_="Outside device limits";return;}
    s.weight = std::clamp(weightIntent_.value_or(s.weight) + delta, r.minWeight, ceiling);
    // The original brief specifies reducing modifiers before capping the base.
    // Trim EO first, then magic; this is part of this explicit knob request.
    for (auto* m : {&s.eccentric, &s.magic}) {
        const int divisor = m->percent ? s.weight : 100;
        const int excess = envelopeHundredths(s) - r.maxWeight*100;
        if (excess > 0 && m->enabled && divisor > 0)
            m->value = std::max(0, m->value - (excess+divisor-1)/divisor);
        if(m->enabled && m->value == 0) m->enabled=false;
    }
    if (!pending_ && s.weight == r.settings.weight && sameModifier(s.magic,r.settings.magic) &&
        sameModifier(s.eccentric,r.settings.eccentric)) return;
    if (sendSettings(s)) {weightIntent_ = s.weight;weightIntentUnit_=r.unit;}
}
void TrainingApplication::toggleWeight() {
    diagnostic("Explicit weight action");
    auto r = report();
    if (!fresh() || r.loaded || r.autoLoadArmed || requestedLoad_) {
        unload(); return;
    }
    const auto e = session_.readiness();
    if (pending() || !settingsAllowed() || !e.ready || !e.cableZero || !e.twinSafe || r.settings.weight < r.minWeight) {
        message_ = r.minWeight>0 && r.settings.weight < r.minWeight ? "Set at least "+weightText(r.minWeight,r.unit)+" "+unitName(r.unit) :
            !e.reason.empty() ? e.reason : "Device not ready"; diagnostic("Load blocked: "+message_);return;
    }
    const auto result = session_.setLoaded(true);
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
    diagnostic("Load request result="+std::to_string(int(result)));
}
void TrainingApplication::unload() {
    diagnostic("Unload requested");
    // Always a cancel/unload, even before a pending load is reported. Never
    // route an editor's unload target through a toggle that could load instead.
    presetIntent_.reset(); settingsIntent_.reset(); resetDrops(); dropsCancelled_ = true;
    dropRestore_.reset(); // Explicit unload cancels any already queued restore.
    requestedLoad_ = false; weightIntent_.reset();
    sent(session_.setLoaded(false));
}
void TrainingApplication::autoLoad() {
    diagnostic("Explicit Auto Load action");
    const auto r = report(); const auto e = session_.readiness();
    if (!settingsAllowed() || pending_ || r.loaded || !r.autoLoadSupported || !e.ready || !e.twinSafe || r.settings.weight < r.minWeight) {
        message_ = e.reason.empty() ? "Auto Load unavailable" : e.reason; return;
    }
    const auto result = session_.triggerAutoLoad();
    // A new explicit Auto Load starts a new execution sequence, preserving the
    // selected local recipe. Countdown reports cannot inherit an old set/hold.
    if(result == TransportResult::Accepted) resetDrops();
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
}
void TrainingApplication::toggleModifier(bool magic) {
    const auto r = report();
    if (dropRestore_ || presetIntent_ || (pending_ && (!settingsIntent_ || r.loaded)) || (r.loaded && !r.liveModifierAmountsSupported) ||
        !(magic ? r.magicSupported[int(r.settings.magicMode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Wait for modifier controls" : "Awaiting device"; return;
    }
    auto s = settingsIntent_.value_or(r.settings); auto& m = magic ? s.magic : s.eccentric;
    m.enabled = !m.enabled;
    sendSettings(s);
}
Preset TrainingApplication::currentPreset() const {
    const auto r = report();
    return {r.settings.magicMode, r.settings.magic, r.settings.eccentric, r.unit, drops()};
}
Preset TrainingApplication::presetForCurrentDevice(Preset p) const {
    const auto target = report().unit;
    if(!p.magic.percent) p.magic.value = convertWeightUnits(p.magic.value,p.unit,target);
    if(!p.eccentric.percent) p.eccentric.value = convertWeightUnits(p.eccentric.value,p.unit,target);
    p.drops.step = convertWeightUnits(p.drops.step,p.unit,target);
    p.unit = target;
    return p;
}
bool TrainingApplication::applyPreset(const Preset& saved) {
    const auto r = report(); presetError_.clear();
    if(!fresh()) { message_ = "Device state unavailable"; return false; }
    auto p = presetForCurrentDevice(saved);
    if(r.sharedModifierUnits) {
        if(p.magic.enabled && p.eccentric.enabled && p.magic.percent!=p.eccentric.percent) {
            presetError_="This VOLTRA shares the Magic / EO unit";message_=presetError_;return false;
        }
        const bool percent=p.magic.enabled ? p.magic.percent : p.eccentric.enabled ? p.eccentric.percent : r.settings.magic.percent;
        if(!p.magic.enabled) p.magic.percent=percent;
        if(!p.eccentric.enabled) p.eccentric.percent=percent;
    }
    if ((p.magic.enabled && !r.magicSupported[int(p.mode)]) ||
        (((p.magic.enabled && p.magic.percent) || (p.eccentric.enabled && p.eccentric.percent)) && !r.percentageSupported) ||
        (p.eccentric.enabled && !r.eccentricSupported) || (p.drops.enabled && !r.liveWeightSupported)) {
        presetError_ = "Preset values not available on Voltra firmware version "+
            (r.firmwareVersion.empty() ? "unknown" : r.firmwareVersion);
        message_ = presetError_; return false;
    }
    if (pending() || r.loaded || std::abs(p.drops.step) > r.maxWeight) {
        message_ = "Unload / wait before applying preset"; return false;
    }
    if(p.drops.enabled && p.drops.mode==DropMode::RestPause &&
       (restPauseHoldMaximum(r)<1 || p.drops.holdSeconds>restPauseHoldMaximum(r))) {
        presetError_=restPauseLimitError;message_=presetError_;return false;
    }
    // A preset replaces all modifiers as one request. Preserve its exact values;
    // if needed, lower only the current base to the conservative integer ceiling.
    auto s = r.settings; s.magicMode = p.mode; s.magic = p.magic; s.eccentric = p.eccentric;
    auto limits = r; limits.settings = s;
    s.weight = std::min(s.weight,baseMaximum(limits));
    if (!sendSettings(s)) return false;
    presetIntent_ = p; return true;
}
bool TrainingApplication::applyModifier(bool magic, MagicMode mode, int value, bool percent) {
    const auto r = report();
    if (dropRestore_ || presetIntent_ || (pending_ && (!settingsIntent_ || r.loaded)) || (r.loaded && !r.liveModifierAmountsSupported) || !(magic ? r.magicSupported[int(mode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Wait for modifier controls" : "Awaiting device"; return false;
    }
    if(r.loaded && ((magic && mode!=r.settings.magicMode) || percent!=(magic ? r.settings.magic.percent : r.settings.eccentric.percent))) {
        message_="Unload to switch curve or units";return false;
    }
    if (value < 1) { message_ = "No headroom for modifier"; return false; }
    auto s = settingsIntent_.value_or(r.settings); auto& m = magic ? s.magic : s.eccentric;
    if (magic) s.magicMode = mode;
    m.enabled = true; m.value = value; m.percent = percent;
    if(r.sharedModifierUnits) convertModifierUnit(magic ? s.eccentric : s.magic,percent,s.weight);
    return sendSettings(s);
}
DropSettings TrainingApplication::drops() const {
    const auto it = dropSettings_.find(selected_);
    if (it != dropSettings_.end()) return it->second;
    auto d = DropSettings{}; d.step *= ticksPerUnit(report().unit); return d;
}
std::optional<int> TrainingApplication::restPauseRemainingPermille() const {
    const auto r=report(); const auto d=drops();
    if(!fresh() || !r.loaded || !r.telemetryValid || r.autoLoadArmed || !r.held || r.moving ||
       !d.enabled || d.mode!=DropMode::RestPause || d.percent<=0 || d.drops<=dropsCompleted_ ||
       d.holdSeconds<1 || d.holdSeconds>restPauseHoldMaximum(r) || !running_ || !movementSeen_ ||
       holdConsumed_ || dropsCancelled_ || dropsExhausted_ || waitingForDropSet_ || pending() || dropTarget_ ||
       !holdSince_ || now_<holdSince_ || !r.motionRevision || now_<r.motionAtMs || now_-r.motionAtMs>motionFreshMs)
        return {};
    const auto elapsed=now_-holdSince_;
    if(elapsed<restPauseBarDelayMs) return {};
    const uint64_t duration=uint64_t(d.holdSeconds)*1000;
    return int((duration-std::min(elapsed,duration))*1000/duration);
}
bool TrainingApplication::applyDrops(DropSettings d) {
    if (!fresh() || selected_.empty()) { message_ = "Device state unavailable"; return false; }
    if(d.enabled && (!report().liveWeightSupported || report().minWeight<1 || report().autoLoadArmed || pending())) {
        message_="Wait for weight controls before arming drops";return false;
    }
    if(d.enabled && d.mode==DropMode::RestPause &&
       (restPauseHoldMaximum(report())<1 || d.holdSeconds>restPauseHoldMaximum(report()))) {
        message_=restPauseLimitError;return false;
    }
    if(dropTarget_) stopDrops("");
    d.step = std::clamp(d.step, -report().maxWeight, report().maxWeight);
    d.drops = std::clamp(d.drops, 0, 9); d.percent = std::clamp(d.percent, 0, 80);
    d.holdSeconds = std::clamp(d.holdSeconds, 1, 9);
    dropSettings_[selected_] = d; resetDrops();
    if(!d.enabled) dropStart_.reset();
    message_.clear();return true;
}
void TrainingApplication::toggleDrops() { auto d = drops(); d.enabled = !d.enabled; applyDrops(d); }
void TrainingApplication::resetDrops() {
    dropsCancelled_ = dropsExhausted_ = false;
    running_ = movementSeen_ = holdConsumed_ = wasLoaded_ = false;
    holdSince_ = telemetryRevision_ = 0; lastRep_ = 0; dropsCompleted_ = 0; dropTarget_.reset();
    repRevision_=dropSetEndRevision_=0;waitingForDropSet_=false;
    repSet_.reset();holdPosition_.reset();
}
void TrainingApplication::stopDrops(const char* reason) {
    if(dropTarget_) {
        session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();dropTarget_.reset();
    }
    if(!selected_.empty()) dropSettings_[selected_].enabled=false;
    running_=false;dropsCancelled_=true;holdSince_=0;holdPosition_.reset();message_=reason;
}
void TrainingApplication::beginDropRestore(const TrainingReport& r) {
    if(!dropStart_) return;
    const auto start=*dropStart_;dropStart_.reset();
    // Only this observed workout's genuine rest/unload can initiate restoration.
    // It is never persisted, replayed after reconnect or caused by a segment close.
    if(!drops().enabled || r.unit!=start.unit || start.weight<r.minWeight) return;
    session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();
    resetDrops();dropsCancelled_=true;
    dropRestore_=DropRestore{start,now_,false};
    if(r.loaded) {
        const auto result=session_.setLoaded(false);sent(result);
        if(result!=TransportResult::Accepted) {dropRestore_.reset();message_="End-of-set unload failed; weight not restored";}
    }
}
void TrainingApplication::runDropRestore() {
    if(!dropRestore_) return;
    constexpr uint64_t restoreDeadlineMs=8000;
    if(now_-dropRestore_->at>restoreDeadlineMs || session_.commandFailed()) {
        session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();
        dropRestore_.reset();message_="Starting weight not restored; check VOLTRA";return;
    }
    if(pending_ || session_.busy() || !fresh()) return;
    const auto r=report();
    if(r.loaded || r.autoLoadArmed || r.unit!=dropRestore_->start.unit) {
        dropRestore_.reset();message_="State changed; starting weight not restored";return;
    }
    if(!r.liveWeightSupported) return; // Wait for settled unloaded control state.
    if(dropRestore_->weightSent) {
        const bool confirmed=r.settings.weight==dropRestore_->start.weight && message_.empty();
        dropRestore_.reset();message_=confirmed ? "" : "Starting weight not confirmed";return;
    }
    // Copy the current confirmed accessories. Restore only base, while unloaded,
    // and refuse an original weight that no longer fits the current limits.
    auto s=r.settings;s.weight=dropRestore_->start.weight;
    if(s.weight==r.settings.weight) {dropRestore_.reset();return;}
    if(!sendSettings(s)) {dropRestore_.reset();message_="Starting weight no longer available";return;}
    dropRestore_->weightSent=true;
}
void TrainingApplication::runDrops(const TrainingReport& r) {
    if(!drops().enabled) return;
    if(fresh() && drops().mode==DropMode::RestPause && drops().holdSeconds>restPauseHoldMaximum(r)) {
        // An external timer change must not silently shorten an armed hold or
        // permit a saved recipe to race Auto Unload. Require a new Apply.
        stopDrops(restPauseLimitError);return;
    }
    if(fresh() && !r.loaded) { resetDrops(); return; }
    if(fresh() && r.resting==true) {
        // A genuine device rest ends this set, unlike AA85 emitted by a base
        // edit while status stays active. No queued automatic change crosses it.
        if(dropTarget_) {session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();message_.clear();}
        const bool cancelled=dropsCancelled_;
        resetDrops();dropsCancelled_=cancelled;
        // Preserve the old completed counter only as a baseline while waiting
        // for the next real set's observed start. Loading need not toggle here.
        wasLoaded_=true;waitingForDropSet_=true;
        lastRep_=r.completedRep;repSet_=r.repSet;repRevision_=r.repRevision;
        telemetryRevision_=r.motionRevision;dropSetEndRevision_=r.setEndRevision;
        return;
    }
    // A confirmed floor/quota ends automation for this loaded sequence. Reps
    // still update independently; there is nothing left to validate or send.
    if(dropsExhausted_) return;
    if(!fresh() || !r.twinKnown || !r.liveWeightSupported || r.minWeight<1 || r.autoLoadArmed) {
        if(wasLoaded_ || dropTarget_) stopDrops("Drops stopped: state unavailable");
        return;
    }
    if (dropsCancelled_) return; // A failed unload must never restart automation.
    const auto d = drops();
    if (dropTarget_ && !pending()) {
        if (session_.commandFailed() || !message_.empty() || r.settings.weight != *dropTarget_) {
            stopDrops("Drop unconfirmed; stopped");return;
        }
        confirmedDropWeight_=*dropTarget_;++dropConfirmationRevision_;
        dropTarget_.reset(); ++dropsCompleted_;
    }
    const bool descending=d.mode==DropMode::RestPause ? d.percent>0 : d.step<0;
    if(!dropTarget_ && !pending() && ((descending && r.settings.weight<=r.minWeight) ||
       (d.mode==DropMode::RestPause && dropsCompleted_>=d.drops))) {
        dropsExhausted_=true;running_=false;holdSince_=0;holdPosition_.reset();return;
    }
    if (!wasLoaded_) {
        wasLoaded_=true;lastRep_=r.completedRep;repSet_=r.repSet;repRevision_=r.repRevision;
        telemetryRevision_=r.motionRevision;dropSetEndRevision_=r.setEndRevision;
        waitingForDropSet_=r.setActive==false;
    }
    constexpr uint64_t repFreshMs=2000;
    const bool motionFresh=r.motionRevision && now_>=r.motionAtMs && now_-r.motionAtMs<=motionFreshMs;
    if(r.setEndRevision!=dropSetEndRevision_) {
        dropSetEndRevision_=r.setEndRevision;waitingForDropSet_=true;
    }
    if(waitingForDropSet_) {
        // Black can finish a set without unloading. Its completed-rep report
        // still belongs to that old set until the next rep finishes. Rebase
        // only after observing the new set's actual zero-count moving stream;
        // a new set number alone must not excuse a gap or replay missed drops.
        running_=movementSeen_=holdConsumed_=false;holdSince_=0;holdPosition_.reset();
        if(motionFresh && r.setActive==true && r.moving && (r.rep==0 || d.mode==DropMode::RestPause) &&
           r.setNumber && r.setNumber!=repSet_) {
            // A base change can close Black's statistics set before readback
            // finishes. Observe the next start now; the pending-drop check
            // below still requires confirmation before another completed rep
            // can trigger a write. Never reset RP's quota on device set IDs:
            // it belongs to the armed loaded sequence, not a weight segment.
            repSet_=r.setNumber;lastRep_=0;waitingForDropSet_=false;
        }
    }
    if(!motionFresh) {
        if(d.mode==DropMode::RestPause) {
            // A short stream gap pauses RP instead of permanently disabling it.
            // Never accrue hold time through the gap. Require fresh movement
            // and a whole new hold; a prolonged loss still disarms automation.
            constexpr uint64_t rpMotionLossStopMs=2000;
            movementSeen_=false;holdConsumed_=true;holdSince_=0;holdPosition_.reset();
            if(r.motionRevision && (now_<r.motionAtMs || now_-r.motionAtMs>rpMotionLossStopMs))
                stopDrops("Drops stopped: motion reports lost");
        } else if(running_) stopDrops("Drops stopped: motion reports lost");
        return; // Wait for first actual sample after loading, never infer a hold.
    }
    running_=!waitingForDropSet_;
    const bool newRep=r.repRevision!=repRevision_;
    bool completed=false;
    if(newRep) {
        repRevision_=r.repRevision;
        if(d.mode==DropMode::Stairs && ((repSet_ && r.repSet!=repSet_) || r.completedRep<lastRep_ || r.completedRep>lastRep_+1 ||
           now_<r.repAtMs || now_-r.repAtMs>repFreshMs)) {
            stopDrops("Rep sequence changed; drops stopped");return;
        }
        completed=r.completedRep==lastRep_+1;
        repSet_=r.repSet;lastRep_=r.completedRep;
    }
    if (dropTarget_) {
        if(completed && d.mode==DropMode::Stairs) stopDrops("Rep before confirmation; drops stopped");
        return;
    }
    if(waitingForDropSet_) return;
    bool trigger = false;
    if (d.mode == DropMode::Stairs) {
        trigger=completed && d.step!=0;
    } else {
        if(r.motionRevision==telemetryRevision_) return;
        telemetryRevision_=r.motionRevision;
        if (r.moving) { movementSeen_ = true; holdConsumed_ = false; holdSince_ = 0;holdPosition_.reset(); }
        else if (r.held && movementSeen_ && !holdConsumed_) {
            // Cumulative drift must not look stationary just because each
            // individual sample moves less than the speed threshold.
            constexpr int holdDriftMm=10;
            if(!holdSince_ || (holdPosition_ && r.positionMm && std::abs(*holdPosition_-*r.positionMm)>holdDriftMm)) {
                holdSince_=r.motionAtMs;holdPosition_=r.positionMm;
            }
            trigger = r.motionAtMs - holdSince_ >= uint64_t(d.holdSeconds)*1000 && dropsCompleted_ < d.drops && d.percent > 0;
        } else {holdSince_ = 0;holdPosition_.reset();}
    }
    if (!trigger) return;
    if(pending()) {stopDrops("Busy at drop trigger; drops stopped");return;}
    auto s = r.settings;
    s.weight = d.mode == DropMode::Stairs ? nextStairWeight(r,d.step) : nextRestPauseWeight(r,d.percent);
    if (s.weight == r.settings.weight) return;
    // This dedicated session operation copies the confirmed accessories and
    // can emit only a base write. No manual modifier trimming or load command.
    const auto result=session_.setLiveWeight(s.weight);sent(result);
    if (result==TransportResult::Accepted) {
        settingsIntent_=s;dropTarget_ = s.weight;holdConsumed_=true;movementSeen_=false;
    } else stopDrops("Drop request refused; stopped");
}
void TrainingApplication::tick(uint64_t now) {
    now_ = now; controller_.tick(now);
    if(connecting()) return;
    if(connectingDevice_ && !connected()) {
        const std::string feedback=controller_.feedback();
        message_=feedback=="Disconnected" && !session_.status().empty() ? session_.status() : feedback;
    }
    else if(connected() && message_.compare(0,14,"Connecting to ")==0) message_.clear();
    if (scanPending_ && !transport_.scanInProgress()) { scanPending_=false; refreshDiscovery(transport_.discovered()); }
    if (!scanned_ || now - scannedAt_ >= discoveryIntervalMs) scan();
    if (!connected()) {
        // Keep the transport's failed-attempt explanation. A protocol session
        // that never opened only knows "Connection lost" and would erase it.
        if(!connectingDevice_ && !selected_.empty() && !session_.status().empty()) message_=session_.status();
        restSince_.reset();restSetRevision_=0;
        dropProgress_={};
        confirmedDropWeight_.reset();
        dropStart_.reset();dropRestore_.reset();
        if(!selected_.empty()) dropSettings_[selected_].enabled=false;
        connectingDevice_.reset(); presetIntent_.reset(); settingsIntent_.reset(); observedLoaded_ = false; pending_ = requestedLoad_ = false; weightIntent_.reset(); resetDrops(); return;
    }
    if (connectingDevice_ && session_.accepted()) rememberConnection();
    const auto r = report();
    if(!fresh()) restSince_.reset();
    else {
        const bool wasDropActive=dropProgress_.active;
        dropProgress_.observe(r,drops().enabled,now_);
        if(!wasDropActive && dropProgress_.active) {
            startingWeight_=r.settings.weight;
            if(!dropRestore_) dropStart_=DropStart{r.settings.weight,r.unit};
        }
        if(dropProgress_.visible) {
            // Ignore statistics closes caused by changing base. Only a device
            // rest phase or confirmed unload starts the grouped rest clock.
            restSetRevision_=r.setEndRevision;
            if(dropProgress_.active && r.setActive!=false) restSince_.reset();
            else if((wasDropActive || dropProgress_.active) && !restSince_) restSince_=now_;
        } else {
            // User-approved fallback (Oct 24): only an observed device set-close
            // starts rest. Taps, load toggles, reconnect and settings never do.
            if(r.setEndRevision>restSetRevision_) {
                restSetRevision_=r.setEndRevision;
                if(r.setEndedAtMs<=now_) restSince_=r.setEndedAtMs;
            }
            // New actual movement in an active set ends the preceding rest period.
            if(restSince_ && r.loaded && r.setActive==true && r.motionRevision && r.motionAtMs>=*restSince_ && r.moving)
                restSince_.reset();
        }
        if(wasDropActive && !dropProgress_.active && dropProgress_.reps>0) beginDropRestore(r);
    }
    // Sessions own their bounded transaction deadlines. Do not clear intent
    // mid-transaction, and do not turn an unrelated report into a successful ACK.
    if (pending_ && !session_.busy() && (session_.commandFailed() || r.revision > sentRevision_ || now - sentAt_ >= 3000)) {
        message_ = session_.commandFailed() ? session_.status() : r.revision > sentRevision_ ? "" : "No confirmation";
        const auto matches=[](Modifier a,Modifier b) {return a.enabled==b.enabled && (!a.enabled || sameModifier(a,b));};
        if(!session_.commandFailed() && settingsIntent_ && (r.revision<=sentRevision_ || !fresh() ||
           r.settings.weight!=settingsIntent_->weight || r.settings.magicMode!=settingsIntent_->magicMode ||
           !matches(r.settings.magic,settingsIntent_->magic) || !matches(r.settings.eccentric,settingsIntent_->eccentric)))
            message_="Settings not confirmed";
        pending_ = requestedLoad_ = false; weightIntent_.reset(); settingsIntent_.reset();
        if (presetIntent_) {
            auto actual = currentPreset(); actual.drops = presetIntent_->drops;
            if (!session_.commandFailed() && r.revision > sentRevision_ && fresh() && !r.loaded &&
                envelopeHundredths(r.settings) <= r.maxWeight*100 && samePreset(actual, *presetIntent_)) applyDrops(presetIntent_->drops);
            else message_ = "Preset not confirmed";
            presetIntent_.reset();
        }
    }
    constexpr uint64_t pendingHintDelayMs = 250;
    if (pending_ && now-sentAt_ >= pendingHintDelayMs && message_.empty()) message_ = "Awaiting device";
    if (fresh() && r.loaded && !observedLoaded_) {
        startingWeight_ = r.settings.weight;
        if(!pending_) message_.clear(); // A confirmed external load resolves a prior load refusal.
    }
    if (fresh()) observedLoaded_ = r.loaded;
    runDropRestore();
    if(!dropRestore_) runDrops(r);
}
}
