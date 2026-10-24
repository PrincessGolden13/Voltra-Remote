#include <Arduino.h>
#include "hardware/boards/guition_k5/Board.h"
#include "hardware/boards/guition_k5/ConfigurationStore.h"
#include "hardware/boards/guition_k5/DeviceStore.h"
#include "hardware/boards/guition_k5/BootMode.h"
#include "ui/UiController.h"
#ifdef VOLTRA_SIMULATOR
#include "MockTransport.h"
#else
#include "ble/NimBleTransport.h"
#include "protocol/PhysicalSession.h"
#include <esp_system.h>
#if __has_include("LocalConfig.h")
#include "LocalConfig.h" // Ignored, local-only diagnostic identities; never Git.
#endif
#endif
namespace {
voltra::guition::Board board;
voltra::guition::BootMode bootMode;
#ifdef VOLTRA_SIMULATOR
voltra::MockTransport transport;
voltra::ControlSession& session=transport;
#else
voltra::NimBleTransport transport;
std::array<uint8_t,16> clientIdentity() {
    std::array<uint8_t,16> identity{};Preferences nvs;
    if(nvs.begin("voltra-client",false)) {
        if(nvs.getBytesLength("identity")!=identity.size() || nvs.getBytes("identity",identity.data(),identity.size())!=identity.size()) {
            esp_fill_random(identity.data(),identity.size());identity[6]=(identity[6]&15)|0x40;identity[8]=(identity[8]&63)|0x80;
            nvs.putBytes("identity",identity.data(),identity.size());
        }
        nvs.end();
    } else esp_fill_random(identity.data(),identity.size());
    return identity;
}
voltra::PhysicalSession* physicalSession=nullptr;
#endif
voltra::guition::ConfigurationStore configuration;
voltra::guition::DeviceStore devices(bootMode.simulated());
voltra::VoltraController* controller=nullptr;
voltra::TrainingApplication* application=nullptr;
voltra::UiController* ui=nullptr;
}
void setup() {
    Serial.begin(115200);delay(1500);
    Serial.printf("VOLTRA Guition K5 - %s\n",bootMode.simulated() ? "SIMULATOR / no physical BLE" : "NORMAL / physical BLE");
    Serial.printf("Flash %u PSRAM %u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
    if(!board.begin()) { Serial.println("Hardware init failed; no radio or machine commands");return; }
    if(!bootMode.begin()) { Serial.println("Normal boot target unavailable; mode switching cannot be validated"); }
#ifndef VOLTRA_SIMULATOR
    transport.onDiagnostic([](const std::string& message){Serial.printf("%llu %s\n",static_cast<unsigned long long>(board.nowMs()),message.c_str());});
    if(!transport.begin()) {Serial.println("BLE init failed");return;}
#if defined(VOLTRA_LOCAL_BOOTSTRAP_PROFILE) && VOLTRA_LOCAL_BOOTSTRAP_PROFILE
    Serial.println("Local bootstrap identity; load requires explicit input and fresh readiness");
    physicalSession=new voltra::PhysicalSession(transport,voltra::local::finishIdentity,voltra::local::helloIdentity);
#else
    physicalSession=new voltra::PhysicalSession(transport,clientIdentity());
#endif
    auto& session=*physicalSession;
#endif
    controller=new voltra::VoltraController(transport,{},&session);
    application=new voltra::TrainingApplication(transport,*controller,session,&devices,
#ifdef VOLTRA_SIMULATOR
        transport.catalog(),false);
#else
        std::vector<voltra::DeviceEntry>{},true);
#endif
    ui=new voltra::UiController(board,*application,&configuration,&bootMode);
#ifdef VOLTRA_SIMULATOR
    Serial.println("Shared UI started. Serial: r=rep, h=hold, m=move, u=unload, d=devices");
#else
    Serial.println("Shared UI started. Serial: d=devices, c=connect single available (no load command)");
#endif
}
void loop() {
    if(!ui) {delay(100);return;}
    board.poll();
    // Deliver each sampled press/release before rendering instead of waiting for
    // LVGL's separate input timer, which can skip a short tap between frames.
    ui->readTouch();
    while(Serial.available()) {
        const char c=Serial.read();
#ifdef VOLTRA_SIMULATOR
        if(c=='r') transport.completeRep();if(c=='h') transport.setHeld(true);if(c=='m') transport.setHeld(false);
        if(c=='u') application->unload();
#else
        // Development connection aid only. It never changes weight or loads.
        if(c=='c' && !application->connected() && !application->connecting()) {
            size_t online=0;voltra::DeviceId target;
            for(const auto& d:application->devices()) if(d.online) {++online;target=d.id;}
            if(online==1 && application->select(target)) ui->navigate(voltra::Screen::Main);
        }
#endif
        if(c=='d') {application->leave();ui->navigate(voltra::Screen::Devices);}
    }
    ui->tick();
#ifndef VOLTRA_SIMULATOR
    static uint64_t diagnosticsAt=0;
    static std::string previousDiagnostics;
    const auto now=board.nowMs();
    if(now-diagnosticsAt>=1000) {
        diagnosticsAt=now;
        size_t online=0;for(const auto& d:application->devices()) if(d.online) ++online;
        const auto report=application->report();
        const auto summary=std::string("VOLTRA session: online=")+std::to_string(online)+
            " connected="+std::to_string(application->connected())+" accepted="+std::to_string(physicalSession->accepted())+
            " fresh="+std::to_string(application->fresh())+" weightTicks="+std::to_string(report.revision ? report.settings.weight : -1)+
            " loaded="+std::to_string(report.revision ? int(report.loaded) : -1)+
            " status="+application->message();
        if(summary!=previousDiagnostics) {Serial.println(summary.c_str());previousDiagnostics=summary;}
    }
#endif
    delay(5);
}
