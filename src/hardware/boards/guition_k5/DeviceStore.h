#pragma once
#include "controller/DeviceMemory.h"
#include <Preferences.h>
#include <nvs.h>
#include <algorithm>
namespace voltra::guition {
// Separate namespaces for real identities and synthetic fixtures. No load/settings.
class DeviceStore final : public DeviceMemory {
    const char* name_;
    bool writable_=false;
public:
    explicit DeviceStore(bool simulated) : name_(simulated ? "voltra-sim-dev" : "voltra-devices") {}
    std::vector<DeviceEntry> load() override {
        writable_=false;
        std::vector<DeviceEntry> result;
        nvs_handle_t handle;
        const auto opened=nvs_open(name_,NVS_READONLY,&handle);
        if(opened==ESP_ERR_NVS_NOT_FOUND) {writable_=true;return result;}
        if(opened!=ESP_OK) return result;
        size_t size=0;
        const auto found=nvs_get_blob(handle,"devices",nullptr,&size);
        if(found==ESP_ERR_NVS_NOT_FOUND) {nvs_close(handle);writable_=true;return result;}
        if(found!=ESP_OK || size<2 || size>16384) {nvs_close(handle);return result;}
        Bytes b(size);size_t actual=size;
        const bool read=nvs_get_blob(handle,"devices",b.data(),&actual)==ESP_OK && actual==size;
        nvs_close(handle);
        if (!read || b[0]!=1 || b[1]>maximumRememberedDevices) return {};
        size_t at=2;
        for (unsigned i=0;i<b[1];++i) {
            if (at+2>b.size()) return {};
            const size_t ids=b[at++], names=b[at++];
            if (!ids || ids>128 || names>128 || at+ids+names>b.size()) return {};
            DeviceEntry d; d.id=std::string(b.begin()+at,b.begin()+at+ids); at+=ids;
            d.name=std::string(b.begin()+at,b.begin()+at+names); at+=names;
            if (std::none_of(result.begin(),result.end(),[&](const auto& e){return e.id==d.id;})) result.push_back(d);
        }
        if(at!=b.size()) return {};
        writable_=true;
        return result;
    }
    bool save(const std::vector<DeviceEntry>& entries) override {
        // A failed/malformed/future-schema read must not become an empty record
        // on the next successful connection. Only genuinely absent data is new.
        if (!writable_ || entries.size()>maximumRememberedDevices) return false;
        Bytes b{1,uint8_t(entries.size())};
        for (const auto& d:entries) {
            if (d.id.empty() || d.id.size()>128 || d.name.size()>128) return false;
            b.push_back(uint8_t(d.id.size())); b.push_back(uint8_t(d.name.size()));
            b.insert(b.end(),d.id.begin(),d.id.end()); b.insert(b.end(),d.name.begin(),d.name.end());
        }
        if (b.size()>16384) return false;
        Preferences nvs; if (!nvs.begin(name_,false)) return false;
        const bool saved=nvs.putBytes("devices",b.data(),b.size())==b.size(); nvs.end(); return saved;
    }
};
}
