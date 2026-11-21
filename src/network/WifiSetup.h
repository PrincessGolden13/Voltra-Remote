#pragma once
#include <string>
namespace voltra {
enum class WifiConnectionCheck { Idle, Waiting, Checking, Reachable, NoReply, Error, Unavailable };
struct WifiSetupStatus {
    bool setupActive=false, connected=false, saved=false, connecting=false;
    bool setupComplete=false, storageReady=true;
    std::string network, address, setupNetwork, message;
    WifiConnectionCheck connectionCheck=WifiConnectionCheck::Idle;
    int pingReplies=0, pingMs=-1;
};
// Remote networking only. No VOLTRA/controller access, no credentials in UI
// configuration, and no dependency on any board GPIO or Bluetooth transport.
class WifiSetup {
public:
    virtual ~WifiSetup()=default;
    virtual WifiSetupStatus status() const=0;
    virtual bool start()=0;
    virtual void stop()=0;
    virtual void tick(bool radioAvailable)=0;
    // Explicit retry of the existing saved profile; never opens setup or saves.
    virtual bool reconnectSaved() { return false; }
    // Installation must not restart with credentials that only exist in RAM.
    virtual bool prepareForUpdate() { return false; }
    // Explicit screen-open check; implementations must not block UI/control work.
    virtual void requestConnectionCheck() {}
    virtual void cancelConnectionCheck() {}
};
}
