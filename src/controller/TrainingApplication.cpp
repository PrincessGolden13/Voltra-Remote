#include "TrainingApplication.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
namespace voltra {
namespace {
constexpr uint64_t discoveryIntervalMs=5000;
// Three scan intervals tolerate missed radio packets; never machine readiness.
constexpr uint64_t discoveryGraceMs=15000;
constexpr size_t maximumDiscoveredDevices=64;
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
    if(const auto deviceTime=report().restSeconds) return deviceTime;
    if(!restSince_ || now_<*restSince_) return {};
    return int(std::min<uint64_t>((now_-*restSince_)/1000,359999));
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
    if (!automaticSelection_ || !scanned_ || scanPending_ || connected() || connecting()) return false;
    const auto count = std::count_if(devices_.begin(), devices_.end(), [](const auto& d) { return d.online; });
    if (!count) return false; // Keep discovering when the remote starts first.
    automaticSelection_ = false; // One startup attempt; never fight manual navigation.
    if (count != 1) return false;
    const auto it = std::find_if(devices_.begin(), devices_.end(), [](const auto& d) { return d.online; });
    return select(it->id);
}
void TrainingApplication::scan() {
    if (connected() || connecting() || scanPending_) return;
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
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == id && d.online; });
    if (it == devices_.end()) return false; // An inactive tap has no side effects.
    const DeviceEntry chosen = *it;
    leave();
    if (!controller_.connect(chosen.id)) { message_ = "Connection unavailable"; scan(); return false; }
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
    return fresh() && r.twinKnown && session_.readiness().validated && r.maxWeight > 0 && !r.autoLoadArmed && !requestedLoad_;
}
void TrainingApplication::sent(TransportResult result) {
    if (result != TransportResult::Accepted) { message_ = session_.status().empty() ? "Request not accepted" : session_.status(); return; }
    pending_ = true; sentAt_ = now_; sentRevision_ = report().revision;
    message_.clear(); // Brief requests do not flash a warning on every input.
}
bool TrainingApplication::sendSettings(TrainingSettings s) {
    const auto r = report();
    if (!settingsAllowed() || !modifierUnitsAllowed(r,s) || s.weight < 1 || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && s.magic.value == 0) || (s.eccentric.enabled && s.eccentric.value == 0) ||
        (s.magic.enabled && s.magic.value > modifierMaximum(r, s, true)) ||
        (s.eccentric.enabled && s.eccentric.value > modifierMaximum(r, s, false)) ||
        envelopeHundredths(s) > r.maxWeight * 100) { message_ = "Outside device limits"; return false; }
    const auto result = session_.setTraining(s); sent(result);
    if (result == TransportResult::Accepted) settingsIntent_ = s;
    return result == TransportResult::Accepted;
}
void TrainingApplication::adjustWeight(int delta) {
    if (presetIntent_) { message_ = "Awaiting preset confirmation"; return; }
    if (!settingsAllowed()) { message_ = "Device state unavailable"; return; }
    auto r = report();
    if (r.loaded && !r.liveWeightSupported) { message_ = "Unload to adjust"; return; }
    if(running_ || dropTarget_) stopDrops("Drops stopped for manual adjustment");
    auto s = settingsIntent_.value_or(r.settings);
    s.weight = std::clamp(weightIntent_.value_or(s.weight) + delta, 1, r.loaded ? std::max(1,baseMaximum(r)) : r.maxWeight);
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
    auto r = report();
    if (!fresh() || r.loaded || r.autoLoadArmed || requestedLoad_) {
        unload(); return;
    }
    const auto e = session_.readiness();
    if (pending_ || !settingsAllowed() || !e.ready || !e.cableZero || !e.twinSafe || r.settings.weight < 1) {
        message_ = r.settings.weight == 0 ? "Set at least "+weightText(1,r.unit)+" "+unitName(r.unit) :
            !e.reason.empty() ? e.reason : "Device not ready"; return;
    }
    const auto result = session_.setLoaded(true);
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
}
void TrainingApplication::unload() {
    // Always a cancel/unload, even before a pending load is reported. Never
    // route an editor's unload target through a toggle that could load instead.
    presetIntent_.reset(); settingsIntent_.reset(); resetDrops(); dropsCancelled_ = true;
    requestedLoad_ = false; weightIntent_.reset();
    sent(session_.setLoaded(false));
}
void TrainingApplication::autoLoad() {
    const auto r = report(); const auto e = session_.readiness();
    if (!settingsAllowed() || pending_ || r.loaded || !r.autoLoadSupported || !e.ready || !e.twinSafe || r.settings.weight < 1) {
        message_ = e.reason.empty() ? "Auto Load unavailable" : e.reason; return;
    }
    const auto result = session_.triggerAutoLoad();
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
}
void TrainingApplication::toggleModifier(bool magic) {
    const auto r = report();
    if (presetIntent_ || (pending_ && !settingsIntent_) || r.loaded ||
        !(magic ? r.magicSupported[int(r.settings.magicMode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Unload before changing mode" : "Awaiting device"; return;
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
    if (pending_ || r.loaded || std::abs(p.drops.step) > r.maxWeight) {
        message_ = "Unload / wait before applying preset"; return false;
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
    if (presetIntent_ || (pending_ && !settingsIntent_) || r.loaded || !(magic ? r.magicSupported[int(mode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Unload before applying" : "Awaiting device"; return false;
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
bool TrainingApplication::applyDrops(DropSettings d) {
    if (!fresh() || selected_.empty()) { message_ = "Device state unavailable"; return false; }
    if(d.enabled && (!report().liveWeightSupported || report().autoLoadArmed || pending())) {
        message_="Wait for weight controls before arming drops";return false;
    }
    if(dropTarget_) stopDrops("");
    d.step = std::clamp(d.step, -report().maxWeight, report().maxWeight);
    d.drops = std::clamp(d.drops, 0, 9); d.percent = std::clamp(d.percent, 0, 80);
    d.holdSeconds = std::clamp(d.holdSeconds, 1, 9);
    dropSettings_[selected_] = d; resetDrops();
    message_.clear();return true;
}
void TrainingApplication::toggleDrops() { auto d = drops(); d.enabled = !d.enabled; applyDrops(d); }
void TrainingApplication::resetDrops() {
    dropsCancelled_ = false;
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
void TrainingApplication::runDrops(const TrainingReport& r) {
    if(!drops().enabled) return;
    if(!fresh() || !r.twinKnown || !r.liveWeightSupported || r.autoLoadArmed) {
        if(wasLoaded_ || dropTarget_) stopDrops("Drops stopped: state unavailable");
        return;
    }
    if (!r.loaded) { resetDrops(); return; }
    if (dropsCancelled_) return; // A failed unload must never restart automation.
    if (!wasLoaded_) {
        wasLoaded_=true;lastRep_=r.completedRep;repSet_=r.repSet;repRevision_=r.repRevision;
        telemetryRevision_=r.motionRevision;dropSetEndRevision_=r.setEndRevision;
        waitingForDropSet_=r.setActive==false;
    }
    constexpr uint64_t motionFreshMs=500, repFreshMs=2000;
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
        if(motionFresh && r.setActive==true && r.moving && r.rep==0 &&
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
        if(running_) stopDrops("Drops stopped: motion reports lost");
        return; // Wait for first actual sample after loading, never infer a hold.
    }
    running_=!waitingForDropSet_;
    const bool newRep=r.repRevision!=repRevision_;
    bool completed=false;
    if(newRep) {
        repRevision_=r.repRevision;
        if((repSet_ && r.repSet!=repSet_) || r.completedRep<lastRep_ || r.completedRep>lastRep_+1 ||
           now_<r.repAtMs || now_-r.repAtMs>repFreshMs) {
            stopDrops("Rep sequence changed; drops stopped");return;
        }
        completed=r.completedRep==lastRep_+1;
        repSet_=r.repSet;lastRep_=r.completedRep;
    }
    if (dropTarget_) {
        if (pending()) {
            if(completed) stopDrops("Rep before confirmation; drops stopped");
            return;
        }
        if (session_.commandFailed() || !message_.empty() || r.settings.weight != *dropTarget_) {
            stopDrops("Drop unconfirmed; stopped");return;
        }
        dropTarget_.reset(); ++dropsCompleted_;
    }
    if(waitingForDropSet_) return;
    const auto d = drops(); bool trigger = false;
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
    s.weight = d.mode == DropMode::Stairs ? nextStairWeight(r,d.step) : std::max(1,s.weight * (100-d.percent)/100);
    if (s.weight == r.settings.weight) return;
    if (sendSettings(s)) {dropTarget_ = s.weight;holdConsumed_=true;movementSeen_=false;}
    else stopDrops("Drop request refused; stopped");
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
        restSince_.reset();restSetRevision_=0;
        if(!selected_.empty()) dropSettings_[selected_].enabled=false;
        connectingDevice_.reset(); presetIntent_.reset(); settingsIntent_.reset(); observedLoaded_ = false; pending_ = requestedLoad_ = false; weightIntent_.reset(); resetDrops(); return;
    }
    if (connectingDevice_ && session_.accepted()) rememberConnection();
    const auto r = report();
    if(!fresh()) restSince_.reset();
    else {
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
    runDrops(r);
}
}
