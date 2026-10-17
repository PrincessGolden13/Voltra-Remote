#pragma once
#include "ble/BleTransport.h"
#include "MockVoltra.h"
#include "controller/ControlSession.h"
#include "controller/DeviceMemory.h"
#include "protocol/WeightLimits.h"
namespace voltra {
class MockTransport final : public BleTransport, public ControlSession {
public:
    MockTransport();
    static constexpr const char* deviceId = "synthetic-voltra";
    std::vector<Advertisement> scan() override;
    TransportResult connect(const DeviceId&) override;
    void disconnect() override;
    std::vector<std::string> discoverServices() override;
    std::vector<Characteristic> discoverCharacteristics(const std::string&) override;
    TransportResult subscribe(const std::string&) override;
    TransportResult write(const std::string&, const Bytes&) override { return TransportResult::Unsupported; }
    void poll(uint64_t nowMs) override;
    void onNotification(Notification callback) override { notification_ = std::move(callback); }
    void onConnection(Connection callback) override { connection_ = std::move(callback); }
    void setProfile(SimulatorProfile profile);
    SessionReadiness readiness() const override;
    TransportResult setWeight(uint16_t pounds) override;
    TransportResult setLoaded(bool loaded) override;
    TrainingReport trainingReport() const override { return reported_; }
    TransportResult setTraining(const TrainingSettings&) override;
    TransportResult triggerAutoLoad() override;
    void toggleAvailability();
    std::vector<DeviceEntry> catalog() const;
    void previewModifier(int value); // Render-only synthetic initial setting.
    void previewUnit(WeightUnit); // Render-only unit-specific limit/layout preview.
    void previewWeight(int ticks); // Render-only base/envelope layout preview.
    void completeRep();
    void setHeld(bool held);
    void ignoreNextRequest() { ignoreNext_ = true; }
private:
    struct SimDevice { DeviceId id; std::string name; bool online = true; TrainingReport state; WeightLimitEvidence limits; };
    std::vector<SimDevice> devices_;
    size_t active_ = 0;
    TrainingReport reported_;
    std::optional<TrainingSettings> pendingSettings_;
    bool pendingAuto_ = false, ignoreNext_ = false;
    uint64_t heldSince_ = 0;
    uint64_t restSince_ = 0;
    bool consumeIgnored();
    MockVoltra device_;
    Notification notification_;
    Connection connection_;
    SimulatorProfile profile_ = SimulatorProfile::Unloaded;
    bool connected_ = false, subscribed_ = false, sent_ = false;
    uint64_t lastMs_ = 0;
    uint64_t nowMs_ = 0, dueMs_ = 0;
    std::optional<uint16_t> pendingWeight_;
    std::optional<bool> pendingLoaded_;
    static constexpr uint64_t confirmationDelayMs = 40;
    static constexpr uint64_t notificationIntervalMs = 100;
};
}
