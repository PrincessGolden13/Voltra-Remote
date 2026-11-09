#include "Esp32DebugReports.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>
#include <ctime>
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);
namespace voltra {
namespace {
constexpr size_t maximumReportBytes=12288,maximumResponseBytes=1024;
constexpr int socketTimeoutMs=10000;
constexpr uint32_t clockWaitMs=10000,workerStackBytes=12288;
constexpr time_t earliestValidTime=1704067200; // 2024-01-01 UTC, certificate clock sanity floor.
constexpr char host[]="voltra-remote-diagnostics.darkheath.chatgpt.site";
std::string hex(const uint8_t* bytes,size_t size) {
    const char* chars="0123456789abcdef";std::string result;
    for(size_t i=0;i<size;++i) {result+=chars[bytes[i]>>4];result+=chars[bytes[i]&15];}return result;
}
std::string field(const std::string& body,const char* name) {
    auto* json=cJSON_Parse(body.c_str());if(!json) return {};
    const auto* value=cJSON_GetObjectItemCaseSensitive(json,name);
    const std::string result=cJSON_IsString(value) && value->valuestring ? value->valuestring : "";
    cJSON_Delete(json);return result;
}
}
void Esp32DebugReports::set(ReportPhase phase,const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);view_.phase=phase;view_.message=message;view_.id=id_.substr(0,8);
}
ReportStatus Esp32DebugReports::status() const {std::lock_guard<std::mutex> lock(mutex_);return view_;}
void Esp32DebugReports::begin() {
    Preferences nvs;
    if(nvs.begin("voltra-debug",true)) {
        const size_t size=nvs.getString("pending").length();
        if(size>0 && size<=maximumReportBytes) payload_=nvs.getString("pending").c_str();
        nvs.end();
    }
    id_=field(payload_,"id");
    if(id_.size()==32) set(ReportPhase::Queued,"Saved report queued for Wi-Fi.");
    else {payload_.clear();id_.clear();}
    diagnostic("Firmware " VOLTRA_VERSION " started");
}
bool Esp32DebugReports::submit(const std::vector<EventLog::Event>& events,const std::string& context) {
    if(active_) return false;
    if(!payload_.empty()) {set(ReportPhase::Queued,"Previous report queued. Disconnect from VOLTRA to send over Wi-Fi.");return true;}
    uint8_t random[16];esp_fill_random(random,sizeof(random));id_=hex(random,sizeof(random));
    payload_="{\"schema\":1,\"id\":"+EventLog::quote(id_)+",\"firmware\":\"" VOLTRA_VERSION "\",\"context\":"+EventLog::quote(context.substr(0,2048))+",\"events\":[";
    const size_t start=events.size()>EventLog::capacity ? events.size()-EventLog::capacity : 0;
    for(size_t i=start;i<events.size();++i) {
        if(i!=start) payload_+=',';
        payload_+="{\"at\":"+std::to_string(events[i].at)+",\"message\":"+EventLog::quote(events[i].message.substr(0,EventLog::maximumMessageBytes))+"}";
    }
    payload_+="]}";
    Preferences nvs;bool saved=false;
    if(payload_.size()<=maximumReportBytes && nvs.begin("voltra-debug",false)) {
        saved=nvs.putString("pending",payload_.c_str())==payload_.size();nvs.end();
    }
    if(!saved) {payload_.clear();set(ReportPhase::Error,"Could not save report. Hold 6s to retry.");return false;}
    set(ReportPhase::Queued,"Report saved. Disconnect from VOLTRA to send over Wi-Fi.");return true;
}
void Esp32DebugReports::tick(bool networkReady,bool controlIdle) {
    permitted_=networkReady && controlIdle;
    if(active_ || status().phase!=ReportPhase::Queued) return;
    if(!networkReady || !controlIdle) return;
    active_=true;set(ReportPhase::Sending,"Sending report...");
    if(xTaskCreate([](void* arg) {
        auto* self=static_cast<Esp32DebugReports*>(arg);self->run();self->active_=false;vTaskDelete(nullptr);
    },"debug-report",workerStackBytes,this,1,nullptr)!=pdPASS) {
        active_=false;set(ReportPhase::Error,"Could not start upload. Hold 6s to retry.");
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
    const auto fail=[&] {set(ReportPhase::Error,"Upload failed; report retained. Check Wi-Fi, then hold 6s and choose Yes to retry.");};
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
    const bool cleared=nvs.remove("pending");nvs.end();
    if(!cleared) {set(ReportPhase::Error,"Report delivered; saved copy could not be cleared. Retry is safe.");return;}
    payload_.clear();set(ReportPhase::Sent,"Report received. Quote this code when describing the bug.");
    diagnostic("Debug report delivered");
}
}
