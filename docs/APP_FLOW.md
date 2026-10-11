# Application flow — October 4, 2026

This is the current five-screen specification. It supersedes earlier UI choices,
including remote LB/KG selection, independent modifier unit flags, Back/Apply in
Magic, red EO, zero-valued active modifiers, and modifier edits preserving base
at the expense of their value. Physical board validation and offline-only
restrictions remain. Auto Load settings remain read-only; an explicit long press
may request the existing behavior once, without retries.

## Devices

Show permanent names and availability with white/grey text. No Online/Offline
badges or instruction sentence. Group known reciprocal Twin peers visually,
retaining both names and selectable circles. Relationships come from the VOLTRA
session layer, not from generic BLE transport or name inference. Scan every five
seconds, remember four successful devices, and keep offline devices untappable.
The simulator also supplies its full known-device directory, including offline E.
More than four logical groups paginate; B+D occupy one group.

| Mock | Unit | Maximum | Step | Mountain | Behavior |
| --- | --- | --- | --- | --- | --- |
| A | LB | 230 | 1 | Yes | Changes confirm |
| B and D | LB | 460 combined | 2 | Yes | Two permanent names, one shared Twin session |
| C | KG | 90 | 1 | No | Base changes accepted by transport but never confirmed |
| E | LB | 230 | 1 | Yes | Offline |

These are user-requested synthetic scenarios, not firmware inference. C uses the
whole-KG ceiling below a 200 LB device rating. B/D's combined limit is explicitly
supplied by the mock; real limits and routing must be independently verified.
A/B/D represent newer firmware, C older firmware, solely for the requested demo.

## Base weight

Use the confirmed device's LB/KG preference; no remote unit selector and no base
percent mode. All numbers are integers. Rotary input uses the reported native
step. Keep base >= one step. Active modifier values do not change when base is
edited; base stops at its remaining allowance. The large number toggles load,
and a long press requests existing Auto Load. A fresh confirmed loaded number is
white; unloaded/unknown is grey, even at maximum. Rings and tick marks may turn
orange at the combined limit. No tap/hold instructional text.

The device name returns to Devices. Show confirmed Twin status. The three lower
controls toggle Magic, EO, or local Stairs/RP; long press opens the editor.
Magic/EO rings reflect the modifier's own fixed scale (300% or device maximum),
not remaining headroom. EO uses green. Unknown capabilities are unavailable.
Unload remains accessible during pending or stale state, including via the
compact editor weight status and physical encoder press. No automatic loading
on navigation, reconnect, restoration, mode or Twin changes.

## Magic and Eccentric

Magic selectors are IC, MTN, C. Omit unsupported selectors entirely and center
the remaining controls. Eccentric's title is simply “Eccentric”. Both have a
central editable number and a compact native-weight/% choice. Tap the central
number to submit and return to Base after confirmation. No Back or Apply button.
A rejected or unconfirmed edit stays visible, without invented state changes.

Magic and EO use one common absolute/percentage interpretation. Changing it
converts both drafts; applying submits both atomically in the mock. Stable
alternate drafts prevent repeated unit toggles from cumulatively rounding values
down. An actual numeric edit refreshes only that modifier's conversion anchor.
Confirmed base/modifier changes invalidate obsolete cached views. The two saved
representations persist across editor navigation in the current process only.

Minimum active modifier is 1% or one native step (single LB 1, Twin LB 2).
Modifiers keep priority when enabled/edited: reduce base, aligned to its step,
to keep the combined envelope within maximum. Base also retains one step;
reject an impossible combination. Percent editors step by one; absolute editors
use reported weightStep. Percentage ceiling remains 300. The combined envelope
reserves both modifiers without negative values. Stairs/RP are weight-change
programs, not extra resistance added to this envelope.

Major ticks fall at multiples of 50 native units or percent; minor ticks and
endpoints fill the scale. Percentage suffix is present on midpoint labels too.
No routine pending text during the first three seconds. Then show “Waiting”.
Newer heartbeat reports alone do not confirm requests: values must match intent.
At eight seconds stop tracking the unanswered request, retain confirmed values
and “Waiting”, and do not retry. A later explicit edit remains possible.

## Stairs

Absolute device units only. Signed central value, unit, and “per rep”. Zero is
at top center of the outer scale; positive steps illuminate clockwise/right,
negative counterclockwise/left. Rotary changes one native step in either
direction. Tap the center to save and return; no Apply button. Zero is a valid
no-op step for this local program, unlike an active resistance modifier.

Only while explicitly enabled, confirmed loaded, and telemetry fresh/valid,
each distinct completed rep adds/subtracts the signed step from confirmed base.
Clamp to one native step and the allowance after modifiers. Stop on disconnect,
stale data, rep reset/gap, or an unconfirmed change; never replay missed reps.
Physical rep-event mapping and live-weight capability require verification.

## Rest Pause

Three vertical text/value rows: Drops, Drop %, Holds. All values are white;
outline the selected row to identify rotary focus. No duplicate field caption
below. Keep the Apply button. Defaults: two drops, 50%, one-second hold.

Require movement before a qualifying stationary hold. One uninterrupted hold
produces one reduction. Movement rearms the next set; reductions compound from
confirmed base, round down to the native step, and never reach zero. Two drops
means three sets and exactly two reductions. Keep all modifier settings.
Disarm on invalid/stale data, disconnect, or an unconfirmed change. Selecting a
different device disarms local automation; no active program survives restart.

## Evidence boundary

All scenarios use offline MockTransport, with no real BLE resistance commands.
The physical TrainingReport adapter, unit mapping, limits, Twin routing and
live rep/hold telemetry remain unverified. RP currently runs in the controller;
public session-template support does not prove a native pause-triggered BLE
configuration. See [protocol findings](PROTOCOL_NOTES.md#october-4--units-shared-modifier-interpretation-and-dropsets)
and [validation status](TESTING.md). Unknown physical support remains disabled.
