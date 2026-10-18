#pragma once
#if defined(VOLTRA_SIMULATOR) || defined(VOLTRA_OFFLINE)
#error "A simulator target must never include a production BLE adapter"
#endif
#include "BleTransport.h"
#include <memory>
namespace voltra {
// Generic GATT only. VOLTRA UUIDs, framing, acceptance and control live elsewhere.
class NimBleTransport final : public BleTransport {
public:
    NimBleTransport();
    ~NimBleTransport() override;
    bool begin();
    void onDiagnostic(std::function<void(const std::string&)>);
    std::vector<Advertisement> scan() override;
    bool scanInProgress() const override;
    std::vector<Advertisement> discovered() const override;
    TransportResult connect(const DeviceId&) override;
    void disconnect() override;
    std::vector<std::string> discoverServices() override;
    std::vector<Characteristic> discoverCharacteristics(const std::string&) override;
    TransportResult subscribe(const std::string&) override;
    TransportResult write(const std::string&, const Bytes&) override;
    void poll(uint64_t) override;
    void onNotification(Notification) override;
    void onConnection(Connection) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
