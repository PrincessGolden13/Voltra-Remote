#include "Parameters.h"
namespace voltra {
bool applyAsyncParameters(const Frame& f, uint64_t now, VoltraState& state) {
    if (f.protocol != wire::protocolId || f.sender != 0x10 || f.receiver != 0xaa ||
        f.command != parameter::asyncUpdate || f.payload.size() < 2) return false;
    auto next = state;
    const auto& p = f.payload;
    const unsigned count = p[0] | (p[1] << 8);
    size_t offset = 2;
    for (unsigned i = 0; i < count; ++i) {
        if (offset + 2 > p.size()) return false;
        const uint16_t id = p[offset] | (p[offset+1] << 8);
        offset += 2;
        size_t size;
        switch (id) {
        case parameter::baseWeight: case parameter::chains: case parameter::eccentric:
        case parameter::fitnessMode: size = 2; break;
        case parameter::battery: case parameter::legacyBattery: case parameter::workoutState: size = 1; break;
        case parameter::coworkerRemoteStatus: size = 4; break;
        default: return false;
        }
        if (offset + size > p.size()) return false;
        uint32_t value = 0;
        for (size_t j = 0; j < size; ++j) value |= uint32_t(p[offset+j]) << (8*j);
        offset += size;
        switch (id) {
        case parameter::baseWeight: next.weightLbs = {uint16_t(value), now}; break;
        case parameter::chains: next.chainsLbs = {uint16_t(value), now}; break;
        case parameter::eccentric: next.eccentricLbs = {int16_t(value <= 32767 ? int(value) : int(value)-65536), now}; break;
        case parameter::fitnessMode: next.fitnessModeRaw = {uint16_t(value), now}; break;
        case parameter::workoutState: next.workoutStateRaw = {uint8_t(value), now}; break;
        case parameter::battery: case parameter::legacyBattery:
            if (value > 100) return false;
            next.batteryPercent = {uint8_t(value), now}; break;
        case parameter::coworkerRemoteStatus: next.twin.remoteStatusRaw = {value, now}; break;
        }
    }
    if (offset != p.size()) return false;
    state = next;
    return true;
}
}
