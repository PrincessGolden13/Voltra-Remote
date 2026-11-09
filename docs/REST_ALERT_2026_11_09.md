# Rest completion notification — November 9, 2026

The owner's current request supersedes the earlier two-second alert in
UI_REVISION_2026_10_11.md. After a reported end of set starts the existing rest
clock, reaching the configured threshold requests 20 seconds of vibration and
shows a **Rest over** button. Tapping it stops vibration early and dismisses the
notice. Otherwise vibration stops at 20 seconds and the button remains until
acknowledged. The screen stays awake while the notice is visible. The button is
an upper banner, not a modal covering the weight/unload control, and survives
navigation. An already-held touch cannot acknowledge a newly appearing notice.
There is one alert per rest; an outstanding notice never extends its buzz.

Rest alert defaults to **Off** on first use and factory reset. Explicit saved
preferences are preserved. Selecting Off cancels an outstanding notification.
No rest intent or notification is persisted across reboot, and notification
handling issues no VOLTRA commands. Existing set-close/movement evidence still
controls the rest clock; merely tapping load/unload does not start a fake set.

## Hardware evidence

CONFIRMED (owner, this session): the factory timer app can buzz this remote.
CONFIRMED (manufacturer documentation): the
[Guition JC3636K518 instructions](https://www.guition.com/icms/upload/fb081940d6fc11f09850077a33e1404f/FTPData/UEditor/file/2026121/1768961095200/JC3636K518%20Instructions-EN.pdf)
describe a vibration motor and a vibrating timer. The related
[Waveshare board documentation](https://www.waveshare.com/product/esp32-s3-knob-touch-lcd-1.8.htm)
identifies a DRV2605 I2C motor driver.

INFERRED board implementation, requiring runtime detection and physical trial:
[matching K5 reference](https://github.com/KrX3D/WaveShare-Knob-Esp32S3/tree/74511f2260e3e7ce3c84ca3136569307cfb8b26b)
uses DRV2605 at 0x5A on SDA 11 / SCL 12 with an LRA motor. Its configuration
references the [driver at 71d63d0](https://github.com/RAR/esphome-drv2605/tree/71d63d0c6a5fd337b952337058650f80ffdab6e1),
which sets the feedback LRA bit while preserving other feedback defaults.
No reference code was vendored or made a dependency.

The new board adapter checks DRV2605/2605L identity before configuration. It uses
[TI's register map](https://www.ti.com/lit/ds/symlink/drv2605l.pdf): GO 0x0C=0,
RTP 0x02=0, FEEDBACK 0x1A bit 7 for LRA, CONTROL3 0x1D bit 3 cleared
for signed RTP. MODE 0x01 stays in standby (0x40) until an alert selects real-time
playback (5); stopping sends both zero amplitude and standby.
It retains voltage/feedback defaults and uses positive signed RTP amplitude 127
only on explicit rest expiry. Startup never starts calibration or vibration.
An absent/unrecognized chip leaves a visual notification and logs the reason.

All Wire access stays on the existing touch task after initialization. The UI
exchanges only a protected deadline; the task stops the motor independently of
UI/BLE stalls, to within normal task scheduling/I2C latency. Requests are capped
at 20 seconds and duration zero cancels. A failed start ACK is treated as possibly
running so zero is still sent at expiry; failed stop writes are retried. Bus or
hardware failures cannot guarantee physical output. No guessed GPIO is driven.

## Checks still requiring the remote

- With Off selected, completing a set produces no alert, including after reboot.
- With a short rest selected, a genuine set end starts timing; expiry buzzes and
  wakes the display. Confirm strength and physical duration on this board.
- Tap Rest over while buzzing: immediate stop, no load action, no repeat for
  that rest. Ignore it: buzz stops at 20 seconds; notice stays visible and awake.
- Navigate with the notice visible; verify it remains tappable and unload remains
  accessible. A new set can rearm a later alert after acknowledgment.
- Verify dim/sleep, factory reset, saved preferences, temporary stale telemetry,
  absent driver, and touch responsiveness during vibration. Confirm no boot buzz.

No physical motor or live BLE test was run in the cloud. New automated test code
remains deferred by the project brief. Build results are recorded in TESTING.md.
