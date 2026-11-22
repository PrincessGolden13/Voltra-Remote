#pragma once
#include "state/WeightUnits.h"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace voltra {
namespace device_weight_detail {
// These helpers describe the two-byte setting word, not load permission or a
// device's allowed resistance. The caller still supplies its reported limits.
constexpr int minimumWireWord=-32768,maximumWireWord=65535;
inline bool validCount(int count) { return count==1 || count==2; }
inline bool validUnit(WeightUnit unit) { return unit==WeightUnit::Pounds || unit==WeightUnit::Kilograms; }
inline int64_t halfKilograms(int pounds,bool baseWeight) {
    if(baseWeight && pounds==5) return 4; // Native base minimum: 5 LB / 2 KG.
    const auto magnitude=pounds<0 ? -int64_t(pounds) : int64_t(pounds);
    const auto ticks=(magnitude*20+11)/22;
    return pounds<0 ? -ticks : ticks;
}
}

// Scoped VOLTRA setting-display profile, not SI recipe conversion. Evidence:
// albertzhang86/voltra-remote c4f2884798941babd83788cf79609620e767bc85,
// app/remote_model.h:13-28 and VALIDATION.md:176-188. Device readings include
// 61 LB -> 27.5 KG, 82/83 -> 37.5 KG, and 179 LB -> 81.5 KG per unit.
// The native half-KG grid uses 2.2 LB/KG, with the separate base-5 minimum.
// A supported pair sums the already-rounded unit displays: 179 LB -> 163 KG.
// Call only after the adapter establishes this setting scope and unit count.
// Unknown motor state, topology or bounds cannot be made valid by conversion.
inline int deviceWeightTicks(int perUnitPounds,WeightUnit unit,int unitCount,bool baseWeight=false) {
    if(!device_weight_detail::validCount(unitCount) || !device_weight_detail::validUnit(unit) ||
       perUnitPounds<device_weight_detail::minimumWireWord || perUnitPounds>device_weight_detail::maximumWireWord) return 0;
    const auto ticks=unit==WeightUnit::Pounds ? int64_t(perUnitPounds) :
        device_weight_detail::halfKilograms(perUnitPounds,baseWeight);
    return int(ticks*unitCount);
}

// Encode a requested combined display value exactly. In particular, never
// convert the combined KG number to LB and then divide: unit rounding differs.
// Clamp the preferred inverse to the actual wire bounds before round-tripping;
// a reported maximum can share a display bucket with an out-of-range inverse
// (e.g. maximum 16 LB displays 7.5 KG, whose preferred inverse is 17 LB).
inline std::optional<int> encodeDeviceWeight(int totalDisplayTicks,WeightUnit unit,int unitCount,
                                              int minimumWire,int maximumWire,bool baseWeight=false) {
    if(!device_weight_detail::validCount(unitCount) || !device_weight_detail::validUnit(unit) ||
       minimumWire>maximumWire || minimumWire<device_weight_detail::minimumWireWord ||
       maximumWire>device_weight_detail::maximumWireWord || totalDisplayTicks%unitCount) return {};
    const auto perUnitTicks=int64_t(totalDisplayTicks)/unitCount;
    const auto magnitude=perUnitTicks<0 ? -perUnitTicks : perUnitTicks;
    const auto roundedPounds=(magnitude*11+5)/10;
    const auto preferred=unit==WeightUnit::Pounds ? perUnitTicks :
        (perUnitTicks<0 ? -roundedPounds : roundedPounds);
    const int pounds=int(std::clamp<int64_t>(preferred,minimumWire,maximumWire));
    if(deviceWeightTicks(pounds,unit,unitCount,baseWeight)!=totalDisplayTicks) return {};
    return pounds;
}
}
