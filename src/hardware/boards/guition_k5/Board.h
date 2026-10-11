#pragma once
#include "hardware/interfaces/HardwarePlatform.h"
#include <driver/spi_master.h>
#include <Arduino.h>
namespace voltra::guition {
// JC3636K518 working target, authorized for offline physical validation.
class Board final : public HardwarePlatform, private Display, private TouchInput, private RotaryInput {
public:
    bool begin();
    void poll();
    HardwareCapabilities capabilities() const override;
    Display& display() override { return *this; }
    TouchInput& touch() override { return *this; }
    RotaryInput& rotary() override { return *this; }
    uint64_t nowMs() const override;
    void setDisplayPower(bool asleep,bool dimmed) override;
private:
    static constexpr int resolution=360, rowsPerTransfer=8;
    // Source pin mapping: referenced JC3636K518 implementation, not prototype_21.
    static constexpr int backlightPin=47, resetPin=21, csPin=14, clockPin=13;
    static constexpr int d0Pin=15,d1Pin=16,d2Pin=17,d3Pin=18;
    static constexpr int touchSda=11,touchScl=12,touchReset=10,touchIrq=9;
    static constexpr int encoderA=8,encoderB=7,pressPin=0;
    static constexpr int encoderDirection=1, transitionsPerDetent=4;
    spi_device_handle_t spi_=nullptr;
    uint8_t* pixels_=nullptr;
    TouchPoint touchPoint_;
    volatile int edges_=0;
    volatile uint8_t encoderState_=0;
    portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
    int remainder_=0;
    bool ready_=false,pressCandidate_=false,pressStable_=false,pressed_=false;
    uint32_t pressChanged_=0,lastTouch_=0;
    static void IRAM_ATTR encoderInterrupt(void*);
    bool transfer(uint8_t reg,const void*,size_t,bool quad=false);
    uint16_t width() const override { return resolution; }
    uint16_t height() const override { return resolution; }
    void flush(int,int,int,int,const uint32_t*) override;
    TouchPoint sample() const override { return touchPoint_; }
    int getDelta() override;
    bool wasPressed() override { bool p=pressed_;pressed_=false;return p; }
};
}
