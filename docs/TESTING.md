# Validation status and pending tests

The explicit instruction under REQUIRED TEST CASES defers new test code for now.
Existing upstream validators are run; no artificial coverage or passing CTest
placeholder is supplied. This is not a tested production control implementation.

## Existing upstream results

- PASS: Ryan `tools/check_packets.py`: 203 SDK packets, generated weight range
  5–230, all 593 pinned modifier packets. This checks bytes, not safe resistance.
- PASS: Ryan `tools/check_upstream.py <SDK checkout>`: exact pinned commit,
  identifiers, customized identity, init/STOP/SETUP/GO, weight table and guided
  capture consistency. Starter pins the current audited SDK commit.
- PASS: Node SDK `vitest run src/voltra/protocol/__tests__ --maxWorkers=2`:
  **229 tests in 10 files**, Vitest 4.1.11, Node 24.19.0. No radio was used.
  The bundled runtime lacked npm, so pnpm 11.19.0 resolved upstream package
  ranges with scripts disabled in the external checkout; this was not a frozen
  npm-lock install. The resulting pnpm lock remains with that checkout.

Android tests were inspected but not executed; no Android/JDK toolchain was
installed. HA was inspected rather than installed as a runtime service. SDK
hardware scripts and automated motor tests were not run.

Build/render results are recorded in BASELINE.md after execution.

## Five-screen update

Native build succeeds with the expanded controller, mock and shared UI.
Offscreen LVGL renders exercise all five screens and both dropset editors.
These are layout checks, not passing interaction or safety tests.

Windows Computer Use was attempted for native interaction checks. Window
capture failed twice with `SetIsBorderRequired failed: No such interface
supported (0x80004002)`. The custom LVGL surface exposes only a pane to UI
Automation, so accessibility does not provide an alternative interaction oracle.
New test code remains deferred under the original instruction.

Manual checks still required: four-entry MRU across relaunch, five-second
availability changes/offline no-op, short/long-press exclusivity, all screen
navigation, mode value transfer, unit/envelope edge cases, zero selection,
Auto Load cancellation and ignored responses, duplicate/gapped reps, RP hold
rearming, confirmed drop counts, and stale/disconnect/unload cancellation.
F7–F11 provide offline inputs for these checks; no physical motor is involved.

## Future regression matrix (pending, not implemented)

| Area | Required cases |
| --- | --- |
| Framing | Known captured vectors; both CRC failures; ordinary/extended length; every fragment split; coalesced frames; noise; truncation; buffer bound; partial expiry |
| Parameters | Signed eccentric; unknown widths/IDs; bad count; invalid battery; no partial merge; actual read/ACK envelopes |
| State | Write success never confirms; wrong/late readback; stale individual fields; disconnect/reconnect generation; failed/time-out intent |
| Safety | Every missing/unknown/false load guard; boot/reconnect/wake/mode/Twin never loads; no force path; Auto Load/Unload writes absent |
| Unload | Available while loaded/pending/stale/error; priority queue; confirm physical report; disconnected feedback |
| Limits | Verified effective maximum; preserve modifiers and cap/reduce base; stop at valid envelope; unknown max disables control |
| Capabilities | Old/stable/beta/unknown reports; unsupported options hidden; no guessed MTN version cutoff; read-only feature discovery |
| Twin | Current role vs historical role; peer stale/lost/incompatible; combined max/routing; no half/double/duplicate assumption |
| UI/hardware | 360/466/480 layout; circle margins; touch targets; positive clockwise events; press debounce; board capability rejection |
| Sessions | Slow/rejected/missing ACK; multiple independent devices; cross-device notification rejection; reconnect cancels intent |
| Telemetry | Reps/sets/force/velocity/position units; ordering; boundaries; separate peer freshness; malformed packets |

Reuse simulator mocks; introduce a deterministic MockClock for lifecycle tests.
CI and simulator targets must not link a physical BLE backend. Protocol/safety/
state/Twin/reconnect/hardware bugs need regression cases as implementation proceeds.

## September 26 Magic display revision

CONFIRMED (Mac build/render only): rebuilt with the LB/% presentation selector,
with Magic renders at 480 (Garage), 466 (KG Studio), and 360 (Travel) pixels.
Mountain remains in the middle; unsupported modes are visibly unavailable.
The app was relaunched on Garage's Magic screen. No physical BLE was linked.

UNVERIFIED: interactive selection/apply checks; the Computer Use inspection did
not return. Future regression cases (new test code remains deferred): repeated
LB/% switches preserve the native draft and issue no writes; KG conversion;
zero-base conversion unavailable; converted modifier limits; unsupported MTN
cannot be selected; confirmed main-screen values follow the display preference.


September 26 increment correction: Mac rebuild succeeded. New test code remains
deferred. Pending regressions: integer-percent to whole-LB approximate display;
view switches preserve native intent; Twin base knob steps 50/52/54; odd base
requests rejected; dropsets round down to the declared step and stop at one
positive step; unknown step blocks settings. KG precision and Twin KG step
remain unverified; the simulator does not yet model half-KG settings.


## Modifier-priority correction

CONFIRMED (source/build scope): removed base adjustment's modifier trimming;
baseMaximum now accepts proposed settings and rounds down to reported steps.
Modifier toggles/apply include any needed base reduction in one request.
Controller and mock reject negative settings, absolute modifiers off the step,
and combined over-limit requests. Percentage arithmetic retains exact integer
ratios for limit validation. New regression code remains deferred.

Pending interaction/regression cases: max 400 with absolute 50 EO gives base
350; absolute Magic 50 + EO 50 leaves 300; 50% EO leaves Twin base 266;
201% Magic leaves Twin base 132 (397.32 mathematical envelope); knob at the
cap preserves modifiers; modifier disable does not raise base; alternate
reported maxima and step sizes; zero allowance prevents load; no unsolicited
writes after report mismatch; dropsets retain configuration and step alignment.
These arithmetic examples are specifications, not executed automated tests.


## September 27 independent modifier units

Supersedes the earlier display-conversion cases. CONFIRMED (source/build/render
scope): modifier values now carry individual unit flags; base adjustment copies
both unchanged. UI percentages are integer drafts stepped by 1; absolute drafts
step by reported weightStep. Rendered Magic at 480 and EO at 360 pixels and
visually inspected both. Mac rebuild succeeds. No new automated test code was
created under the standing deferral, and these are not interaction-test results.

Pending regression cases: apply absolute Magic 50, adjust base and verify Magic
remains 50; apply 42%, rotate once to 43%; EO value and unit independent from
Magic; mixed 50 LB Magic + 50% EO under max 400 produces Twin base cap 232;
Twin absolute 42->44 versus percentage 42->43; repeated edits/reopen/main retain
confirmed units; unit selection changes only draft until Apply; ignored request
never confirms units; zero-base conversion; caps for different reported maxima.
Physical independent modifier-unit support remains unverified; mock only.


## September 29 pending modifier overwrite

CONFIRMED (source): adjustWeight previously built a whole settings request from
old confirmed state while a modifier request was pending. MockTransport accepts
replacement pending settings, so a fast base edit could replace a new modifier
value/unit with old values. The editor also navigated away on rejected Apply.

Correction: only base-only pending requests may coalesce base edits. Modifier
Apply returns acceptance; the UI retains the editor/draft while awaiting a
matching confirmed value, unit, enabled state, and mode. Failure retains the
draft. No retry or automatic reconciliation was added; unload stays available.

CONFIRMED (native Mac interaction, offline Garage max 200): started at base 30,
set Chains 50 LB, applied with a Right key immediately after Apply, then raised
base to 90. Main showed base 90 and Chains 50 LB. Reopened Magic through the new
M shortcut: value 50 and LB selected. Also applied 55%, reopened, selected LB,
adjusted converted 49 to 50, applied with an immediate Right key, and reopened:
50 LB persisted. Screenshots are in ignored build/mac/verified-base90-chains50.png
and build/mac/verified-chains-reopened.png. Mac build passed.

Desktop control could not hold a pointer for the 650 ms gesture, so simulator-
only M/E/D editor shortcuts were added to both native hosts. Windows host changes
were source-reviewed, not compiled here. Long-press navigation, ignored-response
failure rendering, and physical device behavior were not validated in this run.
No new automated regression test source was added under the standing deferral.


## September 30 Back and fixed-base modifier cap

CONFIRMED (Mac interactive, offline Twin): Back left Magic while mock resistance
was active; unloaded via the main weight target. At base 350 with max 400,
30 positive dial inputs from Chains 10 stopped at 50 LB. Apply returned to main
with base 350 and Chains 50 LB, without the previous catch-all error. Screenshot:
build/mac/verified-350-base-50-chains.png (ignored). Build passed; final Back
placement was rendered and visually checked at 360 pixels after moving it below
the Inverse Chains caption. This placement adjustment did not change its action.

No physical load or BLE access. Modifier writes while loaded remain blocked;
individual pending/state/loaded/unsupported messages replace the misleading
catch-all. Pending regression cases: Back during failed/pending requests;
mixed absolute/percent remaining allowance; Twin rounding; alternate maxima;
modifier toggles preserve base; zero-base percentage cap. New test code remains
deferred. Earlier modifier-priority displacement examples are superseded.


## Stable rings and shared unit view

CONFIRMED (Mac offline interaction): 100 LB Chains on a 200 LB profile showed a
half-filled cyan ring at base 50. In percentage view, saved Chains 150% showed
half fill at base 25% of max; raising base to 40% of max preserved both Chains
150% and half fill, changing ring color to orange at the combined limit. Base,
Magic, EO and Stairs main readouts displayed %. Screenshot in ignored
build/mac/ring-half-orange.png. Build passed. No physical BLE operations.

Pending: equivalent interactive checks for IC/MTN/EO and Twin, zero-base
conversion and rejected reports. Percentage base steps were removed by the
October 1 revision. Shared modifier rendering is common to these modes. No new
automated test source was created under the standing deferral.


## October 1 — base units and limit color

CONFIRMED (Mac offline simulator): native build passed after removing base
percentage presentation/input. Visual inspection showed base 45 LB with active
150 LB Chains and 5 LB EO on the 200 LB profile: base number, ring and active
scale markers were orange at the combined limit. Base labels and encoder input
now use device LB/KG and reported weightStep regardless of modifier view choice.
Earlier base-percentage screenshots describe superseded behavior. No new test
source or physical BLE operations. KG/Twin use the same rendering and adjustment
path; interactive rechecks of those profiles remain pending.


Base unit buttons: CONFIRMED native Mac build passed; rendered 360px layout and
480px live simulator inspected for button/number/hint separation. Live KG view
showed converted base and scale with the selected KG button. Interactive scripted
round-trip check was interrupted by concurrent user interaction and is not claimed
as passed. Source inspection confirms unit-button actions only update local UI
preference, with no controller write or load action. No new test source added.


Whole-number KG correction: CONFIRMED Mac build passed after removing decimal
formatting from the shared base formatter. Base number, scale labels and editor
unload status all use this formatter for LB and KG. Source inspection found no
remaining fractional format in UiController. Earlier one-decimal KG presentation
is superseded. Native command increments and limits are unchanged; no physical
operations or new automated test source were added.


## October 4 — compact UI and revised device scenarios

CONFIRMED (offline Mac only): native build passed. Rendered all five screens,
including both Drops variants, at 360 and 480 pixels. Inspected compact device
pair group, C without MTN, green Eccentric, signed Stairs and RP rows.
Manual UI checks: C stayed at 25 KG after Right, with no immediate message and
“Waiting” later. A at 193 LB: Chains 3% -> 5 LB -> 3% preserved the percentage;
central tap returned to Base, with EO also in percent (2%). At base allowance
223 with Chains 3%, enabling EO 2% reduced base to 219; both settings remained
nonzero. The limit number was grey unloaded and white loaded. Positive Stairs
+2 and one simulated rep changed 50 -> 52 LB. RP first hold reduced 52 -> 26.

All changes are mock-only. No physical resistance operation or BLE write.
New test source remains deferred per brief. Future regression cases: delayed
matching versus mismatching reports, disconnect/stale/rep gaps during Stairs,
unit-cache invalidation after external settings changes, zero/unknown capability,
Twin step and peer transitions, and per-model physical unit/readback semantics.


## October 8 — resumed completion

CONFIRMED: rebuilt the saved October 4 changes successfully. Fresh offline RP
session with two 50% drops and 1-second hold produced 50 -> 25 -> 12 LB, with
movement between holds. The extra-hold UI check was interrupted by an unresponsive
computer-use call; it is not claimed as passed. Source review confirms the hold
trigger is bounded by dropsCompleted < d.drops, incremented only after a matching
confirmed drop target. B was visually confirmed as a named Twin session with a
460 LB maximum; scripted peer-switch verification remains pending. No new test
source or physical hardware commands. The application flow and protocol research
are saved in APP_FLOW.md and PROTOCOL_NOTES.md.
