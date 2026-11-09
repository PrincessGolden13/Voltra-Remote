#pragma once
#include <esp_ota_ops.h>
#include <Preferences.h>
namespace voltra::guition {
// Layout 2: two Normal slots and one isolated Simulator, all in 16 MB flash.
struct FirmwareSlots {
    static constexpr int layout=2;
    static constexpr uint32_t normalSize=0x300000;
    static const esp_partition_t* normal(bool second) {
        const auto* p=esp_partition_find_first(ESP_PARTITION_TYPE_APP,
            second ? ESP_PARTITION_SUBTYPE_APP_OTA_1 : ESP_PARTITION_SUBTYPE_APP_OTA_0,
            second ? "normal_b" : "normal");
        return p && p->address==(second ? 0x310000u : 0x10000u) && p->size==normalSize ? p : nullptr;
    }
    static bool isNormal(const esp_partition_t* p) {
        return p && (p==normal(false) || p==normal(true));
    }
    static const esp_partition_t* updateTarget() {
        const auto* running=esp_ota_get_running_partition();
        return isNormal(running) ? normal(running==normal(false)) : nullptr;
    }
    static bool rememberNormal() {
        const auto* running=esp_ota_get_running_partition();
        if(!isNormal(running)) return false;
        Preferences prefs;if(!prefs.begin("voltra-boot",false)) return false;
        const bool ok=prefs.putBool("second",running==normal(true))==1;prefs.end();return ok;
    }
    static const esp_partition_t* savedNormal() {
        Preferences prefs;bool second=false;
        if(prefs.begin("voltra-boot",true)) {second=prefs.getBool("second",false);prefs.end();}
        return normal(second);
    }
};
}
