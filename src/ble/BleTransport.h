#pragma once
#include "BleTypes.h"
#include <functional>
namespace voltra {
// One transport session per controller. Callbacks execute on the application loop;
// future radio adapters must marshal stack callbacks before delivery.
class BleTransport {
public:
    using Notification = std::function<void(const std::string&, const Bytes&)>;
    using Connection = std::function<void(bool)>;
    virtual ~BleTransport() = default;
    virtual std::vector<Advertisement> scan() = 0;
    // Asynchronous adapters expose only a completed scan window to the app.
    virtual bool scanInProgress() const { return false; }
    virtual std::vector<Advertisement> discovered() const { return {}; }
    virtual TransportResult connect(const DeviceId&) = 0;
    virtual void disconnect() = 0;
    virtual std::vector<std::string> discoverServices() = 0;
    virtual std::vector<Characteristic> discoverCharacteristics(const std::string&) = 0;
    virtual TransportResult subscribe(const std::string&) = 0;
    virtual TransportResult write(const std::string&, const Bytes&) = 0;
    virtual void poll(uint64_t nowMs) = 0;
    virtual void onNotification(Notification callback) = 0;
    virtual void onConnection(Connection callback) = 0;
};
}
