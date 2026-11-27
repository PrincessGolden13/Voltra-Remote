#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <optional>
#include <cmath>
#include "WeightUnits.h"
namespace voltra {
enum class MagicMode { InverseChains, Chains, Mountain };
enum class CurrentTwinRole { Unknown, Master, Follower };
struct Modifier { bool enabled = false; int value = 0; bool percent = true; };
struct TrainingSettings {
    int weight = 50; // Display ticks: 1 lb or 0.5 kg; never wire units.
    MagicMode magicMode = MagicMode::Chains;
    Modifier magic, eccentric;
};
struct TwinDisplayCounters { int set, rep; };
// Device-reported display lifecycle. Separate from control and drop evidence.
enum class TwinWorkoutPhase : uint8_t { Unloaded=0, Loading=1, InProgress=2, Resting=3, Unloading=4 };
struct TrainingReport {
    TrainingSettings settings;
    WeightUnit unit = WeightUnit::Pounds;
    std::string firmwareVersion, twinPeerName;
    // Separate read-only presentation from valid load/control state. A known
    // weight or topology must never manufacture a control-report revision.
    bool readbackKnown=false,loadStateKnown=false;
    uint64_t readbackAtMs=0;
    bool twinDetected=false,twinStatusFresh=false,combinedWeightKnown=false;
    // Established primary route retained through TTL expiry for explicit unload
    // only. It never supplies load, settings or current-role permission.
    bool unloadRouteKnown=false;
    uint64_t twinAtMs=0;
    std::string twinPeerAddress,twinPeerId; // RAM-only identity resolution, never diagnostic output.
    std::optional<int> twinLinkState,twinPeerBattery;
    // Independent current-role evidence. Topology, selection order, historical
    // role and motor high bits cannot supply it. Zero time means unavailable.
    CurrentTwinRole currentTwinRole=CurrentTwinRole::Unknown;
    uint64_t twinRoleAtMs=0;
    // Device-reported wire weight (16003), in whole pounds. This is separate
    // from the selected base and from AA81 sensor force. Twin scope is unknown.
    std::optional<int> liveWeightLbs;
    uint64_t liveWeightAtMs=0;
    // Independent B4 device target resistance, not measured cable tension or
    // runtime register 16003. Paired target scope remains unknown.
    std::optional<int> liveTargetTenthsLb;
    uint64_t liveTargetAtMs=0;
    // Independent display feed. These counters do not imply a set boundary,
    // an active motor or permission to issue an automatic drop.
    bool separateTwinDisplayCounters=false;
    std::optional<TwinDisplayCounters> twinDisplayCounters;
    std::optional<TwinWorkoutPhase> twinWorkoutPhase;
    std::optional<bool> twinRepActive;
    // Time covered by uninterrupted idle AA81 reports. Presentation only.
    std::optional<uint64_t> twinRepIdleMs;
    std::optional<bool> repsStarted;
    std::optional<bool> setActive; // Actual device workout phase, not a local timer.
    std::optional<bool> resting; // Confirmed between-set rest, distinct from a weight-induced statistics close.
    std::optional<bool> setEnding; // Device workout phase 4: actual finish, not a statistics segment close.
    std::optional<bool> repActive; // Fresh device rep phase; idle tension is not selected weight.
    bool percentageSupported = false, modifierUnitsSelectable = false;
    bool sharedModifierUnits = false; // Device selects one interpretation for both accessories.
    int maxWeight = 0; // Effective single/Twin ceiling in display ticks, resolved by profile.
    int minWeight = 0; // Profile's effective base floor in display ticks; zero is unknown.
    int weightIncrement = 1; // Smallest absolute-setting step in display ticks; not the base floor.
    std::array<bool, 3> magicSupported{};
    bool eccentricSupported = false, autoLoadSupported = false, liveWeightSupported = false;
    bool liveModifierAmountsSupported = false; // Same curve and units; never a load/reassertion.
    // Explicit base-only dial edits while loaded and idle. Separate from the
    // automatic drop capability; a paired source must observe both channels.
    bool manualLiveWeightSupported=false;
    // Device-confirmed hold-still Auto Unload delay. Zero disables that trigger;
    // absent means unavailable. The remote never changes this configuration.
    std::optional<int> autoUnloadHoldSeconds;
    bool twinKnown = false, twin = false, loaded = false, autoLoadArmed = false;
    // Read-only Auto Load feedback: 11 waiting, 12 countdown, 13 engaging,
    // 14 single completion or 15 Twin completion on the captured profiles.
    std::optional<int> autoLoadPhase, autoLoadCountdownMs;
    uint64_t autoLoadCountdownAtMs = 0;
    bool telemetryValid = false, moving = false, held = false;
    uint32_t rep = 0;
    uint64_t revision = 0, receivedAtMs = 0;
    // Independent clocks: polling settings must never renew motion or rep data.
    uint64_t motionRevision = 0, motionAtMs = 0, repRevision = 0, repAtMs = 0;
    // Receipt of an actual set-close report, never a load/unload button event.
    uint64_t setEndRevision = 0, setEndedAtMs = 0;
    uint32_t completedRep = 0;
    // Source-reported return boundary qualified by a fresh return-to-idle
    // stream transition. False once that stationary endpoint window passes.
    bool completedRepWindow = false;
    std::optional<int> repSet;
    std::optional<int> positionMm;
    // Optional device telemetry; absent fields are omitted by the UI.
    std::optional<int> force, setNumber, completedSets, lastSetReps, restSeconds;
};
inline const char* magicName(MagicMode mode) {
    return mode == MagicMode::InverseChains ? "IC" : mode == MagicMode::Chains ? "C" : "MTN";
}
// A paired per-unit integer-LB setting has a two-LB combined step. Keep this
// quantity grid separate from the selectable minimum and effective maximum.
// Protocol adapters supply the evidenced display step; unknown control limits
// remain unknown even when the readback's increment is known.
inline int weightIncrementTicks(const TrainingReport& r) { return std::max(1,r.weightIncrement); }
inline bool weightOnIncrement(const TrainingReport& r,int ticks) {
    return ticks>=0 && ticks%weightIncrementTicks(r)==0;
}
inline int weightTicksDown(const TrainingReport& r,int ticks) {
    const int step=weightIncrementTicks(r);
    return std::max(0,ticks)/step*step;
}
inline int weightTicksUp(const TrainingReport& r,int ticks) {
    const int step=weightIncrementTicks(r),positive=std::max(0,ticks);
    return positive/step*step+(positive%step ? step : 0);
}
inline int weightTicksNearest(const TrainingReport& r,int ticks) {
    const int step=weightIncrementTicks(r),positive=std::max(0,ticks);
    return (positive/step+(positive%step*2>=step ? 1 : 0))*step;
}
// Conservative additive envelope requested by the user. Real profiles must
// provide verified combined limits and modifier semantics before commands work.
inline int envelopeHundredths(const TrainingSettings& s) {
    return s.weight * 100 + (s.magic.enabled ? s.magic.value * (s.magic.percent ? s.weight : 100) : 0)
        + (s.eccentric.enabled ? s.eccentric.value * (s.eccentric.percent ? s.weight : 100) : 0);
}
inline int modifierMaximum(const TrainingReport& r, const TrainingSettings& s, bool magic) {
    const auto& other = magic ? s.eccentric : s.magic;
    const auto& edited = magic ? s.magic : s.eccentric;
    const int remaining = r.maxWeight * 100 - s.weight * 100
        - (other.enabled ? other.value * (other.percent ? s.weight : 100) : 0);
    return edited.percent ? std::max(0,s.weight == 0 ? 300 : std::min(300, remaining / s.weight)) :
        weightTicksDown(r,remaining/100);
}
inline int baseMaximum(const TrainingReport& r) {
    int percentage = 100, absolute = 0;
    for (const auto& m : {r.settings.magic,r.settings.eccentric}) if(m.enabled) {
        if(m.percent) percentage += m.value; else absolute += m.value;
    }
    return weightTicksDown(r,(r.maxWeight-absolute)*100/percentage);
}
inline int nextStairWeight(const TrainingReport& r,int step) {
    if(!step) return r.settings.weight;
    const int ceiling=baseMaximum(r),minimum=weightTicksUp(r,r.minWeight);
    if (r.minWeight < 1 || ceiling < minimum) return r.settings.weight;
    const int requested=r.settings.weight+step;
    const int aligned=step>0 ? weightTicksUp(r,requested) : weightTicksDown(r,requested);
    const int target = std::clamp(aligned,minimum,ceiling);
    if ((step > 0 && target < r.settings.weight) || (step < 0 && target > r.settings.weight)) return r.settings.weight;
    return target;
}
inline int nextRestPauseWeight(const TrainingReport& r,int percent) {
    if(r.minWeight < 1 || r.settings.weight <= r.minWeight) return r.settings.weight;
    return std::max(weightTicksUp(r,r.minWeight),weightTicksDown(r,r.settings.weight*(100-std::clamp(percent,0,80))/100));
}
inline bool modifierUnitsAllowed(const TrainingReport& r, const TrainingSettings& s) {
    if(r.sharedModifierUnits && s.magic.percent!=s.eccentric.percent) return false;
    if ((s.magic.enabled && s.magic.percent && !r.percentageSupported) ||
        (s.eccentric.enabled && s.eccentric.percent && !r.percentageSupported)) return false;
    return r.modifierUnitsSelectable || (s.magic.percent == r.settings.magic.percent &&
        s.eccentric.percent == r.settings.eccentric.percent);
}
// A shared device selector cannot retain mixed interpretations. Convert the other
// enabled amount at the current base, to the nearest supported display step.
inline void convertModifierUnit(Modifier& m, bool percent, int base) {
    if(m.percent==percent) return;
    m.value=percent ? (base>0 ? int(std::lround(m.value*100.0/base)) : 0) :
        int(std::lround(m.value*base/100.0));
    if(m.enabled) m.value=std::max(1,m.value);
    m.percent=percent;
}
enum class DropMode { Stairs, RestPause };
struct DropSettings {
    bool enabled = false;
    DropMode mode = DropMode::Stairs;
    int step = -2, drops = 2, percent = 50, holdSeconds = 1;
};
// User policy: finish an RP pause at least one second before Auto Unload.
// Preserve the existing 1..9 s range when the device delay is off/unknown.
// A one-second Auto Unload leaves no supported RP pause (maximum zero).
inline int restPauseHoldMaximum(const TrainingReport& r) {
    constexpr int defaultMaximumSeconds=9, autoUnloadMarginSeconds=1;
    return r.autoUnloadHoldSeconds && *r.autoUnloadHoldSeconds>0 ?
        std::clamp(*r.autoUnloadHoldSeconds-autoUnloadMarginSeconds,0,defaultMaximumSeconds) : defaultMaximumSeconds;
}
}
