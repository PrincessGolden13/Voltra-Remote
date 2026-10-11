#pragma once
#include "Display.h"
#include "TouchInput.h"
#include "RotaryInput.h"
#include "state/HardwareCapabilities.h"
#include <optional>
namespace voltra {
class HardwarePlatform {
public:
    virtual ~HardwarePlatform() = default;
    virtual HardwareCapabilities capabilities() const = 0;
    virtual Display& display() = 0;
    virtual TouchInput& touch() = 0;
    virtual RotaryInput& rotary() = 0;
    virtual uint64_t nowMs() const = 0;
    virtual std::optional<uint8_t> batteryPercent() const { return std::nullopt; }
    // Optional outputs; board adapters own electrical timing and power behavior.
    virtual void restAlert(uint32_t durationMs) { (void)durationMs; }
    virtual void setDisplayPower(bool asleep, bool dimmed) { (void)asleep; (void)dimmed; }
};
}
