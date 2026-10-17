#pragma once
#include "state/WeightUnits.h"
#include <algorithm>
#include <optional>
namespace voltra {
enum class WeightLimitSource { Unknown, DeviceReport, FirmwareFallback };
struct WeightLimit {
    int ticks = 0;
    WeightLimitSource source = WeightLimitSource::Unknown;
};
struct UnitWeightLimits {
    std::optional<int> pounds, halfKilograms;
    std::optional<int> forUnit(WeightUnit unit) const {
        const auto native = unit == WeightUnit::Pounds ? pounds : halfKilograms;
        if (native) return native; // Includes zero/invalid: never substitute a higher fallback.
        const auto other = unit == WeightUnit::Pounds ? halfKilograms : pounds;
        if (!other) return {};
        return int(std::floor(convertedWeightTicks(*other,
            unit == WeightUnit::Pounds ? WeightUnit::Kilograms : WeightUnit::Pounds, unit)));
    }
};
// A protocol adapter must establish scope and freshness before supplying reports.
// Single-unit capability fields must never be mistaken for an effective Twin limit.
struct WeightLimitEvidence {
    UnitWeightLimits single, combinedTwin;
    std::string model, firmware, peerFirmware;
    bool twinKnown = false, twin = false, combinedSemanticsKnown = false;
};
inline int firmwareFamily(const std::string& version) {
    // Only the documented 1.x and user-confirmed 2.x families. No >=2 guess.
    size_t i = !version.empty() && version.front() == 'v' ? 1 : 0;
    if (i+2 >= version.size() || (version[i] != '1' && version[i] != '2') || version[i+1] != '.') return 0;
    const int family = version[i]-'0';
    i += 2;
    if (version[i] < '0' || version[i] > '9') return 0;
    for (; i < version.size(); ++i) {
        if (version[i] == '-' || version[i] == '+') return i+1 < version.size() ? family : 0;
        if (version[i] == '.') {
            if (i+1 == version.size() || version[i+1] < '0' || version[i+1] > '9') return 0;
        } else if (version[i] < '0' || version[i] > '9') return 0;
    }
    return family;
}
inline WeightLimit resolveWeightLimit(const WeightLimitEvidence& e, WeightUnit unit) {
    if (!e.twinKnown) return {};
    const auto reported = (e.twin ? e.combinedTwin : e.single).forUnit(unit);
    if (reported) return {std::max(0, *reported), WeightLimitSource::DeviceReport};
    if (e.model != "VOLTRA I") return {};
    const int family = firmwareFamily(e.firmware);
    if (!family) return {};
    if (e.twin && (!e.combinedSemanticsKnown || e.peerFirmware != e.firmware)) return {};
    // Native unit ceilings, not rounded pound conversions. Provenance and scope:
    // docs/WEIGHT_LIMITS.md. Twin 2.x is explicitly confirmed by the user.
    constexpr int legacyPounds = 200, legacyKg = 90, v2Pounds = 230, v2Kg = 104;
    constexpr int legacyTwinPounds = 400, legacyTwinKg = 180, v2TwinPounds = 460, v2TwinKg = 208;
    const int pounds = e.twin ? (family == 2 ? v2TwinPounds : legacyTwinPounds) : (family == 2 ? v2Pounds : legacyPounds);
    const int kg = e.twin ? (family == 2 ? v2TwinKg : legacyTwinKg) : (family == 2 ? v2Kg : legacyKg);
    return {unit == WeightUnit::Pounds ? pounds : kg*ticksPerUnit(unit), WeightLimitSource::FirmwareFallback};
}
}
