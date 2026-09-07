#pragma once
#if defined(VOLTRA_SIMULATOR) || defined(VOLTRA_OFFLINE)
#error "A simulator target must never include a production BLE adapter"
#endif
#include "BleTransport.h"
namespace voltra {
// Deliberately inert adapter scaffold. No NimBLE headers or radio initialization.
// Implement scan/read-only connection after board identification and bootstrap audit.
class NimBleTransport final : public BleTransport {
public:
    std::vector<Advertisement> scan() override { return {}; }
    TransportResult connect(const DeviceId&) override { return TransportResult::Unsupported; }
    void disconnect() override {}
    std::vector<std::string> discoverServices() override { return {}; }
    std::vector<Characteristic> discoverCharacteristics(const std::string&) override { return {}; }
    TransportResult subscribe(const std::string&) override { return TransportResult::Unsupported; }
    TransportResult write(const std::string&, const Bytes&) override { return TransportResult::Unsupported; }
    void poll(uint64_t) override {}
    void onNotification(Notification) override {}
    void onConnection(Connection) override {}
};
}
