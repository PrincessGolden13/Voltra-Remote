#pragma once
#include <cstdint>
#include <string>
#ifndef VOLTRA_VERSION
#define VOLTRA_VERSION "development"
#endif
namespace voltra {
enum class UpdatePhase { Idle, Checking, Current, Available, Installing, Ready, Error };
struct UpdateStatus {
    UpdatePhase phase=UpdatePhase::Idle;
    std::string version, message;
    int percent=0;
    bool busy() const { return phase==UpdatePhase::Checking || phase==UpdatePhase::Installing || phase==UpdatePhase::Ready; }
};
// Platform service: no machine state, BLE commands, or board GPIOs here.
// The caller supplies availability and keeps connections out of installation.
class FirmwareUpdate {
public:
    virtual ~FirmwareUpdate()=default;
    virtual UpdateStatus status() const=0;
    virtual void tick(uint64_t now, bool networkReady, bool controlIdle)=0;
    virtual void check()=0;
    virtual bool install()=0; // Only from a current explicit user action.
    virtual bool startupPending() const=0;
};
}
