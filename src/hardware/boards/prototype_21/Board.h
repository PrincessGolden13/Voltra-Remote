#pragma once
#include "hardware/interfaces/HardwarePlatform.h"
namespace voltra::prototype21 {
// Unidentified physical unit: report no validated capabilities and touch no GPIOs.
class Board final : public HardwarePlatform, private Display, private TouchInput, private RotaryInput {
public:
    HardwareCapabilities capabilities() const override { return {}; }
    Display& display() override { return *this; }
    TouchInput& touch() override { return *this; }
    RotaryInput& rotary() override { return *this; }
    uint64_t nowMs() const override { return 0; }
private:
    uint16_t width() const override { return 0; }
    uint16_t height() const override { return 0; }
    void flush(int, int, int, int, const uint32_t*) override {}
    TouchPoint sample() override { return {}; }
    int getDelta() override { return 0; }
    bool wasPressed() override { return false; }
};
}
