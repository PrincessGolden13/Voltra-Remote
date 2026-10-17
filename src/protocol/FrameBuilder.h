#pragma once
#include "FrameParser.h"
#include <stdexcept>
namespace voltra {
inline void append16(Bytes& b,uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v>>8)); }
inline Bytes buildFrame(uint8_t command,const Bytes& payload,uint16_t sequence,uint8_t sender=0xaa,uint8_t receiver=0x10,uint8_t type=0x04) {
    const size_t size=wire::minFrameBytes+payload.size();
    if(size>wire::maxFrameBytes) throw std::invalid_argument("VOLTRA frame too large");
    Bytes b{wire::magic,uint8_t(size),uint8_t(type|(size>255 ? 1 : 0)),0,sender,receiver};
    b[3]=crc8(b.data(),3); append16(b,sequence); append16(b,wire::protocolId); b.push_back(command);
    b.insert(b.end(),payload.begin(),payload.end()); append16(b,crc16(b.data(),b.size())); return b;
}
}
