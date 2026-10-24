#pragma once
#include "ble/BleTypes.h"
#include "state/VoltraState.h"
#include "state/TrainingState.h"
namespace voltra {
// A VOLTRA-level command boundary, separate from generic GATT transport.
struct SessionReadiness {
    bool validated = false, ready = false, cableZero = false, twinSafe = false;
    VoltraCapabilities capabilities;
    uint16_t minWeightLbs = 0;
    std::string reason;
};
class ControlSession {
public:
    virtual ~ControlSession() = default;
    virtual void connectionChanged(bool) {}
    virtual void notification(const std::string&,const Bytes&) {}
    virtual void tick(uint64_t) {}
    virtual bool accepted() const { return true; }
    virtual bool busy() const { return false; }
    virtual bool commandFailed() const { return false; }
    virtual const std::string& status() const { static const std::string empty; return empty; }
    virtual SessionReadiness readiness() const = 0;
    virtual TransportResult setWeight(uint16_t pounds) = 0;
    virtual TransportResult setLoaded(bool loaded) = 0;
    virtual TrainingReport trainingReport() const { return {}; }
    virtual TransportResult setTraining(const TrainingSettings&) { return TransportResult::Unsupported; }
    virtual void cancelTraining() {} // Cancel unsent settings, never load/unload or retry.
    virtual TransportResult triggerAutoLoad() { return TransportResult::Unsupported; }
};
}
