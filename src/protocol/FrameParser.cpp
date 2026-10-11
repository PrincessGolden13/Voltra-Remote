#include "FrameParser.h"
namespace voltra {
uint8_t crc8(const uint8_t* data, size_t size) {
    uint8_t crc = wire::crc8Initial;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? wire::crc8Polynomial : 0);
    }
    return crc;
}
uint16_t crc16(const uint8_t* data, size_t size) {
    uint16_t crc = wire::crc16Initial;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? wire::crc16Polynomial : 0);
    }
    return crc;
}
static size_t declaredSize(const Bytes& b) {
    if (b.size() < 4 || b[0] != wire::magic || crc8(b.data(), 3) != b[3]) return 0;
    if (b[2] != 0x04 && b[2] != 0x05 && b[2] != 0x08 && b[2] != 0x09) return 0;
    return b[1] + ((b[2] == 0x05 || b[2] == 0x09) ? 256u : 0u);
}
std::optional<Frame> parseFrame(const Bytes& b) {
    if (b.size() < wire::minFrameBytes || b.size() > wire::maxFrameBytes || declaredSize(b) != b.size()) return {};
    auto tail = uint16_t(b[b.size()-2] | (b.back() << 8));
    if (crc16(b.data(), b.size()-2) != tail) return {};
    return Frame{b[2], b[4], b[5], b[10], uint16_t(b[6] | (b[7] << 8)),
        uint16_t(b[8] | (b[9] << 8)), Bytes(b.begin()+11, b.end()-2)};
}
std::vector<Frame> FrameParser::feed(const Bytes& chunk) {
    std::vector<Frame> frames;
    for (auto byte : chunk) {
        buffer_.push_back(byte);
        while (buffer_.size() >= 4) {
            const auto size = declaredSize(buffer_);
            if (size < wire::minFrameBytes || size > wire::maxFrameBytes) {
                buffer_.erase(buffer_.begin());
                continue;
            }
            if (buffer_.size() < size) break;
            Bytes candidate(buffer_.begin(), buffer_.begin()+size);
            if (auto frame = parseFrame(candidate)) {
                frames.push_back(*frame);
                buffer_.erase(buffer_.begin(), buffer_.begin()+size);
            } else buffer_.erase(buffer_.begin());
        }
    }
    return frames;
}
}
