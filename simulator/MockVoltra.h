#pragma once
#ifndef VOLTRA_SIMULATOR
#error "Synthetic device support belongs only in simulator targets"
#endif
#include "ble/BleTypes.h"
namespace voltra {
enum class SimulatorProfile { Unloaded, Loaded, Stale, TwinUnknown };
// Synthetic examples, never captured-device evidence or compatibility profiles.
class MockVoltra {
public:
    Bytes notification(SimulatorProfile profile);
    static constexpr uint16_t initialWeightLbs = 50, minWeightLbs = 0, maxWeightLbs = 230;
    uint16_t weightLbs = initialWeightLbs;
    bool loaded = false;
private:
    uint16_t sequence_ = 0;
};
}
