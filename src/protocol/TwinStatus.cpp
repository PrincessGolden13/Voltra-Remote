#include "TwinStatus.h"
#include <algorithm>
#include <cstdio>

namespace voltra {
namespace {
bool present(const DeviceAddress& address) {
    return std::any_of(address.begin(),address.end(),[](uint8_t b){return b!=0;});
}
int digit(char c) {
    if(c>='0' && c<='9') return c-'0';
    if(c>='a' && c<='f') return c-'a'+10;
    if(c>='A' && c<='F') return c-'A'+10;
    return -1;
}
}
std::optional<TwinStatus> decodeTwinStatus(const Frame& frame) {
    if(frame.command!=twinStatusCommand || frame.protocol!=wire::protocolId ||
       frame.sender!=0x10 || frame.receiver!=0xaa) return {};
    const auto& p=frame.payload;
    const size_t offset=p.size()==23 ? 1 : 0;
    if((p.size()!=22 && p.size()!=23) || (offset && p[0]!=0) ||
       p[offset]>100 || p[offset+1]!=1) return {};
    TwinStatus result;
    result.ownBattery=p[offset];result.state=p[offset+2];
    if(result.state!=twinStatusAlone && result.state!=twinStatusJoining && result.state!=twinStatusPaired) return {};
    std::copy_n(p.begin()+offset+3,6,result.own.begin());
    std::copy_n(p.begin()+offset+9,6,result.peer.begin());
    result.peerPresent=present(result.peer);
    if(!present(result.own) || (result.peerPresent && result.peer==result.own) ||
       (result.state==twinStatusAlone && result.peerPresent) ||
       (result.state==twinStatusPaired && !result.peerPresent)) return {};
    if(p[offset+15]>100) return {};
    if(result.peerPresent) result.peerBattery=int(p[offset+15]);
    return result;
}
std::optional<DeviceAddress> parseDeviceAddress(const std::string& id) {
    if(id.size()!=17) return {};
    DeviceAddress address{};
    for(size_t i=0;i<address.size();++i) {
        const size_t at=i*3;
        const int a=digit(id[at]),b=digit(id[at+1]);
        if(a<0 || b<0 || (i<5 && id[at+2]!=':')) return {};
        address[i]=uint8_t(a*16+b);
    }
    return present(address) ? std::optional<DeviceAddress>(address) : std::nullopt;
}
std::string formatDeviceAddress(const DeviceAddress& address) {
    char buffer[18];
    std::snprintf(buffer,sizeof(buffer),"%02x:%02x:%02x:%02x:%02x:%02x",address[0],address[1],address[2],address[3],address[4],address[5]);
    return buffer;
}
}
