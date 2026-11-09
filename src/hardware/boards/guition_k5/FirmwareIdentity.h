#pragma once
#include <cstdint>
#include <cstring>
#include <esp_ota_ops.h>
namespace voltra::guition {
// ESP-IDF's custom app descriptor follows the 256-byte app descriptor in DROM.
// Verified again by the release packager at byte 288 of the generated .bin.
struct FirmwareIdentity {
    uint32_t magic, layout;
    char version[32], board[16], channel[16];
};
static_assert(sizeof(FirmwareIdentity)==72,"Release descriptor layout changed");
inline bool normalIdentity(const esp_partition_t* partition,const char* version=nullptr) {
    FirmwareIdentity info{};
    return partition && esp_partition_read(partition,288,&info,sizeof(info))==ESP_OK &&
        info.magic==0x56524d54 && info.layout==2 &&
        std::strncmp(info.board,"guition-k5",sizeof(info.board))==0 &&
        std::strncmp(info.channel,"normal",sizeof(info.channel))==0 &&
        (!version || std::strncmp(info.version,version,sizeof(info.version))==0);
}
}
