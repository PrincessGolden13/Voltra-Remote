#include "NimBleTransport.h"
#include <NimBLEDevice.h>
#include <deque>
#include <mutex>
#include <map>
#include <atomic>
#include <algorithm>
namespace voltra {
namespace {
// Radio-link policy only: never retry a protocol or resistance command.
constexpr uint32_t connectionTimeoutMs=10000, connectionRetryDelayMs=500, mtuWaitMs=1000;
constexpr unsigned connectionAttempts=3;
constexpr int transmitPowerDbm=9;
constexpr int establishmentFailure=BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT);
}
struct NimBleTransport::Impl : NimBLEScanCallbacks, NimBLEClientCallbacks {
    NimBLEClient* client=nullptr;
    NimBLEScan* scanner=nullptr;
    Notification notification;
    Connection connection;
    std::function<void(const std::string&)> diagnostic;
    void log(const std::string& message) const { if(diagnostic) diagnostic(message); }
    std::map<std::string,NimBLERemoteCharacteristic*> characteristics;
    struct Received { uint32_t generation; std::string uuid; Bytes bytes; };
    std::mutex mutex;
    std::map<DeviceId,Advertisement> advertisements;
    struct Signal { int rssi=0; bool connectable=false; };
    std::map<DeviceId,Signal> signals;
    void onDiscovered(const NimBLEAdvertisedDevice* d) override { onResult(d); }
    void onResult(const NimBLEAdvertisedDevice* d) override {
        // NimBLE can replace a previous scan response with a later advertisement.
        // Retain the union observed within this scan, not just its final packet.
        Advertisement a;
        a.id=d->getAddress().toString()+"/"+std::to_string(d->getAddressType());
        a.name=d->getName();
        for(unsigned n=0;n<d->getServiceUUIDCount();++n) a.services.push_back(d->getServiceUUID(n).toString());
        std::lock_guard<std::mutex> lock(mutex);
        if(advertisements.size()>=64 && !advertisements.count(a.id)) return;
        auto& saved=advertisements[a.id];saved.id=a.id;
        signals[a.id]={d->getRSSI(),d->isConnectable()};
        if(!a.name.empty()) saved.name=a.name;
        for(const auto& uuid:a.services)
            if(std::find(saved.services.begin(),saved.services.end(),uuid)==saved.services.end()) saved.services.push_back(uuid);
    }
    std::deque<Received> received;
    std::atomic<bool> overflow{false};
    uint32_t generation=0;
    bool connected=false;
    unsigned diagnosticRx=0,diagnosticTx=0;
    bool connecting=false, radioBusy=false, linkObserved=false, cancelling=false;
    NimBLEAddress target;
    unsigned attempts=0;
    uint64_t now=0, retryAt=0, attemptAt=0, linkUpAt=0;
    // Stack callbacks only publish events. Application callbacks run in poll().
    std::atomic<int> linkEvent{0}; // 1=link up, negative=NimBLE failure reason.
    std::atomic<bool> mtuReady{false};
    void onConnect(NimBLEClient*) override { linkEvent=1; }
    void onConnectFail(NimBLEClient*,int reason) override { linkEvent=-std::max(1,reason); }
    void onDisconnect(NimBLEClient*,int reason) override { linkEvent=-std::max(1,reason); }
    void onMTUChange(NimBLEClient*,uint16_t) override { mtuReady=true; }
};
NimBleTransport::NimBleTransport() : impl_(std::make_unique<Impl>()) {}
NimBleTransport::~NimBleTransport() { disconnect(); if(impl_->client) NimBLEDevice::deleteClient(impl_->client); }
bool NimBleTransport::begin() {
    if(!NimBLEDevice::init("VOLTRA Remote")) return false;
    // ESP32-S3 radio range policy: request +9 dBm.
    // This improves the remote's link budget, not the peripheral's RSSI.
    if(!NimBLEDevice::setPower(transmitPowerDbm)) impl_->log("BLE transmit power request unavailable");
    NimBLEDevice::setMTU(247); // Same requested MTU as the pinned ESP32 reference.
    impl_->scanner=NimBLEDevice::getScan();
    impl_->scanner->setScanCallbacks(impl_.get());
    impl_->scanner->setActiveScan(true);
    impl_->scanner->setInterval(100); impl_->scanner->setWindow(80);
    impl_->scanner->setMaxResults(64);
    impl_->client=NimBLEDevice::createClient();
    if(impl_->client) {
        impl_->client->setConnectTimeout(connectionTimeoutMs);
        // 15–30 ms connection interval, zero latency, 4 s supervision timeout.
        // Generic BLE timing; VOLTRA may negotiate a different interval.
        impl_->client->setConnectionParams(12,24,0,400);
        impl_->client->setClientCallbacks(impl_.get(),false);
    }
    return impl_->client!=nullptr;
}
std::vector<Advertisement> NimBleTransport::scan() {
    if(!impl_->scanner || impl_->connected || impl_->connecting || impl_->radioBusy || impl_->scanner->isScanning()) return {};
    impl_->scanner->clearResults();
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->advertisements.clear();impl_->signals.clear();}
    const bool started=impl_->scanner->start(2000,false,true); // Async: complete window before auto-selection.
    if(!started) impl_->log("BLE scan could not start");
    return {};
}
bool NimBleTransport::scanInProgress() const { return impl_->scanner && impl_->scanner->isScanning(); }
std::vector<Advertisement> NimBleTransport::discovered() const {
    std::vector<Advertisement> found;
    if(!impl_->scanner || impl_->scanner->isScanning()) return found;
    {std::lock_guard<std::mutex> lock(impl_->mutex);for(const auto& pair:impl_->advertisements) found.push_back(pair.second);}
    impl_->log("BLE scan complete: "+std::to_string(found.size())+" advertisers");
    for(const auto& a:found) for(const auto& uuid:a.services) impl_->log("BLE advertised service: "+uuid);
    return found;
}
TransportResult NimBleTransport::connect(const DeviceId& id) {
    if(!impl_->client) return TransportResult::Unsupported;
    const auto separator=id.find('/');
    if(separator!=17 || id.size()!=19 || id[18]<'0' || id[18]>'3') return TransportResult::Failed;
    if(impl_->scanner->isScanning()) impl_->scanner->stop();
    disconnect();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it=impl_->signals.find(id);
        if(it!=impl_->signals.end()) impl_->log("BLE target RSSI="+std::to_string(it->second.rssi)+" connectable="+std::to_string(it->second.connectable)+" addressType="+id.substr(18));
    }
    impl_->target=NimBLEAddress(id.substr(0,17),uint8_t(id[18]-'0'));
    impl_->connecting=true;impl_->attempts=0;impl_->retryAt=impl_->now;
    // Accepted means queued; onConnection reports completion/failure later.
    return TransportResult::Accepted;
}
void NimBleTransport::disconnect() {
    impl_->connecting=false;
    if(impl_->radioBusy) impl_->cancelling=true;
    if(impl_->scanner && impl_->scanner->isScanning()) impl_->scanner->stop();
    if(impl_->client && impl_->client->isConnected()) impl_->client->disconnect();
    else if(impl_->client && impl_->radioBusy) impl_->client->cancelConnect();
    ++impl_->generation;
    { std::lock_guard<std::mutex> lock(impl_->mutex); impl_->received.clear(); }
    impl_->overflow=false;
    impl_->diagnosticRx=impl_->diagnosticTx=0;
    impl_->characteristics.clear();
    const bool was=impl_->connected; impl_->connected=false;
    if(was) impl_->log("BLE disconnected");
    if(was && impl_->connection) impl_->connection(false);
}
std::vector<std::string> NimBleTransport::discoverServices() {
    std::vector<std::string> result;
    if(impl_->client && impl_->client->isConnected())
        // NimBLE 2.x defaults to cached-only; true performs actual discovery.
        for(const auto* s:impl_->client->getServices(true)) result.push_back(s->getUUID().toString());
    impl_->log("BLE services: "+std::to_string(result.size()));
    return result;
}
std::vector<Characteristic> NimBleTransport::discoverCharacteristics(const std::string& service) {
    std::vector<Characteristic> result;
    if(!impl_->client || !impl_->client->isConnected()) return result;
    const auto* s=impl_->client->getService(service.c_str());
    if(s) for(auto* c:s->getCharacteristics(true)) {
        const auto uuid=c->getUUID().toString(); impl_->characteristics[uuid]=c;
        result.push_back({uuid,c->canNotify(),c->canWrite() || c->canWriteNoResponse()});
        impl_->log("BLE characteristic "+uuid+" notify="+std::to_string(c->canNotify())+" write="+std::to_string(c->canWrite())+" writeNoResponse="+std::to_string(c->canWriteNoResponse()));
    }
    return result;
}
TransportResult NimBleTransport::subscribe(const std::string& uuid) {
    const auto it=impl_->characteristics.find(uuid);
    if(it==impl_->characteristics.end() || !it->second->canNotify()) return TransportResult::Unsupported;
    // NimBLE subscribe() returns true even when no CCCD is found. A callback
    // alone cannot prove notifications were enabled on the peripheral.
    if(!it->second->getDescriptor(NimBLEUUID(uint16_t(0x2902)))) {
        impl_->log("BLE notification descriptor missing: "+uuid);return TransportResult::Failed;
    }
    const auto generation=impl_->generation;
    const bool ok=it->second->subscribe(true,[this,uuid,generation](NimBLERemoteCharacteristic*,uint8_t* data,size_t size,bool) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Overflow invalidates the session; losing state must never preserve readiness.
        if(size>517 || impl_->received.size()>=32) { impl_->overflow=true; return; }
        impl_->received.push_back({generation,uuid,Bytes(data,data+size)});
    });
    impl_->log(std::string(ok ? "BLE subscribed " : "BLE subscription failed ")+uuid);
    return ok ? TransportResult::Accepted : TransportResult::Failed;
}
TransportResult NimBleTransport::write(const std::string& uuid,const Bytes& bytes) {
    if(!impl_->client || !impl_->client->isConnected()) return TransportResult::Disconnected;
    const auto it=impl_->characteristics.find(uuid);
    if(it==impl_->characteristics.end()) return TransportResult::Unsupported;
    const bool ok=it->second->writeValue(bytes.data(),bytes.size(),it->second->canWrite());
    if(impl_->diagnosticTx++<8) impl_->log("BLE write bytes="+std::to_string(bytes.size())+" accepted="+std::to_string(ok));
    return ok ? TransportResult::Accepted : TransportResult::Failed;
}
void NimBleTransport::poll(uint64_t now) {
    impl_->now=now;
    const int event=impl_->linkEvent.exchange(0);
    if(event<0) {
        impl_->radioBusy=false;impl_->linkObserved=false;impl_->cancelling=false;
        impl_->log("BLE link ended: "+std::to_string(-event));
        if(impl_->connected) { disconnect();return; }
        if(impl_->connecting && impl_->attempts) {
            if(-event==establishmentFailure && impl_->attempts<connectionAttempts) {
                impl_->retryAt=now+connectionRetryDelayMs;
                impl_->log("BLE establishment retry scheduled");
            } else {
                impl_->connecting=false;
                if(impl_->connection) impl_->connection(false);
            }
        }
    } else if(event==1 && !impl_->cancelling) { impl_->linkObserved=true;impl_->linkUpAt=now; }
    // Cancellation must finish before reusing this client for another attempt.
    // A late link-up after cancel never becomes an application connection.
    if((impl_->cancelling || (!impl_->connecting && !impl_->connected)) && impl_->client && impl_->client->isConnected()) impl_->client->disconnect();
    if(impl_->connecting) {
        if(!impl_->radioBusy && now>=impl_->retryAt) {
            impl_->mtuReady=false;impl_->linkObserved=false;impl_->radioBusy=true;
            impl_->attemptAt=now;++impl_->attempts;
            impl_->log("BLE connecting attempt "+std::to_string(impl_->attempts)+"/"+std::to_string(connectionAttempts));
            if(!impl_->client->connect(impl_->target,true,true,true)) impl_->linkEvent=-std::max(1,impl_->client->getLastError());
        }
        if(!impl_->cancelling && impl_->attempts && impl_->linkObserved && impl_->client->isConnected() && (impl_->mtuReady || now-impl_->linkUpAt>=mtuWaitMs)) {
            impl_->connecting=false;impl_->connected=true;
            impl_->log("BLE connected MTU="+std::to_string(impl_->client->getMTU()));
            if(impl_->connection) impl_->connection(true);
        } else if(impl_->attempts && impl_->radioBusy && now-impl_->attemptAt>connectionTimeoutMs+mtuWaitMs) {
            impl_->log("BLE connection timed out");
            disconnect();
            if(impl_->connection) impl_->connection(false);
        }
    }
    if(impl_->connected && (!impl_->client->isConnected() || impl_->overflow)) { disconnect(); return; }
    std::deque<Impl::Received> pending;
    { std::lock_guard<std::mutex> lock(impl_->mutex); pending.swap(impl_->received); }
    for(const auto& r:pending)
        if(impl_->connected && r.generation==impl_->generation && impl_->notification) {
            if(impl_->diagnosticRx++<12) impl_->log("BLE received "+r.uuid+" bytes="+std::to_string(r.bytes.size()));
            impl_->notification(r.uuid,r.bytes);
        }
}
void NimBleTransport::onNotification(Notification callback) { impl_->notification=std::move(callback); }
void NimBleTransport::onConnection(Connection callback) { impl_->connection=std::move(callback); }
void NimBleTransport::onDiagnostic(std::function<void(const std::string&)> callback) { impl_->diagnostic=std::move(callback); }
}
