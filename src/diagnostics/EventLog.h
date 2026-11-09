#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
namespace voltra {
// Bounded semantic events, not a raw BLE/credential trace. RAM only; a report
// service may persist an explicitly approved snapshot. See docs/DEBUG_REPORTS.md.
class EventLog {
public:
    struct Event { uint64_t at=0; std::string message; };
    static constexpr size_t capacity=20, maximumMessageBytes=256;
    static EventLog& instance() { static EventLog log;return log; }
    void clock(uint64_t now) { std::lock_guard<std::mutex> lock(mutex_);now_=now; }
    void add(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto bounded=message.substr(0,maximumMessageBytes);
        if(count_ && events_[(next_+capacity-1)%capacity].message==bounded) return;
        events_[next_]={now_,bounded};next_=(next_+1)%capacity;
        if(count_<capacity) ++count_;
    }
    std::vector<Event> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);std::vector<Event> result;
        for(size_t i=0;i<count_;++i) result.push_back(events_[(next_+capacity-count_+i)%capacity]);
        return result;
    }
    static std::string quote(const std::string& text) {
        std::string out="\"";
        for(unsigned char c:text) {
            if(c=='\\' || c=='\"') {out+='\\';out+=char(c);}
            else if(c=='\n') out+="\\n";
            else if(c>=32 && c<127) out+=char(c);
            else out+='?';
        }
        return out+'"';
    }
private:
    mutable std::mutex mutex_;
    std::array<Event,capacity> events_{};
    size_t next_=0,count_=0;
    uint64_t now_=0;
};
inline void diagnostic(const std::string& message) { EventLog::instance().add(message); }
}
