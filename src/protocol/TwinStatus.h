#pragma once
#include "FrameParser.h"
#include <array>
#include <string>

namespace voltra {
// Captured EP_STATUS shape in stevenpoint/voltra_remote 1c14860, PROTOCOL.md.
// These fields describe topology only: no current-role/peer-lock decoder exists.
constexpr uint8_t twinStatusCommand=0xa7;
constexpr int twinStatusAlone=0x00, twinStatusJoining=0x11, twinStatusPaired=0x12;
using DeviceAddress=std::array<uint8_t,6>;
struct TwinStatus {
    int state=0;
    DeviceAddress own{},peer{};
    int ownBattery=0;
    std::optional<int> peerBattery;
    bool peerPresent=false;
};
// Full, validated 22-byte push or 23-byte successful direct response only.
std::optional<TwinStatus> decodeTwinStatus(const Frame&);
// RAM-only matching against the canonical address supplied by generic transport.
std::optional<DeviceAddress> parseDeviceAddress(const std::string&);
std::string formatDeviceAddress(const DeviceAddress&);
}
