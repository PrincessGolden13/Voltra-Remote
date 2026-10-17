#pragma once
#include "state/RemoteConfiguration.h"
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace voltra {
class DesktopConfigurationStore final : public RemoteConfigurationStore {
public:
    explicit DesktopConfigurationStore(std::filesystem::path path) : path_(std::move(path)) {}
    RemoteConfiguration load() override {
        RemoteConfiguration c; std::ifstream in(path_); int version, count;
        if (!(in >> version >> c.theme >> c.restAlertSeconds >> c.dimMinutes >> c.sleepMinutes >> count)
            || (version < 1 || version > 3) || c.theme < 0 || c.theme > 8 || c.restAlertSeconds < 0 || c.restAlertSeconds > 3599
            || c.dimMinutes < 0 || c.dimMinutes > 120 || c.sleepMinutes < 0 || c.sleepMinutes > 120
            || count < 0 || count > 99) return {};
        for (int i=0; i<count; ++i) {
            Preset p; int mode, unit, dropMode; bool legacyPercent;
            if (!(in >> mode >> p.magic.enabled >> p.magic.value >> p.eccentric.enabled >> p.eccentric.value
                >> legacyPercent >> unit >> p.drops.enabled >> dropMode >> p.drops.step >> p.drops.drops
                >> p.drops.percent >> p.drops.holdSeconds) || mode < 0 || mode > 2 || unit < 0 || unit > 1
                || dropMode < 0 || dropMode > 1 || p.magic.value < 0 || p.magic.value > 10000
                || p.eccentric.value < 0 || p.eccentric.value > 10000 || p.drops.step > 10000 || p.drops.step < -10000
                || p.drops.drops < 0 || p.drops.drops > 9 || p.drops.percent < 0 || p.drops.percent > 80
                || p.drops.holdSeconds < 1 || p.drops.holdSeconds > 9) return {};
            if (version >= 2) { if (!(in >> p.magic.percent >> p.eccentric.percent)) return {}; }
            else p.magic.percent = p.eccentric.percent = legacyPercent;
            p.mode = MagicMode(mode); p.unit = WeightUnit(unit); p.drops.mode = DropMode(dropMode);
            if(version < 3 && p.unit == WeightUnit::Kilograms) { if(!p.magic.percent) p.magic.value*=2; if(!p.eccentric.percent) p.eccentric.value*=2; p.drops.step*=2; }
            if (p.magic.value == 0) p.magic.enabled = false;
            if (p.eccentric.value == 0) p.eccentric.enabled = false;
            c.presets.push_back(p);
        }
        return c;
    }
    bool save(const RemoteConfiguration& c) override {
        auto temp = path_; temp += ".tmp";
        std::ofstream out(temp, std::ios::trunc);
        out << 3 << ' ' << c.theme << ' ' << c.restAlertSeconds << ' ' << c.dimMinutes << ' ' << c.sleepMinutes << ' ' << c.presets.size() << '\n';
        for (const auto& p : c.presets)
            out << int(p.mode) << ' ' << p.magic.enabled << ' ' << p.magic.value << ' ' << p.eccentric.enabled << ' '
                << p.eccentric.value << ' ' << p.magic.percent << ' ' << int(p.unit) << ' ' << p.drops.enabled << ' '
                << int(p.drops.mode) << ' ' << p.drops.step << ' ' << p.drops.drops << ' ' << p.drops.percent << ' ' << p.drops.holdSeconds << ' ' << p.magic.percent << ' ' << p.eccentric.percent << '\n';
        out.close(); if (!out) return false;
#ifdef _WIN32
        return MoveFileExW(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::error_code error; std::filesystem::rename(temp,path_,error); return !error;
#endif
    }
private:
    std::filesystem::path path_;
};
}
