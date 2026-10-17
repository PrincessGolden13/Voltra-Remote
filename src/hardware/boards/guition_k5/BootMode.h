#pragma once
#include "controller/RuntimeMode.h"
#include <esp_ota_ops.h>
#include <Arduino.h>
namespace voltra::guition {
class BootMode final : public RuntimeMode {
public:
    bool simulated() const override {
#ifdef VOLTRA_SIMULATOR
        return true;
#else
        return false;
#endif
    }
    bool begin() {
        // Simulator is a one-session opt-in. Restore the normal boot target now,
        // so power loss/reset never silently returns to synthetic devices.
        if(!simulated()) return true;
        const auto* normal=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"normal");
        esp_app_desc_t info{};
        return normal && esp_ota_get_partition_description(normal,&info)==ESP_OK && esp_ota_set_boot_partition(normal)==ESP_OK;
    }
    bool switchMode() override {
        const auto* target=esp_partition_find_first(ESP_PARTITION_TYPE_APP,
            simulated() ? ESP_PARTITION_SUBTYPE_APP_OTA_0 : ESP_PARTITION_SUBTYPE_APP_OTA_1,
            simulated() ? "normal" : "simulator");
        esp_app_desc_t info{};
        if(!target || esp_ota_get_partition_description(target,&info)!=ESP_OK || esp_ota_set_boot_partition(target)!=ESP_OK) return false;
        ESP.restart();return true;
    }
};
}
