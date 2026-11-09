#pragma once
#include <Preferences.h>
#include <array>
#include <algorithm>
#include <optional>
namespace voltra::guition {
// Device-local bootstrap only. Never packaged, logged, or sent to GitHub.
// A local USB build can retain the owner's working profile across public OTAs.
struct BootstrapProfile {
    std::array<uint8_t,16> finish;
    std::array<uint8_t,6> hello;
};
struct BootstrapStore {
    static bool save(const BootstrapProfile& profile) {
        Preferences nvs;if(!nvs.begin("voltra-client",false)) return false;
        std::array<uint8_t,23> bytes{};bytes[0]=1;
        std::copy(profile.finish.begin(),profile.finish.end(),bytes.begin()+1);
        std::copy(profile.hello.begin(),profile.hello.end(),bytes.begin()+17);
        const bool ok=nvs.putBytes("bootstrap",bytes.data(),bytes.size())==bytes.size();nvs.end();return ok;
    }
    static std::optional<BootstrapProfile> load() {
        Preferences nvs;if(!nvs.begin("voltra-client",true)) return {};
        std::array<uint8_t,23> bytes{};
        const bool ok=nvs.getBytesLength("bootstrap")==bytes.size() && nvs.getBytes("bootstrap",bytes.data(),bytes.size())==bytes.size();nvs.end();
        if(!ok || bytes[0]!=1) return {};
        BootstrapProfile profile{};std::copy_n(bytes.begin()+1,16,profile.finish.begin());std::copy_n(bytes.begin()+17,6,profile.hello.begin());return profile;
    }
};
}
