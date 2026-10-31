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

## October 24 idle-force display follow-up

- User confirmed the preceding rest timer works. Captured idle AA81 force noise
  (0–36 tenths LB, phase 0, stationary 148 mm cable) explained the jumping number
  while the confirmed base remained 10 LB. Live now uses selected base outside a
  fresh active rep, retaining raw force for the controller. No control writes or
  counter/rest logic changed.
- Normal, isolated Simulator and native builds pass. Existing 360 px idle Live
  renders inspected at 10 LB and 10.5 KG. No new test code was created; these are
  presentation/build checks, not a physical retest of the updated display.
- Snapshot: build/guition/2026-10-24-idle-force. Normal binary: 1,215,024 bytes,
  SHA256 DCCC3CD41647F5A58AA76289E67E6F1AA74694FFEC117410D392CF27F18CCECE.
  Simulator binary: 937,296 bytes,
  SHA256 C287B896DC9ACCECDA96D9FD07B41DA09F9CCEBA753EE798E4D1B42A78A25290.

## October 24 dropset boundary and dial rounding follow-up

- Captured Black's failing Stairs trial: completed set 10 / rep 2, then active
  moving set 11 / rep 0 and completed rep 1, with no intervening unload. Drops
  now establish the next set's baseline from that observed start instead of
  comparing with the old set. Within-set gaps/resets and missing start evidence
  still stop execution. Physical reductions require the user's follow-up trial.
- Coarse rotary gears now all land on 5/10 whole-unit grids and retain the grid
  through slowdown. Pause/reversal restores fine input. The gentle gear uses
  two detents per 5-unit step to ease the ramp. Limits still override the grid.
- Native, Normal and isolated Simulator builds pass. Existing upstream validators
  pass (203 SDK packets / 593 modifier packets). No new test code was created;
  these checks do not prove physical drops or rotary feel.
- Snapshot: build/guition/2026-10-24-drops-rounding. Normal binary: 1,215,360 bytes,
  SHA256 82C989F11C7CEDD61D9CEF767D01EAABE6ABE4697B34E9122260C4BC028B1EDA.
  Simulator binary: 937,648 bytes,
  SHA256 594B196BA120C83218DE4645717B4C612853CDCB5C0DB159064E6698C4CAEEB9.

## October 24 confirmation across weight-induced set boundaries

- User and capture confirm the first Stairs change from 10 to 8 LB. Black closed
  statistics set 12 after that rep, stayed in active workout status, and began
  set 13. The controller rejected that start before checking the drop's readback.
  It now observes the start without treating it as confirmation or an error;
  a subsequent drop still requires the preceding change to be confirmed.
- RP's quota persists across device set-number changes until unload/rearming.
  Consecutive physical Stairs reductions and RP remain pending user verification.
- Native, Normal and isolated Simulator builds pass, as do existing upstream
  validators (203 SDK packets / 593 modifier packets). No new test code.
- Snapshot: build/guition/2026-10-24-drops-confirmation. Normal binary: 1,215,312
  bytes, SHA256 319E6BF284AECC126FC41BF7F17799DE5EA5EA7CE326D55DEDBD976FD650E5BA.
  Simulator binary: 937,584 bytes,
  SHA256 126D1386849029725A0D264CEEA47C96BF89C0F4C6B3404A2F0968C2CDEBEBF8.

## October 24 selected-weight display and device minimum

- User/capture confirm consecutive drops 10 -> 8 -> 6 LB. The 4 LB request
  returned 5 LB, consistent with the official minimum; the old code waited for
  4 and stopped at the next completed rep. Profile minimums now bound manual
  base edits, Stairs/RP targets and previews. At Black's 5 LB floor, no further
  descending write is queued and Live shows MIN 5 LB.
- Latest user requirement: Live always displays fresh confirmed selected base,
  including active reps. Measured-force noise no longer changes that number.
  Physical observation of these new changes is pending; KG wire and real Twin
  minimums remain outside the confirmed Black LB evidence.
- Native, Normal and isolated Simulator builds pass. Existing packet/upstream
  validators pass (203 SDK / 593 modifier packets). Existing 360 px renders
  inspected: ordinary 10 LB Live, Stairs 6 / NEXT 5, Stairs MIN 5 LB, and RP
  MIN 2 KG. These are presentation checks, not physical automation tests; no
  new test code was created.
- Snapshot: build/guition/2026-10-24-base-display-floor. Normal binary: 1,217,760
  bytes, SHA256 7C561561E56585244E549509B3AC641ECBDB929598FDB26612F8E69074A0EAC5.
  Simulator binary: 940,176 bytes,
  SHA256 AE88FB0FAE288B38EB435F4FC625D5226A8AF56C3338065C6D01103534268273.

## October 24 grouped dropset counters and session recovery

- Black's capture shows 15 -> 13 -> 11 -> 9 -> 7 -> 5 LB with separate device
  statistics segments. Local presentation now sums observed reps across those
  segments into one loaded dropset, including all RP drops. Raw machine state
  and the event stream used to authorize reductions remain separate.
- The confirmed floor/RP quota ends automatic execution until unload/rearming.
  Further reps still count; no further reduction trigger or minimum write is
  attempted. A subsequent manual dial adjustment switches the recipe off.
- The same capture shows Black exiting Weight Training unloaded while GATT
  remained connected; the exact device error is unknown. Recovery now cancels
  intent and returns to selection after fresh mode-exit/unload evidence, retaining
  the reason. New numeric write-request diagnostics support future investigation.
  No automatic re-entry/load/replay is part of recovery.
- Native, Normal and isolated Simulator builds pass. Existing reference validators
  pass (203 SDK packets / 593 modifier packets). Existing 360 px Stairs-floor and
  RP Live renders inspected; the final floor render matches the inspected image.
  These are build/layout/reference checks, not physical counter/recovery tests.
  No new test code was created; user verification remains pending.
- Snapshot: build/guition/2026-10-24-drop-progress. Normal binary: 1,219,152 bytes,
  SHA256 0598C98840969D85F312DBC446881FF5B13EAC651CDEF8C3729BE160A7FA59E7.
  Simulator binary: 941,232 bytes,
  SHA256 0900A3751B7FEE55260CFE7E2F8CDDDBC64277132F185231A6A67CCA9327490C.
- Installed with recovery/partition bounds and esptool flash-hash verification.
  Black reconnected on the second link attempt, accepted the remembered remote,
  and reported fresh 5 LB, unloaded, Weight Training selected, maximum 230 LB.
  No resistance command was issued by the installation/connection check.

## October 24 RP timing and genuine between-set rest

- User/capture confirm the preceding grouped counting and floor continuation.
  Black's status 3 between-set rest now ends the logical dropset even while
  loaded. The next observed moving set starts a new rep total and RP quota.
  Weight-induced statistics closes while status 2 remains active keep the group.
- RP pauses through brief motion gaps, discards partial holds, and requires
  fresh movement plus a new full hold. A gap beyond 2000 ms still stops it.
  The previous permanent stop at the first 500 ms freshness failure explained
  why no second drop was sent in the captured trial.
- Automatic drops have a dedicated base-only session operation, bypassing the
  manual rotary quiet period and one background poll. Confirmed Magic/EO
  settings are preserved; loaded preparation emits only the base parameter.
  Live highlights a settled target for two seconds. The K5 has no verified
  sound/vibration implementation, so this build's cue is visual.
- The old capture already showed base=7 before movement resumed, while measured
  force remained near 15 LB. Immediate physical application during a hold remains
  UNVERIFIED; the queue change is not evidence that this device behavior is fixed.
  No load reassertion, retry, bypass, or accessory rewrite was introduced.
- Native, Normal and isolated Simulator builds pass. Existing validators pass
  (203 SDK / 593 modifier packets plus pinned upstream checks). Existing 360 px
  RP renders inspected at 7 LB / NEXT 5 and MIN 5 LB. These are build/layout/
  reference checks, not physical verification of rest reset, RP continuation,
  cue timing or modifier-on drops. No new test code was created.
- Snapshot: build/guition/2026-10-24-rp-rest. Normal binary: 1,220,864 bytes,
  SHA256 DEE9909A9F860DFB44CA99B3E0FB5226141345AB49A4D83139178ADD9A0391BD.
  Simulator binary: 942,896 bytes,
  SHA256 99AD93317A7C34D8D0E155B8DE588BB039B0269ED507B3B27E87BC2393BE47B7.
- Installed with verified recovery/partition bounds and flash hashes. Normal
  boot initialized display/touch/encoder and continued BLE discovery; Black
  was not detected in the initial post-install scan window. The connection-only
  diagnostic did not issue any load or resistance command. Physical retest is
  pending; the read-only capture is retained locally.

## October 31 motor report variant on connection

- CONFIRMED (capture): Black accepted the BLE/application session and reported
  Weight Training, 42 LB, motor=256, 25% IC/EO and maximum 230 LB. Whole-word
  comparison of the motor register made the prior build clear its training
  report and show an unknown-state error despite the working connection.
- The physical decoder now uses the low-byte mode for the supported high-byte
  0/1 variants, following the pinned Android/ESP32 references. Raw uint16 values
  remain in diagnostics. Unknown upper bits or modes remain blocked. Readiness,
  load/unload confirmation, mode entry and exit use this same interpretation;
  outgoing commands and required explicit-load guards are unchanged.
- Normal and isolated Simulator builds pass; the Simulator binary is unchanged.
  Existing validators pass (203 SDK / 593 modifier packets and pinned upstream
  checks). No new test code was created; regression cases are tracked in TESTING.
- Installed with recovery/partition checks and verified flash hashes. Black
  connected on the second radio attempt, accepted the remembered remote and
  again returned raw motor=256. The updated application reported connected=1,
  accepted=1, fresh=1, weightTicks=42, loaded=0, with the error cleared. No load,
  base, accessory or mode write was needed for this connection verification.
  Physical loading with the prefixed motor states remains untested.
- Snapshot: build/guition/2026-10-31-motor-mode. Normal binary: 1,221,120 bytes,
  SHA256 D96063FCFFAD3BE3C8461CFA9C8097EEA362D36B9F2656C438F0CCDC01986C20.
  Simulator binary: 942,896 bytes,
  SHA256 99AD93317A7C34D8D0E155B8DE588BB039B0269ED507B3B27E87BC2393BE47B7.

## October 31 pending firmware update and unloaded dial controls

- CONFIRMED (user): the earlier display error cleared, but turning the dial
  produced "Unload / wait for fresh control state". The follow-up capture
  confirmed a working connection with 42 LB unloaded, 25% IC/EO, 230 LB max and
  OTA_STATUS=261. The user identified an update ready to install on Black.
- The prior `OTA_STATUS==0` guard incorrectly blocked that pending-update
  state. The physical profile now accepts fresh 0 or exactly 0x0105, the
  user-correlated pending value. Missing/stale status and other nonzero values
  remain blocked. Feedback now identifies update status when that is the cause.
  No update command, preference change, force-load or control replay was added.
- Normal and isolated Simulator builds pass; the Simulator binary is unchanged.
  Existing validators pass (203 SDK / 593 modifier packets and pinned upstream
  checks). No new test code was created; pending regression cases are tracked.
- Snapshot: build/guition/2026-10-31-pending-update. Normal binary: 1,221,312 bytes,
  SHA256 A39D7C733153FE75762C83EDC5D2F9FB06229EB6AE70D6A8A4A4D9BAB5892F28.
  Simulator binary: 942,896 bytes,
  SHA256 99AD93317A7C34D8D0E155B8DE588BB039B0269ED507B3B27E87BC2393BE47B7.
- Installed with recovery/partition validation and verified flash hashes. After
  initial radio establishment failures, the connection-only retry succeeded.
  Black again reported OTA_STATUS=261, base=42 LB and unloaded; the remote's
  accepted/fresh session cleared its control-state message. The installation
  check sent no resistance, mode, modifier or update command.
- CONFIRMED (user follow-up): with Black unloaded and its update still pending,
  slow dial adjustments and the return adjustment followed on both screens
  without the error. This verifies unloaded base editing on this build; loading,
  RP and other OTA states are outside this particular check.

## October 31 dropsets after Auto Load and control taps

- CONFIRMED (source): the existing live-edit guard only accepted motor 1,
  despite decoding completed Auto Load (35/14) as loaded. The pinned kirby
  capture documents that Auto Load stays in motor 35 after loading. The
  guarded base-only path now accepts fresh 35/14 as well; 11-13 cannot run
  drops. No motor reassertion, safety bypass, retry or Auto Load setting write.
- The local dropset recipe stays selected through countdown. A new accepted
  explicit Auto Load clears execution counters/holds, not the recipe. Main
  retains its values and uses amber while live execution is unavailable;
  tapping an enabled recipe disables it even during countdown.
- Control taps now tolerate drift up to 5% of the short display side (18 px
  on K5), instead of 2.5%/9 px. Device/preset pickers retain strict swipe
  rejection. Small modifier hit-area extensions fill caption/circle gaps
  without overlapping neighboring tiles. Diagnostics distinguish recognized
  touch actions from drag cancellation; session logs include recipe/Auto Load/
  live-edit state. Physical touch improvement remains UNVERIFIED.
- Native, Normal and isolated Simulator builds pass. Existing validators pass
  (203 SDK / 593 modifier packets and pinned upstream checks). Existing 360 px
  main-screen Stairs/RP renders inspected. No new test code was created.
  These checks do not verify physical drops after Auto Load or touch behavior.
- Snapshot: build/guition/2026-10-31-autoload-touch. Normal: 1,222,192 bytes,
  SHA256 81388D609427FE8743102D9E42022BE17E69CB6F30D44DAA4DF761552BA0E57C.
  Simulator: 943,024 bytes,
  SHA256 7730C79155F6DB979049A0FD54ADA19C476FBB5E4A7A833ED370D4BE7C03124B.
- Installed with recovery/partition checks and verified flash hashes. Physical
  Auto Load/drops and single-tap checks are pending human observation.
- Post-install connection verification hit the pre-existing BLE establishment
  failure (574); one remote-only restart and connection-only retry recovered.
  Black accepted the remembered remote and returned 15 LB unloaded, 230 LB max,
  Auto Load enabled/safe, status 0, mount 2 / void lock 6. No motor, base or
  modifier write was issued by this connection check.

## October 31 independent touch capture

- CONFIRMED (user): Auto Load with drops now works, but Drops Apply and other
  controls still sometimes need a second tap. The unloaded diagnostic trial
  logged one recognized Apply and successful local activation, no refusal.
  INFERRED cause: a short gesture can fall inside main-loop BLE/render work;
  increasing the movement tolerance alone cannot recover an unsampled tap.
- K5 now captures input independently in a board-local task, using a bounded
  queue consumed by the shared UI. No application/BLE/LVGL calls run on that
  task. Press/release/movement order is retained; stale (>350 ms), overflow or
  invalid input cancels rather than clicks. Screen changes discard old input
  and suppress held fingers until release. Explicit input reads refresh the
  UI clock; new diagnostics report delivery age and Apply refusals.
- Native, Normal and isolated Simulator builds pass. Existing packet/upstream
  validators pass (203 SDK / 593 modifier packets and pinned upstream checks).
  Existing 360 px Drops render inspected. No new test code was created. The
  desktop interaction comparison was unavailable because window screenshot
  capture timed out; no interactive native or physical touch pass is claimed.
- Snapshot: build/guition/2026-10-31-buffered-touch. Normal: 1,225,296 bytes,
  SHA256 8AB3ACF766DCA5B789E71C03A7AC31FECBCBA48640852B99722C8F4BBA867462.
  Simulator: 946,128 bytes,
  SHA256 2CB6D857572C3CCF9CB12D4B578E602850A58E060708B76A7A875C2A776BB3DA.
- Installed with verified recovery/partition bounds and flash hashes. Boot
  confirmed the buffered sampler started. Black connected on the first radio
  attempt, accepted the remote and returned fresh 15 LB unloaded with controls
  available. No resistance command was issued by installation/verification.
  User verification of repeated first-tap Apply remains pending.

## October 31 RP countdown and Auto Unload limit

- CONFIRMED (user): repeated first-tap Drops Apply works with 540ef06.
- Live now shows a vertical RP hold countdown after 250 ms, derived from the
  executor's hold window. Movement, invalid/stale motion, pending drop,
  rest/unload, floor and quota hide it. No separate UI trigger or device-time
  claim. Confirmed base/rep presentation and the weight unload target remain.
- Read-only Auto Unload holding-time parameter 21525 supplies an optional
  seconds value. Positive values cap RP at delay minus 1 s (maximum 9 s);
  absent/off preserves 1..9. A 1 s delay leaves RP unavailable. Apply, preset
  and ongoing execution guards enforce the same policy. No Auto Unload write.
  See RP_COUNTDOWN_2026_10_31.md for source evidence and scope.
- CONFIRMED (user): Black's holding time is currently 2 s. Bluetooth correlation
  after installation returned 21525=2; RP maximum resolves to 1 s.
- Native, Normal and isolated Simulator builds pass. A C++ LVGL flag-type build
  error was corrected before these successful builds. Existing validators pass
  (203 SDK / 593 modifier packets and pinned upstream checks). No new tests.
- Offline renders inspected at 360/466/480 px: hidden at 200 ms, 75%/50%/10%
  at 250/500/900 ms of a 1 s hold; hidden after a simulated confirmed drop.
  A 3 s hold renders half full at 1.5 s. Auto Unload 2 s editor shows max pause
  1 s; 1 s shows unavailable. These are mock/UI observations, not physical
  timing or protocol compatibility proof.
- Snapshot: build/guition/2026-10-31-rp-countdown. Normal: 1,228,080 bytes,
  SHA256 C2CBE408D5AC936748554698A6C89893B1960887097E43B31CCA0E20B892ECC1.
  Simulator: 948,736 bytes,
  SHA256 D16BF2A39B5313D6F2750DA3E0779D136E10B86241145F98F10BC52DB2DA1021.
- Installed source b6925de with verified recovery/partition checks and flash
  hashes. Black connected on the first radio attempt and returned fresh 15 LB
  unloaded, max 230 LB, Auto Load enabled/safe, status 0, mount 2 / void lock 6,
  and Auto Unload holding time 2. No resistance or Auto Unload configuration
  command was issued by installation/verification. Physical countdown and
  editor behavior await the user's live trial. Ignored capture:
  build/voltra-live/2026-10-31/rp-countdown-installed.txt.

## October 31 training follow-ups and phone Wi-Fi setup

- CONFIRMED (user): RP countdown and Auto Unload editor limit work on Black.
  User clarified dropset completion must unload and restore the original base,
  and only the configured rest-alert duration needs cross-device persistence.
- Implemented one-shot confirmed-unloaded base restoration for Stairs/RP,
  loaded modifier amounts (same curve/units), 5 s global rest-alert adjustment,
  and a password-protected phone Wi-Fi portal isolated from training/simulator.
  See TRAINING_WIFI_2026_10_31.md for evidence and remaining physical limits.
- CONFIRMED (capture): a tap above Stairs missed; another below it reached the
  overlapping unload status. Four captured Apply actions succeeded. Separated
  the status target, enlarged mode/Apply targets and kept cancellation logs.
  This does not prove every occasional missed Apply is resolved.
- Native, Normal and isolated Simulator final builds pass. Existing validators
  pass (203 SDK / 593 modifier packets and pinned upstream checks). No new
  automated test code. Final normal build uses 1,661,225 bytes of application
  flash and 56,036 bytes static RAM, inside the verified partition limits.
- Offline simulator renders: four Stairs reps at 50 produce 42, then explicit
  mock unload restores 50 with 1 set / 4 reps; RP 15 -> 7 restores 15 unloaded.
  These use the real application over mock reports, not physical packets.
  Automatic phase-3/4 unloading and loaded modifier edits remain physical
  trial items. Wi-Fi/Apply pages inspected at 360/466/480; final 360 Drops page
  separates mode/status/Apply. Native interactive capture timed out twice;
  only headless renders are claimed. Static browser portal inspected; phone
  connection, captive detection and credential persistence remain unverified.
- Snapshot: build/guition/2026-10-31-training-wifi. Normal: 1,661,584 bytes,
  SHA256 96896FBCEBCA5C862859D9E64CDB903B72BF9D47012F65AA46042B1ABD7EB8F9.
  Simulator: 953,120 bytes,
  SHA256 E3D210206E9E378C429D2505ABBD6F1F46561C2A2897C2285E233FB53D2EFB88.
  Installation and passive reconnection check are recorded separately below.
- Installed source 6a2c162 with verified recovery/partition boundaries and flash
  hashes. Black reconnected and accepted the remembered remote on the first
  attempt: fresh 70 LB unloaded, control path available, Auto Unload 2 s.
  No motor/base/modifier write was issued by the installation connection check.
  The new Wi-Fi namespace was absent on first boot (expected NVS NOT_FOUND);
  no new crash/reboot appeared during connection. The existing core-dump image
  size warning still appears at boot and is not evidence of a crash this run.
  Physical end-of-set restoration, loaded amount edits, phone join/persistence
  and repeated first-tap behavior remain pending user observation. Ignored log:
  build/voltra-live/2026-10-31/training-wifi-installed.txt.
