#include "TwinTelemetry.h"

namespace voltra {
namespace {
int u16(const Bytes& bytes,size_t at) {
    return int(bytes[at]) | (int(bytes[at+1])<<8);
}
int i16(const Bytes& bytes,size_t at) {
    const int raw=u16(bytes,at);
    return raw>=0x8000 ? raw-0x10000 : raw;
}
}
std::optional<TwinRuntime> decodeTwinRuntime(const Frame& f) {
    // B4 carries its source in the protocol field, not ordinary protocol 0x20.
    if(f.type!=0x04 || f.sender!=0x10 || f.receiver!=0xaa ||
       f.command!=0xb4 || f.protocol<0x10 || f.protocol>0x12) return {};
    const size_t localSize=f.protocol==0x10 ? 6 : 10;
    if(f.payload.size()!=localSize+8) return {};
    const size_t peer=localSize+2;
    TwinRuntime sample;
    sample.localForceTenthsLb=u16(f.payload,0);
    sample.localPositionMm=u16(f.payload,2);
    sample.localVelocityMmPerSecond=i16(f.payload,4);
    sample.peerForceTenthsLb=u16(f.payload,peer);
    sample.peerPositionMm=u16(f.payload,peer+2);
    sample.peerVelocityMmPerSecond=i16(f.payload,peer+4);
    if(f.protocol==0x12) {
        sample.setCount=uint16_t(u16(f.payload,6));
        sample.repCount=uint16_t(u16(f.payload,8));
    }
    return sample;
}
std::optional<TwinReportedCounters> decodeExtendedTwinCounters(const Frame& f) {
    // Steven 1c14860 docs/PROTOCOL.md:97–102 and parse_rep_telemetry:
    // the captured master 81 42 prefix keeps the rep phase, 8-bit set and BE16 rep.
    // Do not substitute Albert's conflicting synthetic LE16 AA81 layout.
    constexpr size_t twinRepPayloadBytes=68;
    constexpr uint8_t extendedRepSubtype=0x81, extendedRepLength=0x42, maximumRepPhase=3;
    constexpr uint16_t maximumRepCount=10000;
    const auto& p=f.payload;
    if(f.type!=0x04 || f.sender!=0x10 || f.receiver!=0xaa || f.protocol!=wire::protocolId ||
        f.command!=0xaa || p.size()!=twinRepPayloadBytes || p[0]!=extendedRepSubtype ||
        p[1]!=extendedRepLength || p[2]>maximumRepPhase) return {};
    const auto rep=uint16_t(uint16_t(p[4])<<8 | p[5]);
    if(rep>maximumRepCount) return {};
    return TwinReportedCounters{p[3],rep,p[2]};
}
}
