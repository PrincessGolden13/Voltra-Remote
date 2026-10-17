#pragma once
#include <cmath>
#include <string>
namespace voltra {
enum class WeightUnit { Pounds, Kilograms };
// Application display ticks, never protocol/wire units. Keep fractional kg exact.
inline int ticksPerUnit(WeightUnit u) { return u == WeightUnit::Kilograms ? 2 : 1; }
inline std::string weightText(int ticks, WeightUnit u) {
    if (u == WeightUnit::Pounds) return std::to_string(ticks);
    return (ticks < 0 ? "-" : "") + std::to_string(std::abs(ticks)/2) + (std::abs(ticks)%2 ? ".5" : "");
}
inline const char* unitName(WeightUnit u) { return u == WeightUnit::Pounds ? "LB" : "KG"; }
inline double convertedWeightTicks(int value, WeightUnit from, WeightUnit to) {
    if (from == to) return value;
    constexpr double kilogramsPerPound = 0.45359237;
    return value / double(ticksPerUnit(from)) *
        (to == WeightUnit::Kilograms ? kilogramsPerPound : 1.0/kilogramsPerPound) * ticksPerUnit(to);
}
// Recipes round to the nearest display tick. Limits always round DOWN.
inline int convertWeightUnits(int value, WeightUnit from, WeightUnit to) {
    return int(std::lround(convertedWeightTicks(value, from, to)));
}
}
