#pragma once
#include "state/RemoteConfiguration.h"
#include <Preferences.h>
#include <nvs.h>
namespace voltra::guition {
// Remote preferences and recipes. No machine identifiers, load state or base weight.
class ConfigurationStore final : public RemoteConfigurationStore {
    static constexpr int version=2, headerWords=6, recipeWords=14;
    // A separate key lets OTA rollback keep reading the existing recipe blob.
    static constexpr const char* loadedAdjustmentKey="loaded-adjust";
    bool writable_=true;
public:
    RemoteConfiguration load() override {
        RemoteConfiguration c;writable_=true;
        nvs_handle_t handle;const auto opened=nvs_open("voltra-ui",NVS_READONLY,&handle);
        if(opened!=ESP_OK) {
            writable_=opened==ESP_ERR_NVS_NOT_FOUND;
            c.loadedWeightAdjustment=writable_;
            return c;
        }
        uint8_t adjustment=0;
        const auto preference=nvs_get_u8(handle,loadedAdjustmentKey,&adjustment);
        c.loadedWeightAdjustment=preference==ESP_ERR_NVS_NOT_FOUND || (preference==ESP_OK && adjustment==1);
        size_t bytes=0;const auto found=nvs_get_blob(handle,"config",nullptr,&bytes);
        if(found==ESP_ERR_NVS_NOT_FOUND) {nvs_close(handle);return c;}
        if(found!=ESP_OK || bytes<headerWords*4 || bytes>(headerWords+99*recipeWords)*4 || bytes%4) {
            writable_=false;nvs_close(handle);return c;
        }
        std::vector<int32_t> data(bytes/4);size_t actual=bytes;
        const bool read=nvs_get_blob(handle,"config",data.data(),&actual)==ESP_OK && actual==bytes;nvs_close(handle);
        if(!read) {writable_=false;return c;}
        // An older firmware must not overwrite a future schema with defaults.
        if(data[0]!=1 && data[0]!=version) {writable_=false;return c;}
        if(data[1]>=0 && data[1]<=8) c.theme=data[1];
        if(data[2]>=0 && data[2]<=3599) c.restAlertSeconds=data[2];
        if(data[3]>=0 && data[3]<=120) c.dimMinutes=data[3];
        if(data[4]>=0 && data[4]<=120) c.sleepMinutes=data[4];
        if(data[1]<0 || data[1]>8 || data[2]<0 || data[2]>3599 || data[3]<0 || data[3]>120 || data[4]<0 || data[4]>120 ||
           data[5]<0 || data[5]>99 || data.size()!=size_t(headerWords+data[5]*recipeWords)) {writable_=false;return c;}
        for(int i=0;i<data[5];++i) {
            const auto* d=data.data()+headerWords+i*recipeWords;
            if(d[0]<0 || d[0]>2 || d[1]<0 || d[1]>1 || d[2]<0 || d[2]>10000 || d[3]<0 || d[3]>1 ||
               d[4]<0 || d[4]>1 || d[5]<0 || d[5]>10000 || d[6]<0 || d[6]>1 || d[7]<0 || d[7]>1 ||
               d[8]<0 || d[8]>1 || d[9]<0 || d[9]>1 || d[10]<-10000 || d[10]>10000 ||
               d[11]<0 || d[11]>9 || d[12]<0 || d[12]>80 || d[13]<1 || d[13]>9) {writable_=false;return c;}
            Preset p;p.mode=MagicMode(d[0]);p.magic={bool(d[1]),d[2],bool(d[3])};p.eccentric={bool(d[4]),d[5],bool(d[6])};
            p.unit=WeightUnit(d[7]);p.drops={bool(d[8]),DropMode(d[9]),d[10],d[11],d[12],d[13]};
            if(data[0]==1 && p.unit==WeightUnit::Kilograms) {
                if(!p.magic.percent) p.magic.value*=2;
                if(!p.eccentric.percent) p.eccentric.value*=2;
                p.drops.step*=2;
            }
            if(p.magic.value==0) p.magic.enabled=false;
            if(p.eccentric.value==0) p.eccentric.enabled=false;
            c.presets.push_back(p);
        }
        return c;
    }
    bool save(const RemoteConfiguration& c) override {
        if(!writable_ || c.presets.size()>99) return false;
        std::vector<int32_t> data={version,c.theme,c.restAlertSeconds,c.dimMinutes,c.sleepMinutes,int(c.presets.size())};
        for(const auto& p:c.presets) {
            const int32_t recipe[]={int(p.mode),p.magic.enabled,p.magic.value,p.magic.percent,p.eccentric.enabled,
                p.eccentric.value,p.eccentric.percent,int(p.unit),p.drops.enabled,int(p.drops.mode),p.drops.step,p.drops.drops,p.drops.percent,p.drops.holdSeconds};
            data.insert(data.end(),std::begin(recipe),std::end(recipe));
        }
        Preferences nvs;if(!nvs.begin("voltra-ui",false)) return false;
        const size_t bytes=data.size()*4;
        std::vector<int32_t> persisted(data.size());
        const bool unchanged=nvs.getType("config")==PT_BLOB && nvs.getBytesLength("config")==bytes &&
            nvs.getBytes("config",persisted.data(),bytes)==bytes && persisted==data;
        const bool saved=(unchanged || nvs.putBytes("config",data.data(),bytes)==bytes) &&
            nvs.getBytesLength("config")==bytes && nvs.getBytes("config",persisted.data(),bytes)==bytes && persisted==data;
        const uint8_t adjustment=c.loadedWeightAdjustment;
        const bool preferenceSaved=saved && (nvs.getUChar(loadedAdjustmentKey,2)==adjustment ||
            (adjustment && !nvs.isKey(loadedAdjustmentKey)) ||
            (nvs.putUChar(loadedAdjustmentKey,adjustment)==1 && nvs.getUChar(loadedAdjustmentKey,2)==adjustment));
        nvs.end();return preferenceSaved;
    }
};
}
