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
    PhysicalSession(BleTransport&,const std::array<uint8_t,16>& clientIdentity);
    void connectionChanged(bool) override;
    void notification(const std::string&,const Bytes&) override;
    void tick(uint64_t) override;
    bool accepted() const override { return accepted_; }
    bool busy() const override { return waitingWrite_.has_value(); }
    const std::string& status() const override { return status_; }
    SessionReadiness readiness() const override;
    TrainingReport trainingReport() const override;
    TransportResult setWeight(uint16_t) override;
    TransportResult setLoaded(bool) override;
    TransportResult setTraining(const TrainingSettings&) override;
    TransportResult triggerAutoLoad() override;
private:
    BleTransport& transport_;
    std::array<uint8_t,16> identity_;
    FrameParser parser_;
    struct Observed { int64_t value; uint64_t at; };
    std::map<uint16_t,Observed> values_;
    std::deque<std::pair<uint8_t,uint16_t>> seen_;
    TrainingReport report_;
    std::string status_,firmware_;
    uint64_t now_=0,connectedAt_=0,lastWrite_=0,fragmentAt_=0,reportRevision_=0;
    uint16_t sequence_=0;
    size_t bootstrap_=0,readGroup_=0;
    bool connected_=false,accepted_=false,activated_=false,connectionFailure_=false;
    std::optional<std::pair<uint16_t,int>> waitingWrite_;
    uint64_t waitingAt_=0;
    std::optional<int64_t> fresh(uint16_t) const;
    bool send(uint8_t,const Bytes&,uint8_t sender=0xaa,uint8_t receiver=0x10,uint8_t type=0x04);
    bool read(std::initializer_list<uint16_t>);
    bool writeParameter(uint16_t,int,int);
    void apply(const Frame&);
    void updateReport();
};
}
