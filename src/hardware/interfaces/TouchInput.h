#pragma once
namespace voltra {
struct TouchPoint {
    float x = 0, y = 0; // [0,1] normalized
    bool pressed = false;
    // Lost/invalid input cancels the gesture; it is not a clickable release.
    bool cancelled = false;
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
