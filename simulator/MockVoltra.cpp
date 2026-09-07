#include "MockVoltra.h"
#include "protocol/FrameParser.h"
#include "protocol/Parameters.h"
namespace voltra {
Bytes MockVoltra::notification(SimulatorProfile profile) {
    Bytes payload{uint8_t(profile == SimulatorProfile::TwinUnknown ? 5 : 4), 0};
    auto append = [&payload](uint16_t id, uint32_t value, unsigned width) {
        payload.push_back(uint8_t(id)); payload.push_back(uint8_t(id >> 8));
        for (unsigned i = 0; i < width; ++i) payload.push_back(uint8_t(value >> (8*i)));
    };
    append(parameter::baseWeight, weightLbs, 2);
    append(parameter::fitnessMode, loaded ? 5 : 4, 2);
    append(parameter::workoutState, 1, 1);
    append(parameter::battery, 80, 1);
    if (profile == SimulatorProfile::TwinUnknown) append(parameter::coworkerRemoteStatus, 1, 4);
    Bytes bytes{wire::magic, uint8_t(payload.size()+wire::minFrameBytes), 0x04, 0,
        0x10, 0xaa, uint8_t(sequence_), uint8_t(sequence_ >> 8), 0x20, 0, parameter::asyncUpdate};
    ++sequence_;
    bytes[3] = crc8(bytes.data(), 3);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    auto tail = crc16(bytes.data(), bytes.size());
    bytes.push_back(uint8_t(tail)); bytes.push_back(uint8_t(tail >> 8));
    return bytes;
}
}
