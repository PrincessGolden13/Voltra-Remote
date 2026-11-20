#pragma once
#include "controller/DeviceMemory.h"
#include <Preferences.h>
#include <nvs.h>
#include <algorithm>
namespace voltra::guition {
// Separate namespaces for real identities and synthetic fixtures. No load/settings.
class DeviceStore final : public DeviceMemory {
    const char* name_;
    bool writable_=false,navigationWritable_=false;
    static constexpr size_t maximumStorageBytes=16384;
public:
    explicit DeviceStore(bool simulated) : name_(simulated ? "voltra-sim-dev" : "voltra-devices") {}
    std::vector<DeviceEntry> load() override {
        writable_=navigationWritable_=false;
        std::vector<DeviceEntry> result;
        nvs_handle_t handle;
        const auto opened=nvs_open(name_,NVS_READONLY,&handle);
        if(opened==ESP_ERR_NVS_NOT_FOUND) {writable_=navigationWritable_=true;return result;}
        if(opened!=ESP_OK) return result;
        size_t size=0;
        const auto found=nvs_get_blob(handle,"devices",nullptr,&size);
        if(found!=ESP_ERR_NVS_NOT_FOUND && (found!=ESP_OK || size<2 || size>maximumStorageBytes)) {nvs_close(handle);return result;}
        if(found==ESP_ERR_NVS_NOT_FOUND) size=2;
        Bytes b(size);size_t actual=size;
        if(found==ESP_ERR_NVS_NOT_FOUND) b={1,0};
        const bool read=found==ESP_ERR_NVS_NOT_FOUND || (nvs_get_blob(handle,"devices",b.data(),&actual)==ESP_OK && actual==size);
        if (!read || b[0]!=1 || b[1]>maximumRememberedDevices) {nvs_close(handle);return result;}
        size_t at=2;
        for (unsigned i=0;i<b[1];++i) {
            if (at+2>b.size()) {nvs_close(handle);return {};}
            const size_t ids=b[at++], names=b[at++];
            if (!ids || ids>128 || names>128 || at+ids+names>b.size()) {nvs_close(handle);return {};}
            DeviceEntry d; d.id=std::string(b.begin()+at,b.begin()+at+ids); at+=ids;
            d.name=std::string(b.begin()+at,b.begin()+at+names); at+=names;
            if (std::none_of(result.begin(),result.end(),[&](const auto& e){return e.id==d.id;})) result.push_back(d);
        }
        if(at!=b.size()) {nvs_close(handle);return {};}
        writable_=true;
        // Keep the schema-1 ID/name record compatible with older firmware and
        // rollback. Optional navigation metadata cannot invalidate that record.
        size_t navigationSize=0;
        const auto navigationFound=nvs_get_blob(handle,"twins",nullptr,&navigationSize);
        if(navigationFound==ESP_ERR_NVS_NOT_FOUND) {nvs_close(handle);navigationWritable_=true;return result;}
        if(navigationFound!=ESP_OK || navigationSize<2 || size+navigationSize>maximumStorageBytes) {nvs_close(handle);return result;}
        Bytes navigation(navigationSize);actual=navigationSize;
        const bool navigationRead=nvs_get_blob(handle,"twins",navigation.data(),&actual)==ESP_OK && actual==navigationSize;
        nvs_close(handle);
        if(!navigationRead || navigation[0]!=1 || navigation[1]>maximumRememberedDevices) return result;
        std::vector<DeviceEntry> links;at=2;
        for(unsigned i=0;i<navigation[1];++i) {
            if(at+4>navigation.size()) return result;
            const size_t ids=navigation[at++],peers=navigation[at++],names=navigation[at++],selections=navigation[at++];
            if(!ids || !peers || !selections || ids>128 || peers>128 || names>128 || selections>128 ||
               at+ids+peers+names+selections>navigation.size()) return result;
            DeviceEntry d;
            const auto field=[&](size_t length){auto value=std::string(navigation.begin()+at,navigation.begin()+at+length);at+=length;return value;};
            d.id=field(ids);d.twinPeerId=field(peers);d.twinPeer=field(names);d.twinSelectionId=field(selections);
            if(d.twinPeerId==d.id || (d.twinSelectionId!=d.id && d.twinSelectionId!=d.twinPeerId) ||
               std::any_of(links.begin(),links.end(),[&](const auto& entry){return entry.id==d.id;})) return result;
            links.push_back(std::move(d));
        }
        if(at!=navigation.size()) return result;
        navigationWritable_=true;
        for(auto& d:result) {
            const auto link=std::find_if(links.begin(),links.end(),[&](const auto& entry){return entry.id==d.id;});
            if(link!=links.end()) {d.twinPeerId=link->twinPeerId;d.twinPeer=link->twinPeer;d.twinSelectionId=link->twinSelectionId;}
        }
        return result;
    }
    bool save(const std::vector<DeviceEntry>& entries) override {
        // Failed/malformed/future-schema reads cannot silently overwrite either
        // record. In particular an unknown optional schema is retained intact.
        if (!writable_ || !navigationWritable_ || entries.size()>maximumRememberedDevices) return false;
        Bytes b{1,uint8_t(entries.size())},navigation{1,0};
        for (const auto& d:entries) {
            if(d.id.empty() || d.id.size()>128 || d.name.size()>128) return false;
            b.push_back(uint8_t(d.id.size()));b.push_back(uint8_t(d.name.size()));
            b.insert(b.end(),d.id.begin(),d.id.end());b.insert(b.end(),d.name.begin(),d.name.end());
            if(d.twinPeerId.empty()) {
                if(!d.twinPeer.empty() || !d.twinSelectionId.empty()) return false;
                continue;
            }
            if(d.twinPeerId==d.id || (d.twinSelectionId!=d.id && d.twinSelectionId!=d.twinPeerId)) return false;
            const std::string* values[]={&d.id,&d.twinPeerId,&d.twinPeer,&d.twinSelectionId};
            for(const auto* value:values) {
                if(value->size()>128) return false;
                navigation.push_back(uint8_t(value->size()));
            }
            for(const auto* value:values) navigation.insert(navigation.end(),value->begin(),value->end());
            ++navigation[1];
        }
        if(b.size()+navigation.size()>maximumStorageBytes) return false;
        Preferences nvs;if(!nvs.begin(name_,false)) return false;
        const auto writeVerified=[&](const char* key,const Bytes& bytes){
            if(nvs.putBytes(key,bytes.data(),bytes.size())!=bytes.size() || nvs.getBytesLength(key)!=bytes.size()) return false;
            Bytes verified(bytes.size());
            return nvs.getBytes(key,verified.data(),verified.size())==verified.size() && verified==bytes;
        };
        // Each NVS blob update is atomic. An interruption between the two leaves
        // navigation old/absent; reciprocal restore validation rejects orphans.
        const bool saved=writeVerified("devices",b) && writeVerified("twins",navigation);
        nvs.end();return saved;
    }
};
}
