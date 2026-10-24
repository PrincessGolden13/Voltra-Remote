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
struct Modifier { bool enabled = false; int value = 0; bool percent = true; };
struct TrainingSettings {
    int weight = 50; // Display ticks: 1 lb or 0.5 kg; never wire units.
    MagicMode magicMode = MagicMode::Chains;
    Modifier magic, eccentric;
};
struct TrainingReport {
    TrainingSettings settings;
    WeightUnit unit = WeightUnit::Pounds;
    std::string firmwareVersion, twinPeerName;
    std::optional<bool> repsStarted;
    std::optional<bool> setActive; // Actual device workout phase, not a local timer.
    std::optional<bool> repActive; // Fresh device rep phase; idle tension is not selected weight.
    bool percentageSupported = false, modifierUnitsSelectable = false;
    bool sharedModifierUnits = false; // Device selects one interpretation for both accessories.
    int maxWeight = 0; // Effective single/Twin ceiling in display ticks, resolved by profile.
    int minWeight = 0; // Profile's effective base floor in display ticks; zero is unknown.
    std::array<bool, 3> magicSupported{};
    bool eccentricSupported = false, autoLoadSupported = false, liveWeightSupported = false;
    bool twinKnown = false, twin = false, loaded = false, autoLoadArmed = false;
    bool telemetryValid = false, moving = false, held = false;
    uint32_t rep = 0;
    uint64_t revision = 0, receivedAtMs = 0;
    // Independent clocks: polling settings must never renew motion or rep data.
    uint64_t motionRevision = 0, motionAtMs = 0, repRevision = 0, repAtMs = 0;
    // Receipt of an actual set-close report, never a load/unload button event.
    uint64_t setEndRevision = 0, setEndedAtMs = 0;
    uint32_t completedRep = 0;
    std::optional<int> repSet;
    std::optional<int> positionMm;
    // Optional device telemetry; absent fields are omitted by the UI.
    std::optional<int> force, setNumber, completedSets, lastSetReps, restSeconds;
};
inline const char* magicName(MagicMode mode) {
    return mode == MagicMode::InverseChains ? "IC" : mode == MagicMode::Chains ? "C" : "MTN";
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
    return std::max(0, edited.percent ? (s.weight == 0 ? 300 : std::min(300, remaining / s.weight)) : remaining / 100);
}
inline int baseMaximum(const TrainingReport& r) {
    int percentage = 100, absolute = 0;
    for (const auto& m : {r.settings.magic,r.settings.eccentric}) if(m.enabled) {
        if(m.percent) percentage += m.value; else absolute += m.value;
    }
    return std::max(0,(r.maxWeight-absolute)*100/percentage);
}
inline int nextStairWeight(const TrainingReport& r,int step) {
    const int ceiling = baseMaximum(r);
    if (r.minWeight < 1 || ceiling < r.minWeight) return r.settings.weight;
    const int target = std::clamp(r.settings.weight + step,r.minWeight,ceiling);
    if ((step > 0 && target < r.settings.weight) || (step < 0 && target > r.settings.weight)) return r.settings.weight;
    return target;
}
inline int nextRestPauseWeight(const TrainingReport& r,int percent) {
    if(r.minWeight < 1 || r.settings.weight <= r.minWeight) return r.settings.weight;
    return std::max(r.minWeight,r.settings.weight*(100-std::clamp(percent,0,80))/100);
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
}
