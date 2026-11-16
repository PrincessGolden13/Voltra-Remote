#pragma once
#include <string>
namespace voltra {
struct WifiSetupStatus {
    bool setupActive=false, connected=false, saved=false;
    bool setupComplete=false, storageReady=true;
    std::string network, address, setupNetwork, message;
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
    // Installation must not restart with credentials that only exist in RAM.
    virtual bool prepareForUpdate() { return false; }
};
}
