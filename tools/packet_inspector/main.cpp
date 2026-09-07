#include "protocol/FrameParser.h"
#include <iostream>
#include <string>
#include <cctype>
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: packet_inspector HEX_FRAME\nOffline only; no BLE access.\n"; return 2; }
    const std::string hex = argv[1];
    if (hex.empty() || hex.size()%2 || hex.size() > 2*voltra::wire::maxFrameBytes) return 2;
    voltra::Bytes bytes;
    for (size_t i = 0; i < hex.size(); i += 2) {
        if (!std::isxdigit(static_cast<unsigned char>(hex[i])) || !std::isxdigit(static_cast<unsigned char>(hex[i+1]))) return 2;
        bytes.push_back(uint8_t(std::stoul(hex.substr(i,2), nullptr, 16)));
    }
    const auto frame = voltra::parseFrame(bytes);
    if (!frame) { std::cerr << "Invalid frame (length/type/checksum)\n"; return 1; }
    std::cout << "Valid framing and CRC; command=0x" << std::hex << unsigned(frame->command)
        << " sequence=" << std::dec << frame->sequence << " payload_bytes=" << frame->payload.size()
        << "\nThis does not validate command meaning or device behavior.\n";
}
