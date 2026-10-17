#pragma once
#include "ble/BleTypes.h"
#include "state/VoltraState.h"
#include "state/TrainingState.h"
namespace voltra {
// A VOLTRA-level command boundary, separate from generic GATT transport.
// Only the simulator implements this in the foundation. No physical wire writes.
struct SessionReadiness {
    bool validated = false, ready = false, cableZero = false, twinSafe = false;
    VoltraCapabilities capabilities;
    uint16_t minWeightLbs = 0;
};
class ControlSession {
public:
    virtual ~ControlSession() = default;
    virtual void connectionChanged(bool) {}
    virtual void notification(const std::string&,const Bytes&) {}
    virtual void tick(uint64_t) {}
    virtual bool accepted() const { return true; }
    virtual bool busy() const { return false; }
    virtual const std::string& status() const { static const std::string empty; return empty; }
    virtual SessionReadiness readiness() const = 0;
    virtual TransportResult setWeight(uint16_t pounds) = 0;
    virtual TransportResult setLoaded(bool loaded) = 0;
    virtual TrainingReport trainingReport() const { return {}; }
    virtual TransportResult setTraining(const TrainingSettings&) { return TransportResult::Unsupported; }
    virtual TransportResult triggerAutoLoad() { return TransportResult::Unsupported; }
};
}
