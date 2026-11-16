#include "NvsProtection.h"
#include <esp_log.h>
#include <esp_partition.h>
#include <nvs_flash.h>

// Arduino 2.0.17 initArduino() otherwise accepts a new OTA before setup() and
// before NVS initialization. Its supported weak hook defers that decision to
// BootMode::confirmStartup(), including the storage-readiness check.
extern "C" bool verifyRollbackLater() { return true; }

extern "C" esp_err_t __real_esp_partition_erase_range(const esp_partition_t*, size_t, size_t);
extern "C" esp_err_t __wrap_esp_partition_erase_range(const esp_partition_t* partition, size_t offset, size_t size) {
    // Arduino 2.0.17 and NimBLE 2.3.6 automatically erase all NVS after
    // NO_FREE_PAGES/NEW_VERSION initialization errors. Keep the existing bytes
    // for recovery instead. Normal NVS sector garbage collection and OTA app
    // erases still go through the unmodified SDK implementation.
    if(partition && partition->type==ESP_PARTITION_TYPE_DATA &&
       partition->subtype==ESP_PARTITION_SUBTYPE_DATA_NVS && offset==0 && size==partition->size) {
        ESP_EARLY_LOGE("voltra-nvs", "Refusing automatic settings erase; existing storage retained");
        return ESP_ERR_INVALID_STATE;
    }
    return __real_esp_partition_erase_range(partition,offset,size);
}

namespace voltra::guition {
bool persistentStorageReady() { return nvs_flash_init()==ESP_OK; }
}
