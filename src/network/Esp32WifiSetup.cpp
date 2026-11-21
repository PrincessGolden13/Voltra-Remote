#ifdef VOLTRA_PHYSICAL
#include "Esp32WifiSetup.h"
#include "WifiPortalPage.h"
#include <WiFi.h>
#include <Preferences.h>
#include <nvs.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cctype>
#include <cstdio>
#include <atomic>
#include <new>
#include <lwip/sockets.h>
namespace voltra {
namespace {
constexpr uint64_t setupTimeoutMs=300000,connectTimeoutMs=20000,successCloseMs=15000;
constexpr uint64_t scanTimeoutMs=15000; // Bound an async scan before accepting credentials.
// User-requested Google DNS address; numeric ICMP does not depend on DNS/TLS.
constexpr char connectionCheckAddress[]="8.8.8.8";
constexpr uint32_t connectionCheckCount=3,connectionCheckTimeoutMs=1000,connectionCheckIntervalMs=1000;
constexpr uint64_t connectionCheckDeadlineMs=6000,connectionCheckWifiWaitMs=25000;
// Same fixed IPv4 address/subnet advertised by the setup AP and captive DNS.
constexpr uint32_t setupAddress=0xc0a80401u,setupNetmask=0xffffff00u;
uint64_t nowMs(){return uint64_t(esp_timer_get_time())/1000;}
bool socketIpv4(const sockaddr_storage& address,socklen_t length,uint32_t& result) {
    if(address.ss_family==AF_INET && length>=sizeof(sockaddr_in)) {
        result=ntohl(reinterpret_cast<const sockaddr_in*>(&address)->sin_addr.s_addr);return true;
    }
#if LWIP_IPV6
    if(address.ss_family==AF_INET6 && length>=sizeof(sockaddr_in6)) {
        // ESP-IDF 4.4's HTTP listener uses PF_INET6 when IPv6 is enabled.
        // IPv4 clients then appear as ::ffff:a.b.c.d, not sockaddr_in.
        const auto* bytes=reinterpret_cast<const uint8_t*>(&reinterpret_cast<const sockaddr_in6*>(&address)->sin6_addr);
        constexpr uint8_t mappedPrefix[12]={0,0,0,0,0,0,0,0,0,0,0xff,0xff};
        if(std::memcmp(bytes,mappedPrefix,sizeof(mappedPrefix))==0) {
            uint32_t value;std::memcpy(&value,bytes+12,sizeof(value));result=ntohl(value);return true;
        }
    }
#endif
    return false;
}
bool setupClient(httpd_req_t* req) {
    sockaddr_storage peer{},local{};socklen_t peerSize=sizeof(peer),localSize=sizeof(local);
    uint32_t peerIp=0,localIp=0;const int socket=httpd_req_to_sockfd(req);
    const bool allowed=getpeername(socket,reinterpret_cast<sockaddr*>(&peer),&peerSize)==0 &&
        getsockname(socket,reinterpret_cast<sockaddr*>(&local),&localSize)==0 &&
        socketIpv4(peer,peerSize,peerIp) && socketIpv4(local,localSize,localIp) &&
        localIp==setupAddress && (peerIp&setupNetmask)==(setupAddress&setupNetmask);
    if(!allowed) std::printf("Wi-Fi portal: client rejected (peer family %d, local family %d)\n",int(peer.ss_family),int(local.ss_family));
    return allowed;
}
struct Lock { SemaphoreHandle_t m; explicit Lock(SemaphoreHandle_t mutex):m(mutex){xSemaphoreTake(m,portMAX_DELAY);} ~Lock(){xSemaphoreGive(m);} };
std::string randomText(size_t count) {
    constexpr char alphabet[]="ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::string text;for(size_t i=0;i<count;++i) text+=alphabet[esp_random()%32];return text;
}
std::string json(const std::string& s) {
    std::string out="\"";for(unsigned char c:s) {
        if(c=='\"' || c=='\\') {out+='\\';out+=char(c);}
        else if(c<32) out+=' '; else out+=char(c);
    }return out+'\"';
}
esp_err_t reply(httpd_req_t* req,const std::string& body,const char* type="application/json",const char* status="200 OK") {
    httpd_resp_set_status(req,status);httpd_resp_set_type(req,type);
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
    httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'");
    return httpd_resp_send(req,body.data(),body.size());
}
esp_err_t wrongNetwork(httpd_req_t* req,bool html=false) {
    return html ? reply(req,
        "<!doctype html><html><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>VOLTRA Remote Wi-Fi</title><h1>Join the remote's setup Wi-Fi</h1>"
        "<p>Connect to the VOLTRA-Setup network shown on your remote, then open "
        "<a href='http://192.168.4.1/'>192.168.4.1</a>.</p></html>","text/html; charset=utf-8","403 Forbidden") :
        reply(req,"{\"message\":\"Join the remote's VOLTRA-Setup Wi-Fi and reopen 192.168.4.1.\"}","application/json","403 Forbidden");
}
bool decode(const std::string& in,std::string& out) {
    auto hex=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
    out.clear();for(size_t i=0;i<in.size();++i) {
        unsigned char c=in[i];if(c=='+') c=' ';
        else if(c=='%') {if(i+2>=in.size() || hex(in[i+1])<0 || hex(in[i+2])<0)return false;c=hex(in[i+1])*16+hex(in[i+2]);i+=2;}
        if(c<32 || c==127) return false;out+=char(c);
    }return true;
}
}
struct Esp32WifiSetup::PingCheck {
    Esp32WifiSetup* owner;
    uint32_t generation;
    esp_ping_handle_t session=nullptr;
    std::atomic<bool> done{false};
    uint64_t startedAt=0;
    int replies=0,lastMs=-1;
};
void Esp32WifiSetup::requestConnectionCheck() {
    if(!mutex_) return;
    const auto w=status();
    if(w.setupActive && w.setupComplete) stop();
    invalidateConnectionCheck(WifiConnectionCheck::Waiting,true);
    Lock lock(mutex_);
    pingRequestedAt_=nowMs();
    if(!view_.storageReady || !view_.saved || !radioAvailable_ || view_.setupActive) {
        pingRequested_=false;view_.connectionCheck=WifiConnectionCheck::Unavailable;
    }
}
void Esp32WifiSetup::cancelConnectionCheck() {
    if(mutex_) invalidateConnectionCheck(WifiConnectionCheck::Idle);
}
void Esp32WifiSetup::invalidateConnectionCheck(WifiConnectionCheck phase,bool keepRequest) {
    // stop() is asynchronous: retain the callback context until on_ping_end.
    // The generation prevents an old callback publishing into a newer check.
    {Lock lock(mutex_);
        ++pingGeneration_;pingRequested_=keepRequest;
        view_.connectionCheck=phase;view_.pingReplies=0;view_.pingMs=-1;
    }
    if(ping_ && !ping_->done.load(std::memory_order_acquire)) esp_ping_stop(ping_->session);
}
void Esp32WifiSetup::pingSuccess(esp_ping_handle_t session,void* arg) {
    auto* check=static_cast<PingCheck*>(arg);
    uint32_t ms=0;ip_addr_t reply{},target{};
    if(esp_ping_get_profile(session,ESP_PING_PROF_TIMEGAP,&ms,sizeof(ms))!=ESP_OK ||
       esp_ping_get_profile(session,ESP_PING_PROF_IPADDR,&reply,sizeof(reply))!=ESP_OK ||
       !ipaddr_aton(connectionCheckAddress,&target) || !ip_addr_cmp(&reply,&target)) return;
    ++check->replies;check->lastMs=int(ms);
    auto* self=check->owner;Lock lock(self->mutex_);
    if(check->generation!=self->pingGeneration_ || !self->pingRequested_) return;
    self->view_.pingReplies=check->replies;self->view_.pingMs=check->lastMs;
}
void Esp32WifiSetup::pingFinished(esp_ping_handle_t,void* arg) {
    auto* check=static_cast<PingCheck*>(arg);auto* self=check->owner;
    {Lock lock(self->mutex_);
        if(check->generation==self->pingGeneration_ && self->pingRequested_) {
            self->pingRequested_=false;
            self->view_.connectionCheck=check->replies ? WifiConnectionCheck::Reachable : WifiConnectionCheck::NoReply;
            self->view_.pingReplies=check->replies;self->view_.pingMs=check->lastMs;
        }
    }
    // Last access to context by the callback; main-loop cleanup acquires this.
    check->done.store(true,std::memory_order_release);
}
void Esp32WifiSetup::tickConnectionCheck() {
    if(ping_ && ping_->done.load(std::memory_order_acquire)) {
        // IDF queues deletion by clearing INIT; no callbacks follow this end.
        // Retain the context if deletion was not accepted, and retry next tick.
        if(esp_ping_delete_session(ping_->session)!=ESP_OK) return;
        delete ping_;ping_=nullptr;
    }
    const auto w=status();
    if(w.connectionCheck==WifiConnectionCheck::Checking && ping_ && nowMs()-ping_->startedAt>=connectionCheckDeadlineMs) {
        invalidateConnectionCheck(WifiConnectionCheck::Error);return;
    }
    bool requested;
    {Lock lock(mutex_);requested=pingRequested_;}
    if(!requested) return;
    if(!w.storageReady || !radioAvailable_ || w.setupActive || !w.saved) {
        invalidateConnectionCheck(WifiConnectionCheck::Unavailable);return;
    }
    if(!w.connected || WiFi.status()!=WL_CONNECTED) {
        if(nowMs()-pingRequestedAt_>=connectionCheckWifiWaitMs) invalidateConnectionCheck(WifiConnectionCheck::Unavailable);
        return;
    }
    if(ping_) return; // A cancelled session is still retiring; never reuse it.
    auto* check=new(std::nothrow) PingCheck;
    if(!check) {invalidateConnectionCheck(WifiConnectionCheck::Error);return;}
    check->owner=this;
    {Lock lock(mutex_);check->generation=pingGeneration_;view_.connectionCheck=WifiConnectionCheck::Checking;}
    esp_ping_config_t config=ESP_PING_DEFAULT_CONFIG();
    config.count=connectionCheckCount;config.timeout_ms=connectionCheckTimeoutMs;
    config.interval_ms=connectionCheckIntervalMs;config.data_size=16;
    ipaddr_aton(connectionCheckAddress,&config.target_addr);
    esp_ping_callbacks_t callbacks{};callbacks.cb_args=check;
    callbacks.on_ping_success=pingSuccess;callbacks.on_ping_end=pingFinished;
    if(esp_ping_new_session(&config,&callbacks,&check->session)!=ESP_OK) {
        delete check;invalidateConnectionCheck(WifiConnectionCheck::Error);return;
    }
    ping_=check;check->startedAt=nowMs();
    if(esp_ping_start(check->session)!=ESP_OK) {
        // A rejected start has no end callback; retire through the same checked
        // delete path on the next tick rather than discarding its context here.
        check->done.store(true,std::memory_order_release);
        invalidateConnectionCheck(WifiConnectionCheck::Error);
    }
}
void Esp32WifiSetup::begin() {
    mutex_=xSemaphoreCreateMutex();if(!mutex_) return;
    // Distinguish a fresh remote from inaccessible or incompatible saved data.
    nvs_handle_t handle;const auto opened=nvs_open("voltra-net",NVS_READONLY,&handle);
    if(opened==ESP_OK) {
        size_t size=0;const auto found=nvs_get_blob(handle,"credentials",nullptr,&size);
        nvs_close(handle);
        view_.storageReady=found==ESP_ERR_NVS_NOT_FOUND || (found==ESP_OK && size==sizeof(saved_) && readSaved(saved_));
    } else view_.storageReady=opened==ESP_ERR_NVS_NOT_FOUND;
    if(!view_.storageReady) {
        saved_={};view_.message="Saved Wi-Fi unavailable; settings kept. Update blocked.";
    } else {
        view_.saved=saved_.ssid[0]!=0;view_.network=saved_.ssid;
        view_.message=view_.saved ? "Saved network" : "Wi-Fi not configured";
    }
    WiFi.persistent(false);WiFi.setAutoReconnect(false);
}
bool Esp32WifiSetup::sameCredentials(const Credentials& a,const Credentials& b) {
    return a.version==b.version && std::memcmp(a.ssid,b.ssid,sizeof(a.ssid))==0 &&
        std::memcmp(a.password,b.password,sizeof(a.password))==0;
}
bool Esp32WifiSetup::readSaved(Credentials& value) const {
    Preferences nvs;if(!nvs.begin("voltra-net",true)) return false;
    const bool read=nvs.getType("credentials")==PT_BLOB && nvs.getBytesLength("credentials")==sizeof(value) &&
        nvs.getBytes("credentials",&value,sizeof(value))==sizeof(value);
    nvs.end();
    return read && value.version==1 && value.ssid[0] && memchr(value.ssid,0,sizeof(value.ssid)) &&
        memchr(value.password,0,sizeof(value.password));
}
bool Esp32WifiSetup::prepareForUpdate() {
    cancelConnectionCheck();
    const auto w=status();
    if(!w.storageReady || !w.connected || !w.saved || w.setupActive || connecting_ || submitted_) return false;
    Credentials persisted;
    const bool verified=readSaved(persisted) && sameCredentials(persisted,saved_);
    if(!verified) {Lock lock(mutex_);view_.message="Wi-Fi save could not be verified. Update blocked.";}
    return verified;
}
WifiSetupStatus Esp32WifiSetup::status() const {
    if(!mutex_) {WifiSetupStatus result;result.storageReady=false;result.message="Wi-Fi unavailable";return result;}
    Lock lock(mutex_);auto result=view_;result.connecting=connecting_;return result;
}
bool Esp32WifiSetup::reconnectSaved() {
    if(!mutex_ || !radioAvailable_) return false;
    const auto w=status();
    if(!w.storageReady || !w.saved || w.setupActive || submitted_ || candidateActive_) return false;
    // Preserve the deadline of an automatic join already in progress.
    if(w.connected || w.connecting) return true;
    connect(saved_,false);return true;
}
void Esp32WifiSetup::connect(const Credentials& c,bool candidate) {
    bool requested;{Lock lock(mutex_);requested=pingRequested_;}
    invalidateConnectionCheck(requested ? WifiConnectionCheck::Waiting : WifiConnectionCheck::Idle,requested);
    WiFi.disconnect(false,false);candidate_=c;candidateActive_=candidate;
    WiFi.mode(server_ ? WIFI_AP_STA : WIFI_STA);WiFi.begin(c.ssid,c.password);
    connectAt_=nowMs();attempted_=true;
    Lock lock(mutex_);connecting_=true;view_.connected=false;view_.network=c.ssid;view_.message="Connecting to Wi-Fi…";
}
bool Esp32WifiSetup::start() {
    if(!mutex_ || !radioAvailable_ || !status().storageReady) return false;
    cancelConnectionCheck();
    if(server_ && status().setupComplete) stop();
    if(server_) return true;
    if(connecting_) {WiFi.disconnect(false,false);Lock lock(mutex_);connecting_=false;view_.connected=false;}
    attempted_=true; // Editing must not race an automatic saved-network attempt.
    // User requested an open temporary AP with no additional page PIN.
    const std::string ap="VOLTRA-Setup-"+randomText(4);
    WiFi.mode(WIFI_AP_STA);
    const IPAddress ip(192,168,4,1);
    if(!WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0)) || !WiFi.softAP(ap.c_str(),nullptr,1,0,2)) {
        Lock lock(mutex_);view_.message="Could not start setup Wi-Fi";return false;
    }
    {Lock lock(mutex_);token_=randomText(24);networks_="[]";scanRequested_=true;scanning_=scanFailed_=false;}
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.stack_size=6144;config.max_open_sockets=3;
    config.recv_wait_timeout=1;config.send_wait_timeout=1;config.lru_purge_enable=true;
    if(httpd_start(&server_,&config)!=ESP_OK) {WiFi.softAPdisconnect(false);Lock lock(mutex_);view_.message="Could not open setup page";return false;}
    const httpd_uri_t routes[]={ {"/",HTTP_GET,page,this},{"/networks",HTTP_GET,networks,this},
        {"/scan",HTTP_POST,rescan,this},{"/status",HTTP_GET,statusPage,this},{"/configure",HTTP_POST,configure,this} };
    for(const auto& route:routes) if(httpd_register_uri_handler(server_,&route)!=ESP_OK) {stop();return false;}
    httpd_register_err_handler(server_,HTTPD_404_NOT_FOUND,captive);
    dns_.start(53,"*",ip);
    setupAt_=nowMs();finishAt_=0;
    std::printf("Wi-Fi portal: temporary open AP started; no password or PIN\n");
    Lock lock(mutex_);view_.setupActive=true;view_.setupComplete=false;view_.setupNetwork=ap;
    view_.message="Connect your phone to setup Wi-Fi";return true;
}
void Esp32WifiSetup::stop() {
    if(!mutex_ || !status().storageReady) return;
    cancelConnectionCheck();
    // Stop the HTTP task before taking its shared-data mutex.
    if(server_) {httpd_stop(server_);server_=nullptr;}
    dns_.stop();if(scanning_) esp_wifi_scan_stop();WiFi.scanDelete();WiFi.softAPdisconnect(false);finishAt_=0;
    // Leaving setup cannot let an unapproved/in-flight replacement save later.
    // Successful OK has already cleared candidateActive_ and keeps the live STA.
    const auto w=status();
    const bool cancelledCandidate=candidateActive_ || submitted_ || (w.connected && !w.saved);
    if(cancelledCandidate) WiFi.disconnect(false,false);
    Lock lock(mutex_);view_.setupActive=false;view_.setupNetwork.clear();
    if(cancelledCandidate) {
        connecting_=candidateActive_=false;candidate_={};
        view_.connected=false;view_.saved=saved_.ssid[0]!=0;
        view_.network=saved_.ssid;view_.address.clear();view_.setupComplete=false;
        view_.message=view_.saved ? "Setup cancelled. Saved network kept." : "Setup cancelled.";
    }
    if(!view_.connected && saved_.ssid[0]) attempted_=false;
    scanRequested_=scanning_=scanFailed_=false;
    submitted_=false;submittedPassword_.clear();token_.clear();
}
void Esp32WifiSetup::tick(bool radioAvailable) {
    if(!mutex_) return;
    radioAvailable_=radioAvailable;
    if(!status().storageReady) {
        invalidateConnectionCheck(WifiConnectionCheck::Unavailable);tickConnectionCheck();return;
    }
    if(!radioAvailable) {
        invalidateConnectionCheck(WifiConnectionCheck::Unavailable);
        if(server_) stop();
        if(WiFi.getMode()!=WIFI_OFF) {WiFi.disconnect(false,false);WiFi.mode(WIFI_OFF);}
        {Lock lock(mutex_);connecting_=false;candidateActive_=false;attempted_=false;
            view_.connected=false;view_.address.clear();view_.network=saved_.ssid;
            view_.connectionCheck=WifiConnectionCheck::Unavailable;
            view_.message="Wi-Fi paused during VOLTRA use";
        }
        tickConnectionCheck();return;
    }
    if(!server_ && !attempted_ && saved_.ssid[0]) connect(saved_,false);
    if(server_) {
        dns_.processNextRequest();
        bool startScan=false;
        {Lock lock(mutex_);if(scanRequested_ && !scanning_ && !connecting_ && !submitted_) {
            scanRequested_=false;scanning_=true;scanFailed_=false;startScan=true;
        }}
        if(startScan) {
            WiFi.scanDelete();scanAt_=nowMs();
            if(WiFi.scanNetworks(true)==WIFI_SCAN_FAILED) {
                Lock lock(mutex_);scanning_=false;scanFailed_=true;
                std::printf("Wi-Fi portal: scan could not start\n");
            }
        }
        const int count=scanning_ ? WiFi.scanComplete() : WIFI_SCAN_RUNNING;
        if(scanning_ && count>=0) {
            std::string list="[";std::vector<std::string> seen;
            for(int i=0;i<count && seen.size()<24;++i) {
                const std::string ssid=WiFi.SSID(i).c_str();
                if(ssid.empty() || std::find(seen.begin(),seen.end(),ssid)!=seen.end()) continue;
                if(!seen.empty()) list+=',';list+=json(ssid);seen.push_back(ssid);
            }
            list+=']';{Lock lock(mutex_);networks_=list;scanning_=false;scanFailed_=false;}WiFi.scanDelete();
            std::printf("Wi-Fi portal: scan complete, %u named networks\n",unsigned(seen.size()));
        } else if(scanning_ && (count==WIFI_SCAN_FAILED || nowMs()-scanAt_>=scanTimeoutMs)) {
            esp_wifi_scan_stop();WiFi.scanDelete();
            Lock lock(mutex_);scanning_=false;scanFailed_=true;
            std::printf("Wi-Fi portal: scan failed or timed out\n");
        }
        bool submit=false;Credentials submitted;
        // Wi-Fi scanning and joining cannot run together. A user can select
        // an old result during refresh; keep that submission until scan ends.
        {Lock lock(mutex_);if(submitted_ && !scanning_) {
            std::copy(submittedSsid_.begin(),submittedSsid_.end(),submitted.ssid);
            std::copy(submittedPassword_.begin(),submittedPassword_.end(),submitted.password);
            submitted_=false;connecting_=true;submittedPassword_.clear();submit=true;
        }}
        if(submit) {setupAt_=nowMs();finishAt_=0;connect(submitted,true);}
        if(nowMs()-setupAt_>=setupTimeoutMs || (finishAt_ && nowMs()>=finishAt_)) stop();
    }
    if(connecting_) {
        if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==candidate_.ssid) {
            bool saved=true;
            if(candidateActive_) {
                Preferences nvs;saved=nvs.begin("voltra-net",false);
                if(saved) {saved=nvs.putBytes("credentials",&candidate_,sizeof(candidate_))==sizeof(candidate_);nvs.end();}
                Credentials persisted;
                saved=saved && readSaved(persisted) && sameCredentials(persisted,candidate_);
                if(saved) saved_=candidate_;
            }
            Lock lock(mutex_);connecting_=false;view_.connected=true;view_.saved=saved;
            view_.address=WiFi.localIP().toString().c_str();view_.message=saved ? "Connected and saved" : "Connected; could not save network";
            if(candidateActive_ && saved) view_.setupComplete=true;
            if(candidateActive_ && saved) finishAt_=nowMs()+successCloseMs;
            candidateActive_=false;memset(candidate_.password,0,sizeof(candidate_.password));
        } else if(nowMs()-connectAt_>=connectTimeoutMs) {
            WiFi.disconnect(false,false);Lock lock(mutex_);connecting_=false;candidateActive_=false;
            memset(candidate_.password,0,sizeof(candidate_.password));view_.connected=false;
            view_.address.clear();view_.network=saved_.ssid;
            view_.message="Connection failed. Check network and password.";
        }
    }
    if(!connecting_ && status().connected && WiFi.status()!=WL_CONNECTED) {
        invalidateConnectionCheck(WifiConnectionCheck::Unavailable);
        Lock lock(mutex_);view_.connected=false;view_.address.clear();view_.message="Wi-Fi connection lost";
        view_.network=saved_.ssid;
        attempted_=false; // One new saved-network attempt; a failed join stays failed.
    }
    tickConnectionCheck();
}
esp_err_t Esp32WifiSetup::page(httpd_req_t* req) {
    if(!setupClient(req)) return wrongNetwork(req,true);
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string html=wifiPortalPage;
    {Lock lock(self->mutex_);html.replace(html.find("%TOKEN%"),7,self->token_);}
    std::printf("Wi-Fi portal: serving setup HTML\n");
    return reply(req,html,"text/html; charset=utf-8");
}
esp_err_t Esp32WifiSetup::networks(httpd_req_t* req) {
    if(!setupClient(req)) return wrongNetwork(req);
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string body;
    {Lock lock(self->mutex_);body="{\"networks\":"+self->networks_+
        ",\"scanning\":"+(self->scanning_ || self->scanRequested_ ? "true" : "false")+
        ",\"failed\":"+(self->scanFailed_ ? "true" : "false")+"}";}
    return reply(req,body);
}
esp_err_t Esp32WifiSetup::rescan(httpd_req_t* req) {
    if(!setupClient(req)) return wrongNetwork(req);
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);
    if(req->content_len<1 || req->content_len>64)
        return reply(req,"{\"message\":\"Reopen the setup page.\"}","application/json","400 Bad Request");
    std::string body(req->content_len,'\0');size_t received=0;
    while(received<body.size()) {
        const int n=httpd_req_recv(req,body.data()+received,body.size()-received);
        if(n<=0) return reply(req,"{\"message\":\"Please refresh again.\"}","application/json","400 Bad Request");
        received+=size_t(n);
    }
    {Lock lock(self->mutex_);
        if(!self->view_.setupActive || self->token_.empty() || body!="token="+self->token_)
            return reply(req,"{\"message\":\"Setup expired. Reopen the setup page.\"}","application/json","400 Bad Request");
        if(self->connecting_ || self->submitted_)
            return reply(req,"{\"message\":\"Wait for the Wi-Fi connection check.\"}","application/json","409 Conflict");
        if(!self->scanning_) self->scanRequested_=true;
    }
    return reply(req,"{\"message\":\"Scanning for networks…\"}");
}
esp_err_t Esp32WifiSetup::statusPage(httpd_req_t* req) {
    if(!setupClient(req)) return wrongNetwork(req);
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string body;
    {Lock lock(self->mutex_);body=std::string("{\"connected\":")+(self->view_.connected ? "true" : "false")+
        ",\"saved\":"+(self->view_.saved ? "true" : "false")+",\"connecting\":"+(self->connecting_ || self->submitted_ ? "true" : "false")+
        ",\"message\":"+json(self->view_.message)+"}";}
    return reply(req,body);
}
esp_err_t Esp32WifiSetup::configure(httpd_req_t* req) {
    if(!setupClient(req)) return wrongNetwork(req);
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);
    auto reject=[&](const char* msg){return reply(req,"{\"message\":"+json(msg)+"}","application/json","400 Bad Request");};
    if(req->content_len<1 || req->content_len>512) return reject("Invalid setup request.");
    std::string body(req->content_len,'\0');size_t received=0;
    while(received<body.size()) {int n=httpd_req_recv(req,body.data()+received,body.size()-received);if(n<=0)return reject("Please submit again.");received+=size_t(n);}
    std::string ssid,password,token;size_t pos=0;bool haveSsid=false,havePassword=false,haveToken=false;
    while(pos<body.size()) {
        const auto end=body.find('&',pos),eq=body.find('=',pos);const auto limit=end==std::string::npos ? body.size() : end;
        if(eq==std::string::npos || eq>=limit) return reject("Invalid form.");
        std::string key,value;if(!decode(body.substr(pos,eq-pos),key) || !decode(body.substr(eq+1,limit-eq-1),value)) return reject("Invalid form.");
        if(key=="ssid" && !haveSsid) {ssid=value;haveSsid=true;}
        else if(key=="password" && !havePassword) {password=value;havePassword=true;}
        else if(key=="token" && !haveToken) {token=value;haveToken=true;}
        else return reject("Invalid form.");
        pos=limit+1;
    }
    const bool hexKey=password.size()==64 && std::all_of(password.begin(),password.end(),[](unsigned char c){return isxdigit(c);});
    if(!haveSsid || !havePassword || ssid.empty() || ssid.size()>32 || (!password.empty() && (password.size()<8 || (password.size()>63 && !hexKey))))
        return reject("Use a Wi-Fi name up to 32 bytes and an 8–63 character password, or leave it empty for an open network.");
    {Lock lock(self->mutex_);
        if(!haveToken || token!=self->token_ || token.empty() || !self->view_.setupActive) return reject("Setup expired. Reopen the setup page.");
        if(self->submitted_ || self->connecting_) return reject("A connection is already being checked.");
        self->submittedSsid_=ssid;self->submittedPassword_=password;self->submitted_=true;self->scanRequested_=false;
        self->view_.setupComplete=false;
        self->view_.connected=false;self->view_.message="Connecting…";
    }
    return reply(req,"{\"message\":\"Connecting. Stay on the setup Wi-Fi.\"}");
}
esp_err_t Esp32WifiSetup::captive(httpd_req_t* req,httpd_err_code_t) {
    if(!setupClient(req)) return wrongNetwork(req,true);
    httpd_resp_set_type(req,"text/html; charset=utf-8");httpd_resp_set_hdr(req,"Cache-Control","no-store");
    httpd_resp_set_status(req,"302 Found");httpd_resp_set_hdr(req,"Location","http://192.168.4.1/");
    return httpd_resp_sendstr(req,"<!doctype html><html><title>VOLTRA Remote Wi-Fi</title><a href='http://192.168.4.1/'>Open VOLTRA Remote Wi-Fi setup</a></html>");
}
}
#endif
