#pragma once
#include "state/RemoteConfiguration.h"
#include <Preferences.h>
namespace voltra::guition {
// A single remote-wide NVS blob. No machine identifiers, load state or base weight.
class ConfigurationStore final : public RemoteConfigurationStore {
    static constexpr int version=1, headerWords=6, recipeWords=14;
public:
    RemoteConfiguration load() override {
        RemoteConfiguration c;Preferences nvs;if(!nvs.begin("voltra-ui",true)) return c;
        const size_t bytes=nvs.getBytesLength("config");
        if(bytes<headerWords*4 || bytes>(headerWords+99*recipeWords)*4 || bytes%4) {nvs.end();return c;}
        std::vector<int32_t> data(bytes/4);const bool read=nvs.getBytes("config",data.data(),bytes)==bytes;nvs.end();
        if(!read || data[0]!=version || data[1]<0 || data[1]>8 || data[2]<0 || data[2]>3599 ||
           data[3]<0 || data[3]>120 || data[4]<0 || data[4]>120 || data[5]<0 || data[5]>99 ||
           data.size()!=size_t(headerWords+data[5]*recipeWords)) return {};
        c.theme=data[1];c.restAlertSeconds=data[2];c.dimMinutes=data[3];c.sleepMinutes=data[4];
        for(int i=0;i<data[5];++i) {
            const auto* d=data.data()+headerWords+i*recipeWords;
            if(d[0]<0 || d[0]>2 || d[1]<0 || d[1]>1 || d[2]<0 || d[2]>10000 || d[3]<0 || d[3]>1 ||
               d[4]<0 || d[4]>1 || d[5]<0 || d[5]>10000 || d[6]<0 || d[6]>1 || d[7]<0 || d[7]>1 ||
               d[8]<0 || d[8]>1 || d[9]<0 || d[9]>1 || d[10]<-10000 || d[10]>10000 ||
               d[11]<0 || d[11]>9 || d[12]<0 || d[12]>80 || d[13]<1 || d[13]>9) return {};
            Preset p;p.mode=MagicMode(d[0]);p.magic={bool(d[1]),d[2],bool(d[3])};p.eccentric={bool(d[4]),d[5],bool(d[6])};
            p.unit=WeightUnit(d[7]);p.drops={bool(d[8]),DropMode(d[9]),d[10],d[11],d[12],d[13]};c.presets.push_back(p);
        }
        return c;
    }
    bool save(const RemoteConfiguration& c) override {
        if(c.presets.size()>99) return false;
        std::vector<int32_t> data={version,c.theme,c.restAlertSeconds,c.dimMinutes,c.sleepMinutes,int(c.presets.size())};
        for(const auto& p:c.presets) {
            const int32_t recipe[]={int(p.mode),p.magic.enabled,p.magic.value,p.magic.percent,p.eccentric.enabled,
                p.eccentric.value,p.eccentric.percent,int(p.unit),p.drops.enabled,int(p.drops.mode),p.drops.step,p.drops.drops,p.drops.percent,p.drops.holdSeconds};
            data.insert(data.end(),std::begin(recipe),std::end(recipe));
        }
        Preferences nvs;if(!nvs.begin("voltra-ui",false)) return false;
        const size_t bytes=data.size()*4;
        const bool saved=nvs.putBytes("config",data.data(),bytes)==bytes;nvs.end();return saved;
    }
};
}
