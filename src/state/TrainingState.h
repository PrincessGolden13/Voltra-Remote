#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
namespace voltra {
enum class WeightUnit { Pounds, Kilograms };
enum class MagicMode { InverseChains, Chains, Mountain };
struct Modifier {
    bool enabled = false;
    int value = 0; // Whole percent or integer device weight units.
    bool percent = false; // Independent setting, confirmed with this modifier.
};
struct TrainingSettings {
    int weight = 50; // Integer steps in the device-reported display unit.
    MagicMode magicMode = MagicMode::Chains;
    Modifier magic, eccentric;
};
inline bool sameSettings(const TrainingSettings& a, const TrainingSettings& b) {
    const auto same = [](const Modifier& x, const Modifier& y) {
        return x.enabled == y.enabled && x.value == y.value && x.percent == y.percent;
    };
    return a.weight == b.weight && a.magicMode == b.magicMode && same(a.magic,b.magic) && same(a.eccentric,b.eccentric);
}
struct TrainingReport {
    TrainingSettings settings;
    WeightUnit unit = WeightUnit::Pounds;
    bool liveModifierSupported = false; // Must be established independently of live base-weight support.
    bool modifierUnitsSelectable = false; // Unknown physical support stays disabled.
    int maxWeight = 0;
    int weightStep = 0; // Reported base-weight increment in native units; zero = unknown.
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
inline int64_t modifierHundredths(const Modifier& m, int base) {
    return m.enabled ? int64_t(m.value) * (m.percent ? base : 100) : 0;
}
inline int64_t envelopeHundredths(const TrainingSettings& s) {
    return int64_t(s.weight) * 100 + modifierHundredths(s.magic, s.weight)
        + modifierHundredths(s.eccentric, s.weight);
}
inline int modifierMaximum(const TrainingReport& r, const TrainingSettings& s, bool magic) {
    const auto& edited = magic ? s.magic : s.eccentric;
    const auto& other = magic ? s.eccentric : s.magic;
    const int64_t remaining = int64_t(r.maxWeight-s.weight) * 100 - modifierHundredths(other, s.weight);
    return int(std::max(int64_t(0), edited.percent
        ? (s.weight == 0 ? int64_t(300) : std::min(int64_t(300), remaining / s.weight))
        : remaining / 100));
}
// Preserve both modifiers' chosen values and units; only base yields.
// Mixed-unit envelope: base * (100 + percentages) + absolute * 100.
inline int baseMaximum(const TrainingReport& r, const TrainingSettings& s) {
    if (r.weightStep <= 0 || r.maxWeight <= 0 || s.magic.value < 0 || s.eccentric.value < 0) return 0;
    int64_t absolute = 0, percentages = 0;
    for (const auto* m : {&s.magic, &s.eccentric}) {
        if (m->enabled) (m->percent ? percentages : absolute) += m->value;
    }
    const int64_t available = std::max(int64_t(0), int64_t(r.maxWeight)-absolute) * 100 / (100+percentages);
    return int(available / r.weightStep * r.weightStep);
}
inline int baseMaximum(const TrainingReport& r) { return baseMaximum(r, r.settings); }
// Editing a modifier preserves base and the other active modifier. Stop at
// remaining capacity, rounding down to whole percent or the reported weight step.
inline int modifierEditMaximum(const TrainingReport& r, const TrainingSettings& s, bool magic) {
    if (r.weightStep <= 0 || r.maxWeight <= 0) return 0;
    const int step = (magic ? s.magic : s.eccentric).percent ? 1 : r.weightStep;
    return modifierMaximum(r, s, magic) / step * step;
}
inline int modifierMinimum(const TrainingReport& r, bool percent) { return percent ? 1 : std::max(1, r.weightStep); }
// Edits may lower base to reserve at least one native step for every active mode.
inline bool reserveModifiers(const TrainingReport& r, TrainingSettings& s) {
    for (auto* m : {&s.magic, &s.eccentric}) if (m->enabled) {
        const int step = modifierMinimum(r, m->percent);
        m->value = std::max(step, m->value / step * step);
    }
    const int allowed = baseMaximum(r, s);
    if (allowed < r.weightStep) return false;
    s.weight = std::clamp(s.weight, r.weightStep, allowed);
    return true;
}
enum class DropMode { Stairs, RestPause };
struct DropSettings {
    bool enabled = false;
    DropMode mode = DropMode::Stairs;
    int step = -2, drops = 2, percent = 50, holdSeconds = 1;
};
}
