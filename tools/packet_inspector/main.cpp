#include "protocol/FrameParser.h"
#include "protocol/TwinStatus.h"
#include "protocol/RuntimeTarget.h"
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
    if(frame->command==voltra::twinStatusCommand) {
        const auto twin=voltra::decodeTwinStatus(*frame);
        if(!twin) {std::cerr << "Unsupported Twin status shape/state/identity consistency\n";return 1;}
        std::cout << "Twin status=" << twin->state << " peer_present=" << twin->peerPresent
            << " own_battery=" << twin->ownBattery.value_or(-1) << " peer_battery=" << twin->peerBattery.value_or(-1)
            << "\nRole and peer safety remain unknown; addresses omitted.\n";
    }
    if(frame->command==0xb4) {
        const auto sample=voltra::decodeRuntimeTarget(*frame);
        if(!sample) {std::cerr << "Unsupported independent B4 shape/source\n";return 1;}
        std::cout << "Independent target_tenths_lb=" <<
            (sample->targetTenthsLb ? std::to_string(*sample->targetTenthsLb) : "unavailable") <<
            "\nThis is a device target, not measured cable force or a verified LCD match.\n";
    }
}
