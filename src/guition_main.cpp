#include <Arduino.h>
#include "hardware/boards/guition_k5/Board.h"
#include "MockTransport.h"
#include "hardware/boards/guition_k5/ConfigurationStore.h"
#include "ui/UiController.h"
#ifndef VOLTRA_OFFLINE
#error "This bring-up firmware must not link real BLE"
#endif
namespace {
voltra::guition::Board board;
voltra::MockTransport transport;
voltra::guition::ConfigurationStore configuration;
voltra::VoltraController controller(transport,{},&transport);
voltra::TrainingApplication application(transport,controller,transport,nullptr,transport.catalog());
voltra::UiController* ui=nullptr;
}
void setup() {
    Serial.begin(115200);delay(1500);
    Serial.println("VOLTRA Guition K5 - OFFLINE hardware validation");
    Serial.printf("Flash %u PSRAM %u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
    if(!board.begin()) { Serial.println("Hardware init failed; no radio or machine commands");return; }
    ui=new voltra::UiController(board,application,&configuration);
    Serial.println("Shared UI started. Serial: r=rep, h=hold, m=move, u=unload, d=devices");
}
void loop() {
    if(!ui) {delay(100);return;}
    board.poll();
    // Deliver each sampled press/release before rendering instead of waiting for
    // LVGL's separate input timer, which can skip a short tap between frames.
    ui->readTouch();
    while(Serial.available()) {
        const char c=Serial.read();
        if(c=='r') transport.completeRep();if(c=='h') transport.setHeld(true);if(c=='m') transport.setHeld(false);
        if(c=='u' && (application.report().loaded || application.report().autoLoadArmed || !application.fresh())) application.toggleWeight();
        if(c=='d') {application.leave();ui->navigate(voltra::Screen::Devices);}
    }
    ui->tick();delay(5);
}
