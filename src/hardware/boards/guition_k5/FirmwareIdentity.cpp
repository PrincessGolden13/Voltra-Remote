#include "FirmwareIdentity.h"
#include "update/FirmwareUpdate.h"
extern const __attribute__((used,section(".rodata_custom_desc"))) voltra::guition::FirmwareIdentity voltraFirmwareIdentity={
    0x56524d54,2,VOLTRA_VERSION,"guition-k5",
#ifdef VOLTRA_SIMULATOR
    "simulator"
#else
    "normal"
#endif
};
