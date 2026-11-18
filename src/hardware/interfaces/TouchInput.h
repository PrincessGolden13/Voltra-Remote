#pragma once
#include <cstdint>
namespace voltra {
struct TouchPoint {
    float x = 0, y = 0; // [0,1] normalized
    bool pressed = false;
    // Lost/invalid input cancels the gesture; it is not a clickable release.
    bool cancelled = false;
    // Capture time on HardwarePlatform's monotonic ms clock, when available.
    // Zero lets synchronous adapters use the processing clock instead.
    uint64_t sampledAtMs = 0;
};
class TouchInput {
public:
    virtual ~TouchInput() = default;
    virtual TouchPoint sample() = 0;
    // Buffered adapters preserve short presses across rendering/transport work.
    virtual bool hasPending() const { return false; }
    virtual void discardPending() {} // Screen changes must not replay old input.
};
}
