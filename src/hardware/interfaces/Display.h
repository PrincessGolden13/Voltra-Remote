#pragma once
#include <cstdint>
namespace voltra {
class Display {
public:
    virtual ~Display() = default;
    virtual uint16_t width() const = 0;
    virtual uint16_t height() const = 0;
    // RGB888 in a 32-bit word. Backend owns physical orientation/driver details.
    virtual void flush(int x, int y, int width, int height, const uint32_t* pixels) = 0;
};
}
