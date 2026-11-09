#pragma once
#include "controller/RuntimeMode.h"
#include "FirmwareSlots.h"
#include "FirmwareIdentity.h"
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
        const auto* normal=FirmwareSlots::savedNormal();
        return normalIdentity(normal) && esp_ota_set_boot_partition(normal)==ESP_OK;
    }
    bool confirmStartup() {
        // Only after display/input, BLE (Normal) and the UI initialized. No motor
        // test or VOLTRA connection is part of firmware startup validation.
        const auto* running=esp_ota_get_running_partition();esp_ota_img_states_t state;
        if(esp_ota_get_state_partition(running,&state)==ESP_OK && state==ESP_OTA_IMG_PENDING_VERIFY &&
           esp_ota_mark_app_valid_cancel_rollback()!=ESP_OK) return false;
        return simulated() || FirmwareSlots::rememberNormal();
    }
    bool switchMode() override {
        if(!simulated() && !FirmwareSlots::rememberNormal()) return false;
        const auto* target=simulated() ? FirmwareSlots::savedNormal() :
            esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_2,"simulator");
        esp_app_desc_t info{};
        if(!target || esp_ota_get_partition_description(target,&info)!=ESP_OK || esp_ota_set_boot_partition(target)!=ESP_OK) return false;
        ESP.restart();return true;
    }
};
}
