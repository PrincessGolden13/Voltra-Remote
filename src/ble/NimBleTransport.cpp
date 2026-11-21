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
// One radio budget per requested connection, including retry delays. Splitting
// it allows a timed-out GAP initiation one fresh attempt without longer waits.
constexpr uint32_t connectionBudgetMs=10000, connectionAttemptMs=4500, connectionRetryDelayMs=500, mtuWaitMs=1000;
// Allow the pinned stack's terminal callback to return before client reuse.
// This is a radio handoff policy, not a VOLTRA command or readiness delay.
constexpr uint32_t clientSettleMs=100;
constexpr unsigned connectionAttempts=3;
constexpr int transmitPowerDbm=9;
constexpr int establishmentFailure=BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT);
bool validDeviceId(const DeviceId& id) {
    if(id.size()!=19 || id[17]!='/' || id[18]<'0' || id[18]>'3') return false;
    for(size_t i=0;i<17;++i) {
        if(i%3==2) {if(id[i]!=':') return false;}
        else if(!((id[i]>='0' && id[i]<='9') || (id[i]>='a' && id[i]<='f') || (id[i]>='A' && id[i]<='F'))) return false;
    }
    return true;
}
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
    NimBLEAddress target,radioTarget;
    unsigned attempts=0;
    uint64_t now=0, retryAt=0, attemptAt=0, radioStartedAt=0, linkUpAt=0, handoffUntil=0;
    uint32_t attemptLimitMs=connectionAttemptMs;
    bool timeoutRetryUsed=false;
    bool terminalObserved=true,mtuReady=false;
    BleConnectionFailure failure;
    void fail(BleConnectionStage stage,int reason=0,bool timedOut=false) {
        const auto elapsed=attempts && now>=radioStartedAt ? now-radioStartedAt : 0;
        failure={stage,reason,timedOut,attempts,elapsed};
        log("BLE connect failure stage="+std::to_string(int(stage))+" code="+std::to_string(reason)+
            " timeout="+std::to_string(timedOut)+" attempt="+std::to_string(attempts)+
            " radioMs="+std::to_string(elapsed)+
            "/"+std::to_string(connectionBudgetMs));
    }
    // Stack callbacks only publish events. Application callbacks run in poll().
    enum class LinkKind { Up, Failed, Down, Mtu };
    struct LinkEvent { uint32_t generation; LinkKind kind; NimBLEAddress peer; int reason; bool cancelled; };
    std::deque<LinkEvent> linkEvents;
    std::atomic<uint32_t> radioGeneration{0},cancelledGeneration{0};
    std::atomic<bool> linkOverflow{false};
    void queueLink(LinkKind kind,NimBLEClient* source,int reason=0,const NimBLEAddress* knownPeer=nullptr) {
        const auto generation=radioGeneration.load();
        const auto peer=knownPeer ? *knownPeer : source->getPeerAddress();
        std::lock_guard<std::mutex> lock(mutex);
        if(linkEvents.size()>=16) {linkOverflow=true;return;}
        linkEvents.push_back({generation,kind,peer,reason,cancelledGeneration.load()==generation});
    }
    void onConnect(NimBLEClient* source) override { queueLink(LinkKind::Up,source); }
    void onConnectFail(NimBLEClient* source,int reason) override { queueLink(LinkKind::Failed,source,reason); }
    void onDisconnect(NimBLEClient* source,int reason) override { queueLink(LinkKind::Down,source,reason); }
    void onMTUChange(NimBLEClient* source,uint16_t) override { queueLink(LinkKind::Mtu,source); }
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
        impl_->client->setConnectTimeout(connectionAttemptMs);
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
std::string NimBleTransport::deviceAddress(const DeviceId& id) const {
    if(!validDeviceId(id)) return {};
    const NimBLEAddress address(id.substr(0,17),uint8_t(id[18]-'0'));
    return address.isNull() ? std::string{} : address.toString();
}
BleConnectionFailure NimBleTransport::connectionFailure() const { return impl_->failure; }
TransportResult NimBleTransport::connect(const DeviceId& id) {
    impl_->failure={};
    if(!impl_->client) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Unsupported;}
    if(!validDeviceId(id)) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Failed;}
    if(impl_->scanner->isScanning()) impl_->scanner->stop();
    disconnect();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it=impl_->signals.find(id);
        if(it!=impl_->signals.end()) impl_->log("BLE target RSSI="+std::to_string(it->second.rssi)+" connectable="+std::to_string(it->second.connectable)+" addressType="+id.substr(18));
    }
    impl_->target=NimBLEAddress(id.substr(0,17),uint8_t(id[18]-'0'));
    impl_->connecting=true;impl_->attempts=0;impl_->retryAt=impl_->now;impl_->timeoutRetryUsed=false;
    // Accepted means queued; onConnection reports completion/failure later.
    return TransportResult::Accepted;
}
void NimBleTransport::disconnect() {
    impl_->connecting=false;
    impl_->cancelledGeneration=impl_->radioGeneration.load();
    if(impl_->scanner && impl_->scanner->isScanning()) impl_->scanner->stop();
    if(!impl_->cancelling && !impl_->terminalObserved && impl_->client && (impl_->radioBusy || impl_->client->isConnected() || ble_gap_conn_active())) {
        impl_->cancelling=true;
        if(impl_->client->isConnected()) impl_->client->disconnect();
        else impl_->client->cancelConnect();
    }
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
    const auto previousError=impl_->client ? impl_->client->getLastError() : 0;
    if(impl_->client && impl_->client->isConnected())
        // NimBLE 2.x defaults to cached-only; true performs actual discovery.
        for(const auto* s:impl_->client->getServices(true)) result.push_back(s->getUUID().toString());
    impl_->log("BLE services: "+std::to_string(result.size()));
    if(result.empty()) {
        const auto error=impl_->client ? impl_->client->getLastError() : 0;
        impl_->fail(BleConnectionStage::Services,error!=previousError ? error : 0);
    }
    return result;
}
std::vector<Characteristic> NimBleTransport::discoverCharacteristics(const std::string& service) {
    std::vector<Characteristic> result;
    if(!impl_->client || !impl_->client->isConnected()) return result;
    const auto previousError=impl_->client->getLastError();
    const auto* s=impl_->client->getService(service.c_str());
    if(s) for(auto* c:s->getCharacteristics(true)) {
        const auto uuid=c->getUUID().toString(); impl_->characteristics[uuid]=c;
        result.push_back({uuid,c->canNotify(),c->canWrite() || c->canWriteNoResponse()});
        impl_->log("BLE characteristic "+uuid+" notify="+std::to_string(c->canNotify())+" write="+std::to_string(c->canWrite())+" writeNoResponse="+std::to_string(c->canWriteNoResponse()));
    }
    if(result.empty()) {
        const auto error=impl_->client->getLastError();
        impl_->fail(BleConnectionStage::Characteristics,error!=previousError ? error : 0);
    }
    return result;
}
TransportResult NimBleTransport::subscribe(const std::string& uuid) {
    const auto it=impl_->characteristics.find(uuid);
    if(it==impl_->characteristics.end() || !it->second->canNotify()) {impl_->fail(BleConnectionStage::Subscription);return TransportResult::Unsupported;}
    // NimBLE subscribe() returns true even when no CCCD is found. A callback
    // alone cannot prove notifications were enabled on the peripheral.
    if(!it->second->getDescriptor(NimBLEUUID(uint16_t(0x2902)))) {
        impl_->fail(BleConnectionStage::Subscription);impl_->log("BLE notification descriptor missing: "+uuid);return TransportResult::Failed;
    }
    const auto generation=impl_->generation;
    const auto previousError=impl_->client->getLastError();
    const bool ok=it->second->subscribe(true,[this,uuid,generation](NimBLERemoteCharacteristic*,uint8_t* data,size_t size,bool) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Overflow invalidates the session; losing state must never preserve readiness.
        if(size>517 || impl_->received.size()>=32) { impl_->overflow=true; return; }
        impl_->received.push_back({generation,uuid,Bytes(data,data+size)});
    });
    impl_->log(std::string(ok ? "BLE subscribed " : "BLE subscription failed ")+uuid);
    if(!ok) {
        const auto error=impl_->client->getLastError();
        impl_->fail(BleConnectionStage::Subscription,error!=previousError ? error : 0);
    }
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
    if(impl_->linkOverflow.exchange(false)) {
        const bool pending=impl_->connecting;
        impl_->fail(BleConnectionStage::Radio);impl_->log("BLE callback queue overflow");
        disconnect();if(pending && impl_->connection) impl_->connection(false);return;
    }
    std::deque<Impl::LinkEvent> events;
    {std::lock_guard<std::mutex> lock(impl_->mutex);events.swap(impl_->linkEvents);}
    for(const auto& event:events) {
        // The radio owner advances only when a physical attempt starts, never
        // when another endpoint is merely queued behind teardown. Keep the
        // source peer too, so an old endpoint cannot end the new connection.
        if(event.generation!=impl_->radioGeneration.load() || !(event.peer==impl_->radioTarget)) {
            impl_->log("BLE ignored stale callback");continue;
        }
        if(event.kind==Impl::LinkKind::Mtu) {
            if(!event.cancelled && !impl_->cancelling) impl_->mtuReady=true;
            continue;
        }
        if(event.kind==Impl::LinkKind::Up) {
            if(event.cancelled || impl_->cancelling || !impl_->connecting) {
                impl_->cancelling=true;
                if(impl_->client->isConnected()) impl_->client->disconnect();
            } else {impl_->linkObserved=true;impl_->linkUpAt=now;}
            continue;
        }
        const bool teardown=event.cancelled || impl_->cancelling || (!impl_->connected && !impl_->attempts);
        impl_->radioBusy=false;impl_->linkObserved=false;impl_->mtuReady=false;impl_->cancelling=false;impl_->terminalObserved=true;
        impl_->handoffUntil=now+clientSettleMs;
        impl_->log("BLE link ended: "+std::to_string(event.reason));
        if(teardown) {impl_->log("BLE previous connection closed");continue;}
        const bool hostTimeout=event.kind==Impl::LinkKind::Failed && event.reason==BLE_HS_ETIMEOUT;
        impl_->fail(impl_->connected ? BleConnectionStage::Link : BleConnectionStage::Radio,event.reason,hostTimeout);
        if(impl_->connected) {disconnect();return;}
        if(impl_->connecting && impl_->attempts) {
            const bool retryable=event.reason==establishmentFailure || (hostTimeout && !impl_->timeoutRetryUsed);
            const auto radioElapsed=now>=impl_->radioStartedAt ? now-impl_->radioStartedAt : connectionBudgetMs;
            if(retryable && impl_->attempts<connectionAttempts && radioElapsed+connectionRetryDelayMs<connectionBudgetMs) {
                if(hostTimeout) impl_->timeoutRetryUsed=true;
                impl_->retryAt=now+connectionRetryDelayMs;
                impl_->log("BLE connect retry code="+std::to_string(event.reason)+" afterMs="+
                    std::to_string(connectionRetryDelayMs)+" radioMs="+std::to_string(radioElapsed)+
                    "/"+std::to_string(connectionBudgetMs));
            } else {
                impl_->connecting=false;
                if(impl_->connection) impl_->connection(false);
            }
        }
    }
    // Cancellation must finish before reusing this client for another attempt.
    // A late link-up after cancel never becomes an application connection.
    if(!impl_->terminalObserved && (impl_->cancelling || (!impl_->connecting && !impl_->connected)) &&
       impl_->client && impl_->client->isConnected()) impl_->client->disconnect();
    if(impl_->connecting) {
        if(!impl_->attempts && now>=impl_->retryAt && now-impl_->retryAt>connectionBudgetMs+mtuWaitMs) {
            // An old cancellation can fail to deliver a callback. Bound the
            // queued request by the existing connect+MTU deadline, without
            // pretending the old radio procedure has ended or reusing it.
            impl_->fail(BleConnectionStage::Teardown,0,true);
            impl_->log("BLE previous connection did not close; reconnect stopped");
            impl_->connecting=false;
            if(impl_->connection) impl_->connection(false);
        }
        else if(impl_->attempts && !impl_->linkObserved && now-impl_->radioStartedAt>=connectionBudgetMs) {
            // A missing terminal callback cannot authorize another attempt.
            // Cancel while retaining the radio owner until its real teardown.
            impl_->fail(BleConnectionStage::Radio,0,true);
            impl_->log("BLE connect radio budget exhausted");
            disconnect();
            if(impl_->connection) impl_->connection(false);
            return;
        }
        // NimBLE 2.3.6 invokes its disconnect/failure callback before clearing
        // the client's connection handle. Drain both the callback and handle
        // before reuse; an immediate reselect must not race the closing link.
        else if(impl_->terminalObserved && !impl_->cancelling && !impl_->radioBusy && !impl_->client->isConnected() && !ble_gap_conn_active() &&
                now>=impl_->retryAt && now>=impl_->handoffUntil) {
            bool launch=false;
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                // Drain all callbacks belonging to the previous radio owner
                // before publishing a new owner. A queued logical selection
                // alone can never retag an old terminal event.
                if(impl_->linkEvents.empty() && !impl_->linkOverflow) {
                    ++impl_->radioGeneration;impl_->radioTarget=impl_->target;
                    impl_->mtuReady=false;impl_->linkObserved=false;impl_->radioBusy=true;impl_->terminalObserved=false;
                    if(!impl_->attempts) impl_->radioStartedAt=now;
                    impl_->attemptLimitMs=std::min<uint32_t>(connectionAttemptMs,
                        uint32_t(connectionBudgetMs-(now-impl_->radioStartedAt)));
                    impl_->attemptAt=now;++impl_->attempts;launch=true;
                }
            }
            if(launch) {
                impl_->log("BLE connecting attempt "+std::to_string(impl_->attempts)+"/"+std::to_string(connectionAttempts)+
                    " limitMs="+std::to_string(impl_->attemptLimitMs)+" radioMs="+
                    std::to_string(now-impl_->radioStartedAt)+"/"+std::to_string(connectionBudgetMs));
                impl_->client->setConnectTimeout(impl_->attemptLimitMs);
                const auto previousError=impl_->client->getLastError();
                if(!impl_->client->connect(impl_->target,true,true,true)) {
                    // Early failures can leave NimBLE's previous lastError intact.
                    // Do not attribute a stale code to this attempt.
                    const auto error=impl_->client->getLastError();
                    impl_->queueLink(Impl::LinkKind::Failed,impl_->client,error!=previousError ? error : 0,&impl_->radioTarget);
                }
            }
        }
        if(!impl_->cancelling && impl_->attempts && impl_->linkObserved && impl_->client->isConnected() && (impl_->mtuReady || now-impl_->linkUpAt>=mtuWaitMs)) {
            impl_->connecting=false;impl_->connected=true;
            impl_->failure={};
            impl_->log("BLE connected MTU="+std::to_string(impl_->client->getMTU()));
            if(impl_->connection) impl_->connection(true);
        } else if(impl_->attempts && impl_->radioBusy && now-impl_->attemptAt>impl_->attemptLimitMs+mtuWaitMs) {
            impl_->fail(BleConnectionStage::Radio,0,true);
            impl_->log("BLE connection timed out");
            disconnect();
            if(impl_->connection) impl_->connection(false);
        }
    }
    if(impl_->connected && (!impl_->client->isConnected() || impl_->overflow)) {
        impl_->fail(impl_->overflow ? BleConnectionStage::Notifications : BleConnectionStage::Link);
        disconnect();return;
    }
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
