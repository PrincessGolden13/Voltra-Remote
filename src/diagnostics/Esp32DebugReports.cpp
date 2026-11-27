#include "Esp32DebugReports.h"
#include "Esp32CrashNotes.h"
#include <algorithm>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <cJSON.h>
#include <esp_core_dump.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>
#include <cstdio>
#include <ctime>
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);
namespace voltra {
namespace {
constexpr size_t maximumReportBytes=12*1024,maximumContextBytes=2048,maximumResponseBytes=1024;
constexpr size_t maximumTraceEvents=11;
// IDF 4.4 limits an NVS string to 4000 bytes including the terminator. A
// bounded blob permits larger reports and leaves the legacy string readable.
constexpr char pendingReportKey[]="pending-v2",legacyReportKey[]="pending";
constexpr char crashSummaryKey[]="crash-summary";
constexpr int socketTimeoutMs=10000;
constexpr uint32_t clockWaitMs=10000,workerStackBytes=12288;
constexpr time_t earliestValidTime=1704067200; // 2024-01-01 UTC, certificate clock sanity floor.
constexpr char host[]="voltra-remote-diagnostics.darkheath.chatgpt.site";
std::string hex(const uint8_t* bytes,size_t size) {
    const char* chars="0123456789abcdef";std::string result;
    for(size_t i=0;i<size;++i) {result+=chars[bytes[i]>>4];result+=chars[bytes[i]&15];}return result;
}
const char* resetName(esp_reset_reason_t reason) {
    switch(reason) {
    case ESP_RST_POWERON:return "power-on";
    case ESP_RST_EXT:return "external";
    case ESP_RST_SW:return "software";
    case ESP_RST_PANIC:return "panic";
    case ESP_RST_INT_WDT:return "interrupt-watchdog";
    case ESP_RST_TASK_WDT:return "task-watchdog";
    case ESP_RST_WDT:return "watchdog";
    case ESP_RST_DEEPSLEEP:return "deep-sleep";
    case ESP_RST_BROWNOUT:return "brownout";
    case ESP_RST_SDIO:return "sdio";
    default:return "unknown";
    }
}
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
// Persist only the reviewed fields, never a copy of a task or crash note.
// Layout: format byte, presence flags, lowercase ELF digest prefix, cause, PC.
constexpr uint8_t crashSummarySchema=1, causePresent=1, pcPresent=2;
constexpr size_t fingerprintOffset=2, causeOffset=fingerprintOffset+fingerprintCharacters, pcOffset=causeOffset+1;
using CrashSummaryRecord=std::array<uint8_t,pcOffset+4>;

CrashSummaryRecord readCrashSummary() {
    CrashSummaryRecord record{};
    Preferences nvs;
    if(nvs.begin("voltra-debug",true) && nvs.isKey(crashSummaryKey) &&
        nvs.getBytesLength(crashSummaryKey)==record.size()) {
        if(nvs.getBytes(crashSummaryKey,record.data(),record.size())!=record.size()) record={};
    }
    return record;
}

CrashSummaryRecord encodeCrashSummary(const CrashNotes& notes) {
    CrashSummaryRecord record{};
    record[0]=crashSummarySchema;
    std::memcpy(record.data()+fingerprintOffset,notes.fingerprint.data(),fingerprintCharacters);
    if(notes.cause) {
        record[1]|=causePresent;
        record[causeOffset]=uint8_t(*notes.cause);
    }
    if(notes.pc) {
        record[1]|=pcPresent;
        for(unsigned byte=0;byte<4;++byte) record[pcOffset+byte]=uint8_t(*notes.pc>>(8*byte));
    }
    return record;
}

bool saveCrashSummary(const CrashSummaryRecord& record) {
    // A prior boot may have saved this exact record but failed to erase it.
    if(readCrashSummary()==record) return true;
    Preferences nvs;
    if(!nvs.begin("voltra-debug",false)) return false;
    const bool written=nvs.putBytes(crashSummaryKey,record.data(),record.size())==record.size();
    nvs.end();
    return written && readCrashSummary()==record;
}

std::string describeCrashSummary(const CrashSummaryRecord& record) {
    if(record[0]!=crashSummarySchema || (record[1]&~(causePresent|pcPresent))) return {};
    std::string fingerprint;
    for(size_t i=fingerprintOffset;i<causeOffset;++i) {
        const auto c=record[i];
        if(!((c>='0' && c<='9') || (c>='a' && c<='f'))) return {};
        fingerprint+=char(c);
    }
    auto result=" storedElf="+fingerprint;
    if(record[1]&causePresent) result+=" storedCause="+std::to_string(record[causeOffset]);
    const auto pc=littleWord(record.data()+pcOffset);
    if((record[1]&pcPresent) && esp_ptr_executable(reinterpret_cast<const void*>(pc))) {
        char formatted[11];std::snprintf(formatted,sizeof(formatted),"0x%08lx",static_cast<unsigned long>(pc));
        result+=" storedPc="+std::string(formatted);
    }
    return result;
}

std::string clearCrashStorage(const std::string& summary) {
    // IDF 4.4.7 erases only the COREDUMP partition and writes a blank sentinel.
    const bool cleared=esp_core_dump_image_erase()==ESP_OK;
    return (cleared ? " storedDump=cleared" : " storedDump=cleanup-failed")+summary;
}

std::string storedCrashContext() {
    const auto previous=describeCrashSummary(readCrashSummary());
    const auto* partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_COREDUMP,nullptr);
    if(!partition) return previous;
    // A successful SDK erase writes {UINT32_MAX, 0, 0, 0} after erasing the
    // partition. If power cut cleanup after erasing its first sector, the size
    // will also be blank but the marker will be incomplete. A verified stored
    // summary lets us finish that cleanup without writing settings again.
    std::array<uint8_t,16> header{};
    if(esp_partition_read(partition,0,header.data(),header.size())!=ESP_OK) return " storedDump=invalid"+previous;
    if(littleWord(header.data())==UINT32_MAX) {
        const bool finishedMarker=littleWord(header.data()+4)==0 && littleWord(header.data()+8)==0 && littleWord(header.data()+12)==0;
        return !finishedMarker && !previous.empty() ? clearCrashStorage(previous) : previous;
    }

    size_t address=0,size=0;
    const auto status=esp_core_dump_image_get(&address,&size);
    if(status!=ESP_OK || address!=partition->address ||
        size<dumpHeaderBytes+elfHeaderBytes+dumpCrcBytes || size>partition->size || esp_core_dump_image_check()!=ESP_OK)
        return " storedDump=invalid"+previous;

    const auto notes=readCrashNotes();
    if(notes.fingerprint.empty()) return " storedDump=unsupported"+previous;
    const auto record=encodeCrashSummary(notes);
    if(!saveCrashSummary(record)) return " storedDump=summary-save-failed"+describeCrashSummary(record);

    return clearCrashStorage(describeCrashSummary(record));
}
#endif
std::string bootContext() {
    const auto reason=esp_reset_reason();
    auto result=std::string("bootReset=")+resetName(reason)+"("+std::to_string(int(reason))+")";
    if(const auto* app=esp_ota_get_app_description())
        result+=" runningElf="+hex(app->app_elf_sha256,fingerprintCharacters/2);
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
    // A stored crash can predate this boot (for example across a power cycle).
    result+=storedCrashContext();
#endif
    return result;
}
std::string field(const std::string& body,const char* name) {
    auto* json=cJSON_Parse(body.c_str());if(!json) return {};
    const auto* value=cJSON_GetObjectItemCaseSensitive(json,name);
    const std::string result=cJSON_IsString(value) && value->valuestring ? value->valuestring : "";
    cJSON_Delete(json);return result;
}
std::string encodeEvent(const EventLog::Event& event) {
    return "{\"at\":"+std::to_string(event.at)+",\"message\":"+
        EventLog::quote(event.message.substr(0,EventLog::maximumMessageBytes))+"}";
}
}
void Esp32DebugReports::set(ReportPhase phase,const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);view_.phase=phase;view_.message=message;view_.id=id_.substr(0,8);
}
ReportStatus Esp32DebugReports::status() const {std::lock_guard<std::mutex> lock(mutex_);return view_;}
bool Esp32DebugReports::restorePending() {
    Preferences nvs;
    if(!nvs.begin("voltra-debug",true)) return false;
    const auto restore=[&](const char* key,std::string saved) {
        auto id=field(saved,"id");
        if(id.size()!=32) return;
        payload_=std::move(saved);id_=std::move(id);pendingKey_=key;
    };
    if(nvs.isKey(legacyReportKey)) {
        const auto saved=nvs.getString(legacyReportKey);
        if(saved.length()>0 && saved.length()<=maximumReportBytes) restore(legacyReportKey,saved.c_str());
    }
    if(!pendingKey_ && nvs.isKey(pendingReportKey)) {
        const size_t size=nvs.getBytesLength(pendingReportKey);
        if(size>0 && size<=maximumReportBytes) {
            std::string saved(size,'\0');
            if(nvs.getBytes(pendingReportKey,saved.data(),size)==size) restore(pendingReportKey,std::move(saved));
        }
    }
    nvs.end();
    return pendingKey_!=nullptr;
}
void Esp32DebugReports::begin() {
    bootContext_=bootContext();
    if(restorePending()) set(ReportPhase::Queued,"Saved report queued for Wi-Fi.");
    diagnostic("Firmware " VOLTRA_VERSION " started");
}
bool Esp32DebugReports::submit(const std::vector<EventLog::Event>& events,const std::string& context,
                              const std::vector<EventLog::Event>& bleTrace) {
    if(active_) return false;
    if(!payload_.empty()) {set(ReportPhase::Queued,"Previous report saved. Waiting for Wi-Fi.");return true;}
    uint8_t random[16];esp_fill_random(random,sizeof(random));id_=hex(random,sizeof(random));
    const auto reportContext=bootContext_+" "+context.substr(0,maximumContextBytes-bootContext_.size()-1);
    payload_="{\"schema\":1,\"id\":"+EventLog::quote(id_)+",\"firmware\":\"" VOLTRA_VERSION "\"";
    const auto contextField=",\"context\":"+EventLog::quote(reportContext);
    const auto traceCount=std::min(bleTrace.size(),maximumTraceEvents);
    // Put an explicitly requested trace first so the bounded inbox preview
    // exposes it. Ordinary reports retain their original JSON field order.
    if(!traceCount) payload_+=contextField;
    payload_+=",\"events\":[";
    for(size_t i=0;i<traceCount;++i) {
        if(i) payload_+=',';
        payload_+=encodeEvent(bleTrace[i]);
    }
    const auto tail=traceCount ? "]"+contextField+"}" : std::string("]}");
    const auto eventLimit=EventLog::capacity-traceCount;
    const auto start=events.size()>eventLimit ? events.size()-eventLimit : 0;
    // Select newest first using the encoded length, including separators and
    // the closing brackets. Then serialize in chronological order.
    std::vector<std::string> encoded;
    encoded.reserve(events.size()-start);
    size_t reportSize=payload_.size()+tail.size();
    for(size_t i=events.size();i>start;--i) {
        auto item=encodeEvent(events[i-1]);
        const size_t size=item.size()+size_t(traceCount || !encoded.empty());
        if(size>maximumReportBytes-reportSize) break;
        reportSize+=size;
        encoded.push_back(std::move(item));
    }
    payload_.reserve(reportSize);
    for(auto item=encoded.rbegin();item!=encoded.rend();++item) {
        if(traceCount || item!=encoded.rbegin()) payload_+=',';
        payload_+=*item;
    }
    payload_+=tail;
    Preferences nvs;bool saved=false;
    if(payload_.size()<=maximumReportBytes && nvs.begin("voltra-debug",false)) {
        saved=nvs.putBytes(pendingReportKey,payload_.data(),payload_.size())==payload_.size();nvs.end();
    }
    if(!saved) {payload_.clear();set(ReportPhase::Error,"Could not save report.\nBluetooth kept connected. Tap RETRY.");return false;}
    pendingKey_=pendingReportKey;
    set(ReportPhase::Queued,"Report saved. Waiting for Wi-Fi.");return true;
}
void Esp32DebugReports::tick(bool networkReady,bool controlIdle) {
    permitted_=networkReady && controlIdle;
    if(active_ || status().phase!=ReportPhase::Queued) return;
    if(!networkReady || !controlIdle) return;
    active_=true;set(ReportPhase::Sending,"Sending report...");
    if(xTaskCreate([](void* arg) {
        auto* self=static_cast<Esp32DebugReports*>(arg);self->run();self->active_=false;vTaskDelete(nullptr);
    },"debug-report",workerStackBytes,this,1,nullptr)!=pdPASS) {
        active_=false;set(ReportPhase::Error,"Could not start upload.\nReport saved. Tap RETRY.");
    }
}
bool Esp32DebugReports::post(const char* path,const std::string& body,const std::string& token,std::string& response) {
    if(!permitted_) return false;
    const std::string url=std::string("https://")+host+path;
    esp_http_client_config_t config{};config.url=url.c_str();config.method=HTTP_METHOD_POST;
    config.timeout_ms=socketTimeoutMs;config.crt_bundle_attach=esp_crt_bundle_attach;
    config.disable_auto_redirect=true;config.buffer_size=2048;config.buffer_size_tx=2048;
    auto client=esp_http_client_init(&config);if(!client) return false;
    esp_http_client_set_header(client,"Content-Type","application/json");
    const std::string authorization="Bearer "+token;
    if(!token.empty()) esp_http_client_set_header(client,"Authorization",authorization.c_str());
    bool ok=esp_http_client_open(client,int(body.size()))==ESP_OK;
    size_t sent=0;const uint32_t writeStarted=millis();
    while(ok && permitted_ && sent<body.size() && uint32_t(millis()-writeStarted)<socketTimeoutMs) {
        const int n=esp_http_client_write(client,body.data()+sent,int(body.size()-sent));
        if(n<=0) ok=false;else sent+=size_t(n);
    }
    if(ok && permitted_ && sent==body.size()) {
        ok=esp_http_client_fetch_headers(client)>=0;
        const int status=esp_http_client_get_status_code(client);
        ok=ok && (status==200 || status==201);
        char buffer[256];const uint32_t start=millis();
        while(ok && permitted_ && !esp_http_client_is_complete_data_received(client) && uint32_t(millis()-start)<socketTimeoutMs) {
            const int n=esp_http_client_read(client,buffer,sizeof(buffer));
            if(n<0) {ok=false;break;}
            if(response.size()+size_t(n)>maximumResponseBytes) {ok=false;break;}
            response.append(buffer,size_t(n));if(n==0) vTaskDelay(pdMS_TO_TICKS(10));
        }
        ok=ok && esp_http_client_is_complete_data_received(client);
    } else ok=false;
    esp_http_client_cleanup(client);return ok && permitted_;
}
void Esp32DebugReports::run() {
    const auto fail=[&] {set(ReportPhase::Error,"Upload failed; report saved.\nCheck Wi-Fi, then tap RETRY.");};
    if(std::time(nullptr)<earliestValidTime) {
        configTime(0,0,"time.cloudflare.com","pool.ntp.org");const uint32_t start=millis();
        while(permitted_ && std::time(nullptr)<earliestValidTime && uint32_t(millis()-start)<clockWaitMs) vTaskDelay(pdMS_TO_TICKS(100));
    }
    IPAddress address;
    if(!permitted_ || std::time(nullptr)<earliestValidTime || !WiFi.hostByName(host,address)) {fail();return;}
    uint8_t hash[32];mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(payload_.data()),payload_.size(),hash,0);
    std::string response;
    if(!post("/api/tickets","{\"id\":"+EventLog::quote(id_)+",\"hash\":"+EventLog::quote(hex(hash,sizeof(hash)))+"}","",response)) {fail();return;}
    const auto token=field(response,"token");response.clear();
    if(token.size()!=64 || !post("/api/reports",payload_,token,response) || field(response,"id")!=id_) {fail();return;}
    Preferences nvs;
    if(!nvs.begin("voltra-debug",false)) {fail();return;}
    const bool cleared=pendingKey_ && nvs.remove(pendingKey_);nvs.end();
    if(!cleared) {set(ReportPhase::Error,"Report delivered; saved copy could not be cleared. Retry is safe.");return;}
    payload_.clear();pendingKey_=nullptr;
    // A rollback can leave both a legacy report and a new blob. Deliver both
    // approved snapshots before accepting another SEND.
    if(restorePending()) set(ReportPhase::Queued,"Another saved report is queued for Wi-Fi.");
    else set(ReportPhase::Sent,"Report received. Quote this code when describing the bug.");
    diagnostic("Debug report delivered");
}
}
