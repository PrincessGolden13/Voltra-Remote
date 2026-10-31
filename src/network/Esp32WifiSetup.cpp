#ifdef VOLTRA_PHYSICAL
#include "Esp32WifiSetup.h"
#include "WifiPortalPage.h"
#include <WiFi.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cctype>
#include <lwip/sockets.h>
namespace voltra {
namespace {
constexpr uint64_t setupTimeoutMs=300000,connectTimeoutMs=20000,successCloseMs=15000;
uint64_t nowMs(){return uint64_t(esp_timer_get_time())/1000;}
bool setupClient(httpd_req_t* req) {
    sockaddr_in peer{};socklen_t size=sizeof(peer);
    return getpeername(httpd_req_to_sockfd(req),reinterpret_cast<sockaddr*>(&peer),&size)==0 &&
        peer.sin_family==AF_INET && (ntohl(peer.sin_addr.s_addr)&0xffffff00u)==0xc0a80400u;
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
bool decode(const std::string& in,std::string& out) {
    auto hex=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
    out.clear();for(size_t i=0;i<in.size();++i) {
        unsigned char c=in[i];if(c=='+') c=' ';
        else if(c=='%') {if(i+2>=in.size() || hex(in[i+1])<0 || hex(in[i+2])<0)return false;c=hex(in[i+1])*16+hex(in[i+2]);i+=2;}
        if(c<32 || c==127) return false;out+=char(c);
    }return true;
}
}
void Esp32WifiSetup::begin() {
    mutex_=xSemaphoreCreateMutex();if(!mutex_) return;
    Preferences nvs;if(nvs.begin("voltra-net",true)) {
        if(nvs.getBytesLength("credentials")==sizeof(saved_)) nvs.getBytes("credentials",&saved_,sizeof(saved_));
        nvs.end();
    }
    if(saved_.version!=1 || !memchr(saved_.ssid,0,sizeof(saved_.ssid)) || !memchr(saved_.password,0,sizeof(saved_.password))) saved_={};
    view_.saved=saved_.ssid[0]!=0;view_.network=saved_.ssid;
    view_.message=view_.saved ? "Saved network" : "Wi-Fi not configured";
    WiFi.persistent(false);WiFi.setAutoReconnect(false);
}
WifiSetupStatus Esp32WifiSetup::status() const {
    if(!mutex_) {WifiSetupStatus result;result.message="Wi-Fi unavailable";return result;}
    Lock lock(mutex_);return view_;
}
void Esp32WifiSetup::connect(const Credentials& c,bool candidate) {
    WiFi.disconnect(false,false);candidate_=c;candidateActive_=candidate;
    WiFi.mode(server_ ? WIFI_AP_STA : WIFI_STA);WiFi.begin(c.ssid,c.password);
    connectAt_=nowMs();attempted_=true;
    Lock lock(mutex_);connecting_=true;view_.connected=false;view_.network=c.ssid;view_.message="Connecting to Wi-Fi…";
}
bool Esp32WifiSetup::start() {
    if(!mutex_ || !radioAvailable_) return false;
    if(server_) return true;
    if(connecting_) {WiFi.disconnect(false,false);Lock lock(mutex_);connecting_=false;view_.connected=false;}
    attempted_=true; // Editing must not race an automatic saved-network attempt.
    const std::string ap="VOLTRA-Setup-"+randomText(4),password=randomText(10);
    WiFi.mode(WIFI_AP_STA);
    const IPAddress ip(192,168,4,1);
    if(!WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0)) || !WiFi.softAP(ap.c_str(),password.c_str(),1,0,2)) {
        Lock lock(mutex_);view_.message="Could not start setup Wi-Fi";return false;
    }
    {Lock lock(mutex_);token_=randomText(24);networks_="[]";}
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.stack_size=6144;config.max_open_sockets=3;
    config.recv_wait_timeout=1;config.send_wait_timeout=1;config.lru_purge_enable=true;
    if(httpd_start(&server_,&config)!=ESP_OK) {WiFi.softAPdisconnect(false);Lock lock(mutex_);view_.message="Could not open setup page";return false;}
    const httpd_uri_t routes[]={ {"/",HTTP_GET,page,this},{"/networks",HTTP_GET,networks,this},
        {"/status",HTTP_GET,statusPage,this},{"/configure",HTTP_POST,configure,this} };
    for(const auto& route:routes) if(httpd_register_uri_handler(server_,&route)!=ESP_OK) {stop();return false;}
    httpd_register_err_handler(server_,HTTPD_404_NOT_FOUND,captive);
    dns_.start(53,"*",ip);WiFi.scanNetworks(true);
    setupAt_=nowMs();finishAt_=0;
    Lock lock(mutex_);view_.setupActive=true;view_.setupNetwork=ap;view_.setupPassword=password;
    view_.message="Connect your phone to setup Wi-Fi";return true;
}
void Esp32WifiSetup::stop() {
    // Stop the HTTP task before taking its shared-data mutex.
    if(server_) {httpd_stop(server_);server_=nullptr;}
    dns_.stop();WiFi.scanDelete();WiFi.softAPdisconnect(false);finishAt_=0;
    if(!mutex_) return;
    Lock lock(mutex_);view_.setupActive=false;view_.setupNetwork.clear();view_.setupPassword.clear();
    submitted_=false;submittedPassword_.clear();token_.clear();
}
void Esp32WifiSetup::tick(bool radioAvailable) {
    if(!mutex_) return;
    radioAvailable_=radioAvailable;
    if(!radioAvailable) {
        if(server_) stop();
        if(WiFi.getMode()!=WIFI_OFF) {WiFi.disconnect(false,false);WiFi.mode(WIFI_OFF);}
        Lock lock(mutex_);connecting_=false;candidateActive_=false;attempted_=false;
        view_.connected=false;view_.message="Wi-Fi paused during VOLTRA use";return;
    }
    if(!attempted_ && saved_.ssid[0]) connect(saved_,false);
    if(server_) {
        dns_.processNextRequest();
        const int count=WiFi.scanComplete();
        if(count>=0) {
            std::string list="[";std::vector<std::string> seen;
            for(int i=0;i<count && seen.size()<24;++i) {
                const std::string ssid=WiFi.SSID(i).c_str();
                if(ssid.empty() || std::find(seen.begin(),seen.end(),ssid)!=seen.end()) continue;
                if(!seen.empty()) list+=',';list+=json(ssid);seen.push_back(ssid);
            }
            list+=']';{Lock lock(mutex_);networks_=list;}WiFi.scanDelete();
        }
        bool submit=false;Credentials submitted;
        {Lock lock(mutex_);if(submitted_) {
            std::copy(submittedSsid_.begin(),submittedSsid_.end(),submitted.ssid);
            std::copy(submittedPassword_.begin(),submittedPassword_.end(),submitted.password);
            submitted_=false;submittedPassword_.clear();submit=true;
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
                if(saved) saved_=candidate_;
            }
            Lock lock(mutex_);connecting_=false;view_.connected=true;view_.saved=saved;
            view_.address=WiFi.localIP().toString().c_str();view_.message=saved ? "Connected and saved" : "Connected; could not save network";
            if(candidateActive_ && saved) finishAt_=nowMs()+successCloseMs;
            candidateActive_=false;memset(candidate_.password,0,sizeof(candidate_.password));
        } else if(nowMs()-connectAt_>=connectTimeoutMs) {
            WiFi.disconnect(false,false);Lock lock(mutex_);connecting_=false;candidateActive_=false;
            memset(candidate_.password,0,sizeof(candidate_.password));view_.connected=false;
            view_.message="Connection failed. Check network and password.";
        }
    }
    if(!connecting_ && status().connected && WiFi.status()!=WL_CONNECTED) {
        Lock lock(mutex_);view_.connected=false;view_.address.clear();view_.message="Wi-Fi connection lost";
        attempted_=false; // One new saved-network attempt; a failed join stays failed.
    }
}
esp_err_t Esp32WifiSetup::page(httpd_req_t* req) {
    if(!setupClient(req)) return reply(req,"{}","application/json","403 Forbidden");
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string html=wifiPortalPage;
    {Lock lock(self->mutex_);html.replace(html.find("%TOKEN%"),7,self->token_);}
    return reply(req,html,"text/html; charset=utf-8");
}
esp_err_t Esp32WifiSetup::networks(httpd_req_t* req) {
    if(!setupClient(req)) return reply(req,"{}","application/json","403 Forbidden");
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string body;
    {Lock lock(self->mutex_);body=self->networks_;}return reply(req,body);
}
esp_err_t Esp32WifiSetup::statusPage(httpd_req_t* req) {
    if(!setupClient(req)) return reply(req,"{}","application/json","403 Forbidden");
    auto* self=static_cast<Esp32WifiSetup*>(req->user_ctx);std::string body;
    {Lock lock(self->mutex_);body=std::string("{\"connected\":")+(self->view_.connected ? "true" : "false")+
        ",\"saved\":"+(self->view_.saved ? "true" : "false")+",\"connecting\":"+(self->connecting_ || self->submitted_ ? "true" : "false")+
        ",\"message\":"+json(self->view_.message)+"}";}
    return reply(req,body);
}
esp_err_t Esp32WifiSetup::configure(httpd_req_t* req) {
    if(!setupClient(req)) return reply(req,"{}","application/json","403 Forbidden");
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
        self->submittedSsid_=ssid;self->submittedPassword_=password;self->submitted_=true;
        self->view_.connected=false;self->view_.message="Connecting…";
    }
    return reply(req,"{\"message\":\"Connecting. Stay on the setup Wi-Fi.\"}");
}
esp_err_t Esp32WifiSetup::captive(httpd_req_t* req,httpd_err_code_t) {
    httpd_resp_set_status(req,"302 Found");httpd_resp_set_hdr(req,"Location","http://192.168.4.1/");
    return httpd_resp_send(req,nullptr,0);
}
}
#endif
