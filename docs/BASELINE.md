# Foundation baseline

## October 17 Normal-mode build and upload

- CONFIRMED: Windows simulator/packet inspector and both remote targets build.
  Normal uses 1,181,369 program bytes / 31,692 static RAM bytes; simulator uses
  927,001 / 21,236. Binary files are 1,181,728 and 927,360 bytes respectively.
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
- Final review correction: invalid unit/motor/workout reports clear normalized
  state immediately, preserving the unknown-state unload target. Report revisions
  remain monotonic when valid state returns. Both images rebuilt and reflashed.

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

## October 18 physical connection follow-up

- Normal 1,195,213 program bytes / 31,768 RAM and isolated Simulator 929,113 /
  21,236 build successfully. Both images flashed with verified esptool hashes.
- Windows native build and existing Devices/Main/Twin 360 px renders pass.
- Physical discovery and a radio link with MTU 247 were captured, alongside
  repeatable HCI establishment failures (574). No resistance was commanded.
- GATT discovery/subscriptions, acceptance after user approval and read-only state
  were captured at approximately ten feet with the ignored local reference profile.
  Generated local identity failed the same corrected handshake comparison. Identity
  provisioning remains unresolved; default builds are not a validated configuration.
- Live readback includes 30 LB base and a device-reported 230 LB maximum. Motor 0/1
  decoding, prioritized reads and unload command 4/readback 0 confirmation are fixed.
  EP1.0 is no longer misidentified as app firmware. Physical training is incomplete.
- Final binary sizes: Normal 1,195,584 bytes; isolated Simulator 929,472 bytes.
- Images/manifests and logs remain ignored locally; see
  [physical connection evidence](LIVE_CONNECTION_2026_10_18.md).

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

## October 18 unloaded weight path

- Normal builds: 1,196,269 program bytes / 31,768 static RAM; binary 1,196,640 bytes.
  Simulator remains isolated: 929,113 program bytes / 21,236 static RAM.
- Flash/recovery/partition checks and esptool hashes passed.
- Accepted session, fresh core state and the observed standalone profile are
  physically confirmed. User-driven LB changes were returned by VOLTRA and
  confirmed on both screens, with motor state still unloaded. Rapid-input
  freshness and intermittent startup link failures remain tracked limitations.
- No autonomous weight or load action was issued; no new test code was created.
- Final images/manifests are retained in build/guition/2026-10-18-unloaded-weight.

## October 18 single-device controls build

- Normal: 1,204,833 program bytes / 31,768 static RAM; isolated Simulator:
  932,033 program bytes / 21,236 static RAM. Both final builds pass. Native
  Windows build passes after the shared style/alignment and intent changes.
- Existing Main renders pass at 360/466/480, plus KG Magic at 360. The final
  Main/KG 360 BMPs are byte-identical to the visually inspected preceding renders.
  This verifies appearance, not interaction or safety behavior.
- Final bounded flash passed recovery/partition checks and esptool hash checks.
  Images and SHA256 manifest: build/guition/2026-10-18-single-controls.
- Final capture: repeated discovery found Black, connection attempt one succeeded,
  and remembered application acceptance completed. Modern percentage fields
  returned 2000/2000; background information and activation reads now progress.
  Black reported Weight Training inactive (20400 = 0), so the application correctly
  withheld normalized training controls. A physical settings test still needs
  the user to reopen Weight Training. No motor/resistance command was automated.
- Existing reference validators passed; no new test code was added. UI interaction,
  modifier/KG/preset changes and long-duration connectivity are not marked passed.
  Loading, Auto Load, Mountain, mixed modifier units and dropsets remain blocked;
  real Twin verification is explicitly deferred by the user.

## October 24 connection and control fixes

- CONFIRMED builds: Normal 1,207,569 program bytes / 31,768 static RAM;
  isolated Simulator 934,365 / 21,236. Windows native build also passes.
- Both final images were installed after recovery-image, partition-boundary and
  esptool hash checks. Snapshot: build/guition/2026-10-24-controls-remember.
  Normal binary: 1,207,936 bytes, SHA256
  384CD8F486329DCE6E36637DCE46FD1152ED6F041D4A0677DDB4F4EC5C78E604.
  Simulator binary: 934,736 bytes, SHA256
  082099BA4BE89FEF1988B0490A2B1EA54503D343AEE0AA86F9D8707C1AA5EBBF.
- CONFIRMED on Black: automatic idle-to-Weight-Training entry returned mode 1
  while remaining unloaded at 57 LB. Final installed-image startup reconnected
  on attempt two, accepted the remembered remote and returned 60 LB unloaded,
  mode already 1, IC 20%, EO off and reported maximum 230 LB. Initial link error
  574 remains intermittent. No autonomous motor or resistance-setting test ran.
- Existing reference validators passed: 203 SDK packets, weights 5–230,
  593 modifier packets and pinned SDK/bootstrap consistency. These checks do
  not establish physical compatibility or new transaction correctness.
- Existing 360 px renders inspected: synthetic Twin at 500 LB and 226 KG,
  standalone unknown Twin status without a Twin banner, and a 60% Magic editor.
  The final Twin-500 render matches the preceding inspected BMP byte-for-byte.
  This verifies layout only. No new test code was created.
- The user reports improved controls. The follow-up remembered-value fix is
  installed; physical disable/reopen verification is pending. Loading, Auto Load,
  Mountain, mixed modifier units, loaded edits and dropsets retain their existing
  gates. Real Twin checks remain deferred at the user's request.

## October 24 Mountain and progressive dial follow-up

- Normal: 1,209,113 program bytes / 31,768 static RAM; Simulator: 935,477 /
  21,236. Both firmware builds and native desktop build pass. No new test code.
- Existing packet/upstream validators pass (203 SDK and 593 modifier packets).
  Existing 360 px Main/Stairs/RP renders inspected; final Main matches byte-for-byte.
- Bounded flash passed recovery, partition and esptool hash checks. Snapshot:
  build/guition/2026-10-24-mountain-controls. Normal binary: 1,209,472 bytes,
  SHA256 53C4FCA716EB83F8AB00BC3D53D585F01C53F55C28131130C0D830D2C884B8DF.
  Simulator binary: 935,840 bytes,
  SHA256 3B7688D6007ED1D527DEEE65C06575E0D954531241971AD4C2328CBF17746F4C.
- User confirmed remembered modifier values work. Final startup connected on
  attempt one and returned Mountain=1, percent display=1, 25% Mountain, 3% EO,
  60 LB unloaded and maximum 230. New Mountain/dial interactions are pending
  user verification. Dropset configuration is visible, but actual real drops
  remain disabled. See [control evidence](CONTROL_FEEDBACK_2026_10_24.md).

## October 24 units, loading, live reps and rest follow-up

- Native, Normal and isolated Simulator builds pass. Normal program/static RAM:
  1,214,589 / 31,768 bytes; Simulator: 936,869 / 21,316 bytes. Existing reference
  validators passed earlier in this change (203 SDK / 593 modifier packets).
  No new test code was created; these passes do not verify dropset behavior.
- Both images installed with recovery-region/partition checks and esptool hash
  verification. Snapshot: build/guition/2026-10-24-rest-final. Normal binary:
  1,214,960 bytes, SHA256
  EB1E6779B591D8672C8F31CFE9E6D1252F88B4B45FB7D1A1F98B72AF55664F5A.
  Simulator binary: 937,232 bytes, SHA256
  13FF7FB6180992FCB20F24F91A0BA8B6CBC1D02B1A1521DCE4ACDF1D5F00BD65.
- User confirmed %/LB editing, remote-tap loading, return to Main, post-set
  counts and first-rep tracking. Restoring subscription 20867=0 with 10 Hz
  restored AA81 live reports; custom masks produced an incompatible AA94 layout.
- Local rest fallback is explicitly authorized and implemented, anchored only
  to a device set-close report. On-device timer observation is pending. Audio/
  vibration has no validated Guition driver; the alert wakes/shows the screen.
  Stairs/RP reductions, KG and Auto Load triggering still need physical checks.
  Real Twin remains deferred. See [full evidence](DROPSETS_UNITS_2026_10_24.md).
