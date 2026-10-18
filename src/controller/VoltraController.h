#pragma once
#include "ble/BleTransport.h"
#include "protocol/FrameParser.h"
#include "state/VoltraState.h"
#include "state/RequestedState.h"
#include "ControlSession.h"
namespace voltra {
struct ControllerPolicy {
    uint64_t staleAfterMs = 2000; // Provisional UI freshness policy, not a safety validation.
};
class VoltraController {
public:
    explicit VoltraController(BleTransport&, ControllerPolicy = {}, ControlSession* = nullptr);
    ~VoltraController();
    VoltraController(const VoltraController&) = delete;
    VoltraController& operator=(const VoltraController&) = delete;
    bool connect(const DeviceId&);
    bool connecting() const { return connecting_; }
    void disconnect();
    void tick(uint64_t nowMs);
    void adjustDraft(int delta);
    const char* requestLoad();
    const char* requestUnload();
    const char* submitDraft();
    const VoltraState& confirmed() const { return confirmed_; }
    const RequestedState& requested() const { return requested_; }
    bool weightFresh() const;
    LoadState loadState() const;
    const char* feedback() const { return feedback_; }
    bool controlPending() const { return pendingLoad_.has_value() || requested_.status == CommandStatus::Pending; }
    bool loadRequested() const { return pendingLoad_ == true; }
private:
    BleTransport& transport_;
    ControllerPolicy policy_;
    VoltraState confirmed_;
    RequestedState requested_;
    FrameParser parser_;
    uint64_t now_ = 0;
    ControlSession* controls_ = nullptr;
    std::optional<bool> pendingLoad_;
    bool connecting_ = false, linkUp_ = false;
    bool finishConnection();
    uint64_t sentAt_ = 0;
    static constexpr uint64_t commandTimeoutMs = 3000;
    const char* feedback_ = "Turn wheel to adjust weight";
    const char* report(const char* text) { feedback_ = text; return text; }
};
}
