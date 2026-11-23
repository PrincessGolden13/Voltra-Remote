#pragma once
#if !defined(VOLTRA_PHYSICAL) || defined(VOLTRA_OFFLINE) || defined(VOLTRA_SIMULATOR)
#error "Physical sessions must only be linked into normal firmware"
#endif
#include "controller/ControlSession.h"
#include "ble/BleTransport.h"
#include "FrameParser.h"
#include "TwinStatus.h"
#include <map>
#include <array>
#include <deque>
namespace voltra {
class PhysicalSession final : public ControlSession {
public:
    PhysicalSession(BleTransport&,const std::array<uint8_t,16>& clientIdentity,
                    std::optional<std::array<uint8_t,6>> helloIdentity = {});
    bool prepareConnection() override;
    void connectionChanged(bool) override;
    void setSelectedAddress(std::string) override;
    bool setReadOnlyProbe(bool) override;
    bool readOnlyProbe() const override { return readOnlyProbe_; }
    void notification(const std::string&,const Bytes&) override;
    void tick(uint64_t) override;
    bool accepted() const override { return accepted_; }
    bool busy() const override { return waitingWrite_.has_value() || !writes_.empty() || target_.has_value(); }
    bool unloading() const override;
    bool commandFailed() const override { return commandFailed_; }
    const std::string& status() const override { return status_; }
    std::string diagnosticState() const override;
    SessionReadiness readiness() const override;
    TrainingReport trainingReport() const override;
    TransportResult setWeight(uint16_t) override;
    TransportResult setLoaded(bool) override;
    TransportResult setTraining(const TrainingSettings&) override;
    TransportResult setLiveWeight(int ticks) override;
    void cancelTraining() override {
        singleAutoFollowup_=SingleAutoFollowup::None;
        if(intentUnit_ && !unloading()) cancelWrites("");
    }
    TransportResult triggerAutoLoad() override;
private:
    BleTransport& transport_;
    std::array<uint8_t,16> identity_;
    std::array<uint8_t,6> helloIdentity_{};
    std::array<FrameParser,3> parsers_; // Command, transport and telemetry response.
    std::array<uint64_t,3> fragmentAt_{};
    struct Observed { int64_t value; uint64_t at; };
    std::map<uint16_t,Observed> values_;
    std::optional<Observed> directMotor_;
    std::optional<DeviceAddress> selectedAddress_;
    std::optional<TwinStatus> twinStatus_;
    std::optional<DeviceAddress> primaryPeer_;
    bool primaryRouteRevoked_=false;
    enum class PairedMotorFamily { Native, Legacy };
    std::optional<PairedMotorFamily> pairedMotorFamily_;
    uint64_t twinStatusAt_=0,twinTopologyAt_=0,twinPollAt_=0;
    bool twinObserved_=false,twinReplyOwnMatch_=false,twinReplyAccepted_=false,twinReplySeen_=false;
    size_t twinReplyBytes_=0;
    std::string twinReplyDiagnostic_;
    uint64_t twinDiagnosticAt_=0;
    std::deque<std::pair<uint8_t,uint16_t>> seen_;
    TrainingReport report_;
    std::string status_,firmware_;
    uint64_t now_=0,connectedAt_=0,lastWrite_=0,reportRevision_=0;
    uint16_t sequence_=0;
    size_t bootstrap_=0,backgroundRead_=0,infoRead_=0;
    std::array<uint64_t,5> pollAt_{};
    bool pairedReadSupported_=false,autoUnloadReadSupported_=false;
    bool autoLoadCountdownProbed_=false,autoLoadCountdownSupported_=false;
    uint64_t autoLoadPollAt_=0;
    uint64_t pairedRefreshAt_=0;
    bool pairedRefreshPending_=false;
    enum class SingleAutoFollowup { None, ReadStatus, RefreshStream };
    SingleAutoFollowup singleAutoFollowup_=SingleAutoFollowup::None;
    bool liveWeightProbed_=false;
    // The terse runtime target belongs to one exact standalone loaded context.
    // A settings/core/topology transition cannot reuse an earlier rep's target.
    struct RuntimeTargetObservation {
        int tenthsLb;
        uint64_t at;
        int64_t motor,workout,unit,base;
    };
    std::optional<RuntimeTargetObservation> runtimeTarget_;
    unsigned runtimeTargetFrames_=0;
    uint16_t runtimeTargetSource_=0;
    size_t runtimeTargetBytes_=0;
    bool runtimeTargetShapeValid_=false;
    unsigned diagnosticFrames_=0;
    unsigned diagnosticVersions_=0;
    bool connected_=false,accepted_=false,activated_=false,connectionFailure_=false;
    // Activation is a same-session status observation, separate from historical
    // coworker diagnostics. Unknown status cannot admit a load.
    bool activationKnown_=false;
    unsigned activationReadAttempts_=0;
    uint64_t activationReadAt_=0,activationStatusAt_=0;
    bool readOnlyProbe_=false; // Connection reset must preserve explicit probe intent.
    bool helloPrepared_=false,connectionClockPending_=false;
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
    int peerMotionVelocity_=0;
    bool pairedMotion_=false;
    bool pairedMotionUsableForEdit_=false;
    uint8_t motionPhase_=0;
    // AA82 phase 2 starts the return; it is not a completed repetition.
    struct ReturnBoundary { int set; uint32_t rep; bool sawReturn; int64_t motor,unit,base; };
    std::optional<ReturnBoundary> pendingReturn_;
    bool completedRepWindow_=false;
    int completedRepPosition_=0;
    uint32_t completedRep_=0;
    std::optional<int> repSet_,boundarySet_; // Raw AA82 set retained for summary association only.
    bool intentLoaded_=false;
    std::optional<TrainingSettings> liveIntent_;
    std::optional<TrainingSettings> liveBefore_;
    // Applies only to automatic base changes, through their first wire write.
    struct AutomaticWeightWindow { uint8_t phase; int set,rep,position; };
    std::optional<AutomaticWeightWindow> automaticWeightWindow_;
    bool automaticWeightWindowCurrent() const;
    void clearRepBoundary();
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
    std::optional<int64_t> freshDirectMotorMode() const;
    bool updateAllowsControls() const;
    const char* updateBlockReason() const;
    bool standalone() const;
    bool primaryTwinFresh() const;
    bool primaryRouteKnown() const;
    bool controlObservationsCurrent(bool loading=false) const;
    std::optional<int64_t> decodeMotorMode(int64_t) const;
    std::optional<int> encodedWeight(int ticks,bool baseWeight) const;
    int motorCommand(bool loaded) const;
    void invalidatePrimaryRoute(const char*);
    bool twinStatusFresh() const;
    CurrentTwinRole currentTwinRole() const;
    const char* readOnlyControlReason() const;
    void updateReadback();
    bool settingsReady() const;
    bool loadSafetyStatusSupported() const;
    bool controlContextReady() const;
    bool liveWeightReady() const;
    bool pairedManualWeightReady() const;
    bool liveAccessoriesExpected() const;
    bool modifiersKnown() const;
    uint16_t percentParameter(bool magic) const;
    void prepareWrites(const TrainingSettings&);
    void cancelWrites(const char*);
    void pollState(size_t);
    bool enterTrainingOnConnect();
    void applyTelemetry(const Frame&);
    bool sendHello(bool preparing=false);
    bool send(uint8_t,const Bytes&,uint8_t sender=0xaa,uint8_t receiver=0x10,uint8_t type=0x04,bool preparingHello=false);
    bool read(std::initializer_list<uint16_t>);
    bool writeParameter(uint16_t,int,int);
    void apply(const Frame&);
    void updateReport();
};
}
