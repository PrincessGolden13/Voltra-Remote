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
enum class DeviceEndObservation { None, MountLockReleased };
class ControlSession {
public:
    virtual ~ControlSession() = default;
    // The write channel and a protocol response subscription are available;
    // optional notification setup and connection completion may still remain.
    // A protocol may send its initial hello only. This cannot accept the
    // session, establish readiness, restore intent or send a training command.
    virtual bool prepareConnection() { return true; }
    virtual void connectionChanged(bool) {}
    virtual void setSelectedAddress(std::string) {} // Selected identity for protocol reply matching only.
    // Explicit discovery connections must not change training/device settings
    // or issue motor commands. A supported backend retains this mode through
    // connection resets; promotion can fail while current role/state is unknown.
    virtual bool setReadOnlyProbe(bool enabled) { return !enabled; }
    virtual bool readOnlyProbe() const { return false; }
    // Fresh local idle proof for leaving a selected follower. It grants no
    // control permission on either member of the pair.
    virtual bool twinHandoffIdle() const { return false; }
    // Hold an idle standalone connection under observation after it leaves
    // Weight Training, while checking for a newly joined Twin relationship.
    virtual bool checkingTwinAfterWorkoutExit() const { return false; }
    // The application has adopted the same-link primary and can use normal
    // idle workout entry after the read-only check. No load is authorized.
    virtual void allowTwinWorkoutReentry(bool) {}
    virtual void notification(const std::string&,const Bytes&) {}
    virtual void tick(uint64_t) {}
    virtual bool accepted() const { return true; }
    virtual bool busy() const { return false; }
    virtual bool unloading() const { return false; }
    virtual bool commandFailed() const { return false; }
    virtual const std::string& status() const { static const std::string empty; return empty; }
    virtual std::string diagnosticState() const { return status(); }
    // A recent machine report, not an inferred cause of a Bluetooth failure.
    virtual DeviceEndObservation endObservation(uint64_t) const { return DeviceEndObservation::None; }
    virtual SessionReadiness readiness() const = 0;
    virtual TransportResult setWeight(uint16_t pounds) = 0;
    virtual TransportResult setLoaded(bool loaded) = 0;
    virtual TrainingReport trainingReport() const { return {}; }
    virtual TransportResult setTraining(const TrainingSettings&) { return TransportResult::Unsupported; }
    // Already-loaded base only, dispatched without rotary coalescing. Never a
    // load/reload command; implementations must preserve all accessory settings.
    virtual TransportResult setLiveWeight(int ticks) { (void)ticks; return TransportResult::Unsupported; }
    virtual void cancelTraining() {} // Cancel unsent settings, never load/unload or retry.
    virtual TransportResult triggerAutoLoad() { return TransportResult::Unsupported; }
};
}
