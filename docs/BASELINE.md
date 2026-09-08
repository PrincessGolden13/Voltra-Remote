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

## Mockup visual update

- Native build passes after adding the smaller/larger dial fonts and vector UI.
- All six layouts (including Stairs and RP) render at 360, 466 and 480 pixels.
- Additional KG, loaded Twin and unsupported-MTN layouts render successfully.
- Visual review corrected device-name wrapping, Magic footer clipping, and
  RP scale-label stacking. RP keeps all three values visible and active.
- Native interaction capture remains blocked as documented above; no new
  interaction test or physical validation is claimed.

## macOS host implementation

- CONFIRMED on Windows: CMake/Ninja/Zig build passes with the shared
  SimulatorApp runtime. All six layouts render at 360, 466 and 480 pixels.
- Sixteen of 18 BMP files match the saved mockup renders byte-for-byte. The
  other two are Main at 466/480, whose old references predate the previously
  committed unit/hint spacing adjustment; UI source is unchanged in this port.
- Mac AppKit host, native build/setup/launch scripts and portable memory/BMP
  handling are implemented. Build output is isolated in build/mac.
- UNVERIFIED: Mac compilation and interactive execution (including Retina,
  trackpad, window lifecycle and persistence). No Mac or Apple SDK is available
  in this Windows session; shell scripts could not be executed here either.
- No new test code, physical BLE implementation or physical validation added.
