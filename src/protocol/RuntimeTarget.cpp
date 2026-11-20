#include "RuntimeTarget.h"

namespace voltra {
std::optional<RuntimeTarget> decodeRuntimeTarget(const Frame& f) {
    // albertzhang86/voltra-remote c4f2884: real independent B4 captures and
    // Cortex field contract. Source occupies the frame's protocol field; it is
    // not ordinary command protocol 0x0020. Twin sources 0x10..12 have a
    // different scope and are deliberately excluded.
    if(f.type!=0x04 || f.sender!=0x10 || f.receiver!=0xaa ||
       f.command!=0xb4 || f.protocol>2) return {};
    const size_t localSize=f.protocol==0 ? 6 : 10;
    if(f.payload.size()!=localSize && f.payload.size()!=localSize+2) return {};
    RuntimeTarget sample;
    if(f.payload.size()==localSize+2) {
        const int target=f.payload[localSize] | (int(f.payload[localSize+1])<<8);
        // Source's bounded telemetry range. This is not a setting capability
        // or permission to control resistance. 0xffff denotes no target.
        constexpr int maximumTargetTenthsLb=10000;
        if(target<=maximumTargetTenthsLb) sample.targetTenthsLb=target;
    }
    return sample;
}
}
