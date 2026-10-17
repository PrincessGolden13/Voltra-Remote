#include "NimBleTransport.h"
#include <NimBLEDevice.h>
#include <deque>
#include <mutex>
#include <map>
#include <atomic>
namespace voltra {
struct NimBleTransport::Impl {
    NimBLEClient* client=nullptr;
    NimBLEScan* scanner=nullptr;
    Notification notification;
    Connection connection;
    std::map<std::string,NimBLERemoteCharacteristic*> characteristics;
    struct Received { uint32_t generation; std::string uuid; Bytes bytes; };
    std::mutex mutex;
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
    impl_->scanner->start(2000,false,true); // Async: complete window before auto-selection.
    return {};
}
bool NimBleTransport::scanInProgress() const { return impl_->scanner && impl_->scanner->isScanning(); }
std::vector<Advertisement> NimBleTransport::discovered() const {
    std::vector<Advertisement> found;
    if(!impl_->scanner || impl_->scanner->isScanning()) return found;
    const auto results=impl_->scanner->getResults();
    for(int i=0;i<results.getCount();++i) {
        const auto* d=results.getDevice(i);
        Advertisement a;
        a.id=d->getAddress().toString()+"/"+std::to_string(d->getAddressType());
        a.name=d->getName();
        for(unsigned n=0;n<d->getServiceUUIDCount();++n) a.services.push_back(d->getServiceUUID(n).toString());
        found.push_back(std::move(a));
    }
    return found;
}
TransportResult NimBleTransport::connect(const DeviceId& id) {
    if(!impl_->client) return TransportResult::Unsupported;
    const auto separator=id.find('/');
    if(separator!=17 || id.size()!=19 || id[18]<'0' || id[18]>'3') return TransportResult::Failed;
    if(impl_->scanner->isScanning()) impl_->scanner->stop();
    disconnect();
    if(!impl_->client->connect(NimBLEAddress(id.substr(0,17),uint8_t(id[18]-'0')),true)) return TransportResult::Failed;
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
    if(was && impl_->connection) impl_->connection(false);
}
std::vector<std::string> NimBleTransport::discoverServices() {
    std::vector<std::string> result;
    if(impl_->client && impl_->client->isConnected())
        for(const auto* s:impl_->client->getServices()) result.push_back(s->getUUID().toString());
    return result;
}
std::vector<Characteristic> NimBleTransport::discoverCharacteristics(const std::string& service) {
    std::vector<Characteristic> result;
    if(!impl_->client || !impl_->client->isConnected()) return result;
    const auto* s=impl_->client->getService(service.c_str());
    if(s) for(auto* c:s->getCharacteristics()) {
        const auto uuid=c->getUUID().toString(); impl_->characteristics[uuid]=c;
        result.push_back({uuid,c->canNotify(),c->canWrite() || c->canWriteNoResponse()});
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
}
