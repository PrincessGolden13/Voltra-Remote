#pragma once
#include "VoltraController.h"
#include "DeviceMemory.h"
#include "DropSetProgress.h"
#include "state/RemoteConfiguration.h"
#include <map>
namespace voltra {
class TrainingApplication {
public:
    TrainingApplication(BleTransport&, VoltraController&, ControlSession&, DeviceMemory* = nullptr, std::vector<DeviceEntry> knownDevices = {}, bool automaticSelection = false);
    void tick(uint64_t now);
    void scan();
    // Reserve an already-disconnected remote for a platform firmware install.
    // Never disconnects, unloads, or changes any confirmed machine state.
    void setMaintenance(bool active) { maintenance_=active; }
    bool select(const DeviceId&);
    bool tryAutomaticSelection();
    bool remembered(const DeviceId&) const;
    bool forget(const DeviceId&);
    void leave(bool preserveFailure = false);
    bool connected() const { return controller_.confirmed().connected; }
    bool connecting() const { return controller_.connecting(); }
    const std::vector<DeviceEntry>& devices() const { return devices_; }
    const std::string& name() const { return name_; }
    TrainingReport report() const { return session_.trainingReport(); }
    // Presentation clock; prefer machine elapsed time, otherwise use its actual
    // set-close event. Never put locally elapsed seconds into TrainingReport.
    std::optional<int> restSeconds() const;
    struct DisplayCounters { std::optional<int> set, rep, completedSets, lastSetReps; };
    DisplayCounters displayCounters() const;
    // Local requested weight for responsive presentation; never machine state.
    std::optional<int> weightPreview() const {
        return pending_ && fresh() && report().unit==weightIntentUnit_ ? weightIntent_ : std::optional<int>{};
    }
    bool fresh() const;
    bool pending() const { return pending_ || session_.busy() || dropRestore_.has_value(); }
    const std::string& message() const { return message_.empty() && connected() ? session_.status() : message_; }
    void adjustWeight(int delta);
    void toggleWeight();
    void unload();
    void autoLoad();
    void toggleModifier(bool magic);
    bool applyModifier(bool magic, MagicMode mode, int value, bool percent);
    DropSettings drops() const;
    bool applyDrops(DropSettings);
    void toggleDrops();
    int dropsCompleted() const { return dropsCompleted_; }
    uint64_t dropConfirmationRevision() const { return dropConfirmationRevision_; }
    std::optional<int> confirmedDropWeight() const { return confirmedDropWeight_; }
    bool dropPending() const { return dropTarget_.has_value(); }
    // Local presentation of the executor's qualified hold, not device telemetry.
    // 1000 = full, 0 = empty; absent hides the bar. Never triggers a command.
    std::optional<int> restPauseRemainingPermille() const;
    Preset currentPreset() const;
    Preset presetForCurrentDevice(Preset) const;
    const std::string& presetCompatibilityError() const { return presetError_; }
    const DeviceId& selectedDevice() const { return selected_; }
    bool applyPreset(const Preset&);
    int startingWeight() const { return startingWeight_; }
private:
    BleTransport& transport_;
    VoltraController& controller_;
    ControlSession& session_;
    DeviceMemory* memory_;
    std::vector<DeviceEntry> recent_, devices_, knownDevices_;
    struct SeenDevice { DeviceEntry device; uint64_t at; };
    std::map<DeviceId,SeenDevice> seenDevices_;
    std::map<DeviceId, DropSettings> dropSettings_;
    DeviceId selected_;
    std::string name_, message_, presetError_;
    uint64_t now_ = 0, scannedAt_ = 0, sentAt_ = 0, sentRevision_ = 0;
    uint64_t restSetRevision_ = 0;
    std::optional<uint64_t> restSince_;
    DropSetProgress dropProgress_;
    bool scanned_ = false, pending_ = false, requestedLoad_ = false;
    bool automaticSelection_ = false;
    bool maintenance_ = false;
    bool scanPending_ = false;
    std::optional<DeviceEntry> connectingDevice_;
    std::optional<int> weightIntent_, dropTarget_;
    WeightUnit weightIntentUnit_=WeightUnit::Pounds;
    std::optional<TrainingSettings> settingsIntent_;
    uint32_t lastRep_ = 0;
    uint64_t telemetryRevision_ = 0, holdSince_ = 0;
    uint64_t repRevision_ = 0, dropSetEndRevision_ = 0;
    std::optional<int> repSet_, holdPosition_;
    bool running_ = false, movementSeen_ = false, holdConsumed_ = false, wasLoaded_ = false;
    bool dropsCancelled_ = false, waitingForDropSet_ = false, dropsExhausted_ = false;
    int dropsCompleted_ = 0;
    uint64_t dropConfirmationRevision_=0;
    std::optional<int> confirmedDropWeight_;
    int startingWeight_ = 0;
    bool observedLoaded_ = false;
    std::optional<Preset> presetIntent_;
    struct DropStart { int weight; WeightUnit unit; };
    struct DropRestore { DropStart start; uint64_t at; bool weightSent=false; };
    std::optional<DropStart> dropStart_;
    std::optional<DropRestore> dropRestore_;
    void beginDropRestore(const TrainingReport&);
    void runDropRestore();
    void sent(TransportResult);
    bool settingsAllowed() const;
    bool sendSettings(TrainingSettings);
    void resetDrops();
    void stopDrops(const char*);
    void runDrops(const TrainingReport&);
    void refreshDiscovery(const std::vector<Advertisement>&);
    void rememberConnection();
};
}
