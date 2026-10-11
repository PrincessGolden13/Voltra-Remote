#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <optional>
namespace voltra {
enum class WeightUnit { Pounds, Kilograms };
enum class MagicMode { InverseChains, Chains, Mountain };
struct Modifier { bool enabled = false; int value = 0; bool percent = true; };
struct TrainingSettings {
    int weight = 50; // Integer steps in the device-reported display unit.
    MagicMode magicMode = MagicMode::Chains;
    Modifier magic, eccentric;
};
struct TrainingReport {
    TrainingSettings settings;
    WeightUnit unit = WeightUnit::Pounds;
    bool percentageSupported = false, modifierUnitsSelectable = false;
    int maxWeight = 0;
    std::array<bool, 3> magicSupported{};
    bool eccentricSupported = false, autoLoadSupported = false, liveWeightSupported = false;
    bool twinKnown = false, twin = false, loaded = false, autoLoadArmed = false;
    bool telemetryValid = false, moving = false, held = false;
    uint32_t rep = 0;
    uint64_t revision = 0, receivedAtMs = 0;
    // Optional device telemetry; absent fields are omitted by the UI.
    std::optional<int> force, setNumber, completedSets, lastSetReps, restSeconds;
};
inline const char* unitName(WeightUnit unit) { return unit == WeightUnit::Pounds ? "LB" : "KG"; }
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
inline bool modifierUnitsAllowed(const TrainingReport& r, const TrainingSettings& s) {
    if ((s.magic.enabled && s.magic.percent && !r.percentageSupported) ||
        (s.eccentric.enabled && s.eccentric.percent && !r.percentageSupported)) return false;
    return r.modifierUnitsSelectable || (s.magic.percent == r.settings.magic.percent &&
        s.eccentric.percent == r.settings.eccentric.percent);
}
enum class DropMode { Stairs, RestPause };
struct DropSettings {
    bool enabled = false;
    DropMode mode = DropMode::Stairs;
    int step = -2, drops = 2, percent = 50, holdSeconds = 1;
};
}
