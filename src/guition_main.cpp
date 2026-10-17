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
    if(!transport.begin()) {Serial.println("BLE init failed");return;}
    physicalSession=new voltra::PhysicalSession(transport,clientIdentity());
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
    Serial.println("Shared UI started. Connection diagnostics; physical loading unavailable. Serial: d=devices");
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
#endif
        if(c=='d') {application->leave();ui->navigate(voltra::Screen::Devices);}
    }
    ui->tick();delay(5);
}
