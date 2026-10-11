#pragma once
#include <cstdint>
namespace voltra {
enum class DisplayShape { Unknown, Round, Rectangular };
struct HardwareCapabilities {
    uint16_t displayWidth = 0, displayHeight = 0;
    DisplayShape displayShape = DisplayShape::Unknown;
    bool touchAvailable = false, rotaryAvailable = false, rotaryPressAvailable = false;
    bool bluetoothAvailable = false;
    bool wifiAvailable = false, hapticsAvailable = false, batteryAvailable = false;
    bool coreInputsValid() const {
        return displayWidth > 0 && displayWidth == displayHeight &&
            displayShape == DisplayShape::Round && touchAvailable && rotaryAvailable && rotaryPressAvailable;
    }
    bool physicalTargetSupported() const { return coreInputsValid() && bluetoothAvailable; }
};
}
