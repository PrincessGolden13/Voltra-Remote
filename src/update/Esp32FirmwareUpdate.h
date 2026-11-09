#pragma once
#if !defined(VOLTRA_PHYSICAL) || defined(VOLTRA_SIMULATOR)
#error "Physical firmware updating must never be linked into a simulator"
#endif
#include "FirmwareUpdate.h"
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
namespace voltra {
class Esp32FirmwareUpdate final : public FirmwareUpdate {
public:
    void begin();
    UpdateStatus status() const override;
    void tick(uint64_t now, bool networkReady, bool controlIdle) override;
    void check() override;
    bool install() override;
    bool startupPending() const override { return startupPending_; }
private:
    struct Release { std::string version, sha256; uint32_t size=0; } release_;
    mutable SemaphoreHandle_t mutex_=nullptr;
    UpdateStatus view_;
    std::atomic<bool> workerActive_{false}, permitted_{false};
    bool networkReady_=false, controlIdle_=false, startupPending_=true, bootCheckDone_=false;
    uint64_t startedAt_=0, readyAt_=0;
    void set(UpdatePhase, const std::string&, int percent=0);
    void launch(bool installing);
    void run(bool installing);
    bool fetchRelease();
    bool download();
};
}
