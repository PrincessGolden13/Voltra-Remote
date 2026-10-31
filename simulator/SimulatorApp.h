#pragma once
#include "DesktopHardwarePlatform.h"
#include "DesktopDeviceMemory.h"
#include "DesktopConfigurationStore.h"
#include "MockTransport.h"
#include "ui/UiController.h"
#include <iostream>
#include <stdexcept>

// Shared offline runtime: native hosts only supply windows, input and storage paths.
namespace voltra {
struct SimulatorOptions {
    int size = 480, profile = 0, screen = 0, device = 0, dropMode = 0, liveDrops = 0, modifierValue = -1, weightUnit = -1, weightTicks = -1;
    int holdElapsedMs=-1, holdSeconds=1, autoUnloadSeconds=-1;
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
            else if (option == "--modifier-value") o.modifierValue = number;
            else if (option == "--weight-unit") o.weightUnit = number;
            else if (option == "--weight-ticks") o.weightTicks = number;
            else if (option == "--live-drops") o.liveDrops = number;
            else if (option == "--hold-ms") o.holdElapsedMs=number;
            else if (option == "--hold-seconds") o.holdSeconds=number;
            else if (option == "--auto-unload-seconds") o.autoUnloadSeconds=number;
            else throw std::runtime_error("Unknown argument: " + option);
        }
        if ((o.size != 360 && o.size != 466 && o.size != 480) || o.profile < 0 || o.profile > 3)
            throw std::runtime_error("Supported sizes: 360, 466, 480. Profiles: 0-3.");
        if (o.screen < 0 || o.screen > 15 || o.device < 0 || o.device > 5 || o.dropMode < 0 || o.dropMode > 1 || o.liveDrops < 0 || o.liveDrops > 1)
            throw std::runtime_error("Screens 0-15; devices 0-5; drops 0 stairs, 1 rest-pause.");
        if(o.modifierValue < -1 || o.modifierValue > 300 || (o.modifierValue >= 0 && o.renderPath.empty()))
            throw std::runtime_error("Modifier preview 0-300 requires --render");
        if (o.weightUnit < -1 || o.weightUnit > 1 || (o.weightUnit >= 0 && o.renderPath.empty()))
            throw std::runtime_error("Unit preview 0 LB / 1 KG requires --render");
        if (o.weightTicks < -1 || o.weightTicks > 10000 || (o.weightTicks >= 0 && o.renderPath.empty()))
            throw std::runtime_error("Weight preview 0-10000 ticks requires --render");
        if(o.holdElapsedMs < -1 || o.holdElapsedMs > 10000 || o.holdSeconds<1 || o.holdSeconds>9 ||
           o.autoUnloadSeconds < -1 || o.autoUnloadSeconds>10 ||
           ((o.holdElapsedMs>=0 || o.holdSeconds!=1 || o.autoUnloadSeconds>=0) && o.renderPath.empty()))
            throw std::runtime_error("Render-only hold: 0-10000 ms / 1-9 s; Auto Unload: 0-10 s");
        if(o.holdElapsedMs>=0 && (o.profile!=1 || o.dropMode!=1 || !o.liveDrops))
            throw std::runtime_error("Hold preview requires loaded profile 1 and enabled RP drops");
        return o;
    }
};

struct SimulatorApp {
    DesktopHardwarePlatform hardware;
    MockTransport transport;
    VoltraController controller;
    DesktopDeviceMemory memory;
    DesktopConfigurationStore configuration;
    TrainingApplication application;
    UiController ui;
    SimulatorApp(const SimulatorOptions& o, const std::filesystem::path& memoryPath)
        : hardware(uint16_t(o.size)), controller(transport, {}, &transport), memory(memoryPath),
          configuration(memoryPath.parent_path() / "remote-settings.txt"),
          application(transport, controller, transport, o.renderPath.empty() ? &memory : nullptr, transport.catalog()),
          ui(hardware, application, o.renderPath.empty() ? &configuration : nullptr) {
        ui.tick();
        if(!o.renderPath.empty()) hardware.previewClock(hardware.nowMs()+1000);
        if (o.screen != 0) {
            application.select(o.device == 0 ? MockTransport::deviceId :
                std::string(MockTransport::deviceId) + '-' + std::to_string(o.device));
            transport.setProfile(static_cast<SimulatorProfile>(o.profile));
            if(o.weightUnit >= 0) transport.previewUnit(static_cast<WeightUnit>(o.weightUnit));
            if(o.weightTicks >= 0) transport.previewWeight(o.weightTicks);
            if(o.modifierValue >= 0) transport.previewModifier(o.modifierValue);
            if(o.autoUnloadSeconds>=0) transport.previewAutoUnload(o.autoUnloadSeconds);
            application.tick(hardware.nowMs());
            if (o.dropMode == 1 || o.liveDrops == 1) {
                auto drops = application.drops(); drops.mode = o.dropMode ? DropMode::RestPause : DropMode::Stairs;
                drops.enabled = o.liveDrops == 1;
                drops.holdSeconds=o.holdSeconds;
                application.applyDrops(drops);
            }
            if(o.holdElapsedMs>=0) {
                // Exercise the ordinary mock motion -> controller hold path;
                // no direct UI progress injection and no physical transport.
                application.tick(hardware.nowMs());
                hardware.previewClock(hardware.nowMs()+100);
                transport.completeRep();application.tick(hardware.nowMs());
                hardware.previewClock(hardware.nowMs()+100);
                transport.setHeld(true);application.tick(hardware.nowMs());
                const auto holdStart=hardware.nowMs();
                for(int elapsed=50;elapsed<=o.holdElapsedMs;elapsed+=50) {
                    hardware.previewClock(holdStart+elapsed);application.tick(hardware.nowMs());
                }
                hardware.previewClock(holdStart+o.holdElapsedMs);application.tick(hardware.nowMs());
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
