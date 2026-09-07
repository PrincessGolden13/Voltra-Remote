#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
namespace voltra {
enum class WeightUnit { Pounds, Kilograms };
enum class MagicMode { InverseChains, Chains, Mountain };
struct Modifier { bool enabled = false; int value = 0; };
struct TrainingSettings {
    int weight = 50; // Integer steps in the device-reported display unit.
    MagicMode magicMode = MagicMode::Chains;
    Modifier magic, eccentric;
};
struct TrainingReport {
    TrainingSettings settings;
    WeightUnit unit = WeightUnit::Pounds;
    bool percent = true; // Device setting, never a remote-selectable option.
    int maxWeight = 0;
    std::array<bool, 3> magicSupported{};
    bool eccentricSupported = false, autoLoadSupported = false, liveWeightSupported = false;
    bool twinKnown = false, twin = false, loaded = false, autoLoadArmed = false;
    bool telemetryValid = false, moving = false, held = false;
    uint32_t rep = 0;
    uint64_t revision = 0, receivedAtMs = 0;
};
inline const char* unitName(WeightUnit unit) { return unit == WeightUnit::Pounds ? "LB" : "KG"; }
inline const char* magicName(MagicMode mode) {
    return mode == MagicMode::InverseChains ? "IC" : mode == MagicMode::Chains ? "C" : "MTN";
}
// Conservative additive envelope requested by the user. Real profiles must
// provide verified combined limits and modifier semantics before commands work.
inline int envelopeHundredths(const TrainingSettings& s, bool percent) {
    return s.weight * 100 + (s.magic.enabled ? s.magic.value * (percent ? s.weight : 100) : 0)
        + (s.eccentric.enabled ? s.eccentric.value * (percent ? s.weight : 100) : 0);
}
inline int modifierMaximum(const TrainingReport& r, const TrainingSettings& s, bool magic) {
    const auto& other = magic ? s.eccentric : s.magic;
    const int remaining = r.maxWeight * 100 - s.weight * 100
        - (other.enabled ? other.value * (r.percent ? s.weight : 100) : 0);
    return std::max(0, r.percent ? (s.weight == 0 ? 300 : std::min(300, remaining / s.weight)) : remaining / 100);
}
inline int baseMaximum(const TrainingReport& r) {
    const auto& s = r.settings;
    const int extra = (s.magic.enabled ? s.magic.value : 0) + (s.eccentric.enabled ? s.eccentric.value : 0);
    return std::max(0, r.percent ? r.maxWeight * 100 / (100 + extra) : r.maxWeight - extra);
}
enum class DropMode { Stairs, RestPause };
struct DropSettings {
    bool enabled = false;
    DropMode mode = DropMode::Stairs;
    int step = -2, drops = 2, percent = 50, holdSeconds = 1;
};
}
