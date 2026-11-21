#pragma once
#ifdef _WIN32
#include <windows.h>
#endif
#include "controller/DeviceMemory.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <algorithm>
namespace voltra {
class DesktopDeviceMemory final : public DeviceMemory {
public:
    explicit DesktopDeviceMemory(std::filesystem::path path) : path_(std::move(path)) {}
    std::vector<DeviceEntry> load() override {
        navigationWritable_=false;
        std::ifstream input(path_);std::vector<DeviceEntry> result;
        std::string id,name;
        while(result.size()<maximumRememberedDevices && input >> std::quoted(id) >> std::quoted(name)) {
            if(id.empty() || id.size()>128 || name.size()>128) continue;
            if(std::none_of(result.begin(),result.end(),[&](const auto& d){return d.id==id;})) {
                DeviceEntry d;d.id=id;d.name=name;result.push_back(std::move(d));
            }
        }
        auto navigationPath=path_;navigationPath += ".twins";
        std::error_code error;
        if(!std::filesystem::exists(navigationPath,error)) {navigationWritable_=!error;return result;}
        const auto bytes=std::filesystem::file_size(navigationPath,error);
        if(error || bytes>16384) return result;
        std::ifstream navigation(navigationPath);
        std::string marker;unsigned version=0,count=0;
        if(!(navigation >> marker >> version >> count) || marker!="VOLTRA_TWIN_NAV" || version!=1 || count>maximumRememberedDevices) return result;
        std::vector<DeviceEntry> links;
        for(unsigned i=0;i<count;++i) {
            DeviceEntry d;
            if(!(navigation >> std::quoted(d.id) >> std::quoted(d.twinPeerId) >> std::quoted(d.twinPeer) >> std::quoted(d.twinSelectionId)) ||
               d.id.empty() || d.id.size()>128 || d.twinPeerId.empty() || d.twinPeerId.size()>128 || d.twinPeer.size()>128 ||
               d.twinSelectionId.size()>128 || d.twinPeerId==d.id ||
               (d.twinSelectionId!=d.id && d.twinSelectionId!=d.twinPeerId) ||
               std::any_of(links.begin(),links.end(),[&](const auto& entry){return entry.id==d.id;})) return result;
            links.push_back(std::move(d));
        }
        navigation >> std::ws;if(!navigation.eof()) return result;
        navigationWritable_=true;
        for(auto& d:result) {
            const auto link=std::find_if(links.begin(),links.end(),[&](const auto& entry){return entry.id==d.id;});
            if(link!=links.end()) {d.twinPeerId=link->twinPeerId;d.twinPeer=link->twinPeer;d.twinSelectionId=link->twinSelectionId;}
        }
        return result;
    }
    bool save(const std::vector<DeviceEntry>& recent) override {
        if(!navigationWritable_ || recent.size()>maximumRememberedDevices) return false;
        size_t navigationCount=0,totalBytes=4;
        for(const auto& d:recent) {
            if(d.id.empty() || d.id.size()>128 || d.name.size()>128 || d.twinPeerId.size()>128 ||
               d.twinPeer.size()>128 || d.twinSelectionId.size()>128) return false;
            totalBytes+=2+d.id.size()+d.name.size();
            if(d.twinPeerId.empty()) {
                if(!d.twinPeer.empty() || !d.twinSelectionId.empty()) return false;
            } else {
                if(d.twinPeerId==d.id || (d.twinSelectionId!=d.id && d.twinSelectionId!=d.twinPeerId)) return false;
                ++navigationCount;totalBytes+=4+d.id.size()+d.twinPeerId.size()+d.twinPeer.size()+d.twinSelectionId.size();
            }
        }
        if(totalBytes>16384) return false;
        auto temp=path_;temp += ".tmp";
        auto navigationPath=path_;navigationPath += ".twins";
        auto navigationTemp=navigationPath;navigationTemp += ".tmp";
        {
            std::ofstream out(temp,std::ios::trunc),navigation(navigationTemp,std::ios::trunc);
            // Keep the original ID/name file readable by older simulators.
            navigation << "VOLTRA_TWIN_NAV 1 " << navigationCount << '\n';
            for(const auto& d:recent) {
                out << std::quoted(d.id) << ' ' << std::quoted(d.name) << '\n';
                if(!d.twinPeerId.empty()) navigation << std::quoted(d.id) << ' ' << std::quoted(d.twinPeerId)
                    << ' ' << std::quoted(d.twinPeer) << ' ' << std::quoted(d.twinSelectionId) << '\n';
            }
            out.close();navigation.close();if(!out || !navigation) return false;
        }
        const auto replace=[](const std::filesystem::path& source,const std::filesystem::path& target) {
#ifdef _WIN32
            return MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)!=0;
#else
            std::error_code error;std::filesystem::rename(source,target,error);return !error;
#endif
        };
        // Each file replaces atomically; a partial pair is ignored on restore.
        return replace(temp,path_) && replace(navigationTemp,navigationPath);
    }
private:
    std::filesystem::path path_;
    bool navigationWritable_=false;
};
}
