> Current UI and simulator policy: see [October 4 flow](APP_FLOW.md).
> Older dated entries below are historical where contradicted by that revision.

# Constants and configurable policies

No device address, GPIO pinout, serial, key or physical maximum is hard-coded.
Necessary fixed values are named and their meaning is documented:

| Location | Values | Reason / change policy |
| --- | --- | --- |
| protocol/VoltraGattProfile.h | Three UUIDs | Cross-source GATT evidence; use a new profile only if discovery proves incompatibility |
| protocol/FrameParser.h | Magic, CRC seeds/polynomials, 13–511 byte range, protocol ID | Wire format evidence; change only with captures and regression coverage |
| protocol/Parameters.h | Small ID allowlist | Canonical parameter catalog/wire observations; not permission to write |
| controller/VoltraController.h | 2,000 ms stale display timeout | Provisional configurable UI policy; not an approved physical safety threshold |
| controller/VoltraController.h | 3,000 ms command timeout | Offline confirmation timeout; production timing unvalidated |
| simulator/MockVoltra.cpp | 50 lb, 80% battery, modes 4/5 and workout state 1 | Explicitly synthetic scenario data, not defaults for real devices |
| simulator/MockTransport.h/.cpp | 500 ms reporting, 7-byte fragments | Synthetic transport behavior to exercise reassembly |
| simulator/MockTransport.h/.cpp | 350 ms command delay; device examples 200 LB, 90 KG, 400 LB | Mock behavior only; no physical capability inference |
| simulator/SimulatorApp.h and native hosts | Supported 360/466/480 sizes, 16 ms UI timer | Desktop validation scope, configurable per run via --size |
| simulator/main_mac.mm | 10 trackpad points per encoder step; momentum ignored | Tunable Mac input policy; arrow keys provide exact steps |
| tools/Build-Desktop.sh | macOS deployment target 12.0 | Initial Mac target, native architecture by default; Mac compile/runtime validation pending |
| ui/UiController.cpp | Cyan/pink palette; percentage geometry; scaled 48 px weight font; 41 markers over 240 degrees | User mockup reference; labels/progress follow the current limit; physical legibility validation still needed |
| ui/UiController.cpp / lv_conf.h | 12/14/18/28/36/48 px fonts; four device tiles per touch page | Readable native layout at 360/466/480 px; no rasterized UI text |
| HardwarePlatform / DesktopHardwarePlatform | Optional battery reading; desktop reports synthetic 75% | Remote battery, not VOLTRA battery; unknown when no hardware reading exists |
| controller/TrainingApplication.cpp | 5,000 ms discovery refresh; four saved connections | Revised user flow |
| ui/UiController.cpp | 650 ms long press | Tunable gesture policy; short-click event excludes long holds |
| state/TrainingState.h | 300% modifier cap, additive combined envelope | Revised user flow; physical representation needs verification |
| state/TrainingState.h and TrainingApplication.cpp | Stairs -2; RP 2 drops, 50%, 1 s; 80% cap; 0–9 drops, 1–9 s hold; reduction floor 1 | Revised defaults and single-digit field policy |
| simulator/MockTransport.cpp | Auto Load steady hold 3,000 ms | Synthetic device-side behavior modeled after official UI documentation |
| platformio.ini / setup tooling | Pinned compiler/framework/library releases | Reproducibility; review upgrades as separate changes |

Wire peer IDs and the Weight Training reported-state mapping remain localized in
the decoder/controller. They must be profile-driven if a verified device differs.
There is no fixed 480x480 application geometry or guessed Twin arithmetic.


Modifier priority (latest user revision): preserve active modifiers and reduce
base. Percentage envelope arithmetic uses hundredths in int64_t; base and
absolute modifier choices round down to the reported weightStep. Percentage
intent is not rounded to even percentages. Twin mock step remains 2 lb; real
steps/maxima require capability evidence. The September 27 revision removes derived LB/% display equivalents. Each
modifier has an independent integer value and percent flag. One detent means
1% in percentage mode, including Twin; absolute mode uses weightStep. Unit
conversion happens once on explicit selection, rounding down, then remains
fixed until edited. Mixed-unit limits reserve absolute values first, divide
remaining capacity by (100 + percentage sum), then floor base to weightStep.
Unknown independent-unit capability remains disabled for physical sessions.


September 30 supersession: modifier editing/enabling no longer lowers base.
modifierEditMaximum uses the remaining combined envelope at the current base,
then floors to whole percent or weightStep. Base adjustment retains its existing
modifier-preserving cap. Back placement derives from display dimensions.


Stable modifier display scale: 300% or reported device maximum in absolute
units. Never use remaining headroom as a ring denominator. Limit orange is
0xff9838, used when fresh known state has base >= its step-aligned allowance.
Shared unit-view preference is presentation state for this process, not a
physical device global-unit command. Base always uses reported LB/KG and
weightStep; modifier/Stairs conversions use base and truncate to whole numbers.
Base number/ring/scale use limit orange at the same allowance; unloaded number
uses darker orange 0x9a5b24 to preserve load-state brightness. Underlying
settings remain confirmed; only explicit editor Apply/Back changes a modifier.


Base display conversion uses exactly 0.45359237 KG per international pound.
LB and KG presentation round to whole numbers, including converted values and
scale labels. No fractional readouts are displayed. No conversion is written
back to confirmed or requested state. Base unit buttons are 24% of central-disc
width by 14% height, at 59% height; the hint starts at 75%. Selection is local
presentation state, defaults to the device unit and survives screen navigation.


October 1 temporary maximum: default Garage simulator uses 400 LB, as requested
by the user (MockVoltra::maxWeightLbs). This is a synthetic preview limit, not a
claim about any model. Other mock profiles retain their individual maxima.
Base plus active modifiers must still fit this limit; it does not set base to
400 automatically. Scale top label is the midpoint, right endpoint the maximum.


## October 4 policies

- A/E maximum 230 LB, C 90 KG (whole-KG ceiling below 200 LB), B/D explicitly
  share one 460 LB session. Single step 1 native unit, Twin step 2 LB. All are
  synthetic requested scenarios; real capability limits are not inferred.
- E starts offline. C silently accepts but never applies base-changing writes.
- EO green: 0x43e58d. Base text: white loaded/fresh, grey otherwise; limit orange
  0xff9838 belongs to indicators only. No base unit conversion or selector.
- Active modifier floor: 1% or weightStep. Base floor: weightStep. Modifiers
  reserve their requested values by reducing base, rounded down to weightStep.
- One common modifier interpretation. Stable paired draft representations avoid
  cumulative rounding; numeric edits refresh only the edited modifier's anchor.
- Pending message delay 3000 ms; unanswered intent tracking expires at 8000 ms
  with “Waiting” retained. Exact settings/load state must match a fresh report.
- Major marks every 50; minor spacing 1 for maxima <=10, 5 for <=100, else 10.
  Exact endpoints and selected value included. 101 allocated ticks cover current
  signed +/-460 mock scales. Layout derives from hardware display metrics.
- Stairs zero-center signed scale, native weightStep in both directions. Positive
  changes cap at baseMaximum; negatives stop at weightStep. RP is controller
  managed and stops after the configured number of confirmed drops.
