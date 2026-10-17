# Constants and configurable policies

No device address, serial or key is hard-coded. Board pins stay in their board
profile. Nominal firmware ceilings stay in the protocol policy, with reported
effective limits preferred. Necessary constants and their provenance:

| Location | Values | Reason / change policy |
| --- | --- | --- |
| protocol/VoltraGattProfile.h | Three UUIDs | Cross-source GATT evidence; use a new profile only if discovery proves incompatibility |
| protocol/FrameParser.h | Magic, CRC seeds/polynomials, 13–511 byte range, protocol ID | Wire format evidence; change only with captures and regression coverage |
| protocol/Parameters.h | Small ID allowlist | Canonical parameter catalog/wire observations; not permission to write |
| controller/VoltraController.h | 2,000 ms stale display timeout | Provisional configurable UI policy; not an approved physical safety threshold |
| controller/VoltraController.h | 3,000 ms command timeout | Offline confirmation timeout; production timing unvalidated |
| simulator/MockVoltra.cpp | 50 lb, 80% battery, modes 4/5 and workout state 1 | Explicitly synthetic scenario data, not defaults for real devices |
| simulator/MockTransport.h/.cpp | 100 ms reporting, 7-byte fragments | Synthetic transport behavior to exercise reassembly |
| simulator/MockTransport.h/.cpp | 40 ms confirmation delay | Responsive synthetic behavior, not a measured BLE latency |
| protocol/WeightLimits.h | 1.x 200 LB / 90 KG, 2.x 230 LB / 104 KG; Twin 400/180 or 460/208 | Report-first fallback; provenance, peer gates and inference limits in WEIGHT_LIMITS.md |
| state/WeightUnits.h | 1 tick = 1 LB or 0.5 KG; exact conversion factor 0.45359237 | Presets round nearest; limits round down |
| ui/UiController.cpp | Intervals below 130 ms: 5 ticks; below 55 ms: 10 ticks | Tunable rotary acceleration; direction reversal/navigation reset |
| controller/TrainingApplication.cpp | 250 ms pending hint delay | Keep brief confirmation waits from flashing messages; no state optimism |
| simulator/SimulatorApp.h and native hosts | Supported 360/466/480 sizes, 16 ms UI timer | Desktop validation scope, configurable per run via --size |
| simulator/main_mac.mm | 10 trackpad points per encoder step; momentum ignored | Tunable Mac input policy; arrow keys provide exact steps |
| tools/Build-Desktop.sh | macOS deployment target 12.0 | Initial Mac target, native architecture by default; Mac compile/runtime validation pending |
| ui/UiController.cpp | Cyan/pink palette; percentage geometry; scaled 48 px weight font; 41 markers over 240 degrees | User mockup reference; labels/progress follow the current limit; physical legibility validation still needed |
| ui/UiController.cpp / lv_conf.h | 12/14/18/28/36/48 px fonts; four device tiles per touch page | Readable native layout at 360/466/480 px; no rasterized UI text |
| HardwarePlatform / DesktopHardwarePlatform | Optional battery reading; desktop reports synthetic 75% | Remote battery, not VOLTRA battery; unknown when no hardware reading exists |
| controller/TrainingApplication.cpp | 5,000 ms discovery refresh; four saved connections | Revised user flow |
| ui/UiController.cpp | 650 ms hold; width/40 motion cancels tap; width/8 horizontal swipe | Tunable gestures; handled hold suppresses release click, ordinary buttons accept slow taps |
| state/TrainingState.h | 300% modifier cap, additive combined envelope | Revised user flow; physical representation needs verification |
| state/TrainingState.h and TrainingApplication.cpp | Stairs -2; RP 2 drops, 50%, 1 s; 80% cap; 0–9 drops, 1–9 s hold; reduction floor 1 | Revised defaults and single-digit field policy |
| simulator/MockTransport.cpp | Auto Load steady hold 3,000 ms | Synthetic device-side behavior modeled after official UI documentation |
| platformio.ini / setup tooling | Pinned compiler/framework/library releases | Reproducibility; review upgrades as separate changes |

Wire peer IDs and the Weight Training reported-state mapping remain localized in
the decoder/controller. They must be profile-driven if a verified device differs.
There is no fixed 480x480 application geometry or guessed Twin arithmetic.
