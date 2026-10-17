# Foundation baseline

## October 17 Normal-mode build and upload

- CONFIRMED: Windows simulator/packet inspector and both remote targets build.
  Normal uses 1,181,001 program bytes / 31,692 static RAM bytes; simulator uses
  927,001 / 21,236. Binary files are 1,181,360 and 927,360 bytes respectively.
- ELF symbol inspection: no mock classes in Normal, no NimBLE/physical-session
  classes in Simulator; each expected backend is present in its own image.
- Fifteen offscreen renders at 360/466/480 cover Devices, Main, Updates, About and
  ForgetDevice. Visual review of the 360 px screens confirms the new dialog and
  labels fit. These checks do not prove physical touch/gesture behavior.
- Existing pinned Ryan validators pass: 203 vectors, generated 5–230 LB range,
  593 modifier packets and SDK consistency. They validate reference bytes,
  not this project's new physical session. No new test code was created.
- CONFIRMED: added OTA region recovery verified against COM3 before writing.
  Both firmware images plus bootloader/partition/boot selection uploaded; all
  written hashes verified. A 15-second restart capture reports Normal/physical
  BLE, display initialized, touch ACK, encoder active and shared UI startup.
  No reset loop appeared in that interval. Existing old-coredump warning and
  first-use missing device-list NVS namespace did not prevent startup.
- Build log: ignored build/normal-mode-build.log. Renders: build/normal-mode-renders.
  Image copies, SHA256 manifest, upload log and startup capture:
  build/guition/2026-10-17-normal. Prior baseline: build/mode-switch-baseline.
- UNVERIFIED: mode-switch gesture on hardware, Simulator boot-return behavior,
  physical device discovery/approval, remembered devices across board reset,
  and all resistance/real-telemetry behavior. Physical training implementation
  remains incomplete; see NORMAL_MODE_2026_10_17.md for the exact blockers.

## October 17 feedback build

- CONFIRMED: final Windows simulator/packet inspector and `guition_k5_offline`
  builds pass. Embedded program usage 915,553 bytes; static RAM 21,760 bytes.
- 39 final offscreen renders at 360/466/480 px include device/main/editor/preset
  layouts, RP values, 460 LB Twin, fractional KG, the 81% endpoint, and no-headroom
  OFF. Visual review corrected number/status overlap and endpoint margins.
- Existing pinned Ryan packet and SDK-consistency validators pass (203 vectors,
  generated 5–230 range, 593 modifier packets). No new automated test code.
- Prior executable preserved in ignored `build/native/voltra_simulator-before-oct17.exe`.
  Final firmware copies and SHA256 manifest: ignored `build/guition/2026-10-17/`.
- After the user connected the remote, CONFIRMED: COM3 upload completed with
  every written image hash verified. A captured restart initialized the display,
  acknowledged touch, enabled encoder inputs and started the shared UI; no reset
  loop appeared during the 10-second capture. Physical input feel remains
  UNVERIFIED. Real BLE, wire capability discovery and Twin routing remain
  unimplemented/unverified; these are offline artifacts.
- Implementation/evidence: UI_FIXES_2026_10_17.md and WEIGHT_LIMITS.md.

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

## October 11 revision

- CONFIRMED: Windows native build passes. Pre-edit simulator retained locally.
- 45 offscreen renders completed across 360/466/480, including two live dropset
  layouts, all new settings pages, New preset, KG, Twin and unsupported MTN.
- Visual inspection corrected label coordinates, small-screen layout and footer
  spacing. Existing-preset/override interactions still need manual verification.
- Native Windows capture again failed twice with the previously documented
  SetIsBorderRequired error. No interaction test pass is claimed.
- Existing external reference directories are present but their validator source
  files are absent in this environment; upstream validators were not rerun.
- User approved unit selection/immediate editing; the resulting per-modifier
  model, controller guards and pickers build successfully. Additional final
  editor renders cover percent/LB/KG and unsupported-percent profiles.
- Physical network, update, haptic and BLE adapters remain unavailable.
