#pragma once
#include "Display.h"
#include "TouchInput.h"
#include "RotaryInput.h"
#include "state/HardwareCapabilities.h"
namespace voltra {
class HardwarePlatform {
public:
    virtual ~HardwarePlatform() = default;
    virtual HardwareCapabilities capabilities() const = 0;
    virtual Display& display() = 0;
    virtual TouchInput& touch() = 0;
    virtual RotaryInput& rotary() = 0;
    virtual uint64_t nowMs() const = 0;
};
}
