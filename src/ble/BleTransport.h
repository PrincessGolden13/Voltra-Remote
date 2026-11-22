#pragma once
#include "BleTypes.h"
#include <functional>
namespace voltra {
enum class BleConnectionStage { None, Setup, Teardown, Radio, Services, Characteristics, Subscription, Link, Notifications };
struct BleConnectionFailure {
    BleConnectionStage stage=BleConnectionStage::None;
    int reason=0; // A reported host/controller code; zero means unavailable.
    bool timedOut=false;
    unsigned attempts=0;
    uint64_t elapsedMs=0; // Radio budget elapsed at failure, before app handoff.
    int logicalAddressType=-1, radioAddressType=-1;
    bool observedRadioType=false; // Actual type learned from this boot's advertisements.
};
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
    // Optional canonical radio address for matching observed device metadata.
    // Only the adapter interprets its opaque DeviceId format.
    virtual std::string deviceAddress(const DeviceId&) const { return {}; }
    virtual BleConnectionFailure connectionFailure() const { return {}; }
    // Accepted may mean queued. onConnection(true/false) delivers link outcome;
    // neither acceptance nor a link establishes a VOLTRA protocol session.
    virtual TransportResult connect(const DeviceId&) = 0;
    virtual void disconnect() = 0;
    virtual std::vector<std::string> discoverServices() = 0;
    // UUID-filtered discovery avoids enumerating unrelated services before a
    // time-sensitive protocol handshake. Adapters may provide a direct query.
    virtual bool discoverService(const std::string& uuid) {
        for(const auto& service:discoverServices()) if(service==uuid) return true;
        return false;
    }
    virtual std::vector<Characteristic> discoverCharacteristics(const std::string&) = 0;
    virtual TransportResult subscribe(const std::string&) = 0;
    virtual TransportResult write(const std::string&, const Bytes&) = 0;
    virtual void poll(uint64_t nowMs) = 0;
    virtual void onNotification(Notification callback) = 0;
    virtual void onConnection(Connection callback) = 0;
};
}
