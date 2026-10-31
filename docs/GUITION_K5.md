# Guition K5 hardware validation

October 17: [Normal/Simulator firmware](NORMAL_MODE_2026_10_17.md) supersedes the
single-image offline build below. Build-Guition now builds both images; Flash-Guition
uploads both and boots Normal. The second OTA region has its own verified factory
recovery file. Real training remains unavailable pending a validated profile.
The following original bring-up notes are historical.

CONFIRMED October 17 Normal upload: recovery checks passed, all five uploaded
images verified, and USB restart reported Normal/physical BLE and shared UI startup
after display/touch/encoder initialization. No reset loop in the 15-second capture.
Artifacts are in ignored build/guition/2026-10-17-normal. Physical mode-switch
gesture and actual VOLTRA connection remain unverified. See BASELINE.md.

User authorization: October 11, proceed with JC3636K518 as the working target
without dismantling the unit. User subsequently confirmed the screen and touch
basically work; input calibration follow-up is below. This target does not mark
the older prototype_21 as identified.

## Evidence

CONFIRMED on the attached USB Serial/JTAG connection: ESP32-S3 revision 0.2,
8 MB embedded OPI PSRAM, 16 MB quad flash, 40 MHz crystal, no secure boot or flash
encryption. Product-family identification and manufacturer documentation are
recorded in HARDWARE_COMPATIBILITY.md. Device identifiers stay out of source.

Reference implementation (read outside this repository):
https://github.com/andrew-malitchuk/prompt-knob-firmware/tree/b738e065b7dec7a13501cdd81c8d3b511850bce4

CONFIRMED in that reference, not yet physical validation here: ST77916 QSPI
360x360 initialization, CST816 touch and pin map. PanelInit.h adapts its register
table to fixed C++17 records. Its Apache-2.0 license is retained under
third_party/licenses. No upstream app, BLE, networking, or build scripts are used.
The new driver uses the installed Espressif SPI master and Arduino Wire APIs.

## Board adapter

- QSPI: clock 13, CS 14, data 15/16/17/18, reset 21, backlight 47.
- Touch: I2C address 0x15, SDA 11, SCL 12, reset 10, interrupt 9.
- Rotary directional contacts: 8 = clockwise, 7 = counterclockwise according to
  the reference's bidi_switch_knob driver. Each independently completed low pulse
  produces one signed unit, without a quadrature divider. Minimum low duration
  is 3 ms, matching the reference's sampling interval; shorter bounce pulses are
  discarded. Interrupt capture accumulates clicks during display transfers.
  Direction and one-click calibration await physical confirmation of this fix.
- Press candidate: GPIO 0, 30 ms debounce. Physical accessibility must be checked.
- Display: RGB565 wire conversion from the shared XRGB8888 UI buffer, synchronous
  eight-row transfers at a conservative 20 MHz. Init table retains panel delays.
- Backlight: LEDC channel 0, 5 kHz, 10 bits; normal duty 700, dim 100, asleep 0.
- Flash: ROM boot header DIO 80 MHz (preserve compiled header); runtime QIO 80 MHz. PSRAM: OPI 80 MHz. Small allocations stay internal; large
  LVGL buffers use PSRAM. The DMA transfer buffer is explicitly internal RAM.
- Initialization checks memory geometry, SPI results and touch acknowledgement;
  failures stop UI startup and are reported over USB.

Battery percentage and vibration remain absent until validated. A USB-powered
unit must not show an invented battery percentage. No Wi-Fi or physical BLE is
initialized; the firmware's explicit source list includes only MockTransport.
It retains the six synthetic firmware profiles and the shared UI/controller.
Preferences uses one versioned NVS blob in the voltra-ui namespace for remote-wide
configuration. Presets contain no load state or stored base resistance.

## Build and initial exercise

Run tools/Build-Guition.ps1. Environment: guition_k5_offline, Arduino ESP32
2.0.17, espressif32 6.12.0, LVGL 9.3.0. Outputs are in
.pio/build/guition_k5_offline. The default prototype target stays blocked.

This is a first hardware-validation build, not a real VOLTRA control release.
Start on the Devices page; use touch and knob on synthetic devices. The footer
says SIM. USB serial at 115200 accepts r (synthetic rep), h (held), m (moving),
u (unload/cancel only), d (return to Devices). Input logs report touch coordinates,
knob deltas and presses. No serial command can enable a physical radio.

A factory recovery image of offsets 0x0-0x1fffff (2 MB) was saved and verified
against the device with esptool verify_flash (digest matched). It is in ignored
build/hardware-backups/guition-factory-first-2MB.bin, with a SHA256 manifest.
It is NOT a full 16 MB backup. The bring-up partition map deliberately confines
all writes, including NVS and crash dumps, to this backed-up region. Remaining
flash has no mapped filesystem/OTA slot and is left untouched. Before flashing,
retain this recovery image and
record the binary hashes. Upload bootloader at 0x0, partition table at 0x8000,
boot_app0 at 0xe000 and application at 0x10000, using the ESP32-S3 target and
16 MB flash geometry. Never run erase_flash or change eFuses.

Physical checks still needed: image orientation/colors, readable fonts, touch
alignment, clockwise direction, detents and press, persistence across reboot.
No new automated test code is introduced for this milestone.

## October 17 feedback upload

CONFIRMED: the user reconnected the remote and the feedback build was uploaded
on COM3. Built image hashes matched the saved October 17 manifest before upload;
esptool verified every written image. The recovery image and partition checks
passed. Firmware file size: 915,920 bytes. Local artifacts and startup capture
are in ignored `build/guition/2026-10-17/`.

A restart using esptool's Windows-compatible USB reset sequence reported display
initialization, touch ACK, active encoder inputs and shared UI startup. No reset
loop appeared during the 10-second capture. The previously documented old
crash-dump partition warning remains; startup continued successfully. Physical
touch/encoder feel still requires user observation. This is offline firmware;
no real VOLTRA command was sent.

## October 11 first upload result

CONFIRMED: the offline target compiled and was uploaded over COM3. esptool
verified hashes for bootloader, partition table, boot selection and application.
Final firmware is 912,048 bytes; reported static RAM is 21,720 bytes. Build copies
and SHA256 records are in ignored build/guition. Device-specific USB identifiers
are kept out of Git.

Two bring-up issues were diagnosed from serial logs and fixed before handoff:
(1) forcing QIO in the ROM image header caused a boot loop; preserve the compiled
DIO header instead, as also seen in the factory image. Runtime flash remains QIO.
(2) Arduino getPsramSize reports allocator-usable capacity (8,386,295 bytes on this
unit), not the exact physical 8 MiB; require at least 7 MiB usable plus detected
PSRAM, consistent with the independent chip identification.

Final startup reports: Display initialized; touch ACK; encoder inputs active;
Shared UI started. No reset loop appeared in the captured startup interval.
Physical image quality, touch alignment, encoder direction and press remain
UNVERIFIED until user observation. First-start diagnostics report missing voltra-ui
NVS namespace (defaults are used) and invalid old bytes at the new crash-dump
partition; neither prevents UI startup. No whole-chip erase was performed.

## October 11 input follow-up

CONFIRMED by user: screen and touch basically work, but Green took two taps;
clockwise reduced weight and many clicks were needed for one unit. Source audit
found a driver mismatch: this board's reference uses independent directional
pulses, whereas the first build incorrectly decoded quadrature transitions.
Replaced that decoder with one debounced pulse per adjustment. The pulse mapping
is source-confirmed; physical calibration remains UNVERIFIED until retested.

Touch samples now reach LVGL immediately before the main UI tick. Unchanged text
no longer causes repeated label allocation/layout work. These address possible
missed short taps; the user's exact double-tap cause is not physically confirmed.
No new test code was added. Embedded and Windows simulator builds passed; USB
upload verified the new application hash. Application size: 912,192 bytes.

Real-device discovery remains planned after offline UI validation, as agreed with
the user. The Devices flow already accepts scan results through BleTransport;
the hardware release needs the real transport, VOLTRA service filtering,
connection/handshake, device-reported capabilities, and saved recent devices.
This build continues using the six synthetic devices and initializes no radio.
Discovery/connection must never itself issue a load command.

## October 31 touch capture independent of Bluetooth

CONFIRMED (user): the larger drift tolerance did not remove the need to tap
Apply twice. The requested unloaded trial logged one recognized Apply action
and successful local dropset activation, with no application refusal. INFERRED:
the missing gesture was lost before the controller; main-loop sampling could
miss an entire short tap during synchronous BLE writes/display transfer.

The K5 now samples CST816 in a dedicated priority-2 task on the Arduino core,
with a 4096-byte stack and 8 ms delay between reads. It owns Wire after board
initialization and never calls LVGL/application/BLE. A protected fixed queue
preserves up to 64 changed normalized samples in order. Unchanged contact still
refreshes its timestamp. The application drains samples via TouchInput and
LVGL continue_reading; board-specific task/GPIO/I2C details stay in Board.

Tunable input policy: samples older than 350 ms, queue overflow, I2C failure or
invalid pressed coordinates cancel the whole gesture and require release.
Cancellation is explicit, never a fake clickable release. Rebuilding a screen
discards queued old-screen input and suppresses a held contact until release.
The UI clock is refreshed before explicit input reads. Diagnostics include
capture-to-delivery age, down/up/cancel and recognized actions. Physical
responsiveness is pending the user's post-install trial; these policies do not
relax controller readiness checks or repeat a rejected command.

CONFIRMED (October 31 user follow-up): repeated Drops Apply now returns on the
first tap. The independent sampler resolves the reported double-tap behavior
in that trial; this does not prove every gesture/timing edge case.
