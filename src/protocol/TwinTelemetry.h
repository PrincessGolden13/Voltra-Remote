#pragma once
#include "FrameParser.h"

namespace voltra {
// One complete native Twin B4 sample. All six motion fields belong to the same
// packet; decode failure never yields a partial channel or cached fallback.
// Provenance: albertzhang86/voltra-remote c4f2884798941babd83788cf79609620e767bc85,
// voltra_protocol.cpp native B4 layout and test_protocol.cpp real Twin captures.
// Forces are measurements in 0.1 lb, positions are mm, and velocities signed
// mm/s. The word between the local and other channel is deliberately ignored:
// it is not established as a combined target. Summing measured forces does not
// produce the VOLTRA's selected/Live target. These fields provide no motor,
// readiness, phase or resistance-control authorization.
struct TwinRuntime {
    int localForceTenthsLb=0,peerForceTenthsLb=0;
    int localPositionMm=0,peerPositionMm=0;
    int localVelocityMmPerSecond=0,peerVelocityMmPerSecond=0;
    std::optional<uint16_t> setCount,repCount;
};
// Accept only captured sources 0x10/0x11/0x12 and exact complete lengths. Their
// mapping is crossed: 0x12 has set/rep counts; 0x11's extra words are ranges.
// Standalone B4, extended AA81 and unknown variants remain separate profiles.
std::optional<TwinRuntime> decodeTwinRuntime(const Frame&);
// Extended Twin AA81 exposes the reported counters and rep phase for display.
// It supplies no target or resistance-control evidence.
struct TwinReportedCounters { uint16_t set, rep; uint8_t phase; };
std::optional<TwinReportedCounters> decodeExtendedTwinCounters(const Frame&);
}
