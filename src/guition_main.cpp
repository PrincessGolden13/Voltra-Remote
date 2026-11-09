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
#include "network/Esp32WifiSetup.h"
#include "update/Esp32FirmwareUpdate.h"
#include "hardware/boards/guition_k5/BootstrapStore.h"
#include <esp_system.h>
#if !defined(VOLTRA_PUBLIC_BUILD) && __has_include("LocalConfig.h")
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
voltra::Esp32WifiSetup wifi;
voltra::Esp32FirmwareUpdate updates;
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
    const auto* running=esp_ota_get_running_partition();
    Serial.printf("Firmware %s / slot %s\n",VOLTRA_VERSION,running ? running->label : "unknown");
    Serial.printf("Flash %u PSRAM %u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
    if(!board.begin()) { Serial.println("Hardware init failed; no radio or machine commands");return; }
#ifndef VOLTRA_SIMULATOR
    transport.onDiagnostic([](const std::string& message){Serial.printf("%llu %s\n",static_cast<unsigned long long>(board.nowMs()),message.c_str());});
    if(!transport.begin()) {Serial.println("BLE init failed");return;}
#if defined(VOLTRA_LOCAL_BOOTSTRAP_PROFILE) && VOLTRA_LOCAL_BOOTSTRAP_PROFILE
    Serial.println("Local bootstrap identity; load requires explicit input and fresh readiness");
    if(!voltra::guition::BootstrapStore::save({voltra::local::finishIdentity,voltra::local::helloIdentity})) {
        Serial.println("Could not retain local bootstrap for firmware updates");return;
    }
    physicalSession=new voltra::PhysicalSession(transport,voltra::local::finishIdentity,voltra::local::helloIdentity);
#else
    const auto bootstrap=voltra::guition::BootstrapStore::load();
    Serial.println(bootstrap ? "Retained device-local bootstrap loaded" : "Generated client identity; pairing compatibility unverified");
    physicalSession=bootstrap ? new voltra::PhysicalSession(transport,bootstrap->finish,bootstrap->hello) :
        new voltra::PhysicalSession(transport,clientIdentity());
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
#ifdef VOLTRA_SIMULATOR
    ui=new voltra::UiController(board,*application,&configuration,&bootMode);
#else
    wifi.begin();updates.begin();ui=new voltra::UiController(board,*application,&configuration,&bootMode,&wifi,&updates);
#endif
#ifdef VOLTRA_SIMULATOR
    Serial.println("Shared UI started. Serial: r=rep, h=hold, m=move, u=unload, d=devices");
#else
    Serial.println("Shared UI started. Serial: d=devices, c=connect single available, v=check updates (no load command)");
#endif
    if(!bootMode.confirmStartup()) Serial.println("Firmware startup confirmation failed");
    if(!bootMode.begin()) Serial.println("Normal boot target unavailable; mode switching cannot be validated");
}
void loop() {
    if(!ui) {delay(100);return;}
    board.poll();
    // Drain the independent hardware sampler before rendering. Short gestures
    // remain ordered even when BLE work held up the preceding application loop.
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
        if(c=='v') updates.check(); // Read-only update check; never installs or controls resistance.
#endif
        if(c=='d') {application->leave();ui->navigate(voltra::Screen::Devices);}
    }
    ui->tick();
#ifndef VOLTRA_SIMULATOR
    static uint64_t diagnosticsAt=0;
    static std::string previousDiagnostics;
    static std::string previousWifiDiagnostics;
    const auto now=board.nowMs();
    if(now-diagnosticsAt>=1000) {
        diagnosticsAt=now;
        const auto ws=wifi.status();
        const auto wifiSummary=std::string("Remote Wi-Fi: saved=")+std::to_string(ws.saved)+
            " connected="+std::to_string(ws.connected)+" "+ws.message;
        if(wifiSummary!=previousWifiDiagnostics) {Serial.println(wifiSummary.c_str());previousWifiDiagnostics=wifiSummary;}
        size_t online=0;for(const auto& d:application->devices()) if(d.online) ++online;
        const auto report=application->report();
        const auto summary=std::string("VOLTRA session: online=")+std::to_string(online)+
            " connected="+std::to_string(application->connected())+" accepted="+std::to_string(physicalSession->accepted())+
            " fresh="+std::to_string(application->fresh())+" weightTicks="+std::to_string(report.revision ? report.settings.weight : -1)+
            " loaded="+std::to_string(report.revision ? int(report.loaded) : -1)+
            " autoArmed="+std::to_string(report.autoLoadArmed)+
            " drops="+std::to_string(application->drops().enabled)+
            " liveWeight="+std::to_string(report.liveWeightSupported)+
            " status="+application->message();
        if(summary!=previousDiagnostics) {Serial.println(summary.c_str());previousDiagnostics=summary;}
    }
#endif
    delay(5);
}
