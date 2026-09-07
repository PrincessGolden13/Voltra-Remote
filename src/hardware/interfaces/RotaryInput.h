#pragma once
namespace voltra {
class RotaryInput {
public:
    virtual ~RotaryInput() = default;
    virtual int getDelta() = 0; // Clockwise is positive on every board.
    virtual bool wasPressed() = 0;
};
}
