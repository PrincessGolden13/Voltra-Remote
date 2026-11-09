#include "Esp32FirmwareUpdate.h"
#include "hardware/boards/guition_k5/FirmwareSlots.h"
#include "hardware/boards/guition_k5/FirmwareIdentity.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <memory>

// Arduino 2.0.17 shadows IDF's esp_crt_bundle.h with a different, empty-bundle
// wrapper. Use the IDF 4.4 API from libmbedtls and its embedded full CA bundle.
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);

namespace voltra {
namespace {
// Units/policy: see docs/FIRMWARE_UPDATES.md. No credentials in this feed.
constexpr uint64_t startupGraceMs=25000, restartNoticeMs=2000;
constexpr uint32_t clockWaitMs=10000, requestTimeoutMs=30000, downloadTimeoutMs=180000;
constexpr size_t maximumManifestBytes=2048, transferBlockBytes=4096, requestBufferBytes=8192;
constexpr int maximumRedirects=5;
const std::string repositoryBase="https://github.com/" VOLTRA_UPDATE_REPOSITORY;
struct Lock {
    SemaphoreHandle_t mutex;
    explicit Lock(SemaphoreHandle_t m):mutex(m) { xSemaphoreTake(mutex,portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(mutex); }
};
bool allowedUrl(const std::string& url) {
    for(const char* host : {"https://github.com/", "https://release-assets.githubusercontent.com/", "https://objects.githubusercontent.com/"})
        if(url.rfind(host,0)==0) return true;
    return false;
}
bool parseVersion(const std::string& text,std::array<unsigned,3>& result) {
    if(text.empty() || text.size()>26) return false;
    size_t pos=0;
    for(int i=0;i<3;++i) {
        const size_t start=pos;unsigned n=0;
        while(pos<text.size() && text[pos]>='0' && text[pos]<='9') {
            n=n*10+unsigned(text[pos++]-'0');if(n>999999) return false;
        }
        if(pos==start || (pos-start>1 && text[start]=='0')) return false;
        result[i]=n;
        if(i<2 && (pos==text.size() || text[pos++]!='.')) return false;
    }
    return pos==text.size();
}
// Explicit redirects keep every hop on HTTPS and GitHub's download hosts.
// The system CA bundle validates certificates and hostname; never setInsecure.
struct Download {
    esp_http_client_handle_t client=nullptr;
    std::string location;
    ~Download() { close(); }
    void close() { if(client) {esp_http_client_cleanup(client);client=nullptr;} }
    static esp_err_t event(esp_http_client_event_t* e) {
        auto* self=static_cast<Download*>(e->user_data);
        if(e->event_id==HTTP_EVENT_DISCONNECTED && e->data) {
            int tls=0,flags=0;const auto error=esp_tls_get_and_clear_last_error(static_cast<esp_tls_error_handle_t>(e->data),&tls,&flags);
            if(error) Serial.printf("Update TLS: error=%x code=%x flags=%x\n",unsigned(error),unsigned(tls),unsigned(flags));
        }
        if(e->event_id==HTTP_EVENT_ON_HEADER && e->header_key && e->header_value && strcasecmp(e->header_key,"Location")==0)
            self->location=e->header_value;
        return ESP_OK;
    }
    bool open(std::string url,const std::atomic<bool>& permitted) {
        for(int hop=0;hop<=maximumRedirects;++hop) {
            if(!permitted || !allowedUrl(url)) return false;
            close();location.clear();
            // Resolve separately from the TLS connection window. Arduino's
            // bounded IPv4 lookup populates the same lwIP cache used by ESP-TLS.
            const auto hostEnd=url.find('/',8);const auto host=url.substr(8,hostEnd-8);
            IPAddress resolved;
            if(!WiFi.hostByName(host.c_str(),resolved) || !permitted) return false;
            esp_http_client_config_t config{};
            config.url=url.c_str();config.crt_bundle_attach=esp_crt_bundle_attach;
            config.timeout_ms=requestTimeoutMs;config.disable_auto_redirect=true;
            config.user_agent="Voltra-Remote/" VOLTRA_VERSION;
            config.event_handler=event;config.user_data=this;config.buffer_size=2048;
            // GitHub's signed asset request target is longer than IDF's 512-byte
            // default. This fits our 4096-byte URL cap plus ordinary headers.
            config.buffer_size_tx=requestBufferBytes;
            client=esp_http_client_init(&config);
            if(!client) return false;
            const auto opened=esp_http_client_open(client,0);
            if(opened!=ESP_OK) {
                Serial.printf("Update HTTPS open: %s errno=%d heap=%u largest=%u\n",esp_err_to_name(opened),esp_http_client_get_errno(client),ESP.getFreeHeap(),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
                return false;
            }
            if(esp_http_client_fetch_headers(client)<0) {Serial.println("Update HTTPS: response headers unavailable");return false;}
            const int code=esp_http_client_get_status_code(client);
            Serial.printf("Update HTTPS: hop=%d status=%d\n",hop,code);
            if(code==200) return true;
            if((code!=301 && code!=302 && code!=303 && code!=307 && code!=308) || location.empty() || location.size()>4096) return false;
            if(location.front()=='/' && (location.size()==1 || location[1]!='/')) {
                const auto slash=url.find('/',8);url=url.substr(0,slash)+location;
            } else url=location;
        }
        return false;
    }
};
std::string jsonString(cJSON* object,const char* key) {
    const auto* value=cJSON_GetObjectItemCaseSensitive(object,key);
    return cJSON_IsString(value) && value->valuestring ? value->valuestring : "";
}
bool jsonNumber(cJSON* object,const char* key,double expected) {
    const auto* value=cJSON_GetObjectItemCaseSensitive(object,key);
    return cJSON_IsNumber(value) && value->valuedouble==expected;
}
}
void Esp32FirmwareUpdate::begin() {
    mutex_=xSemaphoreCreateMutex();
    if(mutex_) {Lock lock(mutex_);view_.message="Checks at power-on when Wi-Fi is available";}
}
UpdateStatus Esp32FirmwareUpdate::status() const {
    if(!mutex_) return {UpdatePhase::Error,"","Update service unavailable",0};
    Lock lock(mutex_);return view_;
}
void Esp32FirmwareUpdate::set(UpdatePhase phase,const std::string& message,int percent) {
    bool changed=false;
    {Lock lock(mutex_);changed=view_.phase!=phase || view_.message!=message || view_.percent/10!=percent/10;
        view_.phase=phase;view_.message=message;view_.percent=percent;}
    if(changed) Serial.printf("Firmware update: %s (%d%%)\n",message.c_str(),percent);
}
void Esp32FirmwareUpdate::tick(uint64_t now,bool networkReady,bool controlIdle) {
    if(!mutex_) {startupPending_=false;return;}
    if(!startedAt_) startedAt_=now;
    networkReady_=networkReady;controlIdle_=controlIdle;
    permitted_=networkReady && controlIdle;
    if(now-startedAt_>=startupGraceMs) startupPending_=false;
    if(!bootCheckDone_ && permitted_ && !workerActive_) {bootCheckDone_=true;launch(false);}
    const auto s=status();
    if(!workerActive_ && s.phase!=UpdatePhase::Idle && s.phase!=UpdatePhase::Checking) startupPending_=false;
    if(s.phase==UpdatePhase::Ready && !workerActive_) {
        if(!controlIdle) {set(UpdatePhase::Error,"Restart cancelled: VOLTRA in use");return;}
        if(!readyAt_) readyAt_=now;
        if(now-readyAt_>=restartNoticeMs) ESP.restart();
    }
}
void Esp32FirmwareUpdate::check() {
    if(!mutex_ || workerActive_ || status().phase==UpdatePhase::Ready) return;
    bootCheckDone_=true;startupPending_=false;
    if(!controlIdle_) {set(UpdatePhase::Error,"Return to Devices before updating");return;}
    if(!networkReady_) {set(UpdatePhase::Error,"Connect Wi-Fi in Settings first");return;}
    launch(false);
}
bool Esp32FirmwareUpdate::install() {
    if(!mutex_ || workerActive_ || status().phase!=UpdatePhase::Available) return false;
    if(!controlIdle_ || !networkReady_) {set(UpdatePhase::Error,"Connect Wi-Fi and leave VOLTRA first");return false;}
    if(!guition::FirmwareSlots::updateTarget()) {set(UpdatePhase::Error,"USB installation required for this layout");return false;}
    launch(true);return workerActive_;
}
void Esp32FirmwareUpdate::launch(bool installing) {
    workerActive_=true;readyAt_=0;
    set(installing ? UpdatePhase::Installing : UpdatePhase::Checking, installing ? "Downloading... Keep power connected" : "Checking GitHub...");
    struct Job {Esp32FirmwareUpdate* self;bool installing;};
    auto* job=new(std::nothrow) Job{this,installing};
    if(!job || xTaskCreate([](void* arg) {
        std::unique_ptr<Job> j(static_cast<Job*>(arg));auto* self=j->self;
        self->run(j->installing);j.reset();self->workerActive_=false;vTaskDelete(nullptr);
    },"firmware-update",12288,job,1,nullptr)!=pdPASS) {
        delete job;workerActive_=false;set(UpdatePhase::Error,"Not enough memory to check updates");
    }
}
void Esp32FirmwareUpdate::run(bool installing) {
    Serial.printf("Update network: signal=%d dBm\n",WiFi.RSSI());
    // SNTP is needed for TLS certificate validity. Failing to obtain time never
    // falls back to unauthenticated HTTP or an insecure TLS connection.
    constexpr time_t earliestValidTime=1735689600; // 2025-01-01 UTC
    if(std::time(nullptr)<earliestValidTime) {
        configTime(0,0,"time.cloudflare.com","pool.ntp.org");
        const uint32_t start=millis();
        while(permitted_ && std::time(nullptr)<earliestValidTime && uint32_t(millis()-start)<clockWaitMs) vTaskDelay(pdMS_TO_TICKS(100));
    }
    if(!permitted_) {set(UpdatePhase::Error,"Check paused during VOLTRA use");return;}
    if(std::time(nullptr)<earliestValidTime) {set(UpdatePhase::Error,"Could not set clock. Try again on Wi-Fi");return;}
    if(installing) download();else fetchRelease();
}
bool Esp32FirmwareUpdate::fetchRelease() {
    Download http;
    if(!http.open(repositoryBase+"/releases/latest/download/guition-k5.json",permitted_)) {
        set(UpdatePhase::Error,"Cannot reach a release. Try again later");return false;
    }
    std::string body;char buffer[512];const uint32_t start=millis();
    while(permitted_ && uint32_t(millis()-start)<requestTimeoutMs) {
        const int n=esp_http_client_read(http.client,buffer,sizeof(buffer));
        if(n<0) break;
        if(n==0) {
            if(esp_http_client_is_complete_data_received(http.client)) break;
            vTaskDelay(pdMS_TO_TICKS(10));continue;
        }
        body.append(buffer,n);if(body.size()>maximumManifestBytes) break;
    }
    if(!permitted_ || body.size()>maximumManifestBytes || !esp_http_client_is_complete_data_received(http.client)) {
        set(UpdatePhase::Error,"Incomplete update information. Try again");return false;
    }
    std::unique_ptr<cJSON,decltype(&cJSON_Delete)> json(cJSON_ParseWithLengthOpts(body.c_str(),body.size()+1,nullptr,true),cJSON_Delete);
    if(!json) {set(UpdatePhase::Error,"Invalid update information");return false;}
    Release candidate;candidate.version=jsonString(json.get(),"version");candidate.sha256=jsonString(json.get(),"sha256");
    std::array<unsigned,3> latest{},current{};
    const auto* size=cJSON_GetObjectItemCaseSensitive(json.get(),"size");
    if(!jsonNumber(json.get(),"schema",1) || !jsonNumber(json.get(),"layout",guition::FirmwareSlots::layout) ||
       jsonString(json.get(),"board")!="guition-k5" || jsonString(json.get(),"channel")!="normal" ||
       !parseVersion(candidate.version,latest) || !parseVersion(VOLTRA_VERSION,current) ||
       !cJSON_IsNumber(size) || size->valuedouble<288 || size->valuedouble>guition::FirmwareSlots::normalSize || size->valuedouble!=size->valueint ||
       candidate.sha256.size()!=64 || candidate.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos) {
        set(UpdatePhase::Error,"Release is not compatible with this remote");return false;
    }
    candidate.size=uint32_t(size->valueint);
    {Lock lock(mutex_);release_=candidate;view_.version=candidate.version;}
    if(latest<=current) set(UpdatePhase::Current,"You have the latest firmware");
    else set(UpdatePhase::Available,"Update available. Install and restart?");
    return true;
}
bool Esp32FirmwareUpdate::download() {
    Release release;{Lock lock(mutex_);release=release_;}
    const auto* target=guition::FirmwareSlots::updateTarget();
    if(!target || release.size>target->size) {set(UpdatePhase::Error,"USB installation required for this layout");return false;}
    Download http;
    if(!http.open(repositoryBase+"/releases/download/v"+release.version+"/voltra-remote-guition-k5.bin",permitted_) ||
       esp_http_client_get_content_length(http.client)!=int64_t(release.size)) {
        set(UpdatePhase::Error,"Download unavailable or size mismatch");return false;
    }
    esp_ota_handle_t handle=0;
    if(!permitted_ || esp_ota_begin(target,OTA_WITH_SEQUENTIAL_WRITES,&handle)!=ESP_OK) {set(UpdatePhase::Error,"Could not start firmware installation");return false;}
    mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);mbedtls_sha256_starts_ret(&hash,0);
    std::array<uint8_t,transferBlockBytes> block{};uint32_t received=0;const uint32_t start=millis();bool ok=true;
    while(permitted_ && received<release.size && uint32_t(millis()-start)<downloadTimeoutMs) {
        const int n=esp_http_client_read(http.client,reinterpret_cast<char*>(block.data()),std::min<size_t>(block.size(),release.size-received));
        if(n<0) {ok=false;break;}
        if(n==0) {if(esp_http_client_is_complete_data_received(http.client)) break;vTaskDelay(pdMS_TO_TICKS(10));continue;}
        if(esp_ota_write(handle,block.data(),n)!=ESP_OK || mbedtls_sha256_update_ret(&hash,block.data(),n)!=0) {ok=false;break;}
        received+=n;set(UpdatePhase::Installing,"Installing... Keep power connected",int(uint64_t(received)*100/release.size));
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    std::array<unsigned char,32> digest{};ok=mbedtls_sha256_finish_ret(&hash,digest.data())==0 && ok;mbedtls_sha256_free(&hash);
    std::string hex;constexpr char digits[]="0123456789abcdef";
    for(auto byte:digest) {hex+=digits[byte>>4];hex+=digits[byte&15];}
    if(!ok || !permitted_ || received!=release.size || hex!=release.sha256 || !esp_http_client_is_complete_data_received(http.client)) {
        esp_ota_abort(handle);set(UpdatePhase::Error,"Update failed. Current firmware kept");return false;
    }
    if(esp_ota_end(handle)!=ESP_OK) {set(UpdatePhase::Error,"Firmware validation failed. Current kept");return false;}
    if(!permitted_ || !guition::normalIdentity(target,release.version.c_str()) ||
       esp_ota_set_boot_partition(target)!=ESP_OK) {
        set(UpdatePhase::Error,"Firmware activation failed. Current kept");return false;
    }
    set(UpdatePhase::Ready,"Update installed. Restarting...",100);return true;
}
}
