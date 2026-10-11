#pragma once
#include "ble/BleTypes.h"
#include <optional>
namespace voltra {
namespace wire {
constexpr uint8_t magic = 0x55;
constexpr size_t minFrameBytes = 13;
constexpr size_t maxFrameBytes = 511;
constexpr uint8_t crc8Initial = 0x77, crc8Polynomial = 0x8c;
constexpr uint16_t crc16Initial = 0x3692, crc16Polynomial = 0x8408;
constexpr uint16_t protocolId = 0x0020;
}
uint8_t crc8(const uint8_t* data, size_t size);
uint16_t crc16(const uint8_t* data, size_t size);
struct Frame {
    uint8_t type = 0, sender = 0, receiver = 0, command = 0;
    uint16_t sequence = 0, protocol = 0;
    Bytes payload;
};
std::optional<Frame> parseFrame(const Bytes& bytes);
// Bounded incremental reassembly, one instance per notification characteristic.
class FrameParser {
public:
    std::vector<Frame> feed(const Bytes& chunk);
    void reset() { buffer_.clear(); }
private:
    Bytes buffer_;
};
}
