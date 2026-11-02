#pragma once
#if !defined(VOLTRA_PHYSICAL) || defined(VOLTRA_SIMULATOR)
#error "Physical Wi-Fi setup is not part of simulator builds"
#endif
#include "WifiSetup.h"
#include <DNSServer.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
namespace voltra {
class Esp32WifiSetup final : public WifiSetup {
public:
    void begin();
    WifiSetupStatus status() const override;
    bool start() override;
    void stop() override;
    void tick(bool radioAvailable) override;
private:
    struct Credentials { uint32_t version=1; char ssid[33]{}; char password[65]{}; };
    Credentials saved_{}, candidate_{};
    mutable SemaphoreHandle_t mutex_=nullptr;
    WifiSetupStatus view_;
    std::string token_,networks_="[]",submittedSsid_,submittedPassword_;
    bool submitted_=false,connecting_=false,candidateActive_=false,radioAvailable_=true,attempted_=false;
    bool scanRequested_=false,scanning_=false,scanFailed_=false;
    uint64_t setupAt_=0,connectAt_=0,finishAt_=0,scanAt_=0;
    httpd_handle_t server_=nullptr;
    DNSServer dns_;
    void connect(const Credentials&,bool candidate);
    static esp_err_t page(httpd_req_t*);
    static esp_err_t networks(httpd_req_t*);
    static esp_err_t rescan(httpd_req_t*);
    static esp_err_t statusPage(httpd_req_t*);
    static esp_err_t configure(httpd_req_t*);
    static esp_err_t captive(httpd_req_t*,httpd_err_code_t);
};
}
