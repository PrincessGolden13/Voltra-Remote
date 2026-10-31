#pragma once
#include "hardware/interfaces/HardwarePlatform.h"
#include <driver/spi_master.h>
#include <Arduino.h>
#include <array>
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
    // K5 is a bidirectional pulse switch, not an A/B quadrature encoder.
    // Reference bidi_switch_knob: GPIO 8 = right, GPIO 7 = left.
    static constexpr int encoderDirection=1;
    static constexpr uint32_t minimumPulseUs=3000;
    // Input capture must not wait for a synchronous BLE write or frame flush.
    // Policies: poll every 8 ms; at most 64 changed samples; discard input older
    // than 350 ms rather than performing a delayed tap/hold after a long stall.
    static constexpr uint32_t touchPollMs=8, maximumTouchAgeMs=350;
    static constexpr size_t touchQueueCapacity=64;
    spi_device_handle_t spi_=nullptr;
    uint8_t* pixels_=nullptr;
    struct CapturedTouch { TouchPoint point; uint64_t at=0; };
    CapturedTouch touchCurrent_;
    std::array<CapturedTouch,touchQueueCapacity> touchQueue_{};
    size_t touchHead_=0,touchCount_=0;
    bool touchBlocked_=false;
    TouchPoint deliveredTouch_;
    mutable portMUX_TYPE touchMux_=portMUX_INITIALIZER_UNLOCKED;
    TaskHandle_t touchTask_=nullptr;
    volatile int clicks_=0;
    volatile uint8_t encoderState_=0;
    volatile uint32_t pulseStarted_[2]={0,0};
    volatile bool pulseActive_[2]={false,false};
    portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
    bool ready_=false,pressCandidate_=false,pressStable_=false,pressed_=false;
    uint32_t pressChanged_=0;
    static void IRAM_ATTR encoderInterrupt(void*);
    static void touchTask(void*);
    void pollTouch();
    void publishTouchLocked(const CapturedTouch&);
    void cancelTouchLocked(uint64_t);
    bool transfer(uint8_t reg,const void*,size_t,bool quad=false);
    uint16_t width() const override { return resolution; }
    uint16_t height() const override { return resolution; }
    void flush(int,int,int,int,const uint32_t*) override;
    TouchPoint sample() override;
    bool hasPending() const override;
    void discardPending() override;
    int getDelta() override;
    bool wasPressed() override { bool p=pressed_;pressed_=false;return p; }
};
}
