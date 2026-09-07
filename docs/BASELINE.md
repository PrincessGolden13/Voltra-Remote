# Foundation baseline

Validated on Windows 10 x64, 2026-09-06.

- Offline native build: CMake 3.31.6 + Ninja 1.11.1 + Zig 0.14.1 completed
  `voltra_simulator.exe` and `packet_inspector.exe`.
- Offline render smoke checks completed at 360, 466 and 480 pixels. The output
  is synthetic and does not connect to Bluetooth.
- Packet inspector accepted the documented base-weight state frame and rejected
  corrupt CRC, truncated input and non-hex input.
- Ryan starter reference compiled successfully for `elecrow21_ui` using
  PlatformIO 6.1.19, Espressif32 6.12.0, Arduino framework, NimBLE 2.3.6 and
  Arduino GFX 1.4.7. It used only the template dummy address in ignored
  `LocalConfig.h`; no upload was performed.
- Existing upstream checks passed: 203 packet vectors and the pinned SDK check.
- Existing Node SDK protocol suite passed: 229 tests in 10 files. No radio was
  used.

This baseline does not validate the unidentified physical board, VOLTRA firmware,
BLE handshake, resistance behavior, safety readiness or Twin semantics. Those
remain explicit future milestones.

## Five-screen update — September 7, 2026

- Native CMake/Ninja/Zig build passes for the expanded application and simulator.
- LVGL offscreen renders completed for Devices, Main, Magic, EO, Stairs and RP
  at 360, 466 and 480 px; additional loaded, KG, Twin and no-MTN renders completed.
- Visual review corrected small-screen RP header overlap and Magic label clipping.
- Native window launched and closed successfully. Interaction verification was
  blocked by the Windows capture helper's unsupported SetIsBorderRequired API.
- No new test code was created; no new interaction/safety pass is claimed.
- Physical BLE remains unimplemented and no physical motor was engaged.
