#ifdef VOLTRA_PHYSICAL
#include "PhysicalSession.h"
#include "FrameBuilder.h"
#include "VoltraGattProfile.h"
#include "WeightLimits.h"
#include <algorithm>
#include <Arduino.h>
namespace voltra {
namespace {
constexpr uint16_t base=0x3e86, chains=0x3e87, eccentric=0x3e88, motor=0x3e89, workout=0x4fb0;
constexpr uint16_t position=0x3e82, offset=0x506a, unit=20590, twin=20844, maxForce=21268;
// Only catalogued widths. Unknown/truncated reports are rejected atomically.
int width(uint16_t id) {
    switch(id) {
    case 0x4fb0: case 0x4e2d: case 0x1b5d: case 20844: case 20772: case 20566:
    case 0x5103: case 0x5106: case 0x538d: case 0x53c7: case 0x53c9: case 0x53c6:
    case 0x5182: case 0x53b6: case 0x52e3: case 0x52ca: case 0x5102: case 0x4ee1: return 1;
    case 0x3e82: case 0x3e83: case 0x3e86: case 0x3e87: case 0x3e88: case 0x3e89:
    case 0x506a: case 0x53b0: case 21268: case 21476: case 21534: case 0x53c8:
    case 0x5362: case 0x53b7: case 0x5011: case 0x5467: return 2;
    case 20590: case 21023: case 0x5183: case 0x5165: case 0x5355: case 0x53cf: return 4;
    default:return 0;
    }
}
uint16_t u16(const Bytes& b,size_t i) { return uint16_t(b[i])|uint16_t(b[i+1])<<8; }
}
PhysicalSession::PhysicalSession(BleTransport& transport,const std::array<uint8_t,16>& identity)
    : transport_(transport),identity_(identity) {}
void PhysicalSession::connectionChanged(bool connected) {
    connected_=connected;accepted_=activated_=false; bootstrap_=readGroup_=0;sequence_=0;
    connectedAt_=now_;lastWrite_=0;parser_.reset();values_.clear();seen_.clear();report_={};firmware_.clear();
    waitingWrite_.reset();
    if(connected) {connectionFailure_=false;status_="Connecting / approve on VOLTRA";}
    else if(!connectionFailure_) status_="Connection lost";
}
std::optional<int64_t> PhysicalSession::fresh(uint16_t id) const {
    const auto it=values_.find(id);
    if(it==values_.end() || now_<it->second.at || now_-it->second.at>2000) return {};
    return it->second.value;
}
bool PhysicalSession::send(uint8_t command,const Bytes& payload,uint8_t sender,uint8_t receiver,uint8_t type) {
    if(!connected_) return false;
    lastWrite_=now_;
    if(transport_.write(VoltraGattProfile::write,buildFrame(command,payload,sequence_++,sender,receiver,type))!=TransportResult::Accepted) {
        status_="Bluetooth write failed";connectionFailure_=true;
        transport_.disconnect();return false;
    }
    return true;
}
bool PhysicalSession::read(std::initializer_list<uint16_t> ids) {
    Bytes b; append16(b,uint16_t(ids.size())); for(auto id:ids) append16(b,id); return send(0x0f,b);
}
bool PhysicalSession::writeParameter(uint16_t id,int value,int bytes) {
    Bytes b{1,0};append16(b,id);
    for(int i=0;i<bytes;++i) b.push_back(uint8_t(uint32_t(value)>>(i*8)));
    if(!send(0x11,b)) return false;
    waitingWrite_=std::make_pair(id,value);waitingAt_=now_;return true;
}
void PhysicalSession::notification(const std::string& uuid,const Bytes& bytes) {
    if(!connected_ || uuid!=VoltraGattProfile::notify) return;
    if(now_-fragmentAt_>1000) parser_.reset();
    fragmentAt_=now_;
    for(const auto& f:parser_.feed(bytes)) {
        if(f.protocol!=wire::protocolId || f.receiver!=0xaa) continue;
        const auto key=std::make_pair(f.command,f.sequence);
        if(std::find(seen_.begin(),seen_.end(),key)!=seen_.end()) continue;
        seen_.push_back(key);if(seen_.size()>32) seen_.pop_front();
        apply(f);
    }
}
void PhysicalSession::apply(const Frame& f) {
    const auto& p=f.payload;
    if(f.command==0x27 && p.size()==3 && p[0]==0x82 && p[1]==1) {
        accepted_=p[2]==0;
        status_=accepted_ ? "Reading VOLTRA state" : "VOLTRA refused connection";
        Serial.printf("VOLTRA acceptance %u\n",unsigned(p[2]));
        if(!accepted_) {connectionFailure_=true;transport_.disconnect();}
        return;
    }
    if(!accepted_) return;
    if(f.command==0xab && p.size()>=2 && p[0]==0) {
        if(activated_!=(p[1]==1)) Serial.printf("VOLTRA activation %u\n",unsigned(p[1]));
        activated_=p[1]==1;
    }
    if(f.command==0x77) {
        // Prefer the EP firmware tag. Do not interpret BMS/MC versions as app firmware.
        std::string text(p.begin(),p.end());
        for(size_t at=0;(at=text.find("EP",at))!=std::string::npos;++at) {
            size_t start=at+2;
            while(start<text.size() && (text[start]<'0' || text[start]>'9') && start-at<12) ++start;
            size_t end=start;while(end<text.size() && ((text[end]>='0' && text[end]<='9') || text[end]=='.')) ++end;
            const auto version=text.substr(start,end-start);
            if(firmwareFamily(version)) {
                if(firmware_!=version) Serial.printf("VOLTRA EP firmware %s\n",version.c_str());
                firmware_=version;break;
            }
        }
    }
    if(f.command!=0x0f && f.command!=0x10) return;
    size_t at=f.command==0x0f ? 1 : 0;
    if(p.size()<at+2 || (f.command==0x0f && p[0]!=0)) return;
    const auto count=u16(p,at);at+=2;if(count>32) return;
    std::map<uint16_t,int64_t> decoded;
    for(unsigned n=0;n<count;++n) {
        if(at+2>p.size()) return; const auto id=u16(p,at);at+=2;
        const int bytes=width(id);
        if(!bytes || at+bytes>p.size()) { Serial.printf("VOLTRA unparsed parameter %u\n",unsigned(id));return; }
        uint32_t value=0;for(int i=0;i<bytes;++i) value|=uint32_t(p[at++])<<(8*i);
        decoded[id]=(id==eccentric || id==0x3e83) ? int16_t(value) : int64_t(value);
    }
    if(at!=p.size()) return;
    for(const auto& pair:decoded) {
        const auto previous=values_.find(pair.first);
        const bool changed=previous==values_.end() || previous->second.value!=pair.second;
        values_[pair.first]={pair.second,now_};
        // Only normalized parameter values; never serial numbers or BLE identities.
        if(changed) Serial.printf("VOLTRA param %u = %lld\n",unsigned(pair.first),static_cast<long long>(pair.second));
        if(waitingWrite_ && waitingWrite_->first==pair.first && waitingWrite_->second==pair.second && now_>=waitingAt_) waitingWrite_.reset();
    }
    updateReport();
}
void PhysicalSession::updateReport() {
    const auto w=fresh(base),m=fresh(motor),mode=fresh(workout),u=fresh(unit);
    if(!w || !m || !mode || !u || (*u!=0 && *u!=1)) return;
    report_.unit=*u==0 ? WeightUnit::Pounds : WeightUnit::Kilograms;
    report_.firmwareVersion=firmware_;
    report_.settings.weight=convertWeightUnits(int(*w),WeightUnit::Pounds,report_.unit);
    report_.loaded=*m==5 || *m==39;report_.autoLoadArmed=*m==38;
    // The current public catalog names this register but does not define its
    // enum/peer-health semantics. Preserve raw diagnostics; never guess readiness.
    report_.twinKnown=false;report_.twin=false;
    WeightLimitEvidence limits;limits.model="VOLTRA I";limits.firmware=firmware_;
    limits.twinKnown=report_.twinKnown;limits.twin=report_.twin;
    if(const auto max=fresh(maxForce)) limits.single.pounds=int(*max);
    report_.maxWeight=resolveWeightLimit(limits,report_.unit).ticks;
    report_.settings.magicMode=MagicMode::Chains;
    const auto c=fresh(chains),e=fresh(eccentric);
    report_.settings.magic={c && *c>0,c ? convertWeightUnits(int(*c),WeightUnit::Pounds,report_.unit) : 0,false};
    report_.settings.eccentric={e && *e>0,e ? convertWeightUnits(int(*e),WeightUnit::Pounds,report_.unit) : 0,false};
    // The additional modes await verified selector/capability mappings.
    report_.magicSupported={false,c.has_value(),false};report_.eccentricSupported=e && *e>=0;
    report_.receivedAtMs=std::min({values_[base].at,values_[motor].at,values_[workout].at,values_[unit].at});
    ++report_.revision;
    if(!busy()) status_=*mode!=1 ? "Select Weight Training on VOLTRA" : !report_.twinKnown ? "Twin state needs validation" : "Connection diagnostic build";
}
TrainingReport PhysicalSession::trainingReport() const { return report_; }
SessionReadiness PhysicalSession::readiness() const {
    SessionReadiness r;
    r.validated=accepted_;r.capabilities.weightTraining=fresh(workout)==1 ? Knowledge::Yes : Knowledge::Unknown;
    if(report_.maxWeight>0) r.capabilities.effectiveMaxWeightLbs=uint16_t(convertWeightUnits(report_.maxWeight,report_.unit,WeightUnit::Pounds));
    r.cableZero=fresh(position).has_value() && fresh(offset).has_value() && fresh(position)==fresh(offset);
    r.twinSafe=false;r.minWeightLbs=5;
    // Do not invent the dock/interlock bit meanings. Readback validation is required
    // before physical load or settings that can interact with Auto Load are enabled.
    r.ready=false;
    return r;
}
TransportResult PhysicalSession::setWeight(uint16_t pounds) {
    auto settings=report_.settings; settings.weight=convertWeightUnits(pounds,WeightUnit::Pounds,report_.unit);
    return setTraining(settings);
}
TransportResult PhysicalSession::setTraining(const TrainingSettings&) {
    status_="Physical safety profile not validated";return TransportResult::Unsupported;
}
TransportResult PhysicalSession::setLoaded(bool loaded) {
    if(!connected_) return TransportResult::Disconnected;
    if(loaded) {status_="Physical safety profile not validated";return TransportResult::Unsupported;}
    // Explicit unload remains available even if state is unknown or another request is pending.
    waitingWrite_.reset();
    return writeParameter(motor,4,2) ? TransportResult::Accepted : TransportResult::Failed;
}
TransportResult PhysicalSession::triggerAutoLoad() { status_="Auto Load profile not validated";return TransportResult::Unsupported; }
void PhysicalSession::tick(uint64_t now) {
    now_=now;if(!connected_) return;
    if(!accepted_ && now_-connectedAt_>30000) { status_="Connection approval timed out";connectionFailure_=true;transport_.disconnect();return; }
    if(waitingWrite_ && now_-waitingAt_>3000) {waitingWrite_.reset();status_="No device confirmation";}
    if(now_-lastWrite_<90) return;
    if(bootstrap_==0) {
        Bytes hello(21,0);const std::string name="VOLTRA Remote";std::copy(name.begin(),name.end(),hello.begin());
        hello.insert(hello.end(),identity_.begin(),identity_.begin()+6);hello.push_back(1);
        if(send(0x4f,hello,1)) ++bootstrap_;
    } else if(bootstrap_==1) {if(send(0xff,{0,0xaa},0xaa,0xd2,0x08)) ++bootstrap_;}
    else if(bootstrap_==2) {Bytes payload{0x81,0x10};payload.insert(payload.end(),identity_.begin(),identity_.end());if(send(0x27,payload)) ++bootstrap_;}
    else if(accepted_) {
        // Small read groups avoid the upstream cold-bootstrap wide-query failure.
        switch(readGroup_++%15) {
        case 0:read({base,motor,workout});break;
        case 1:read({chains,eccentric});break;
        case 2:read({position,offset});break;
        case 3:read({unit});break;
        case 4:read({twin});break;
        case 5:read({20772});break; // Child lock, read-only.
        case 6:read({20566});break; // Dock/quick-release status, raw until mapped.
        case 7:read({21476});break; // OTA status, read-only.
        case 8:read({0x4e2d});break;
        case 9:read({maxForce});break;
        case 10:read({0x5355,0x53cf});break;
        case 11:send(0x74,{});break;
        case 12:send(0x77,{0});break;
        case 13:send(0x77,{1});break;
        case 14:send(0xab,{1});break;
        }
    }
}
}
#endif
