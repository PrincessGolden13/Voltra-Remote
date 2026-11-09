#pragma once
#if !defined(VOLTRA_PHYSICAL) || defined(VOLTRA_SIMULATOR)
#error "Diagnostic networking is physical firmware only"
#endif
#include "DebugReports.h"
#include <atomic>
namespace voltra {
class Esp32DebugReports final : public DebugReports {
public:
    void begin();
    ReportStatus status() const override;
    bool submit(const std::vector<EventLog::Event>&,const std::string&) override;
    void tick(bool networkReady,bool controlIdle) override;
private:
    mutable std::mutex mutex_;
    ReportStatus view_;
    std::string payload_,id_;
    std::atomic<bool> active_{false},permitted_{false};
    void set(ReportPhase,const std::string&);
    void run();
    bool post(const char* path,const std::string& body,const std::string& token,std::string& response);
};
}
