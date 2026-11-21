#pragma once
#include "FrameParser.h"
#include <array>
#include <string>

namespace voltra {
// Captured EP_STATUS layout in stevenpoint/voltra_remote 1c14860 and the
// identity decoder in albertzhang86/voltra-remote c4f288. Identity, optional
// battery metadata and known topology semantics have independent validity.
// The exact marker-1 paired words additionally form a bounded read-only role
// profile: published controller 0x12 and owner-identified follower 0x22. This
// inference does not supply peer safety or resistance-control authorization.
constexpr uint8_t twinStatusCommand=0xa7;
constexpr int twinStatusUnknown=-1,twinStatusAlone=0x00,twinStatusJoining=0x11,twinStatusPaired=0x12;
enum class TwinStatusRole { Unknown, Master, Follower };
using DeviceAddress=std::array<uint8_t,6>;
struct TwinStatus {
    // Normalized semantic state: only consistent alone/paired is established.
    // Transitions and unrecognized values retain identity with unknown state.
    int state=twinStatusUnknown,rawState=0,marker=0;
    TwinStatusRole role=TwinStatusRole::Unknown;
    DeviceAddress own{},peer{};
    std::optional<int> ownBattery;
    std::optional<int> peerBattery;
    bool peerPresent=false;
};
// Full 22-byte push or 23-byte successful response with known type 04/08 only.
// An unknown marker/state or unavailable battery cannot erase valid identity.
std::optional<TwinStatus> decodeTwinStatus(const Frame&);
// Bounded nonidentifying rejection/shape details. Address bytes never leave this
// helper; selectedAddress is compared independently to both fixed address slots,
// even when semantic metadata is unknown. Slot position does not establish role.
std::string twinStatusDiagnostic(const Frame&,std::optional<DeviceAddress> selectedAddress={});
// RAM-only matching against the canonical address supplied by generic transport.
std::optional<DeviceAddress> parseDeviceAddress(const std::string&);
std::string formatDeviceAddress(const DeviceAddress&);
}
