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
constexpr uint16_t remoteCoworker=21023, pairedDevice=21605;
constexpr uint16_t inverse=21424, extraMode=21446, chainsPercent=21722, eccentricPercent=21456;
// kirby6365/voltra_remote 8e1ea1c: polled uint8 switches, absent from older CSV.
constexpr uint16_t mountain=0x556f, accessoryDisplayPercent=0x5569;
constexpr uint16_t legacyChainsPercent=20873, legacyEccentricPercent=20874;
constexpr uint16_t autoLoadEnabled=21272, autoLoadSafetyEnabled=21389, autoLoadStatus=21447;
// kirby 8e1ea1c captured Auto Load remaining in mode 35 after completion.
// Status 13 is still engaging; only 14 permits the loaded base-register path.
constexpr int autoLoadMotorMode=35, autoLoadWaiting=11, autoLoadComplete=14;
constexpr uint16_t childLock=20772, otaState=21476, battery=20013;
constexpr uint16_t mountLock=20642, voidLock=21224;
constexpr uint16_t fitnessRestTime=21377; // Android da514d2 catalog: uint16 seconds; read only here.
// Android da514d2: AUTO_UNLOAD_HOLDING_TIME, uint8 0..10. Official Cortex
// training-control documents seconds and zero=off. Read only, never a write.
constexpr uint16_t autoUnloadHoldTime=21525;
// Black, Oct 24: user released/resecured mount, 2 -> 1 -> 2. Keep the
// successful profile's companion void-lock value exact; its bits are unmapped.
constexpr int securedMount=2, observedVoidLock=6;
// Black, Oct 31: OTA_STATUS=0x0105 while the user confirmed an update was
// ready to install and Weight Training remained open. Pending availability
// is not an active installation. No other nonzero status is authorized here.
constexpr int otaIdleStatus=0, otaUpdateAvailableStatus=0x0105;
constexpr int cableZeroToleranceCm=1; // Integer-cm quantization; source capture 37/38 cm at rest.
constexpr uint64_t stateFreshMs=2000, writeConfirmationMs=3000;
// A bounded unloaded transaction can contain several individually confirmed
// writes. Keep critical polls ahead of it; never retry a timed-out step.
constexpr uint64_t settingsDeadlineMs=10000, criticalPollIntervalMs=1300;
// Send the latest explicit setting after a short pause in rotary input.
constexpr uint64_t settingsQuietMs=100;
// Read the parameter after settling; a transient asynchronous settings echo is
// not enough (the derived accessory pair can revert about 100 ms later).
constexpr uint64_t settingsSettleMs=250;
constexpr uint64_t motionFreshMs=500;
// SDK 79f6c55 stream units: mm, mm/s, tenths of pounds. Tunable detection
// policy, not a device safety interlock. Require extension and tension for RP.
constexpr int stationarySpeedMmPerSec=20, heldExtensionMm=20, heldForceTenthsLb=20;
constexpr uint16_t telemetrySubscribe=20867;
constexpr uint16_t telemetryRate=20866;
constexpr uint8_t telemetryHz=10; // Catalogued 5..50 Hz; Android resets it to 0 on exit.
// Restore the catalogued default subscription. Nonzero custom masks produced
// AA94 on Black, not the AA81 layout used by the source-validated WT decoder.
// SDK 79f6c55 / Android da514d2 both encode this reset; no resistance changes.
constexpr uint32_t fitnessNotificationMask=0;
// HA/Android pace bootstrap at 90 ms. Their captured VOLTRA connect-request
// deadline is ~5 s from radio connection; a 3 s post-HELLO wait plus full GATT
// discovery exceeded it on this remote. Keep every bootstrap step nonblocking.
constexpr uint64_t helloSettleMs=90, initPacingMs=90, readPacingMs=90;
// Android da514d2 normalizedFitnessMode and kirby 8e1ea1c use the low
// byte of the uint16 motor report. Black reported 0x0100 on Oct 31 while
// in Weight Training. Accept only the observed high-byte variant (0 or 1),
// not arbitrary upper bits. Keep the full raw value in values_/diagnostics.
std::optional<int64_t> motorMode(int64_t raw) {
    constexpr int64_t supportedReportBits=0x01ff, modeMask=0x00ff;
    if(raw<0 || (raw & ~supportedReportBits)!=0) return {};
    return raw & modeMask;
}
// Only catalogued widths. Unknown/truncated reports are rejected atomically.
int width(uint16_t id) {
    switch(id) {
    case 0x4fb0: case 0x4e2d: case 0x1b5d: case 20844: case 20772: case 20566:
    case 0x5103: case 0x5106: case 0x538d: case 0x53c7: case 0x53c9: case 0x53c6:
    case 0x5182: case 0x53b6: case 0x52e3: case 0x52ca: case 0x5102: case 0x4ee1:
    case 21007: case 21424: case 21272: case 21273: case 21716: case 20642:
    case mountain: case accessoryDisplayPercent: case autoUnloadHoldTime: return 1;
    case 0x3e82: case 0x3e83: case 0x3e86: case 0x3e87: case 0x3e88: case 0x3e89:
    case 0x506a: case 21268: case 21476: case 21534: case 0x53c8: case 20873: case 20874:
    case 0x5362: case 0x53b7: case 0x5011: case 0x5467: case 21377:
    case 21002: case 21003: case 21004: return 2; // BLE interval reports, Android capture.
    case 20590: case 21023: case 0x5165: case 0x5355: case 0x53cf:
    case 21028: case 21722: case 21456: case 21224: case telemetrySubscribe: return 4;
    case pairedDevice: return 8;
    default:return 0;
    }
}
uint16_t u16(const Bytes& b,size_t i) { return uint16_t(b[i])|uint16_t(b[i+1])<<8; }
}
PhysicalSession::PhysicalSession(BleTransport& transport,const std::array<uint8_t,16>& identity,
                                 std::optional<std::array<uint8_t,6>> helloIdentity)
    : transport_(transport),identity_(identity) {
    if(helloIdentity) helloIdentity_=*helloIdentity;
    else std::copy_n(identity.begin(),helloIdentity_.size(),helloIdentity_.begin());
}
void PhysicalSession::connectionChanged(bool connected) {
    connected_=connected;accepted_=activated_=false; bootstrap_=backgroundRead_=infoRead_=0;sequence_=0;pollAt_={};
    diagnosticFrames_=diagnosticVersions_=0;
    connectedAt_=now_;lastWrite_=reportRevision_=0;
    for(auto& parser:parsers_) parser.reset();fragmentAt_={};
    values_.clear();seen_.clear();report_={};firmware_.clear();
    waitingWrite_.reset();
    writes_.clear();target_.reset();telemetrySubscribed_=false;entryHandled_=targetClockPending_=false;
    trainingSeen_=false;
    targetImmediate_=false;
    telemetryAt_=unitSwitchAt_=0;commandFailed_=false;intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();intentDisplay_.reset();
    deviceSet_.reset();deviceRep_.reset();completedSets_.reset();completedReps_.reset();
    closedSet_.reset();setEndRevision_=setEndedAt_=0;
    motionAt_=motionRevision_=repAt_=repRevision_=telemetryLogAt_=0;
    motionPosition_=motionVelocity_=motionForce_=0;motionPhase_=0;completedRep_=0;repSet_.reset();intentLoaded_=false;liveIntent_.reset();
    workoutStatus_.reset();telemetryHeaders_=telemetrySetup_=0;
    telemetryTypes_={};rawTelemetryHeaders_=0;
    bootstrapWriteJustCompleted_=false;
    if(connected) {connectionFailure_=false;status_="Opening VOLTRA session...";}
    else if(!connectionFailure_) status_="Connection lost";
}
std::optional<int64_t> PhysicalSession::fresh(uint16_t id) const {
    const auto it=values_.find(id);
    if(it==values_.end() || now_<it->second.at || now_-it->second.at>stateFreshMs) return {};
    return it->second.value;
}
std::optional<int64_t> PhysicalSession::freshMotorMode() const {
    const auto raw=fresh(motor);
    return raw ? motorMode(*raw) : std::nullopt;
}
bool PhysicalSession::updateAllowsControls() const {
    const auto state=fresh(otaState);
    return state==otaIdleStatus || state==otaUpdateAvailableStatus;
}
const char* PhysicalSession::updateBlockReason() const {
    return fresh(otaState) ? "Check VOLTRA update status" : "Reading VOLTRA update status";
}
uint16_t PhysicalSession::percentParameter(bool magic) const {
    // Black returns zero in both legacy fields even with active 20% modifiers.
    // Missing modern data is unknown, never a reason to use those legacy zeros.
    return magic ? chainsPercent : eccentricPercent;
}
bool PhysicalSession::modifiersKnown() const {
    const auto c=fresh(chains),e=fresh(eccentric),direction=fresh(inverse),interpretation=fresh(extraMode);
    const auto curve=fresh(mountain);
    if(!c || !e || *c<0 || *e<0 || !direction || (*direction!=0 && *direction!=1) ||
       !curve || (*curve!=0 && *curve!=1) || (*curve==1 && *direction==1) ||
       !interpretation || *interpretation!=0 ||
       (fresh(accessoryDisplayPercent)!=0 && fresh(accessoryDisplayPercent)!=1)) return false;
    if(fresh(accessoryDisplayPercent)==1) {
        for(const bool magic:{true,false}) {
            const auto pct=fresh(percentParameter(magic));
            if(!pct || *pct<0 || *pct>30000) return false;
        }
    }
    return true;
}
bool PhysicalSession::settingsReady() const {
    const auto m=freshMotorMode();
    return controlContextReady() && m && (*m==0 || *m==4) && (!workoutStatus_ || *workoutStatus_==0);
}
bool PhysicalSession::controlContextReady() const {
    const auto u=fresh(unit),maximum=fresh(maxForce);
    return connected_ && accepted_ && standalone() &&
        fresh(workout)==1 && u && (*u==0 || *u==1) && maximum && *maximum>0 &&
        fresh(base).has_value() && fresh(childLock)==0 && updateAllowsControls() && modifiersKnown();
}
bool PhysicalSession::liveWeightReady() const {
    // The source-validated loaded-idle register path changes base only. Mode 5
    // needs a load reassertion in another remote's capture; do not do that here.
    const auto m=freshMotorMode();
    const bool loaded=m==1 || (m==autoLoadMotorMode && fresh(autoLoadStatus)==autoLoadComplete);
    return controlContextReady() && fresh(mountLock)==securedMount && fresh(voidLock)==observedVoidLock &&
        loaded && (!workoutStatus_ || *workoutStatus_==2 || *workoutStatus_==3) &&
        (fresh(accessoryDisplayPercent)==0 || (*fresh(chainsPercent)%100==0 && *fresh(eccentricPercent)%100==0));
}
void PhysicalSession::cancelWrites(const char* reason) {
    waitingWrite_.reset();writes_.clear();target_.reset();status_=reason;
    targetClockPending_=targetImmediate_=false;
    liveIntent_.reset();liveBefore_.reset();
    intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();intentDisplay_.reset();unitSwitchAt_=0;commandFailed_=*reason!=0;
}
bool PhysicalSession::liveAccessoriesExpected() const {
    if(!liveIntent_ || !liveBefore_) return true;
    const auto& r=report_.settings;
    const auto expected=[&](Modifier actual,Modifier before,Modifier target,bool magic) {
        const auto same=[](Modifier a,Modifier b) {return a.percent==b.percent && a.enabled==b.enabled && (!a.enabled || a.value==b.value);};
        const auto id=actual.percent ? percentParameter(magic) : magic ? chains : eccentric;
        const bool inFlight=waitingWrite_ && waitingWrite_->first==id && fresh(id)==waitingWrite_->second && actual.percent==before.percent;
        return same(actual,before) || same(actual,target) || inFlight;
    };
    return r.magicMode==liveBefore_->magicMode &&
        expected(r.magic,liveBefore_->magic,liveIntent_->magic,true) &&
        expected(r.eccentric,liveBefore_->eccentric,liveIntent_->eccentric,false);
}
bool PhysicalSession::standalone() const {
    // October 18: the user confirmed standalone while BOTH current coworker
    // status reports were zero. Missing/stale/nonzero current reports fail closed.
    // This firmware omits the newer paired-EP field. If a device does report it,
    // require its fresh absence too; never let expiry erase a known relationship.
    // No historical role, active Twin routing or load authorization is inferred.
    const auto paired=values_.find(pairedDevice);
    return fresh(twin)==0 && fresh(remoteCoworker)==0 &&
        (paired==values_.end() || fresh(pairedDevice)==0);
}
bool PhysicalSession::send(uint8_t command,const Bytes& payload,uint8_t sender,uint8_t receiver,uint8_t type) {
    if(!connected_) return false;
    if(!accepted_) Serial.printf("%llu VOLTRA bootstrap command %02x\n",static_cast<unsigned long long>(now_),unsigned(command));
    lastWrite_=now_;
    // All three captured bootstrap headers use sequence zero. Ordinary reads
    // and explicit control requests use the session's incrementing sequence.
    const uint16_t sequence=accepted_ ? sequence_++ : 0;
    if(transport_.write(VoltraGattProfile::write,buildFrame(command,payload,sequence,sender,receiver,type))!=TransportResult::Accepted) {
        status_="Bluetooth write failed";connectionFailure_=true;
        transport_.disconnect();return false;
    }
    if(!accepted_) bootstrapWriteJustCompleted_=true;
    return true;
}
bool PhysicalSession::read(std::initializer_list<uint16_t> ids) {
    Bytes b; append16(b,uint16_t(ids.size())); for(auto id:ids) append16(b,id); return send(0x0f,b);
}
bool PhysicalSession::writeParameter(uint16_t id,int value,int bytes) {
    Bytes b{1,0};append16(b,id);
    for(int i=0;i<bytes;++i) b.push_back(uint8_t(uint32_t(value)>>(i*8)));
    // Numeric control trace only; no handshake identity or device address.
    Serial.printf("%llu VOLTRA write request id=%u value=%d\n",static_cast<unsigned long long>(now_),unsigned(id),value);
    if(!send(0x11,b)) return false;
    waitingWrite_=std::make_pair(id,value);waitingAt_=now_;return true;
}
void PhysicalSession::notification(const std::string& uuid,const Bytes& bytes) {
    if(!connected_ || (uuid!=VoltraGattProfile::notify && uuid!=VoltraGattProfile::write && uuid!=VoltraGattProfile::telemetry)) return;
    const size_t channel=uuid==VoltraGattProfile::notify ? 0 : uuid==VoltraGattProfile::write ? 1 : 2;
    if(accepted_ && channel==2 && rawTelemetryHeaders_<4 && !bytes.empty()) {
        ++rawTelemetryHeaders_;
        Serial.printf("VOLTRA telemetry transport bytes=%u marker=%02x",unsigned(bytes.size()),unsigned(bytes[0]));
        if(bytes[0]==0x55 && bytes.size()>=11) Serial.printf(" declared=%u type=%02x cmd=%02x",unsigned(bytes[1]),unsigned(bytes[2]),unsigned(bytes[10]));
        Serial.println();
    }
    // Bounded transport diagnosis only; never use unvalidated bytes as state.
    if(accepted_ && telemetryHeaders_<3 && bytes.size()>=17 && bytes[0]==0x55 && bytes[10]==0xaa && bytes[11]==0x81) {
        ++telemetryHeaders_;
        Serial.printf("VOLTRA stream header channel=%u bytes=%u declared=%u receiver=%02x protocol=%02x%02x subtype=%02x%02x\n",
            unsigned(channel),unsigned(bytes.size()),unsigned(bytes[1]),unsigned(bytes[5]),unsigned(bytes[9]),unsigned(bytes[8]),unsigned(bytes[11]),unsigned(bytes[12]));
    }
    auto& parser=parsers_[channel];
    if(now_-fragmentAt_[channel]>1000) parser.reset();
    fragmentAt_[channel]=now_;
    for(const auto& f:parser.feed(bytes)) {
        if(accepted_ && f.command==0xaa && f.payload.size()>=2 && !telemetryTypes_[f.payload[0]]) {
            telemetryTypes_[f.payload[0]]=true;
            Serial.printf("VOLTRA telemetry subtype=%02x%02x bytes=%u channel=%u receiver=%02x protocol=%04x\n",
                unsigned(f.payload[0]),unsigned(f.payload[1]),unsigned(f.payload.size()),unsigned(channel),unsigned(f.receiver),unsigned(f.protocol));
        }
        // Bounded headers only: no identity, activation key or raw payload logging.
        if(diagnosticFrames_++<24) Serial.printf("VOLTRA frame channel=%u cmd=%02x sender=%02x receiver=%02x bytes=%u\n",unsigned(channel),unsigned(f.command),unsigned(f.sender),unsigned(f.receiver),unsigned(f.payload.size()));
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
    if(f.command==0xaa) {applyTelemetry(f);return;}
    if(f.command==0xab && p.size()>=2 && p[0]==0) {
        if(activated_!=(p[1]==1)) Serial.printf("VOLTRA activation %u\n",unsigned(p[1]));
        activated_=p[1]==1;
    }
    if(f.command==0x77 && diagnosticVersions_++<2) {
        // These are module/product identifiers, not the installed app release.
        // EP1.0 is also present on the user's 2.x machine. Never use it to select
        // a 1.x resistance limit. Log only bounded known module/version tokens.
        std::string text(p.begin(),p.end());
        for(const std::string prefix:{"EP","BP","MainControlv","MotorControl","BMS","PMU"}) {
            for(size_t at=0;(at=text.find(prefix,at))!=std::string::npos;++at) {
                size_t end=at+prefix.size();
                while(end<text.size() && end-at<32 && ((text[end]>='0' && text[end]<='9') || text[end]=='.')) ++end;
                const auto token=text.substr(at,end-at);
                if(end>at+prefix.size() && token.find('.')!=std::string::npos)
                    Serial.printf("VOLTRA module tag %s (not app firmware)\n",token.c_str());
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
        uint64_t value=0;for(int i=0;i<bytes;++i) value|=uint64_t(p[at++])<<(8*i);
        // Only retain presence for a paired identifier; do not log/persist it.
        decoded[id]=id==pairedDevice ? int64_t(value!=0) :
            (id==chainsPercent || id==eccentricPercent) ? int32_t(value) :
            (id==eccentric || id==0x3e83) ? int16_t(value) : int64_t(value);
    }
    if(at!=p.size()) return;
    bool acknowledgedMagic=false,acknowledgedEccentric=false;
    for(const auto& pair:decoded) {
        const auto previous=values_.find(pair.first);
        const bool changed=previous==values_.end() || previous->second.value!=pair.second;
        values_[pair.first]={pair.second,now_};
        // Only normalized parameter values; never serial numbers or BLE identities.
        if(changed && pair.first==pairedDevice) Serial.printf("VOLTRA paired endpoint present = %u\n",unsigned(pair.second!=0));
        else if(changed) Serial.printf("VOLTRA param %u = %lld\n",unsigned(pair.first),static_cast<long long>(pair.second));
        if(waitingWrite_ && waitingWrite_->first==pair.first && now_>=waitingAt_ &&
           (pair.first==motor || (f.command==0x0f && now_-waitingAt_>=settingsSettleMs))) {
            // Motor writes and readbacks have different enums on current devices:
            // explicit unload command 4 is confirmed by unloaded readback 0.
            const auto observed=pair.first==motor ? motorMode(pair.second) : std::optional<int64_t>(pair.second);
            const bool unloaded=pair.first==motor && waitingWrite_->second==4 && observed==0;
            const bool loaded=pair.first==motor && waitingWrite_->second==5 && observed==1;
            const auto autoState=fresh(autoLoadStatus);
            const bool autoArmed=pair.first==motor && waitingWrite_->second==38 && observed==autoLoadMotorMode &&
                autoState && *autoState>=autoLoadWaiting && *autoState<=autoLoadComplete;
            if(unloaded || loaded || autoArmed || observed==waitingWrite_->second) {
                if(pair.first==accessoryDisplayPercent) unitSwitchAt_=now_;
                acknowledgedMagic=acknowledgedMagic || pair.first==chains || pair.first==percentParameter(true);
                acknowledgedEccentric=acknowledgedEccentric || pair.first==eccentric || pair.first==percentParameter(false);
                waitingWrite_.reset();
            }
        }
    }
    updateReport();
    if(liveBefore_) {
        if(acknowledgedMagic) liveBefore_->magic=report_.settings.magic;
        if(acknowledgedEccentric) liveBefore_->eccentric=report_.settings.eccentric;
    }
}
void PhysicalSession::applyTelemetry(const Frame& f) {
    const auto& p=f.payload;
    // SDK 79f6c55's on-device validated AA subtypes. Per-rep means are NOT
    // stationary-cable telemetry. No locally invented rep/set/rest counters.
    if(p.size()==61 && p[0]==0x82 && p[1]==0x3b && (p[2]==1 || p[2]==2)) {
        if(p[2]==1) {deviceSet_=u16(p,4);deviceRep_=u16(p,6);telemetryAt_=now_;}
        // Return-phase statistics are a completed repetition; pull statistics
        // and the stream's in-progress rep counter cannot trigger Stairs.
        if(p[2]==2) {
            completedRep_=u16(p,6);repSet_=u16(p,4);repAt_=now_;++repRevision_;
            Serial.printf("VOLTRA completed rep set=%d rep=%u\n",*repSet_,unsigned(completedRep_));
        }
    } else if(p.size()==127 && p[0]==0x86 && p[1]==0x7d) {
        completedSets_=u16(p,3);completedReps_=u16(p,5);telemetryAt_=now_;
        Serial.printf("VOLTRA workout summary sets=%d reps=%d\n",*completedSets_,*completedReps_);
    } else if(p.size()==97 && p[0]==0x85 && p[1]==0x5f) {
        // SDK 79f6c55: set-close report after the final rep. Its decoded schema
        // has no set index; retain the index actually observed in this session.
        completedSets_=repSet_ ? repSet_ : deviceSet_;
        completedReps_=u16(p,15);telemetryAt_=now_;
        // A repeated close/summary for this set must not restart the rest clock.
        if(completedSets_ && *completedReps_>0 && closedSet_!=completedSets_) {
            closedSet_=completedSets_;setEndedAt_=now_;++setEndRevision_;
        }
        Serial.printf("VOLTRA set summary set=%d reps=%d\n",completedSets_.value_or(-1),*completedReps_);
    } else if(p.size()==45 && p[0]==0x81 && p[1]==0x2b && p[2]<=3) {
        // kirby 8e1ea1c counters; SDK 79f6c55 live sensor offsets (frame - 11).
        const int rep=int(p[4])*256+p[5];
        if(rep>10000) return;
        deviceSet_=p[3];deviceRep_=rep;telemetryAt_=motionAt_=now_;++motionRevision_;
        motionPhase_=p[2];motionForce_=u16(p,11);motionPosition_=u16(p,13);motionVelocity_=int16_t(u16(p,15));
        if(now_-telemetryLogAt_>=250) {
            telemetryLogAt_=now_;
            Serial.printf("VOLTRA motion phase=%u set=%d rep=%d posMm=%d speedMmS=%d forceTenthsLb=%d\n",
                unsigned(p[2]),*deviceSet_,rep,motionPosition_,motionVelocity_,motionForce_);
        }
    } else if(p.size()==39 && p[0]==0x80 && p[1]==0x25 && p[2]==1 && p[3]==0) {
        // Retain unknown values as unknown phases, never the previous active
        // phase. Readiness accepts only the explicitly recognized states.
        workoutStatus_=p[4];
        Serial.printf("VOLTRA workout status=%u\n",unsigned(p[4]));
    }
    updateReport();
}
void PhysicalSession::updateReport() {
    const auto w=fresh(base),rawMotor=fresh(motor),m=freshMotorMode(),mode=fresh(workout),u=fresh(unit);
    if(!w || !rawMotor || !mode || !u || (*u!=0 && *u!=1)) {report_={};return;}
    // Ryan dff6227 documents readback 0/1, distinct from write commands 4/5.
    // The October 18 unloaded capture reports 0. Retain recognized legacy
    // report values, but never turn an unknown value into an unloaded state.
    const auto autoState=fresh(autoLoadStatus);
    const bool knownAuto=m==autoLoadMotorMode && autoState && *autoState>=autoLoadWaiting && *autoState<=autoLoadComplete;
    if(*mode!=1 || !m || (*m!=0 && *m!=1 && *m!=4 && *m!=5 && *m!=38 && *m!=39 && !knownAuto)) {
        // A newly unknown/non-training mode must invalidate the previous report
        // immediately. The weight target then remains an unconditional unload,
        // rather than treating an unrecognized motor state as confirmed unloaded.
        report_={};
        if(!commandFailed_) status_=*mode!=1 ?
            (waitingWrite_ && waitingWrite_->first==workout ? "Opening Weight Training..." :
             !entryHandled_ ? "Preparing Weight Training..." : "Weight Training inactive") :
            "Unknown motor state / tap to unload";
        return;
    }
    report_.unit=*u==0 ? WeightUnit::Pounds : WeightUnit::Kilograms;
    trainingSeen_=true;
    report_.firmwareVersion=firmware_;
    report_.settings.weight=convertWeightUnits(int(*w),WeightUnit::Pounds,report_.unit);
    const bool loaded=*m==1 || *m==5 || *m==39 || (knownAuto && *autoState==autoLoadComplete);
    if(loaded && !report_.loaded) {
        deviceSet_.reset();deviceRep_.reset();repSet_.reset();completedRep_=0;
        repAt_=motionAt_=0;motionRevision_=repRevision_=0;
    }
    report_.loaded=loaded;report_.autoLoadArmed=*m==38 || (knownAuto && *autoState!=autoLoadComplete);
    // This profile recognizes only verified absence, never active Twin roles,
    // peer readiness, combined limits or routing.
    report_.twinKnown=standalone();report_.twin=false;
    WeightLimitEvidence limits;limits.model="VOLTRA I";limits.firmware=firmware_;
    limits.twinKnown=report_.twinKnown;limits.twin=report_.twin;
    if(const auto max=fresh(maxForce)) limits.single.pounds=int(*max);
    report_.maxWeight=resolveWeightLimit(limits,report_.unit).ticks;
    report_.minWeight=report_.twinKnown ? voltraISingleMinimum(report_.unit) : 0;
    report_.settings.magicMode=fresh(mountain)==1 ? MagicMode::Mountain :
        fresh(inverse)==1 ? MagicMode::InverseChains : MagicMode::Chains;
    const auto c=fresh(chains),e=fresh(eccentric);
    report_.settings.magic={c && *c>0,c ? convertWeightUnits(int(*c),WeightUnit::Pounds,report_.unit) : 0,false};
    report_.settings.eccentric={e && *e>0,e ? convertWeightUnits(int(*e),WeightUnit::Pounds,report_.unit) : 0,false};
    const bool modifiers=modifiersKnown();
    // Only the pair selected by 0x5569 is authoritative. Writing the derived
    // pair can briefly echo success and then revert (upstream capture 8e1ea1c).
    report_.percentageSupported=modifiers;
    report_.modifierUnitsSelectable=modifiers;
    report_.sharedModifierUnits=true;
    if(modifiers && fresh(accessoryDisplayPercent)==1) {
        report_.settings.magic.value=int(*fresh(percentParameter(true)))/100;report_.settings.magic.percent=true;
        report_.settings.eccentric.value=int(*fresh(percentParameter(false)))/100;report_.settings.eccentric.percent=true;
        report_.settings.magic.enabled=report_.settings.magic.value>0;
        report_.settings.eccentric.enabled=report_.settings.eccentric.value>0;
    }
    report_.magicSupported={modifiers,modifiers,modifiers};report_.eccentricSupported=modifiers;
    report_.autoLoadSupported=fresh(autoLoadEnabled)==1 && fresh(autoLoadSafetyEnabled)==1;
    report_.autoUnloadHoldSeconds.reset();
    if(const auto delay=fresh(autoUnloadHoldTime); delay && *delay>=0 && *delay<=10)
        report_.autoUnloadHoldSeconds=int(*delay);
    // Rep/summary events are sparse, not a motion heartbeat. Keep received
    // counts while the connection's core state is fresh. Never use them to
    // infer a stationary cable or authorize automatic resistance changes.
    report_.telemetryValid=telemetryAt_!=0;
    report_.rep=deviceRep_.value_or(0);report_.setNumber=deviceSet_;
    report_.setActive=workoutStatus_ && *workoutStatus_<=4 ? std::optional<bool>(*workoutStatus_==2) : std::optional<bool>{};
    report_.resting=workoutStatus_ && *workoutStatus_<=4 ? std::optional<bool>(*workoutStatus_==3) : std::optional<bool>{};
    report_.setEnding=workoutStatus_ && *workoutStatus_<=4 ? std::optional<bool>(*workoutStatus_==4) : std::optional<bool>{};
    report_.repsStarted=deviceRep_ ? std::optional<bool>(*deviceRep_>0) : std::optional<bool>{};
    report_.completedSets=completedSets_;report_.lastSetReps=completedReps_;
    report_.setEndRevision=setEndRevision_;report_.setEndedAtMs=setEndedAt_;
    report_.motionAtMs=motionAt_;report_.motionRevision=motionRevision_;
    report_.positionMm=motionAt_ ? std::optional<int>(motionPosition_) : std::optional<int>{};
    report_.repAtMs=repAt_;report_.repRevision=repRevision_;
    report_.completedRep=completedRep_;report_.repSet=repSet_;
    const bool live=motionAt_ && now_>=motionAt_ && now_-motionAt_<=motionFreshMs;
    report_.repActive=live ? std::optional<bool>(motionPhase_!=0) : std::optional<bool>{};
    report_.moving=live && std::abs(motionVelocity_)>stationarySpeedMmPerSec;
    report_.held=live && !report_.moving && fresh(offset) &&
        motionPosition_>=*fresh(offset)*10+heldExtensionMm && motionForce_>=heldForceTenthsLb;
    report_.force=live ? std::optional<int>(int(std::lround(convertedWeightTicks(motionForce_,WeightUnit::Pounds,report_.unit)/10.0))) : std::optional<int>{};
    // Rest-time register semantics have not been verified on this firmware.
    report_.restSeconds.reset();
    // Capability describes the source-backed base-register path. Execution also
    // requires fresh actual motion and ordinary or completed Auto Load state;
    // never enables loading itself or changes Auto Load settings.
    report_.liveWeightSupported=settingsReady() || liveWeightReady();
    // kirby 8e1ea1c writes accessory amounts using the same parameter path
    // while loaded. Accept only the already-supported motor 1 / Auto Load 35/14;
    // curve/unit switches and motor reassertion remain excluded.
    report_.liveModifierAmountsSupported=report_.liveWeightSupported;
    report_.receivedAtMs=std::min({values_[base].at,values_[motor].at,values_[workout].at,values_[unit].at});
    report_.revision=++reportRevision_;
    if(!busy() && !commandFailed_) status_=settingsReady() || liveWeightReady() ? "" :
        !updateAllowsControls() ? updateBlockReason() : report_.loaded || report_.autoLoadArmed ? "Unload to adjust" : "Reading control state";
}
TrainingReport PhysicalSession::trainingReport() const { return report_; }
SessionReadiness PhysicalSession::readiness() const {
    SessionReadiness r;
    r.validated=accepted_;r.capabilities.weightTraining=fresh(workout)==1 ? Knowledge::Yes : Knowledge::Unknown;
    if(report_.maxWeight>0) r.capabilities.effectiveMaxWeightLbs=uint16_t(convertWeightUnits(report_.maxWeight,report_.unit,WeightUnit::Pounds));
    const auto cable=fresh(position),zero=fresh(offset);
    r.cableZero=cable && zero && std::abs(*cable-*zero)<=cableZeroToleranceCm;
    r.twinSafe=standalone();r.minWeightLbs=5;
    r.ready=accepted_ && activated_ && settingsReady() && freshMotorMode()==0 && cable && zero &&
        fresh(mountLock)==securedMount && fresh(voidLock)==observedVoidLock && fresh(autoLoadStatus)==0;
    if(!accepted_) r.reason="Waiting for VOLTRA acceptance";
    else if(!standalone()) r.reason="Device connection state unavailable";
    else if(!updateAllowsControls()) r.reason=updateBlockReason();
    else if(!settingsReady()) r.reason="Control state unavailable";
    else if(fresh(mountLock)!=securedMount) r.reason="Secure VOLTRA mounting lock";
    else if(fresh(voidLock)!=observedVoidLock) r.reason="VOLTRA safety state unavailable";
    else if(!r.ready) r.reason="Waiting for fresh load readiness";
    else if(!r.cableZero) r.reason="Retract cable to load";
    return r;
}
TransportResult PhysicalSession::setWeight(uint16_t pounds) {
    auto settings=report_.settings; settings.weight=convertWeightUnits(pounds,WeightUnit::Pounds,report_.unit);
    return setTraining(settings);
}
TransportResult PhysicalSession::setTraining(const TrainingSettings& settings) {
    if(!connected_ || !accepted_) return TransportResult::Disconnected;
    if((!settingsReady() && !liveWeightReady()) || (waitingWrite_ && waitingWrite_->first==motor)) {
        status_=!updateAllowsControls() ? updateBlockReason() : "Unload / wait for fresh control state";return TransportResult::Unsupported;
    }
    if(report_.loaded) {
        if(!liveWeightReady() || settings.magicMode!=report_.settings.magicMode ||
           settings.magic.percent!=report_.settings.magic.percent || settings.eccentric.percent!=report_.settings.eccentric.percent) {
            status_="Unload to switch curve or units";return TransportResult::Unsupported;
        }
    }
    const int mode=int(settings.magicMode);
    if(mode<0 || mode>=int(report_.magicSupported.size()) || !report_.magicSupported[mode] || !modifierUnitsAllowed(report_,settings)) {
        status_="Mode / modifier units unavailable";return TransportResult::Unsupported;
    }
    for(const auto m:{settings.magic,settings.eccentric})
        if(m.value<0 || (m.enabled && (m.value<1 || (m.percent && m.value>300)))) {
            status_="Invalid modifier value";return TransportResult::Failed;
        }
    auto pounds=settings;
    pounds.weight=convertWeightUnits(settings.weight,report_.unit,WeightUnit::Pounds);
    for(auto* m:{&pounds.magic,&pounds.eccentric}) if(!m->percent)
        m->value=convertWeightUnits(m->value,report_.unit,WeightUnit::Pounds);
    if(report_.minWeight<1 || settings.weight<report_.minWeight || settings.weight>report_.maxWeight ||
       envelopeHundredths(settings)>report_.maxWeight*100 ||
       envelopeHundredths(pounds)>*fresh(maxForce)*100) {
        status_="Outside device limits";return TransportResult::Failed;
    }
    // Coalesce only new explicit settings intents. Never replay on reconnect,
    // timeout, mode change, or after unload. Do not overwrite an in-flight ACK.
    target_=settings;targetAt_=now_;targetClockPending_=true;targetImmediate_=false;writes_.clear();status_.clear();commandFailed_=false;
    intentUnit_=report_.unit;intentInterpretation_=fresh(extraMode);intentMaximum_=fresh(maxForce);
    intentLoaded_=report_.loaded;
    liveIntent_=intentLoaded_ ? std::optional<TrainingSettings>(settings) : std::optional<TrainingSettings>{};
    liveBefore_=intentLoaded_ ? std::optional<TrainingSettings>(report_.settings) : std::optional<TrainingSettings>{};
    if(!intentDisplay_) intentDisplay_=fresh(accessoryDisplayPercent);
    return TransportResult::Accepted;
}
TransportResult PhysicalSession::setLiveWeight(int ticks) {
    if(!report_.loaded || !liveWeightReady() || busy() || !motionAt_ || now_<motionAt_ || now_-motionAt_>motionFreshMs)
        return TransportResult::Unsupported;
    auto s=report_.settings;s.weight=ticks;
    const auto result=setTraining(s);
    if(result==TransportResult::Accepted) targetImmediate_=true;
    return result;
}
void PhysicalSession::prepareWrites(const TrainingSettings& s) {
    const auto add=[&](uint16_t id,int value,int bytes) {writes_.push_back({id,value,bytes});};
    const int pounds=convertWeightUnits(s.weight,report_.unit,WeightUnit::Pounds);
    const int oldBase=int(*fresh(base)); // settingsReady checked before planning.
    // Monotone envelope ordering: reductions first, then increases. An unchanged
    // modifier is never cleared/restored. Changing IC must not disturb EO.
    if(pounds<oldBase) add(base,pounds,2);
    const auto modifiers=[&](bool decreases) {
        for(const bool magic:{true,false}) {
            const auto m=magic ? s.magic : s.eccentric;
            const auto id=m.percent ? percentParameter(magic) : magic ? chains : eccentric;
            const int value=!m.enabled ? 0 : m.percent ? m.value*100 : convertWeightUnits(m.value,report_.unit,WeightUnit::Pounds);
            const int current=int(*fresh(id));
            if(decreases ? value<current : value>current) add(id,value,width(id));
        }
    };
    modifiers(true);
    const int direction=s.magicMode==MagicMode::InverseChains ? 1 : 0;
    const int mountainCurve=s.magicMode==MagicMode::Mountain ? 1 : 0;
    // Clear the previous special curve before enabling another. Never enable
    // both switches, and never touch EO just because the chain style changed.
    if(!intentLoaded_) {
        if(!direction && fresh(inverse)!=0) add(inverse,0,1);
        if(!mountainCurve && fresh(mountain)!=0) add(mountain,0,1);
        if(direction && fresh(inverse)!=1) add(inverse,1,1);
        if(mountainCurve && fresh(mountain)!=1) add(mountain,1,1);
    }
    if(pounds>oldBase) add(base,pounds,2);
    modifiers(false);
}
TransportResult PhysicalSession::setLoaded(bool loaded) {
    if(!connected_) return TransportResult::Disconnected;
    if(loaded) {
        const auto ready=readiness();
        if(busy() || !ready.validated || !ready.ready || !ready.cableZero || !ready.twinSafe ||
           !settingsReady() || !activated_ || fresh(base).value_or(0)<ready.minWeightLbs) {
            status_=ready.reason.empty() ? "VOLTRA not ready" : ready.reason;return TransportResult::Unsupported;
        }
        commandFailed_=false;status_.clear();
        return writeParameter(motor,5,2) ? TransportResult::Accepted : TransportResult::Failed;
    }
    // Explicit unload remains available even if state is unknown or another request is pending.
    entryHandled_=true; // Cancel any connection-time mode entry too.
    cancelWrites("");
    return writeParameter(motor,4,2) ? TransportResult::Accepted : TransportResult::Failed;
}
TransportResult PhysicalSession::triggerAutoLoad() {
    const auto ready=readiness();
    if(!connected_ || !accepted_) return TransportResult::Disconnected;
    if(busy() || !ready.ready || !ready.twinSafe || !settingsReady() || !activated_ ||
       !report_.autoLoadSupported || fresh(autoLoadStatus)!=0 || fresh(base).value_or(0)<ready.minWeightLbs) {
        status_=ready.reason.empty() ? "Auto Load unavailable" : ready.reason;return TransportResult::Unsupported;
    }
    // Official-app captured AA12: one trigger, no setting changes / bypass / retry.
    commandFailed_=false;status_.clear();
    if(!send(0xaa,{0x12})) return TransportResult::Failed;
    waitingWrite_=std::make_pair(motor,38);waitingAt_=now_;return TransportResult::Accepted;
}
void PhysicalSession::pollState(size_t group) {
    pollAt_[group]=now_;
    switch(group) {
    case 0:read({base,motor,workout,unit});break;
    case 1:read({chains,eccentric,inverse,extraMode,percentParameter(true),percentParameter(false),mountain,accessoryDisplayPercent});break;
    case 2:read({childLock,otaState,twin,remoteCoworker,pairedDevice,maxForce});break;
    case 3:read({position,offset,battery,autoLoadEnabled,autoLoadSafetyEnabled});break;
    case 4:read({autoLoadStatus,mountLock,voidLock,fitnessRestTime,autoUnloadHoldTime});break;
    }
}
bool PhysicalSession::enterTrainingOnConnect() {
    if(entryHandled_ || busy()) return false;
    const auto mode=fresh(workout),m=freshMotorMode();
    if(!mode || !m) return false;
    if(*mode==1) {entryHandled_=true;return false;}
    // User-authorized connection setup: enter from the idle home screen only.
    // Never interrupt another workout, unload automatically, or issue a load.
    if(*mode!=0 || (*m!=0 && *m!=4)) {
        entryHandled_=true;status_="Finish current workout on VOLTRA";return false;
    }
    if(!standalone() || fresh(childLock)!=0) return false;
    if(!updateAllowsControls()) {status_=updateBlockReason();return false;}
    entryHandled_=true;status_="Opening Weight Training...";
    // Android da514d2 enterWeightTrainingPayload: 20400 uint8 = 1.
    // One attempt per connection, confirmed by mode readback, never retried.
    writeParameter(workout,1,1);
    return true;
}
void PhysicalSession::tick(uint64_t now) {
    now_=now;if(!connected_) return;
    // A fault/manual exit can end Weight Training without dropping the GATT
    // link. Return to selection only after confirmed unload; unknown/loaded
    // state must retain the immediately accessible unload target. No retry,
    // automatic mode re-entry, load, or queued settings survives this exit.
    const auto mode=fresh(workout),motorState=freshMotorMode();
    if(trainingSeen_ && mode && *mode!=1 && motorState && (*motorState==0 || *motorState==4) &&
       values_[motor].at>=values_[workout].at) {
        cancelWrites("VOLTRA left Weight Training; reconnect");
        connectionFailure_=true;transport_.disconnect();return;
    }
    // Input arrives after the previous protocol tick, possibly after a blocking
    // GATT operation. Start quiet time on this fresh loop instead of backdating it.
    if(target_ && targetClockPending_) {targetAt_=now;targetClockPending_=false;}
    // tick() time predates a synchronous GATT write. Start the settling deadline
    // on the next loop, after that write completes, rather than backdating it.
    if(bootstrapWriteJustCompleted_) {lastWrite_=now;bootstrapWriteJustCompleted_=false;}
    if(!accepted_ && now_-connectedAt_>30000) { status_="VOLTRA session timed out";connectionFailure_=true;transport_.disconnect();return; }
    if(waitingWrite_ && now_-waitingAt_>writeConfirmationMs) cancelWrites("No device confirmation; stopped");
    if(busy() && intentUnit_ && now_-targetAt_>settingsDeadlineMs) cancelWrites("Settings request expired; stopped");
    const auto pacing=bootstrap_==1 ? helloSettleMs : bootstrap_==2 ? initPacingMs : readPacingMs;
    if(now_-lastWrite_<pacing) return;
    if(bootstrap_==0) {
        Bytes hello(21,0);const std::string name="VOLTRA Remote";std::copy(name.begin(),name.end(),hello.begin());
        hello.insert(hello.end(),helloIdentity_.begin(),helloIdentity_.end());hello.push_back(1);
        if(send(0x4f,hello,1)) ++bootstrap_;
    } else if(bootstrap_==1) {if(send(0xff,{0,0xaa},0xaa,0xd2,0x08)) ++bootstrap_;}
    else if(bootstrap_==2) {Bytes payload{0x81,0x10};payload.insert(payload.end(),identity_.begin(),identity_.end());if(send(0x27,payload)) ++bootstrap_;}
    else if(accepted_) {
        if(busy() && intentUnit_ && (!(intentLoaded_ ? liveWeightReady() : settingsReady()) || report_.unit!=*intentUnit_ ||
           !liveAccessoriesExpected() ||
           fresh(extraMode)!=intentInterpretation_ || fresh(maxForce)!=intentMaximum_ ||
           (!(waitingWrite_ && waitingWrite_->first==accessoryDisplayPercent) && fresh(accessoryDisplayPercent)!=intentDisplay_))) {
            cancelWrites(!updateAllowsControls() ? updateBlockReason() : "Control state changed; request stopped");
        }
        // Deadline-based group scheduling keeps readiness fresh even while a
        // sequence of settings and readbacks occupies the link. Track requests,
        // not optional-field replies, so an unsupported field cannot monopolize it.
        const auto oldest=std::min_element(pollAt_.begin(),pollAt_.end());
        if(!(target_ && targetImmediate_ && !waitingWrite_) && (*oldest==0 || now_-*oldest>=criticalPollIntervalMs)) {
            pollState(size_t(oldest-pollAt_.begin()));return;
        }
        if(enterTrainingOnConnect()) return;
        if(!waitingWrite_ && target_ && (targetImmediate_ || now_-targetAt_>=settingsQuietMs)) {
            const int display=target_->magic.percent ? 1 : 0;
            if(fresh(accessoryDisplayPercent)!=display) {
                // Switch only while unloaded; read it back before planning any
                // amount writes against the newly authoritative registers.
                intentDisplay_=display;unitSwitchAt_=now_;
                if(!writeParameter(accessoryDisplayPercent,display,1)) cancelWrites("Unit switch failed");
                return;
            }
            if(unitSwitchAt_ && (values_[chains].at<=unitSwitchAt_ || values_[eccentric].at<=unitSwitchAt_ ||
               values_[chainsPercent].at<=unitSwitchAt_ || values_[eccentricPercent].at<=unitSwitchAt_)) {
                pollState(1);return;
            }
            unitSwitchAt_=0;
            prepareWrites(*target_);target_.reset();targetImmediate_=false;
        }
        if(!waitingWrite_ && !writes_.empty()) {
            const auto step=writes_.front();writes_.pop_front();
            if(!writeParameter(step.id,step.value,step.bytes)) cancelWrites("Settings write failed; stopped");
            return;
        }
        if(waitingWrite_) {
            read({waitingWrite_->first});return;
        }
        if(!target_) {intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();intentDisplay_.reset();liveIntent_.reset();liveBefore_.reset();}
        if(!telemetrySubscribed_) {
            Bytes payload{1,0};
            if(telemetrySetup_++==0) {
                append16(payload,telemetrySubscribe);
                for(int i=0;i<4;++i) payload.push_back(uint8_t(fitnessNotificationMask>>(i*8)));
            } else {
                append16(payload,telemetryRate);payload.push_back(telemetryHz);telemetrySubscribed_=true;
            }
            send(0x11,payload);return; // Notification selection only; no load/settings replay.
        }
        if(backgroundRead_++%2) {
            switch(infoRead_++%9) {
            case 0:read({pairedDevice,20566});break;
            case 1:read({0x5355,0x53cf});break;
            case 2:send(0x74,{});break;
            case 3:send(0x77,{0});break;
            case 4:send(0x77,{1});break;
            case 5:send(0xab,{1});break;
            case 6:read({chainsPercent,eccentricPercent,legacyChainsPercent,legacyEccentricPercent});break;
            case 7:read({21028,21273,21716,telemetryRate,telemetrySubscribe});break;
            case 8:send(0xaa,{0x13,1});break; // Captured vendor state refresh, no resistance write.
            }
        } else pollState(0);
    }
}
}
#endif
