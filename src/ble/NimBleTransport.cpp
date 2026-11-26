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
struct RadioPolicy {
    uint32_t budgetMs, attemptMs;
    unsigned maximumAttempts;
};
constexpr RadioPolicy standardRadio{10000,4500,3};
// The longer window covers the ~50 s recovery observed in Steven's capture.
// Backoff and individual attempts share this bound.
constexpr RadioPolicy recoveryRadio{55000,10000,7};
constexpr uint32_t standardRetryDelayMs=500, recoveryFirstDelayMs=1500, recoveryMaxDelayMs=15000, mtuWaitMs=1000;
// Refresh the exact endpoint's radio route in the existing retry gaps.
constexpr uint32_t recoveryScanMs=1200;
// Allow the pinned stack's terminal callback to return before client reuse.
// This is a radio handoff policy, not a VOLTRA command or readiness delay.
constexpr uint32_t clientSettleMs=100;
// Type knowledge is retained for this boot, independently of radio presence.
// RSSI/connectable observations stop being current after the app's 15 s grace.
constexpr uint64_t observationFreshMs=15000;
constexpr size_t maximumObservedRadios=64;
// NimBLE units are 10 ms. A Twin controller also serves its other VOLTRA;
// stevenpoint/voltra_remote 1c14860 uses 10 s to tolerate those quiet intervals.
constexpr uint16_t supervisionTimeoutUnits=1000;
constexpr int transmitPowerDbm=9;
constexpr int establishmentFailure=BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT);
bool validAddress(const std::string& id) {
    if(id.size()!=17) return false;
    for(size_t i=0;i<17;++i) {
        if(i%3==2) {if(id[i]!=':') return false;}
        else if(!((id[i]>='0' && id[i]<='9') || (id[i]>='a' && id[i]<='f') || (id[i]>='A' && id[i]<='F'))) return false;
    }
    return true;
}
bool validDeviceId(const DeviceId& id) {
    return id.size()==19 && id[17]=='/' && id[18]>='0' && id[18]<='3' && validAddress(id.substr(0,17));
}
std::string attributeKey(const std::string& text) {
    // SDK toString() prefixes short UUIDs with 0x, while its string
    // constructor expects the unprefixed digits.
    const bool prefixed=text.size()>2 && text[0]=='0' && (text[1]=='x' || text[1]=='X');
    NimBLEUUID uuid(prefixed ? text.substr(2) : text);
    if(!uuid.bitSize()) return {};
    // The same attribute may be spelled with a short Bluetooth UUID, its
    // 128-bit base form, or different case. All must share one SDK snapshot.
    return uuid.to128().toString();
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
    // One immutable characteristic snapshot per service and connection. SDK
    // filtered characteristic lookups traverse the same ATT declaration range;
    // repeating them would rescan it and cache only one channel each time.
    std::map<std::string,std::vector<Characteristic>> characteristicSnapshots;
    struct Received { uint32_t generation; std::string uuid; Bytes bytes; };
    std::mutex mutex;
    std::map<std::string,Advertisement> advertisements;
    std::vector<std::string> scanServices;
    std::string recoveryScanTarget;
    struct ObservedRadio {
        NimBLEAddress address;
        int rssi=0;
        bool connectable=false;
        uint64_t at=0,order=0;
    };
    std::map<std::string,ObservedRadio> observedRadios;
    uint64_t observationOrder=0;
    void onDiscovered(const NimBLEAdvertisedDevice* d) override { onResult(d); }
    void onResult(const NimBLEAdvertisedDevice* d) override {
        // NimBLE can replace a previous scan response with a later advertisement.
        // Retain the union observed within this scan, not just its final packet.
        // Address type is a current radio attribute, not a second logical device.
        const auto address=d->getAddress();
        const auto canonical=address.toString();
        Advertisement a;
        a.id=canonical+"/"+std::to_string(address.getType());
        a.name=d->getName();
        for(unsigned n=0;n<d->getServiceUUIDCount();++n) a.services.push_back(d->getServiceUUID(n).toString());
        std::lock_guard<std::mutex> lock(mutex);
        // The profile configures service discovery. Unrelated gym advertisers
        // must not fill the bounded result/type caches before a target arrives.
        const bool matches=scanServices.empty() || std::any_of(a.services.begin(),a.services.end(),[&](const auto& uuid) {
            return std::find(scanServices.begin(),scanServices.end(),uuid)!=scanServices.end();
        });
        if(!recoveryScanTarget.empty() && canonical!=recoveryScanTarget) return;
        if(!matches && canonical!=recoveryScanTarget && !observedRadios.count(canonical)) return;
        if(observedRadios.size()>=maximumObservedRadios && !observedRadios.count(canonical)) {
            // Old neighbours cannot prevent learning a newer target/type.
            const auto oldest=std::min_element(observedRadios.begin(),observedRadios.end(),
                [](const auto& a,const auto& b){return a.second.order<b.second.order;});
            if(recoveryScanTarget.empty()) advertisements.erase(oldest->first);
            observedRadios.erase(oldest);
        }
        // The callback uses the last poll's monotonic clock under this mutex.
        // A stalled application makes the observation older, never falsely fresh.
        observedRadios[canonical]={address,d->getRSSI(),d->isConnectable(),now,++observationOrder};
        if(!recoveryScanTarget.empty()) return; // Keep the completed Devices result intact.
        auto& saved=advertisements[canonical];saved.id=a.id;
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
    std::string targetAddress;
    std::optional<NimBLEAddress> target;
    NimBLEAddress radioTarget;
    bool radioTypeObserved=false,radioConnectable=false;
    int radioRssi=0;
    uint64_t radioObservedAt=0;
    unsigned attempts=0;
    uint64_t now=0, retryAt=0, attemptAt=0, radioStartedAt=0, linkUpAt=0, handoffUntil=0;
    uint32_t attemptLimitMs=standardRadio.attemptMs;
    bool extendedRecovery=false,recoveryScanDue=false,recoveryScanActive=false;
    uint64_t recoveryObservationOrder=0;
    const RadioPolicy& radioPolicy() const { return extendedRecovery ? recoveryRadio : standardRadio; }
    uint32_t retryDelay() const {
        if(!extendedRecovery) return standardRetryDelayMs;
        const unsigned shift=std::min(4u,attempts ? attempts-1 : 0);
        return std::min(recoveryFirstDelayMs<<shift,recoveryMaxDelayMs);
    }
    bool timeoutRetryUsed=false;
    bool terminalObserved=true,mtuReady=false;
    BleConnectionFailure failure;
    bool resolveRadioTarget() {
        // Called under mutex only when a physical attempt may start. Keep the
        // original logical target unchanged; late events retain their old owner.
        radioTypeObserved=false;
        const auto observed=observedRadios.find(targetAddress);
        if(observed==observedRadios.end()) {
            if(!target) return false;
            radioTarget=*target;
            return true;
        }
        radioTarget=observed->second.address;radioTypeObserved=true;
        radioRssi=observed->second.rssi;radioConnectable=observed->second.connectable;
        radioObservedAt=observed->second.at;
        return true;
    }
    bool scanRecoveryRoute() {
        if(recoveryScanActive) {
            bool seen=false;
            {
                std::lock_guard<std::mutex> lock(mutex);
                const auto observed=observedRadios.find(targetAddress);
                seen=observed!=observedRadios.end() && observed->second.order>recoveryObservationOrder;
                recoveryScanTarget.clear();
            }
            log(seen ? "BLE recovery scan saw exact endpoint" : "BLE recovery scan did not see endpoint");
            recoveryScanActive=false;
        }
        if(!recoveryScanDue) return false;
        recoveryScanDue=false;
        // Subsequent windows fit inside the existing backoff.
        const uint64_t remaining=retryAt>now ? retryAt-now : 0;
        const auto duration=attempts ? std::min<uint64_t>(recoveryScanMs,remaining) : recoveryScanMs;
        if(!duration) return false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            recoveryScanTarget=targetAddress;
            recoveryObservationOrder=observationOrder;
        }
        recoveryScanActive=scanner->start(uint32_t(duration),false,true);
        if(!recoveryScanActive) {
            {std::lock_guard<std::mutex> lock(mutex);recoveryScanTarget.clear();}
            log("BLE recovery scan could not start");
        }
        return recoveryScanActive;
    }
    std::string radioDetail() const {
        const bool validAge=radioTypeObserved && now>=radioObservedAt;
        const auto age=validAge ? now-radioObservedAt : 0;
        const bool signalFresh=validAge && age<=observationFreshMs;
        return " radioType="+std::to_string(radioTarget.getType())+
            " typeObserved="+std::to_string(radioTypeObserved)+
            " observationAgeMs="+(validAge ? std::to_string(age) : "unknown")+
            " rssi="+(signalFresh ? std::to_string(radioRssi) : "unknown")+
            " connectable="+(signalFresh ? std::to_string(radioConnectable) : "unknown");
    }
    void fail(BleConnectionStage stage,int reason=0,bool timedOut=false) {
        const auto failedAttempts=stage==BleConnectionStage::Setup ? 0u : attempts;
        const auto elapsed=failedAttempts && now>=radioStartedAt ? now-radioStartedAt : 0;
        failure={stage,reason,timedOut,failedAttempts,elapsed};
        // Setup can fail before a new logical request starts; never report the
        // previous physical owner's type as that invalid request's metadata.
        if(failedAttempts) {
            failure.logicalAddressType=target ? target->getType() : -1;
            failure.radioAddressType=radioTarget.getType();
            failure.observedRadioType=radioTypeObserved;
        }
        log("BLE connect failure stage="+std::to_string(int(stage))+" code="+std::to_string(reason)+
            " timeout="+std::to_string(timedOut)+" attempt="+std::to_string(failedAttempts)+
            " radioMs="+std::to_string(elapsed)+
            "/"+std::to_string(radioPolicy().budgetMs)+
            (failedAttempts && stage!=BleConnectionStage::Teardown ? radioDetail() : ""));
    }
    // Stack callbacks only publish events. Application callbacks run in poll().
    enum class LinkKind { Up, Failed, Down, Mtu };
    struct LinkEvent { uint32_t generation; LinkKind kind; NimBLEAddress peer; int reason; bool cancelled; };
    std::deque<LinkEvent> linkEvents;
    std::atomic<uint32_t> radioGeneration{0},cancelledGeneration{0};
    std::atomic<bool> linkOverflow{false};
    std::optional<LinkEvent> pendingTerminal() {
        const auto owner=radioGeneration.load();
        if(cancelling || cancelledGeneration.load()==owner || linkOverflow.load()) return {};
        std::lock_guard<std::mutex> lock(mutex);
        for(auto it=linkEvents.rbegin();it!=linkEvents.rend();++it)
            if(it->generation==owner && it->peer==radioTarget && !it->cancelled &&
               (it->kind==LinkKind::Down || it->kind==LinkKind::Failed)) return *it;
        return {};
    }
    void failGatt(BleConnectionStage stage,int reason=0) {
        // Blocking GATT can return after the SDK queued a disconnect but before
        // poll() can deliver it. Preserve that cause before local teardown marks
        // the event cancelled. Do not consume terminal/ownership evidence here.
        if(linkOverflow.load() || overflow.load()) {
            if(failure.stage!=BleConnectionStage::Notifications) fail(BleConnectionStage::Notifications);
            return;
        }
        const bool rawLink=client && client->isConnected();
        const auto terminal=pendingTerminal();
        bool timedOut=false;
        if(terminal) {
            stage=BleConnectionStage::Link;reason=terminal->reason;
            timedOut=terminal->kind==LinkKind::Failed && reason==BLE_HS_ETIMEOUT;
        } else if(connected && !cancelling && !rawLink) {
            stage=BleConnectionStage::Link;reason=0;
        }
        if(failure.stage==stage && failure.reason==reason && failure.timedOut==timedOut) return;
        fail(stage,reason,timedOut);
    }
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
    // Callback-only mode: the SDK's bounded stored-results list otherwise
    // discards later advertisers before our service filter can inspect them.
    impl_->scanner->setMaxResults(0);
    impl_->client=NimBLEDevice::createClient();
    if(impl_->client) {
        impl_->client->setConnectTimeout(standardRadio.attemptMs);
        // 15–30 ms connection interval, zero latency, 10 s supervision timeout.
        // Generic BLE timing; VOLTRA may negotiate a different interval.
        impl_->client->setConnectionParams(12,24,0,supervisionTimeoutUnits);
        impl_->client->setClientCallbacks(impl_.get(),false);
    }
    return impl_->client!=nullptr;
}
void NimBleTransport::setScanServices(const std::vector<std::string>& services) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if(impl_->scanServices==services) return;
    impl_->scanServices=services;
    impl_->advertisements.clear();impl_->observedRadios.clear();
}
std::vector<Advertisement> NimBleTransport::scan() {
    if(!impl_->scanner || impl_->connected || impl_->connecting || impl_->radioBusy || impl_->scanner->isScanning()) return {};
    impl_->scanner->clearResults();
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->recoveryScanTarget.clear();impl_->advertisements.clear();}
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
    if(!validDeviceId(id) && !validAddress(id)) return {};
    // Type is irrelevant to formatting identity; a bare address is not a
    // connectable typed target until an advertisement supplies that metadata.
    const NimBLEAddress address(id.substr(0,17),0);
    return address.isNull() ? std::string{} : address.toString();
}
DeviceId NimBleTransport::deviceIdForAddress(const std::string& address) const {
    const auto canonical=deviceAddress(address);
    if(canonical.empty()) return {};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    const auto observed=impl_->observedRadios.find(canonical);
    return observed==impl_->observedRadios.end() ? canonical :
        canonical+"/"+std::to_string(observed->second.address.getType());
}
BleConnectionFailure NimBleTransport::connectionFailure() const { return impl_->failure; }
bool NimBleTransport::linkAlive() const {
    const bool rawLink=impl_->client && impl_->client->isConnected();
    const bool alive=impl_->connected && !impl_->cancelling && rawLink &&
        !impl_->linkOverflow.load() && !impl_->overflow.load() && !impl_->pendingTerminal();
    if(!alive && impl_->connected && !impl_->cancelling) impl_->failGatt(BleConnectionStage::Link);
    return alive;
}
void NimBleTransport::clearConnectionFailure() {
    if(linkAlive()) impl_->failure={};
}
TransportResult NimBleTransport::connect(const DeviceId& id) {
    return connect(id,BleConnectionPolicy::Standard);
}
TransportResult NimBleTransport::connect(const DeviceId& id,BleConnectionPolicy policy) {
    impl_->failure={};
    if(!impl_->client) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Unsupported;}
    if(!validDeviceId(id) && !validAddress(id)) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Failed;}
    const auto address=deviceAddress(id);
    const bool extended=policy==BleConnectionPolicy::RecoverKnownEndpoint;
    std::optional<NimBLEAddress> target;
    if(validAddress(id)) {
        bool observed=false;
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            const auto found=impl_->observedRadios.find(address);
            if(found!=impl_->observedRadios.end()) {target=found->second.address;observed=true;}
        }
        if(!observed && !extended) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Unsupported;}
    } else target=NimBLEAddress(address,uint8_t(id[18]-'0'));
    if(address.empty()) {impl_->fail(BleConnectionStage::Setup);return TransportResult::Failed;}
    if(impl_->scanner->isScanning()) impl_->scanner->stop();
    disconnect();
    impl_->targetAddress=address;
    impl_->target=target;
    impl_->extendedRecovery=extended;
    impl_->recoveryScanDue=extended;
    impl_->connecting=true;impl_->attempts=0;impl_->retryAt=impl_->now;impl_->timeoutRetryUsed=false;
    // Accepted means queued; onConnection reports completion/failure later.
    return TransportResult::Accepted;
}
void NimBleTransport::disconnect() {
    impl_->connecting=false;
    impl_->cancelledGeneration=impl_->radioGeneration.load();
    if(impl_->scanner && impl_->scanner->isScanning()) impl_->scanner->stop();
    const bool scanStopped=!impl_->scanner || !impl_->scanner->isScanning();
    impl_->recoveryScanDue=impl_->recoveryScanActive=false;
    if(!impl_->cancelling && !impl_->terminalObserved && impl_->client && (impl_->radioBusy || impl_->client->isConnected() || ble_gap_conn_active())) {
        impl_->cancelling=true;
        if(impl_->client->isConnected()) impl_->client->disconnect();
        else impl_->client->cancelConnect();
    }
    ++impl_->generation;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->received.clear();
        // If cancel failed, keep filtering this short window until it drains.
        if(scanStopped) impl_->recoveryScanTarget.clear();
    }
    impl_->overflow=false;
    impl_->diagnosticRx=impl_->diagnosticTx=0;
    impl_->characteristics.clear();
    impl_->characteristicSnapshots.clear();
    const bool was=impl_->connected; impl_->connected=false;
    if(was) impl_->log("BLE disconnected");
    if(was && impl_->connection) impl_->connection(false);
}
std::vector<std::string> NimBleTransport::discoverServices() {
    std::vector<std::string> result;
    const auto previousError=impl_->client ? impl_->client->getLastError() : 0;
    if(impl_->characteristicSnapshots.empty()) impl_->characteristics.clear();
    if(impl_->client && impl_->client->isConnected())
        // NimBLE 2.x defaults to cached-only; true performs actual discovery.
        // Once attributes are in use, keep their service objects/callbacks alive.
        for(const auto* s:impl_->client->getServices(impl_->characteristicSnapshots.empty())) result.push_back(s->getUUID().toString());
    impl_->log("BLE services: "+std::to_string(result.size()));
    if(result.empty()) {
        const auto error=impl_->client ? impl_->client->getLastError() : 0;
        impl_->failGatt(BleConnectionStage::Services,error!=previousError ? error : 0);
    }
    return result;
}
bool NimBleTransport::discoverService(const std::string& uuid) {
    const auto previousError=impl_->client ? impl_->client->getLastError() : 0;
    const auto canonical=attributeKey(uuid);
    const bool found=!canonical.empty() && impl_->client && impl_->client->isConnected() && impl_->client->getService(canonical.c_str());
    if(!found) {
        const auto error=impl_->client ? impl_->client->getLastError() : 0;
        impl_->failGatt(BleConnectionStage::Services,error!=previousError ? error : 0);
    }
    return found;
}
std::optional<Characteristic> NimBleTransport::discoverCharacteristic(const std::string& service,const std::string& uuid) {
    const auto canonical=attributeKey(uuid);
    if(canonical.empty()) {impl_->log("BLE characteristic UUID invalid");return {};}
    const auto snapshot=discoverCharacteristics(service);
    const auto found=std::find_if(snapshot.begin(),snapshot.end(),[&](const auto& c){return attributeKey(c.uuid)==canonical;});
    if(found!=snapshot.end()) return *found;
    impl_->log("BLE characteristic unavailable: "+uuid);
    // A successfully read table can legitimately omit an optional channel.
    // Record absence, but never invent a transport error or guessed code.
    return {};
}
std::vector<Characteristic> NimBleTransport::discoverCharacteristics(const std::string& service) {
    if(!linkAlive()) {impl_->failGatt(BleConnectionStage::Characteristics);return {};}
    // UUID spelling/case must not refresh an already subscribed SDK service.
    const auto canonical=attributeKey(service);
    if(canonical.empty()) {impl_->failGatt(BleConnectionStage::Characteristics);return {};}
    const auto cached=impl_->characteristicSnapshots.find(canonical);
    if(cached!=impl_->characteristicSnapshots.end()) return cached->second;
    std::vector<Characteristic> result;
    const auto previousError=impl_->client->getLastError();
    const auto* s=impl_->client->getService(canonical.c_str());
    // Drop adapter aliases before the SDK refresh deletes its prior objects.
    // Refresh only once, before returning any channel to a subscriber/writer.
    if(s) for(const auto* previous:s->getCharacteristics(false)) {
        for(auto it=impl_->characteristics.begin();it!=impl_->characteristics.end();)
            if(it->second==previous) it=impl_->characteristics.erase(it);else ++it;
    }
    if(s) for(auto* c:s->getCharacteristics(true)) {
        const auto uuid=c->getUUID().toString(); impl_->characteristics[attributeKey(uuid)]=c;
        result.push_back({uuid,c->canNotify(),c->canWrite() || c->canWriteNoResponse()});
        impl_->log("BLE characteristic "+uuid+" notify="+std::to_string(c->canNotify())+" write="+std::to_string(c->canWrite())+" writeNoResponse="+std::to_string(c->canWriteNoResponse()));
    }
    impl_->log("BLE characteristic table count="+std::to_string(result.size()));
    const bool alive=linkAlive();
    if(!alive || result.empty()) {
        const auto error=impl_->client->getLastError();
        if(!alive) impl_->characteristics.clear();
        impl_->failGatt(BleConnectionStage::Characteristics,error!=previousError ? error : 0);
        result.clear();
    }
    impl_->characteristicSnapshots.emplace(canonical,result);
    return result;
}
TransportResult NimBleTransport::subscribe(const std::string& uuid) {
    if(!linkAlive()) {impl_->failGatt(BleConnectionStage::Subscription);return TransportResult::Disconnected;}
    const auto canonical=attributeKey(uuid);
    const auto it=impl_->characteristics.find(canonical);
    if(it==impl_->characteristics.end() || !it->second->canNotify()) {impl_->fail(BleConnectionStage::Subscription);return TransportResult::Unsupported;}
    // NimBLE subscribe() returns true even when no CCCD is found. A callback
    // alone cannot prove notifications were enabled on the peripheral.
    if(!it->second->getDescriptor(NimBLEUUID(uint16_t(0x2902)))) {
        impl_->failGatt(BleConnectionStage::Subscription);impl_->log("BLE notification descriptor missing: "+uuid);return TransportResult::Failed;
    }
    const auto generation=impl_->generation;
    const auto routeUuid=it->second->getUUID().toString();
    const auto previousError=impl_->client->getLastError();
    const bool ok=it->second->subscribe(true,[this,routeUuid,generation](NimBLERemoteCharacteristic*,uint8_t* data,size_t size,bool) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Overflow invalidates the session; losing state must never preserve readiness.
        if(size>517 || impl_->received.size()>=32) { impl_->overflow=true; return; }
        impl_->received.push_back({generation,routeUuid,Bytes(data,data+size)});
    });
    impl_->log(std::string(ok ? "BLE subscribed " : "BLE subscription failed ")+uuid);
    if(!ok) {
        const auto error=impl_->client->getLastError();
        impl_->failGatt(BleConnectionStage::Subscription,error!=previousError ? error : 0);
    }
    return ok ? TransportResult::Accepted : TransportResult::Failed;
}
TransportResult NimBleTransport::write(const std::string& uuid,const Bytes& bytes) {
    if(!linkAlive()) {impl_->failGatt(BleConnectionStage::Link);return TransportResult::Disconnected;}
    const auto it=impl_->characteristics.find(attributeKey(uuid));
    if(it==impl_->characteristics.end()) return TransportResult::Unsupported;
    const auto previousError=impl_->client->getLastError();
    const bool ok=it->second->writeValue(bytes.data(),bytes.size(),it->second->canWrite());
    if(impl_->diagnosticTx++<8) impl_->log("BLE write bytes="+std::to_string(bytes.size())+" accepted="+std::to_string(ok));
    if(!ok) {
        const auto error=impl_->client->getLastError();
        impl_->failGatt(BleConnectionStage::Link,error!=previousError ? error : 0);
    }
    return ok ? TransportResult::Accepted : TransportResult::Failed;
}
void NimBleTransport::poll(uint64_t now) {
    const auto policy=impl_->radioPolicy();
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->now=now;}
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
            const bool retryable=event.reason==establishmentFailure ||
                (hostTimeout && (impl_->extendedRecovery || !impl_->timeoutRetryUsed));
            const auto radioElapsed=now>=impl_->radioStartedAt ? now-impl_->radioStartedAt : policy.budgetMs;
            const auto delayMs=impl_->retryDelay();
            if(retryable && impl_->attempts<policy.maximumAttempts && radioElapsed+delayMs<policy.budgetMs) {
                if(hostTimeout) impl_->timeoutRetryUsed=true;
                impl_->retryAt=now+delayMs;
                impl_->recoveryScanDue=impl_->extendedRecovery;
                impl_->log("BLE connect retry code="+std::to_string(event.reason)+" afterMs="+
                    std::to_string(delayMs)+" radioMs="+std::to_string(radioElapsed)+
                    "/"+std::to_string(policy.budgetMs));
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
        if(!impl_->attempts && now>=impl_->retryAt && now-impl_->retryAt>standardRadio.budgetMs+mtuWaitMs) {
            // An old cancellation can fail to deliver a callback. Bound the
            // queued request by the existing connect+MTU deadline, without
            // pretending the old radio procedure has ended or reusing it.
            impl_->fail(BleConnectionStage::Teardown,0,true);
            impl_->log("BLE previous connection did not close; reconnect stopped");
            impl_->connecting=false;
            if(impl_->connection) impl_->connection(false);
        }
        else if(impl_->attempts && !impl_->linkObserved && now-impl_->radioStartedAt>=policy.budgetMs) {
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
                !impl_->scanner->isScanning() && now>=impl_->handoffUntil) {
            if(impl_->scanRecoveryRoute() || now<impl_->retryAt) return;
            bool launch=false,targetMissing=false;
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                // Drain all callbacks belonging to the previous radio owner
                // before publishing a new owner. A queued logical selection
                // alone can never retag an old terminal event.
                if(impl_->linkEvents.empty() && !impl_->linkOverflow) {
                    if(impl_->resolveRadioTarget()) {
                        ++impl_->radioGeneration;
                        impl_->mtuReady=false;impl_->linkObserved=false;impl_->radioBusy=true;impl_->terminalObserved=false;
                        if(!impl_->attempts) impl_->radioStartedAt=now;
                        impl_->attemptLimitMs=std::min<uint32_t>(policy.attemptMs,
                            uint32_t(policy.budgetMs-(now-impl_->radioStartedAt)));
                        impl_->attemptAt=now;++impl_->attempts;launch=true;
                    } else targetMissing=true;
                }
            }
            if(targetMissing) {
                impl_->log("BLE recovery has no observed address type");
                impl_->fail(BleConnectionStage::Setup);
                impl_->connecting=false;
                if(impl_->connection) impl_->connection(false);
                return;
            }
            if(launch) {
                impl_->log("BLE connecting attempt "+std::to_string(impl_->attempts)+"/"+std::to_string(policy.maximumAttempts)+
                    " limitMs="+std::to_string(impl_->attemptLimitMs)+" radioMs="+
                    std::to_string(now-impl_->radioStartedAt)+"/"+std::to_string(policy.budgetMs)+impl_->radioDetail());
                impl_->client->setConnectTimeout(impl_->attemptLimitMs);
                const auto previousError=impl_->client->getLastError();
                if(!impl_->client->connect(impl_->radioTarget,true,true,true)) {
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
