#pragma once
namespace voltra {
struct TouchPoint { float x = 0, y = 0; bool pressed = false; }; // [0,1] normalized
class TouchInput {
public:
    virtual ~TouchInput() = default;
    virtual TouchPoint sample() const = 0;
};
}
