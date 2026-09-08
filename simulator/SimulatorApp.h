#pragma once
#include "DesktopHardwarePlatform.h"
#include "DesktopDeviceMemory.h"
#include "MockTransport.h"
#include "ui/UiController.h"
#include <iostream>
#include <stdexcept>

// Shared offline runtime: native hosts only supply windows, input and storage paths.
namespace voltra {
struct SimulatorOptions {
    int size = 480, profile = 0, screen = 0, device = 0, dropMode = 0;
    std::string renderPath;
    static SimulatorOptions parse(int argc, char** argv) {
        SimulatorOptions o;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (i + 1 >= argc) throw std::runtime_error("Missing argument value");
            const std::string value = argv[++i];
            if (option == "--render") { o.renderPath = value; continue; }
            size_t consumed = 0;
            const int number = std::stoi(value, &consumed);
            if (consumed != value.size()) throw std::runtime_error("Expected an integer argument");
            if (option == "--size") o.size = number;
            else if (option == "--profile") o.profile = number;
            else if (option == "--screen") o.screen = number;
            else if (option == "--device") o.device = number;
            else if (option == "--drop-mode") o.dropMode = number;
            else throw std::runtime_error("Unknown argument: " + option);
        }
        if ((o.size != 360 && o.size != 466 && o.size != 480) || o.profile < 0 || o.profile > 3)
            throw std::runtime_error("Supported sizes: 360, 466, 480. Profiles: 0-3.");
        if (o.screen < 0 || o.screen > 4 || o.device < 0 || o.device > 4 || o.dropMode < 0 || o.dropMode > 1)
            throw std::runtime_error("Screens 0-4; devices 0-4; drops 0 stairs, 1 rest-pause.");
        return o;
    }
};

struct SimulatorApp {
    DesktopHardwarePlatform hardware;
    MockTransport transport;
    VoltraController controller;
    DesktopDeviceMemory memory;
    TrainingApplication application;
    UiController ui;
    SimulatorApp(const SimulatorOptions& o, const std::filesystem::path& memoryPath)
        : hardware(uint16_t(o.size)), controller(transport, {}, &transport), memory(memoryPath),
          application(transport, controller, transport, o.renderPath.empty() ? &memory : nullptr),
          ui(hardware, application) {
        ui.tick();
        if (o.screen != 0) {
            application.select(o.device == 0 ? MockTransport::deviceId :
                std::string(MockTransport::deviceId) + '-' + std::to_string(o.device));
            transport.setProfile(static_cast<SimulatorProfile>(o.profile));
            application.tick(hardware.nowMs());
            if (o.dropMode == 1) {
                auto drops = application.drops(); drops.mode = DropMode::RestPause;
                application.applyDrops(drops);
            }
            ui.navigate(static_cast<Screen>(o.screen));
            ui.tick();
        }
    }
    void functionKey(int key, bool repeat) {
        if (key >= 1 && key <= 4) {
            application.leave(); application.scan(); application.select(MockTransport::deviceId);
            transport.setProfile(static_cast<SimulatorProfile>(key - 1)); ui.navigate(Screen::Main);
        }
        if (key == 5) { application.leave(); ui.navigate(Screen::Devices); }
        if (key == 6) application.scan();
        if (key == 7 && !repeat) transport.toggleAvailability();
        if (key == 8 && !repeat) transport.completeRep();
        if (key == 9) transport.setHeld(true);
        if (key == 10) transport.setHeld(false);
        if (key == 11 && !repeat) transport.ignoreNextRequest();
    }
    bool render(const SimulatorOptions& o) {
        if (o.renderPath.empty()) return false;
        lv_refr_now(nullptr);
        // Portable, little-endian, top-down XRGB8888 BMP, identical on both hosts.
        std::ofstream out(o.renderPath, std::ios::binary);
        auto word = [&](uint32_t value, int bytes) {
            for (int i = 0; i < bytes; ++i) out.put(static_cast<char>((value >> (i * 8)) & 255));
        };
        const auto byteCount = uint32_t(hardware.pixels().size() * 4);
        word(0x4d42, 2); word(54 + byteCount, 4); word(0, 4); word(54, 4);
        word(40, 4); word(hardware.width(), 4); word(uint32_t(-int(hardware.height())), 4);
        word(1, 2); word(32, 2); word(0, 4); word(byteCount, 4);
        for (int i = 0; i < 4; ++i) word(0, 4);
        for (auto pixel : hardware.pixels()) word(pixel, 4);
        out.close();
        if (!out) throw std::runtime_error("Could not save render");
        std::cout << "Rendered offline LVGL simulator to " << o.renderPath << '\n';
        return true;
    }
};
}
