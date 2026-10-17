#pragma once
#include "ble/BleTypes.h"
namespace voltra {
struct DeviceEntry { DeviceId id; std::string name; bool online = false; std::string twinPeer; };
class DeviceMemory {
public:
    virtual ~DeviceMemory() = default;
    virtual std::vector<DeviceEntry> load() = 0;
    virtual bool save(const std::vector<DeviceEntry>& recent) = 0;
};
}
