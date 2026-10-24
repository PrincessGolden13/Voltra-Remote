# Validation status and pending tests

October 18: see [physical discovery observations](LIVE_CONNECTION_2026_10_18.md).
Discovery, GATT subscriptions, acceptance after user approval and read-only state
are physically established with the local diagnostic profile at ten feet.
Training is unvalidated. Queue the cases below; no new test code was added.

October 17 Normal-mode work: [implementation and blockers](NORMAL_MODE_2026_10_17.md).
New test code remains deferred. Queue future cases for zero/one/multiple completed
scans, late power-on, accepted-only persistence, failed GATT/approval, stale scan
refresh during a hold, forget/persistence failure, retained offline devices,
mode gesture versus page swipes, simulator-to-normal reboot selection, per-image
link isolation, notification overflow/generation, truncated read groups and
unknown Twin/readiness. Physical training is not validated.
Also cover immediate invalidation on unknown units/motor/workout mode, unload
from that state, and monotonic report revisions after valid state returns.

October 17: see [feedback changes and pending regression cases](UI_FIXES_2026_10_17.md).
Existing pinned Ryan packet and SDK-consistency validators were rerun successfully.
New UI/state tests remain deferred; builds and render checks are not interaction
or physical-resistance validation.

The explicit instruction under REQUIRED TEST CASES defers new test code for now.
Existing upstream validators are run; no artificial coverage or passing CTest
placeholder is supplied. This is not a tested production control implementation.

## October 18 connection checks

Normal, isolated Simulator and Windows native builds pass after the asynchronous
connection change; existing offline renders open Devices/Main/Twin successfully.
Physical captures reproduce error 574 and show a later successful radio link/MTU
negotiation. They exposed cached-only GATT enumeration; the correction now reaches
three services, three subscriptions, acceptance and live state. No autonomous
resistance command or new test code was used. Queue fresh empty GATT caches, bounded 574-only
retries, cancellation during connect/backoff/MTU, late callbacks before a different
target, no pre-subscription bootstrap, failed service/subscription feedback, and
missed-scan availability grace versus unchanged machine-state freshness. Also
cover missing CCCDs, independent three-channel fragmentation, bootstrap timing
after blocking discovery/write, approved versus refused/expired sessions, 0/1
motor readbacks versus 4/5 commands, core-read prioritization, and module tags
that must not be interpreted as installed app firmware. Local identity/profile
comparison is a physical diagnostic, not a validated provisioning algorithm.

Unloaded LB write cases queued: explicit input only; fresh zero/zero standalone
reports; nonzero/stale/unknown peer fields; optional paired field omitted versus
previously present then stale; lock/OTA/unit/motor changes between inputs; modifier
freshness; no settings or load on reconnect; accepted write without readback;
readback mismatch/clamping; rapid explicit edits; unload priority; KG and modifier
edits rejected. Auto Load enabled versus actually armed must remain distinct.

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

Manual checks still required: saved-device list across relaunch, five-second
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
| Limits | Verified effective maximum; reduce overload modifiers first while increasing base; stop at valid envelope; unknown max disables control |
| Capabilities | Old/stable/beta/unknown reports; unsupported options hidden; no guessed MTN version cutoff; read-only feature discovery |
| Twin | Current role vs historical role; peer stale/lost/incompatible; combined max/routing; no half/double/duplicate assumption |
| UI/hardware | 360/466/480 layout; circle margins; touch targets; positive clockwise events; press debounce; board capability rejection |
| Sessions | Slow/rejected/missing ACK; multiple independent devices; cross-device notification rejection; reconnect cancels intent |
| Telemetry | Reps/sets/force/velocity/position units; ordering; boundaries; separate peer freshness; malformed packets |

Reuse simulator mocks; introduce a deterministic MockClock for lifecycle tests.
CI and simulator targets must not link a physical BLE backend. Protocol/safety/
state/Twin/reconnect/hardware bugs need regression cases as implementation proceeds.

## October 11 review

Windows build and responsive offscreen renders pass (see BASELINE.md). These
are not interaction/safety test results. No new test code was added. Add future
coverage for mixed-unit envelope limits, preset unit migration/readback mismatch,
queued explicit edits versus unsolicited reports, and wake-input consumption.

Follow-up regression cases queued (new test code still deferred): continuous
rotary input beyond two seconds must not starve mock reports; every theme at
and below maximum; confirmed vs ignored maximum requests; modifier amount tap
with pending edits; no X controls; exact preset replacement at high base,
70% IC + 20% EO / max 200 -> base 105; mixed absolute/percentage recipes;
unsupported recipe rejection; knob input during preset confirmation; mismatch
must not activate dropsets or the preset indicator. No new automated pass claimed.

## October 18 single-device follow-up

No new test code was created. Existing Ryan packet and pinned SDK validators
passed again (203 SDK packets / 593 modifier packets); this is reference-data
validation, not proof of new transaction behavior or physical readiness.

Future regression cases: modern percentage values overriding misleading legacy
zeros; uint8 inverse selector and sport-mode field 21007; settings ACK mismatch;
ten-second transaction vs three-second per-write timeout; continued critical polls
during rapid explicit input; units/interpretation/maximum/Twin/motor changes during
a batch; unload cancels all queued steps; expired intent is never replayed;
unrelated reports cannot erase a failure; half-KG round trips and wire envelopes;
disabled modifiers write zero while enabled values stay positive; no redundant
style/alignment invalidation or hidden scale-label rewrite on idle frames;
sparse rep events are not motion freshness; no synthetic physical rest timer.

## October 24 feedback

Existing packet/upstream validators pass again; no new test code was created.
Future cases: one automatic idle-to-Weight-Training request per accepted connection;
mode already selected; refusal/timeout/stale/loaded/armed/other-workout context;
manual exit is not reversed; unload cancels setup; no load command during setup.
One-modifier changes must never write the other parameter; all reduction/increase
orderings must remain within the envelope. Repeated taps use latest requested
state; coalesced edits preserve both independent fields. Continuous rotary input
must retain fresh reports while waiting for the 100 ms quiet interval. Requested
weight is labelled, discarded on failure/disconnect and never authorizes loading.
Missing standalone Twin status must not create a Twin banner. Synthetic pair
limits are 500 LB / 226 KG; ordinary 2.x fallback stays 230 LB / 104 KG.
Disabled physical reports contain zero, so UI editor recall must retain the last
confirmed positive Magic/EO amounts independently per device. Future cases:
60% IC -> disable -> reopen at 60%; EO unaffected; report-driven changes update
memory; no writes on editor entry/reconnect; lower headroom clamps the draft;
absolute-unit conversion; unsupported unit interpretations are not restored;
forget/reset clears the local editor memory. This memory is session-local and
does not override machine-confirmed enabled state or preset matching.

The user confirmed IC disable/reopen retains its value. Further October 24 cases
are queued for progressive rotary input: start/reversal/300 ms pause use fine
steps; noisy batches cannot jump directly from level 0 to level 3; slowdown
brakes immediately; 73 -> 75 -> 80 clockwise and 73 -> 70 counterclockwise at
the 5-unit level; 0.5 KG precision, whole-unit coarse grids, signed stairs across
zero, pending-intent accumulation and final maximum/minimum clamps. No new
automated test pass is claimed for these cases.

Mountain cases: fresh valid 0x556F and accessory-display reports; omitted/stale/
invalid values; both inverse/Mountain switches set; clear prior special curve
before enabling the new one; readback required at every step; EO unchanged;
changing the device's display interpretation cancels percentage edits. Dropset
preview/save on an unsupported connection must not arm it, write resistance,
activate a preset or imply automatic execution is available.
