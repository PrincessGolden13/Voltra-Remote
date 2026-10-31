#pragma once
#if !defined(VOLTRA_PHYSICAL) || defined(VOLTRA_OFFLINE) || defined(VOLTRA_SIMULATOR)
#error "Physical sessions must only be linked into normal firmware"
#endif
#include "controller/ControlSession.h"
#include "ble/BleTransport.h"
#include "FrameParser.h"
#include <map>
#include <array>
#include <deque>
namespace voltra {
class PhysicalSession final : public ControlSession {
public:
    PhysicalSession(BleTransport&,const std::array<uint8_t,16>& clientIdentity,
                    std::optional<std::array<uint8_t,6>> helloIdentity = {});
    void connectionChanged(bool) override;
    void notification(const std::string&,const Bytes&) override;
    void tick(uint64_t) override;
    bool accepted() const override { return accepted_; }
    bool busy() const override { return waitingWrite_.has_value() || !writes_.empty() || target_.has_value(); }
    bool commandFailed() const override { return commandFailed_; }
    const std::string& status() const override { return status_; }
    SessionReadiness readiness() const override;
    TrainingReport trainingReport() const override;
    TransportResult setWeight(uint16_t) override;
    TransportResult setLoaded(bool) override;
    TransportResult setTraining(const TrainingSettings&) override;
    TransportResult setLiveWeight(int ticks) override;
    void cancelTraining() override { if(intentUnit_) cancelWrites(""); }
    TransportResult triggerAutoLoad() override;
private:
    BleTransport& transport_;
    std::array<uint8_t,16> identity_;
    std::array<uint8_t,6> helloIdentity_{};
    std::array<FrameParser,3> parsers_; // Command, transport and telemetry response.
    std::array<uint64_t,3> fragmentAt_{};
    struct Observed { int64_t value; uint64_t at; };
    std::map<uint16_t,Observed> values_;
    std::deque<std::pair<uint8_t,uint16_t>> seen_;
    TrainingReport report_;
    std::string status_,firmware_;
    uint64_t now_=0,connectedAt_=0,lastWrite_=0,reportRevision_=0;
    uint16_t sequence_=0;
    size_t bootstrap_=0,backgroundRead_=0,infoRead_=0;
    std::array<uint64_t,5> pollAt_{};
    unsigned diagnosticFrames_=0;
    unsigned diagnosticVersions_=0;
    bool connected_=false,accepted_=false,activated_=false,connectionFailure_=false;
    bool bootstrapWriteJustCompleted_=false;
    std::optional<std::pair<uint16_t,int>> waitingWrite_;
    uint64_t waitingAt_=0;
    struct Write { uint16_t id; int value; int bytes; };
    std::deque<Write> writes_;
    std::optional<TrainingSettings> target_;
    uint64_t targetAt_=0, telemetryAt_=0;
    std::optional<int> deviceSet_, deviceRep_, completedSets_, completedReps_;
    std::optional<int> closedSet_;
    uint64_t setEndRevision_=0, setEndedAt_=0;
    std::optional<WeightUnit> intentUnit_;
    std::optional<int64_t> intentInterpretation_, intentMaximum_, intentDisplay_;
    uint64_t unitSwitchAt_=0;
    uint64_t motionAt_=0, motionRevision_=0, repAt_=0, repRevision_=0, telemetryLogAt_=0;
    int motionPosition_=0, motionVelocity_=0, motionForce_=0;
    uint8_t motionPhase_=0;
    uint32_t completedRep_=0;
    std::optional<int> repSet_;
    bool intentLoaded_=false;
    std::optional<TrainingSettings> liveIntent_;
    std::optional<int> workoutStatus_;
    unsigned telemetryHeaders_=0, telemetrySetup_=0;
    std::array<bool,256> telemetryTypes_{};
    unsigned rawTelemetryHeaders_=0;
    bool telemetrySubscribed_=false, commandFailed_=false;
    bool entryHandled_=false, targetClockPending_=false;
    bool targetImmediate_=false;
    bool trainingSeen_=false;
    std::optional<int64_t> fresh(uint16_t) const;
    std::optional<int64_t> freshMotorMode() const;
    bool standalone() const;
    bool settingsReady() const;
    bool controlContextReady() const;
    bool liveWeightReady() const;
    bool modifiersKnown() const;
    uint16_t percentParameter(bool magic) const;
    void prepareWrites(const TrainingSettings&);
    void cancelWrites(const char*);
    void pollState(size_t);
    bool enterTrainingOnConnect();
    void applyTelemetry(const Frame&);
    bool send(uint8_t,const Bytes&,uint8_t sender=0xaa,uint8_t receiver=0x10,uint8_t type=0x04);
    bool read(std::initializer_list<uint16_t>);
    bool writeParameter(uint16_t,int,int);
    void apply(const Frame&);
    void updateReport();
};
}
