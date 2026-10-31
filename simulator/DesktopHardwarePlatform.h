#pragma once
#include "hardware/interfaces/HardwarePlatform.h"
#include <algorithm>
#include <chrono>
#include <vector>
namespace voltra {
class DesktopHardwarePlatform final : public HardwarePlatform, public Display, public TouchInput, public RotaryInput {
public:
    explicit DesktopHardwarePlatform(uint16_t size) : size_(size), pixels_(size_t(size)*size, 0) {}
    HardwareCapabilities capabilities() const override {
        HardwareCapabilities result;
        result.displayWidth = result.displayHeight = size_;
        result.displayShape = DisplayShape::Round;
        result.touchAvailable = result.rotaryAvailable = result.rotaryPressAvailable = true;
        return result; // No physical BLE capability in a simulator.
    }
    Display& display() override { return *this; }
    TouchInput& touch() override { return *this; }
    RotaryInput& rotary() override { return *this; }
    std::optional<uint8_t> batteryPercent() const override { return 75; } // Explicit simulated remote battery.
    uint16_t width() const override { return size_; }
    uint16_t height() const override { return size_; }
    uint64_t nowMs() const override {
        if(previewMs_) return *previewMs_;
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start_).count();
    }
    void previewClock(uint64_t ms) { previewMs_=ms; } // Render-only offline timeline.
    void flush(int x, int y, int width, int height, const uint32_t* source) override {
        for (int row = 0; row < height; ++row)
            for (int col = 0; col < width; ++col)
                if (x+col >= 0 && y+row >= 0 && x+col < size_ && y+row < size_)
                    pixels_[size_t(y+row)*size_+x+col] = source[size_t(row)*width+col];
    }
    TouchPoint sample() override { return touch_; }
    void pointer(int x, int y, bool pressed) {
        touch_ = {std::clamp(float(x)/float(size_-1), 0.f, 1.f),
            std::clamp(float(y)/float(size_-1), 0.f, 1.f), pressed};
    }
    int getDelta() override { int result = delta_; delta_ = 0; return result; }
    bool wasPressed() override { bool result = pressed_; pressed_ = false; return result; }
    void rotate(int delta) { delta_ += delta; }
    void press() { pressed_ = true; }
    const std::vector<uint32_t>& pixels() const { return pixels_; }
private:
    uint16_t size_;
    std::vector<uint32_t> pixels_;
    TouchPoint touch_;
    int delta_ = 0;
    bool pressed_ = false;
    std::optional<uint64_t> previewMs_;
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
};
}
