#pragma once
#include "EventLog.h"
namespace voltra {
enum class ReportPhase { Idle, Queued, Sending, Sent, Error };
struct ReportStatus {
    ReportPhase phase=ReportPhase::Idle;
    std::string id,message;
    bool busy() const {return phase==ReportPhase::Sending;}
};
class DebugReports {
public:
    virtual ~DebugReports()=default;
    virtual ReportStatus status() const=0;
    // Captured on SEND, including dialog unload actions and before BLE handoff.
    // Only SEND may queue or persist it. No BLE operations in this service.
    virtual bool submit(const std::vector<EventLog::Event>& events,const std::string& context)=0;
    virtual void tick(bool networkReady,bool controlIdle)=0;
};
}
