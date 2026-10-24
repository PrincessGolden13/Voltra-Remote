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
constexpr uint16_t legacyChainsPercent=20873, legacyEccentricPercent=20874;
constexpr uint16_t autoLoadEnabled=21272, autoLoadSafetyEnabled=21389, autoLoadStatus=21447;
constexpr uint16_t childLock=20772, otaState=21476, battery=20013;
constexpr uint64_t stateFreshMs=2000, writeConfirmationMs=3000;
// A bounded unloaded transaction can contain several individually confirmed
// writes. Keep critical polls ahead of it; never retry a timed-out step.
constexpr uint64_t settingsDeadlineMs=10000, criticalPollIntervalMs=1300;
// Send the latest explicit setting after a short pause in rotary input.
constexpr uint64_t settingsQuietMs=100;
constexpr uint16_t telemetrySubscribe=20867;
// Android da514d2's captured notification subscription (not a resistance setting).
constexpr uint32_t fitnessNotificationMask=0x00657bf5;
// HA/Android pace bootstrap at 90 ms. Their captured VOLTRA connect-request
// deadline is ~5 s from radio connection; a 3 s post-HELLO wait plus full GATT
// discovery exceeded it on this remote. Keep every bootstrap step nonblocking.
constexpr uint64_t helloSettleMs=90, initPacingMs=90, readPacingMs=90;
// Only catalogued widths. Unknown/truncated reports are rejected atomically.
int width(uint16_t id) {
    switch(id) {
    case 0x4fb0: case 0x4e2d: case 0x1b5d: case 20844: case 20772: case 20566:
    case 0x5103: case 0x5106: case 0x538d: case 0x53c7: case 0x53c9: case 0x53c6:
    case 0x5182: case 0x53b6: case 0x52e3: case 0x52ca: case 0x5102: case 0x4ee1:
    case 21007: case 21424: case 21272: case 21273: case 21716: case 20642: return 1;
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
    telemetryAt_=0;commandFailed_=false;intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();
    deviceSet_.reset();deviceRep_.reset();completedSets_.reset();completedReps_.reset();
    bootstrapWriteJustCompleted_=false;
    if(connected) {connectionFailure_=false;status_="Opening VOLTRA session...";}
    else if(!connectionFailure_) status_="Connection lost";
}
std::optional<int64_t> PhysicalSession::fresh(uint16_t id) const {
    const auto it=values_.find(id);
    if(it==values_.end() || now_<it->second.at || now_-it->second.at>stateFreshMs) return {};
    return it->second.value;
}
uint16_t PhysicalSession::percentParameter(bool magic) const {
    // Black returns zero in both legacy fields even with active 20% modifiers.
    // Missing modern data is unknown, never a reason to use those legacy zeros.
    return magic ? chainsPercent : eccentricPercent;
}
bool PhysicalSession::modifiersKnown() const {
    const auto c=fresh(chains),e=fresh(eccentric),direction=fresh(inverse),interpretation=fresh(extraMode);
    if(!c || !e || *c<0 || *e<0 || !direction || (*direction!=0 && *direction!=1) ||
       !interpretation || *interpretation!=0) return false;
    if(*interpretation==0) {
        for(const bool magic:{true,false}) {
            const auto pct=fresh(percentParameter(magic));
            if(!pct || *pct<0 || *pct>30000 || *pct%100) return false;
        }
    }
    return true;
}
bool PhysicalSession::settingsReady() const {
    const auto m=fresh(motor),u=fresh(unit),maximum=fresh(maxForce);
    return connected_ && accepted_ && standalone() && m && (*m==0 || *m==4) &&
        fresh(workout)==1 && u && (*u==0 || *u==1) && maximum && *maximum>0 &&
        fresh(base).has_value() && fresh(childLock)==0 && fresh(otaState)==0 && modifiersKnown();
}
void PhysicalSession::cancelWrites(const char* reason) {
    waitingWrite_.reset();writes_.clear();target_.reset();status_=reason;
    targetClockPending_=false;
    intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();commandFailed_=*reason!=0;
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
    if(!send(0x11,b)) return false;
    waitingWrite_=std::make_pair(id,value);waitingAt_=now_;return true;
}
void PhysicalSession::notification(const std::string& uuid,const Bytes& bytes) {
    if(!connected_ || (uuid!=VoltraGattProfile::notify && uuid!=VoltraGattProfile::write && uuid!=VoltraGattProfile::telemetry)) return;
    const size_t channel=uuid==VoltraGattProfile::notify ? 0 : uuid==VoltraGattProfile::write ? 1 : 2;
    auto& parser=parsers_[channel];
    if(now_-fragmentAt_[channel]>1000) parser.reset();
    fragmentAt_[channel]=now_;
    for(const auto& f:parser.feed(bytes)) {
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
    for(const auto& pair:decoded) {
        const auto previous=values_.find(pair.first);
        const bool changed=previous==values_.end() || previous->second.value!=pair.second;
        values_[pair.first]={pair.second,now_};
        // Only normalized parameter values; never serial numbers or BLE identities.
        if(changed && pair.first==pairedDevice) Serial.printf("VOLTRA paired endpoint present = %u\n",unsigned(pair.second!=0));
        else if(changed) Serial.printf("VOLTRA param %u = %lld\n",unsigned(pair.first),static_cast<long long>(pair.second));
        if(waitingWrite_ && waitingWrite_->first==pair.first && now_>=waitingAt_) {
            // Motor writes and readbacks have different enums on current devices:
            // explicit unload command 4 is confirmed by unloaded readback 0.
            const bool unloaded=pair.first==motor && waitingWrite_->second==4 && pair.second==0;
            const bool loaded=pair.first==motor && waitingWrite_->second==5 && pair.second==1;
            if(unloaded || loaded || waitingWrite_->second==pair.second) waitingWrite_.reset();
        }
    }
    updateReport();
}
void PhysicalSession::applyTelemetry(const Frame& f) {
    const auto& p=f.payload;
    // SDK 79f6c55's on-device validated AA subtypes. Per-rep means are NOT
    // stationary-cable telemetry. No locally invented rep/set/rest counters.
    if(p.size()==61 && p[0]==0x82 && p[1]==0x3b && (p[2]==1 || p[2]==2)) {
        if(p[2]==1) {deviceSet_=u16(p,4);deviceRep_=u16(p,6);telemetryAt_=now_;}
    } else if(p.size()==127 && p[0]==0x86 && p[1]==0x7d) {
        completedSets_=u16(p,3);completedReps_=u16(p,5);telemetryAt_=now_;
    }
    updateReport();
}
void PhysicalSession::updateReport() {
    const auto w=fresh(base),m=fresh(motor),mode=fresh(workout),u=fresh(unit);
    if(!w || !m || !mode || !u || (*u!=0 && *u!=1)) {report_={};return;}
    // Ryan dff6227 documents readback 0/1, distinct from write commands 4/5.
    // The October 18 unloaded capture reports 0. Retain recognized legacy
    // report values, but never turn an unknown value into an unloaded state.
    if(*mode!=1 || (*m!=0 && *m!=1 && *m!=4 && *m!=5 && *m!=38 && *m!=39)) {
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
    report_.firmwareVersion=firmware_;
    report_.settings.weight=convertWeightUnits(int(*w),WeightUnit::Pounds,report_.unit);
    const bool loaded=*m==1 || *m==5 || *m==39;
    if(loaded && !report_.loaded) {deviceSet_.reset();deviceRep_.reset();}
    report_.loaded=loaded;report_.autoLoadArmed=*m==38;
    // This profile recognizes only verified absence, never active Twin roles,
    // peer readiness, combined limits or routing.
    report_.twinKnown=standalone();report_.twin=false;
    WeightLimitEvidence limits;limits.model="VOLTRA I";limits.firmware=firmware_;
    limits.twinKnown=report_.twinKnown;limits.twin=report_.twin;
    if(const auto max=fresh(maxForce)) limits.single.pounds=int(*max);
    report_.maxWeight=resolveWeightLimit(limits,report_.unit).ticks;
    report_.settings.magicMode=fresh(inverse)==1 ? MagicMode::InverseChains : MagicMode::Chains;
    const auto c=fresh(chains),e=fresh(eccentric);
    report_.settings.magic={c && *c>0,c ? convertWeightUnits(int(*c),WeightUnit::Pounds,report_.unit) : 0,false};
    report_.settings.eccentric={e && *e>0,e ? convertWeightUnits(int(*e),WeightUnit::Pounds,report_.unit) : 0,false};
    const bool modifiers=modifiersKnown();
    // The user confirmed extraMode=0 and the modern 2000/2000 reports as
    // 20% IC + 20% EO on Black. A missing/unsupported selector is never guessed.
    report_.percentageSupported=modifiers && fresh(extraMode)==0;
    report_.modifierUnitsSelectable=false; // Global interpretation mapping only; mixed units unverified.
    if(report_.percentageSupported) {
        report_.settings.magic.value=int(*fresh(percentParameter(true)))/100;report_.settings.magic.percent=true;
        report_.settings.eccentric.value=int(*fresh(percentParameter(false)))/100;report_.settings.eccentric.percent=true;
        report_.settings.magic.enabled=report_.settings.magic.value>0;
        report_.settings.eccentric.enabled=report_.settings.eccentric.value>0;
    }
    report_.magicSupported={modifiers,modifiers,false};report_.eccentricSupported=modifiers;
    report_.autoLoadSupported=fresh(autoLoadEnabled)==1 && fresh(autoLoadSafetyEnabled)==1;
    // Rep/summary events are sparse, not a motion heartbeat. Keep received
    // counts while the connection's core state is fresh. Never use them to
    // infer a stationary cable or authorize automatic resistance changes.
    report_.telemetryValid=telemetryAt_!=0;
    report_.rep=deviceRep_.value_or(0);report_.setNumber=deviceSet_;
    report_.repsStarted=deviceRep_ ? std::optional<bool>(*deviceRep_>0) : std::optional<bool>{};
    report_.completedSets=completedSets_;report_.lastSetReps=completedReps_;
    // Rest-time register semantics have not been verified on this firmware.
    report_.restSeconds.reset();
    report_.liveWeightSupported=false; // No validated physical live-write/motion capability yet.
    report_.receivedAtMs=std::min({values_[base].at,values_[motor].at,values_[workout].at,values_[unit].at});
    report_.revision=++reportRevision_;
    if(!busy() && !commandFailed_) status_=settingsReady() ? "" : report_.loaded || report_.autoLoadArmed ? "Unload to adjust" : "Reading control state";
}
TrainingReport PhysicalSession::trainingReport() const { return report_; }
SessionReadiness PhysicalSession::readiness() const {
    SessionReadiness r;
    r.validated=accepted_;r.capabilities.weightTraining=fresh(workout)==1 ? Knowledge::Yes : Knowledge::Unknown;
    if(report_.maxWeight>0) r.capabilities.effectiveMaxWeightLbs=uint16_t(convertWeightUnits(report_.maxWeight,report_.unit,WeightUnit::Pounds));
    r.cableZero=fresh(position).has_value() && fresh(offset).has_value() && fresh(position)==fresh(offset);
    r.twinSafe=standalone();r.minWeightLbs=5;
    // Command 5 and AA12 are implemented below, but an unloaded motor report
    // alone is NOT a validated load-readiness/lock profile. Keep this gate closed
    // until the missing interlock enums have been established, not inferred from
    // one idle observation (Black currently reports lock=2, void-lock=6).
    r.ready=false;r.reason="Load interlock mapping unverified";
    if(!accepted_) r.reason="Waiting for VOLTRA acceptance";
    else if(!standalone()) r.reason="Device connection state unavailable";
    else if(!settingsReady()) r.reason="Control state unavailable";
    return r;
}
TransportResult PhysicalSession::setWeight(uint16_t pounds) {
    auto settings=report_.settings; settings.weight=convertWeightUnits(pounds,WeightUnit::Pounds,report_.unit);
    return setTraining(settings);
}
TransportResult PhysicalSession::setTraining(const TrainingSettings& settings) {
    if(!connected_ || !accepted_) return TransportResult::Disconnected;
    if(!settingsReady() || (waitingWrite_ && waitingWrite_->first==motor)) {
        status_="Unload / wait for fresh control state";return TransportResult::Unsupported;
    }
    if(settings.magicMode==MagicMode::Mountain || !modifierUnitsAllowed(report_,settings)) {
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
    if(settings.weight<1 || settings.weight>report_.maxWeight ||
       envelopeHundredths(settings)>report_.maxWeight*100 ||
       envelopeHundredths(pounds)>*fresh(maxForce)*100) {
        status_="Outside device limits";return TransportResult::Failed;
    }
    // Coalesce only new explicit settings intents. Never replay on reconnect,
    // timeout, mode change, or after unload. Do not overwrite an in-flight ACK.
    target_=settings;targetAt_=now_;targetClockPending_=true;writes_.clear();status_.clear();commandFailed_=false;
    intentUnit_=report_.unit;intentInterpretation_=fresh(extraMode);intentMaximum_=fresh(maxForce);
    return TransportResult::Accepted;
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
    if(fresh(inverse)!=direction) add(inverse,direction,1);
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
    case 1:read({chains,eccentric,inverse,extraMode,percentParameter(true),percentParameter(false)});break;
    case 2:read({childLock,otaState,twin,remoteCoworker,pairedDevice,maxForce});break;
    case 3:read({position,offset,battery,autoLoadEnabled,autoLoadSafetyEnabled});break;
    case 4:read({autoLoadStatus,20642,21224});break;
    }
}
bool PhysicalSession::enterTrainingOnConnect() {
    if(entryHandled_ || busy()) return false;
    const auto mode=fresh(workout),m=fresh(motor);
    if(!mode || !m) return false;
    if(*mode==1) {entryHandled_=true;return false;}
    // User-authorized connection setup: enter from the idle home screen only.
    // Never interrupt another workout, unload automatically, or issue a load.
    if(*mode!=0 || (*m!=0 && *m!=4)) {
        entryHandled_=true;status_="Finish current workout on VOLTRA";return false;
    }
    if(!standalone() || fresh(childLock)!=0 || fresh(otaState)!=0) return false;
    entryHandled_=true;status_="Opening Weight Training...";
    // Android da514d2 enterWeightTrainingPayload: 20400 uint8 = 1.
    // One attempt per connection, confirmed by mode readback, never retried.
    writeParameter(workout,1,1);
    return true;
}
void PhysicalSession::tick(uint64_t now) {
    now_=now;if(!connected_) return;
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
        if(busy() && intentUnit_ && (!settingsReady() || report_.unit!=*intentUnit_ ||
           fresh(extraMode)!=intentInterpretation_ || fresh(maxForce)!=intentMaximum_)) {
            cancelWrites("Control state changed; request stopped");
        }
        // Deadline-based group scheduling keeps readiness fresh even while a
        // sequence of settings and readbacks occupies the link. Track requests,
        // not optional-field replies, so an unsupported field cannot monopolize it.
        const auto oldest=std::min_element(pollAt_.begin(),pollAt_.end());
        if(*oldest==0 || now_-*oldest>=criticalPollIntervalMs) {
            pollState(size_t(oldest-pollAt_.begin()));return;
        }
        if(enterTrainingOnConnect()) return;
        if(!waitingWrite_ && target_ && now_-targetAt_>=settingsQuietMs) {
            prepareWrites(*target_);target_.reset();
        }
        if(!waitingWrite_ && !writes_.empty()) {
            const auto step=writes_.front();writes_.pop_front();
            if(!writeParameter(step.id,step.value,step.bytes)) cancelWrites("Settings write failed; stopped");
            return;
        }
        if(waitingWrite_) {
            read({waitingWrite_->first});return;
        }
        if(!target_) {intentUnit_.reset();intentInterpretation_.reset();intentMaximum_.reset();}
        if(!telemetrySubscribed_) {
            telemetrySubscribed_=true;
            Bytes payload{1,0};append16(payload,telemetrySubscribe);
            for(int i=0;i<4;++i) payload.push_back(uint8_t(fitnessNotificationMask>>(i*8)));
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
            case 7:read({21028,21273,21716});break;
            case 8:send(0xaa,{0x13,1});break; // Captured vendor state refresh, no resistance write.
            }
        } else pollState(0);
    }
}
}
#endif
