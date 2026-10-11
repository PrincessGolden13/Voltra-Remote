# Remote hardware compatibility

| Target | Status | Evidence |
| --- | --- | --- |
| Windows simulator, 360/466/480 round viewport | Foundation development target | Shared LVGL/controller/protocol, synthetic transport |
| prototype_21 | UNVERIFIED / build blocked | User-described ESP32-S3 rotary touchscreen; exact PCB not identified |

Ryan's reference describes **ESP32 Display 2.1 V1.0**, 480x480 RGB panel, CST826
touch, 16 MB flash and 8 MB PSRAM. CONFIRMED as an upstream claim only. It does
not identify the user's unit. No GPIO map, memory geometry, display/touch driver,
encoder polarity or COM port is adopted into the application.

Board adapter currently reports no validated capabilities, touches no pins and
fails target validation. The candidate PlatformIO board entry is a compiler
placeholder, not an assertion of hardware compatibility; a pre-build gate rejects
it before any upload. There is no bypass flag.

## Identification needed for milestone 1

Obtain product/manufacturer link, clear PCB markings and revision, connector
labels, controller markings where visible, and the supplied vendor demo/files.
With the unit connected, inspect USB/serial enumeration and chip/flash identity
before choosing a board definition. Avoid dismantling powered hardware.

Then validate display, touch normalization, clockwise-positive rotation, press,
USB reset/boot behavior and power supply. Enable BLE scanning only after board
validation. Optional Wi-Fi, battery and haptics must not become core requirements.

Every physical target must have suitable BLE, a round display, touch, encoder
and press. The simulator reports no physical BLE and is validated as a desktop
target separately. Future board adapters share core application code.

LVGL consumes display metrics and normalized touch. The initial desktop policy
supports 360, 466 and 480 square round viewports; product touch/font sizing still
needs physical validation. Pixel dimensions alone do not establish usability.

## October 11 connected-unit identification

CONFIRMED by read-only esptool 4.9.0 queries on the connected COM3 interface:
ESP32-S3 QFN56 revision 0.2, 40 MHz crystal, embedded 8 MB AP_3v3 PSRAM,
16 MB quad-SPI flash at 3.3 V, USB Serial/JTAG, secure boot disabled, flash
encryption disabled. No flash writes, erases, or eFuse changes were made.
Device-specific serial/MAC values are intentionally omitted here.

User purchase link: https://www.aliexpress.us/item/3256808637654546.html
The listing itself was inaccessible to the web reader. INFERRED model family:
Guition K5 / JC3636K518, based on this exact link in the board developer's
[reference project](https://github.com/nkinnan/Waveshare-ESP32-S3-Knob-Touch-LCD-1.8_and_Guition-K5-Knob-Series-JC3636K518).

CONFIRMED in manufacturer documentation only: the
[JC3636K518 instructions](https://www.guition.com/ku/icms/upload/fb081940d6fc11f09850077a33e1404f/FTPData/UEditor/file/2026121/1768961095200/JC3636K518%20Instructions-EN.pdf)
describe a 360x360 ST77916 display, CST816 capacitive touch, ESP32-S3R8,
8 MB PSRAM and 16 MB flash. This supersedes the earlier 2.1-inch/480x480
candidate as the likely target; it does not establish the connected PCB revision.
Exact model/suffix still needs an external label or order-variant observation.

The manufacturer's JC3636K518CN_knob_EN.zip public download resolves to a
622,866,565-byte, 13-shard archive. Its manifest was retrieved; shard downloads
stalled and were stopped. No partial archive was accepted as verified source.
Reference downloads stay outside this repository under the temporary audit folder.

An attempted full 16 MB flash backup stalled after entering the RAM stub and
was stopped. No completed backup exists. A subsequent no-stub identification
query succeeded and performed a normal hard reset to exit the inspection state.
Do not describe the backup as successful or overwrite factory firmware yet.
The prototype_21 build gate remains intact; no board pinout was adopted and no
flashable application was created by these identification steps.

## Working-target authorization and bring-up

The user explicitly authorized proceeding with the JC3636K518 working target
without a label photo or dismantling. A dedicated guition_k5_offline adapter now
exists; see GUITION_K5.md for reference pin data, build and validation status.
The original prototype_21 remains blocked. No physical BLE backend is included.

A 2 MB factory recovery image (offsets 0 through 0x1fffff) has since been read and
verified against flash: esptool verify_flash reported digest matched. All planned
writable partitions are confined to that region, including crash dumps. The rest
of the 16 MB flash is left untouched. This is a region backup, not a full backup.
