# Validation status and pending tests

October 24 idle-force regression (new test code deferred): with confirmed base
10 LB, fresh idle phase 0 and force samples 0–36 tenths LB at a stationary
148 mm cable, Live must retain 10. Active phases 1–3 use fresh reported force;
missing/stale phase or force returns to confirmed base, never a fabricated force.
Unknown core state remains unavailable. Check modifiers during active reps,
half-KG formatting, delayed set-close, unchanged drops/raw telemetry, first rep,
set summary and rest timer. The user confirmed the preceding rest-timer update.

October 24: [units, guarded loading and telemetry](DROPSETS_UNITS_2026_10_24.md)
supersedes the older unvalidated-training status below. The user confirmed
%/LB controls, remote-tap loading, return to Main, post-set counts and first-rep
tracking. Stairs/RP reductions, KG and Auto Load triggering remain unverified.
No new test code was created. Future regression cases: subscription masks must
not silently change the expected stream layout; sparse summaries cannot freshen
motion; duplicate set closes do not restart rest; no timer from load toggles,
settings, reconnect or old summaries; stale-state cancellation; new-set motion
ends rest; reported elapsed time takes precedence; alert fires once and wakes
the display; local elapsed time never becomes machine-confirmed state.

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

Later October 24 cases (new test code still deferred): arm Stairs while loaded
at rest with completed set 10 / rep 2, observe active moving set 11 / rep 0,
then completed rep 1 and request exactly one reduction. Repeat without unloading;
reject a new set without its observed start, a gap/reset within the same set,
and a completed rep while a prior drop is unconfirmed. A fresh new-set start
may precede weight readback; observing it must neither stop drops nor confirm
the write. Duplicate completions do not drop twice. Confirm RP requires actual
movement to rearm and retains its quota across weight-induced set boundaries.
For rotary input, the later policy supersedes immediate return to fine steps:
slowing retains a 5-unit grid until pause/reversal/touch reset. Check the gentle
two-detent cadence, batched clicks, both directions, negative stairs, KG ticks,
pending previews and off-grid device limits. No delayed write on release.

Next captured ordering to cover: set 12 completed rep 1 -> request 8 LB ->
set-close while workout remains active -> base=8 -> set 13 moving rep 0 ->
settled readback -> set 13 completed rep 1 -> request 6 LB. Also cover readback
before the new start, readback with the next completed rep in one controller
tick, failed/mismatched/late confirmation, and no quota reset for RP segments.

Latest October 24 cases (new test code deferred): Live remains 10 LB while
sensor force changes 9/10/11, and while force or rep-phase reports are missing
but core state stays fresh. Stale/disconnected core state still shows unknown.
Stairs 10 -> 8 -> 6 -> 5 must confirm the final 5, then issue no more writes;
NEXT and MIN labels must agree. RP floors, signed ascending Stairs, manual
rotary limits, below-floor incoming state, invalid/unknown minimum, presets with
insufficient base headroom, and explicit mock Twin/native-KG minima are queued.
Never reinterpret an unexpected mismatch as confirmation or automatic unloading.

Dropset presentation/recovery cases queued (new test code still deferred):
15/13/11/9 LB with one reported rep per machine segment displays one logical
set / four reps; multiple reps within each RP segment accumulate into one set;
duplicate and late completions do not add twice; continuation at 5 LB keeps
counting with no further automation writes. Confirmed unload preserves the last
total and starts rest; a second loaded dropset increments the logical set once.
Weight-induced AA85 closes while active must not start rest. Disable/stop drops
mid-set without discarding existing totals. Missing/stale telemetry, revisited
set IDs, disconnect/reconnect and ordinary training outside drops must not
invent counts. Final minimum readback must settle before exhaustion; at the
floor/quota, later rep gaps/motion loss must not restart automation. Recovery
requires fresh non-training mode plus an equally recent unloaded motor; stale,
unknown or loaded state must retain unload access. Preserve the recovery reason,
clear pending intent, and never replay resistance commands after reconnect.
Manual adjustment after exhaustion disables the recipe; it must not silently
resume drops or leave an active NEXT preview for a dormant executor.

RP/rest cases queued (new test code deferred): status 3 after returning the cable
ends a grouped set without unload; next moving zero-rep start increments SET and
resets REP/RP quota while retaining the old summary during rest. Status 2 plus
AA85 from a base edit does not end it. Preserve failed-unload cancellation across
rest. A 500-2000 ms motion gap discards partial hold time and requires new motion;
a longer loss stops automation. An unchanged extended hold cannot consume two
drops, and a pending drop cannot trigger another. Two confirmed RP holds produce
15 -> 7 -> 5 LB; the floor produces no further write. Strict sequence guards
still apply to Stairs. Verify only base is sent with percent or absolute Magic/EO,
including delayed/mismatched accessory reports. Loaded drops skip rotary quiet
time but preserve readback/deadline checks. Cue occurs once per settled target,
never on GATT success, mismatched target, timeout or reconnect. It must not imply
measured force has already changed. Hardware sound/vibration remains unverified.

October 31 motor-report cases queued (new test code deferred): preserve raw
16009=0x0100 while interpreting its mode as unloaded; read fresh base/modifiers
without a false connection error. Check 0x0101 / 0x0105 loaded, 0x0104 unloaded
command echo, and prefixed Auto Load states against the same additional status
guards as their plain values. Unknown upper bits (e.g. 0x0200), unknown low-byte
modes, and stale reports must not establish readiness or acknowledge a command.
Load/unload acknowledgements use the normalized mode but all outgoing command
values remain unchanged. Reconnect, mode entry, base edits and decoder recovery
must never issue a load. A low-byte-only match cannot bypass locks, cable zero,
activation, standalone context, or current explicit user intent.
