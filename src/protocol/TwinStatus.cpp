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
std::optional<TwinStatus> inspectTwinStatus(const Frame& frame,const char*& reason) {
    reason="envelope";
    if(frame.command!=twinStatusCommand || frame.protocol!=wire::protocolId ||
       frame.sender!=0x10 || frame.receiver!=0xaa) return {};
    const auto& p=frame.payload;
    reason="layout";
    if((p.size()!=22 && p.size()!=23) || (frame.type!=0x04 && frame.type!=0x08)) return {};
    const size_t offset=p.size()==23 ? 1 : 0;
    reason="reply-status";
    if(offset && p[0]!=0) return {};
    TwinStatus result;
    if(p[offset]<=100) result.ownBattery=int(p[offset]);
    result.marker=p[offset+1];result.rawState=p[offset+2];
    std::copy_n(p.begin()+offset+3,6,result.own.begin());
    std::copy_n(p.begin()+offset+9,6,result.peer.begin());
    result.peerPresent=present(result.peer);
    reason="own-absent";
    if(!present(result.own)) return {};
    reason="same-identities";
    if(result.peerPresent && result.peer==result.own) return {};
    if(result.peerPresent && p[offset+15]<=100) result.peerBattery=int(p[offset+15]);
    if(result.rawState==twinStatusAlone && !result.peerPresent) result.state=twinStatusAlone;
    else if((result.rawState==twinStatusPaired || result.rawState==0x22) && result.peerPresent)
        result.state=twinStatusPaired;
    // Exact observed words only, not a generalized role/connection bitfield.
    // Host 0x12: stevenpoint 1c14860 / albert c4f288 native controller captures.
    // Follower 0x22: owner-identified Red, report 2ab11a8e, marker1/own-slot match.
    // PhysicalSession separately binds this read-only profile to the selected
    // endpoint, fresh current relationship fields and the topology epoch.
    if(result.marker==1 && result.state==twinStatusPaired) {
        if(result.rawState==0x12) result.role=TwinStatusRole::Master;
        else if(result.rawState==0x22) result.role=TwinStatusRole::Follower;
    }
    reason=result.state==twinStatusUnknown ? "identity-known-state-unknown" : "accepted";
    return result;
}
}
std::optional<TwinStatus> decodeTwinStatus(const Frame& frame) {
    const char* reason=nullptr;
    return inspectTwinStatus(frame,reason);
}
std::string twinStatusDiagnostic(const Frame& frame,std::optional<DeviceAddress> selectedAddress) {
    const char* reason=nullptr;
    const auto decoded=inspectTwinStatus(frame,reason);
    std::string text="reason="+std::string(reason)+" type="+std::to_string(frame.type)+
        " sender="+std::to_string(frame.sender)+" receiver="+std::to_string(frame.receiver)+
        " protocol="+std::to_string(frame.protocol)+" bytes="+std::to_string(frame.payload.size());
    const auto& p=frame.payload;
    if(p.size()!=22 && p.size()!=23) return text;
    const size_t offset=p.size()==23 ? 1 : 0;
    DeviceAddress own{},peer{};
    std::copy_n(p.begin()+offset+3,6,own.begin());
    std::copy_n(p.begin()+offset+9,6,peer.begin());
    text+=" reply="+(offset ? std::to_string(p[0]) : "push")+
        " header="+std::to_string(p[offset])+","+std::to_string(p[offset+1])+","+std::to_string(p[offset+2])+
        " peerBattery="+std::to_string(p[offset+15])+
        " ownPresent="+std::to_string(present(own))+" peerPresent="+std::to_string(present(peer))+
        " distinct="+std::to_string(own!=peer)+
        " slot0Match="+(selectedAddress ? std::to_string(own==*selectedAddress) : "unknown")+
        " slot1Match="+(selectedAddress ? std::to_string(peer==*selectedAddress) : "unknown")+
        " state="+(decoded ? std::to_string(decoded->state) : "unknown");
    return text;
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
