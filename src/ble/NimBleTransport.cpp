#include "NimBleTransport.h"
#include <NimBLEDevice.h>
#include <deque>
#include <mutex>
#include <map>
#include <atomic>
#include <algorithm>
namespace voltra {
struct NimBleTransport::Impl : NimBLEScanCallbacks {
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
        if(!a.name.empty()) saved.name=a.name;
        for(const auto& uuid:a.services)
            if(std::find(saved.services.begin(),saved.services.end(),uuid)==saved.services.end()) saved.services.push_back(uuid);
    }
    std::deque<Received> received;
    std::atomic<bool> overflow{false};
    uint32_t generation=0;
    bool connected=false;
};
NimBleTransport::NimBleTransport() : impl_(std::make_unique<Impl>()) {}
NimBleTransport::~NimBleTransport() { disconnect(); if(impl_->client) NimBLEDevice::deleteClient(impl_->client); }
bool NimBleTransport::begin() {
    if(!NimBLEDevice::init("VOLTRA Remote")) return false;
    NimBLEDevice::setMTU(517);
    impl_->scanner=NimBLEDevice::getScan();
    impl_->scanner->setScanCallbacks(impl_.get());
    impl_->scanner->setActiveScan(true);
    impl_->scanner->setInterval(100); impl_->scanner->setWindow(80);
    impl_->scanner->setMaxResults(64);
    impl_->client=NimBLEDevice::createClient();
    if(impl_->client) impl_->client->setConnectTimeout(5000);
    return impl_->client!=nullptr;
}
std::vector<Advertisement> NimBleTransport::scan() {
    if(!impl_->scanner || impl_->connected || impl_->scanner->isScanning()) return {};
    impl_->scanner->clearResults();
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->advertisements.clear();}
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
    impl_->log("BLE connecting");
    if(!impl_->client->connect(NimBLEAddress(id.substr(0,17),uint8_t(id[18]-'0')),true)) {
        impl_->log("BLE connection failed: "+std::to_string(impl_->client->getLastError()));
        return TransportResult::Failed;
    }
    impl_->log("BLE connected");
    impl_->connected=true;
    if(impl_->connection) impl_->connection(true);
    return TransportResult::Accepted;
}
void NimBleTransport::disconnect() {
    if(impl_->scanner && impl_->scanner->isScanning()) impl_->scanner->stop();
    if(impl_->client && impl_->client->isConnected()) impl_->client->disconnect();
    ++impl_->generation;
    { std::lock_guard<std::mutex> lock(impl_->mutex); impl_->received.clear(); }
    impl_->overflow=false;
    impl_->characteristics.clear();
    const bool was=impl_->connected; impl_->connected=false;
    if(was) impl_->log("BLE disconnected");
    if(was && impl_->connection) impl_->connection(false);
}
std::vector<std::string> NimBleTransport::discoverServices() {
    std::vector<std::string> result;
    if(impl_->client && impl_->client->isConnected())
        for(const auto* s:impl_->client->getServices()) result.push_back(s->getUUID().toString());
    impl_->log("BLE services: "+std::to_string(result.size()));
    return result;
}
std::vector<Characteristic> NimBleTransport::discoverCharacteristics(const std::string& service) {
    std::vector<Characteristic> result;
    if(!impl_->client || !impl_->client->isConnected()) return result;
    const auto* s=impl_->client->getService(service.c_str());
    if(s) for(auto* c:s->getCharacteristics()) {
        const auto uuid=c->getUUID().toString(); impl_->characteristics[uuid]=c;
        result.push_back({uuid,c->canNotify(),c->canWrite() || c->canWriteNoResponse()});
        impl_->log("BLE characteristic "+uuid+" notify="+std::to_string(c->canNotify())+" write="+std::to_string(c->canWrite())+" writeNoResponse="+std::to_string(c->canWriteNoResponse()));
    }
    return result;
}
TransportResult NimBleTransport::subscribe(const std::string& uuid) {
    const auto it=impl_->characteristics.find(uuid);
    if(it==impl_->characteristics.end() || !it->second->canNotify()) return TransportResult::Unsupported;
    const auto generation=impl_->generation;
    const bool ok=it->second->subscribe(true,[this,uuid,generation](NimBLERemoteCharacteristic*,uint8_t* data,size_t size,bool) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Overflow invalidates the session; losing state must never preserve readiness.
        if(size>517 || impl_->received.size()>=32) { impl_->overflow=true; return; }
        impl_->received.push_back({generation,uuid,Bytes(data,data+size)});
    });
    impl_->log(ok ? "BLE subscribed" : "BLE subscription failed");
    return ok ? TransportResult::Accepted : TransportResult::Failed;
}
TransportResult NimBleTransport::write(const std::string& uuid,const Bytes& bytes) {
    if(!impl_->client || !impl_->client->isConnected()) return TransportResult::Disconnected;
    const auto it=impl_->characteristics.find(uuid);
    if(it==impl_->characteristics.end()) return TransportResult::Unsupported;
    return it->second->writeValue(bytes.data(),bytes.size(),it->second->canWrite()) ? TransportResult::Accepted : TransportResult::Failed;
}
void NimBleTransport::poll(uint64_t) {
    if(impl_->connected && (!impl_->client->isConnected() || impl_->overflow)) { disconnect(); return; }
    std::deque<Impl::Received> pending;
    { std::lock_guard<std::mutex> lock(impl_->mutex); pending.swap(impl_->received); }
    for(const auto& r:pending)
        if(impl_->connected && r.generation==impl_->generation && impl_->notification) impl_->notification(r.uuid,r.bytes);
}
void NimBleTransport::onNotification(Notification callback) { impl_->notification=std::move(callback); }
void NimBleTransport::onConnection(Connection callback) { impl_->connection=std::move(callback); }
void NimBleTransport::onDiagnostic(std::function<void(const std::string&)> callback) { impl_->diagnostic=std::move(callback); }
}
