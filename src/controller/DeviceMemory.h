#pragma once
#include "ble/BleTypes.h"
namespace voltra {
// Bounded local metadata storage; never silently evict previously added devices.
inline constexpr size_t maximumRememberedDevices = 64;
struct DeviceEntry {
    DeviceId id;
    std::string name;
    bool online = false;
    std::string twinPeer;
    // Machine-observed navigation relationship; never pairing configuration.
    // Storage may retain IDs/names, but freshness/availability always restart false.
    DeviceId twinPeerId{};
    bool twinFresh = false, twinPeerAvailable = false;
    // One navigation endpoint for the observed pair; not current role/readiness.
    DeviceId twinSelectionId{};
};
class DeviceMemory {
public:
    virtual ~DeviceMemory() = default;
    virtual std::vector<DeviceEntry> load() = 0;
    virtual bool save(const std::vector<DeviceEntry>& recent) = 0;
};
}
