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
        std::ifstream input(path_); std::vector<DeviceEntry> result;
        std::string id, name;
        while (result.size() < maximumRememberedDevices && input >> std::quoted(id) >> std::quoted(name)) {
            if (id.empty() || id.size() > 128 || name.size() > 128) continue;
            if (std::none_of(result.begin(), result.end(), [&](const auto& d) { return d.id == id; }))
                result.push_back({id, name, false});
        }
        return result;
    }
    bool save(const std::vector<DeviceEntry>& recent) override {
        if (recent.size() > maximumRememberedDevices) return false;
        auto temp = path_; temp += ".tmp";
        { std::ofstream out(temp, std::ios::trunc);
          for (size_t i = 0; i < recent.size(); ++i)
              out << std::quoted(recent[i].id) << ' ' << std::quoted(recent[i].name) << '\n';
          out.close(); if (!out) return false; }
#ifdef _WIN32
        return MoveFileExW(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        // POSIX rename atomically replaces an existing file on the same filesystem.
        std::error_code error;
        std::filesystem::rename(temp, path_, error);
        return !error;
#endif
    }
private:
    std::filesystem::path path_;
};
}
