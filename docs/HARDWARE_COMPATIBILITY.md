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
