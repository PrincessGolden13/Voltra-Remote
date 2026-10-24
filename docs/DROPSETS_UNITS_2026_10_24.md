# Real-device accessory units and dropsets

This supersedes the preview-only dropset and percent-only limitations in
CONTROL_FEEDBACK_2026_10_24.md. The user confirmed remote Mountain → Chains →
Mountain works on Black with EO unchanged, then requested absolute accessory
units and real local dropsets. Real Twin testing remains deferred.

## Units and transactions

CONFIRMED (source): kirby6365/voltra_remote commit
8e1ea1cb752acd3108c61f1a502b63978a172832 documents global selector 21865/0x5569:
uint8 1 percent / 0 absolute. A write to the nonselected accessory pair can
echo success and revert about 100 ms later. This implementation switches the
selector unloaded, confirms it, refreshes both pairs, then plans amount writes.
Each settings step requires a direct read at least 250 ms after transmission;
an asynchronous echo is insufficient. Timeout stops without retry or rollback.

The UI offers % and the machine's reported LB/KG unit. The physical report
declares shared modifier units. Switching one editor converts the other amount
at the current base to the nearest supported step, retaining its enabled state
and using at least one when enabled. Rounding can slightly change resistance;
the combined ceiling still applies. Mixed-unit presets are rejected with an
explanation. Simulated independent-unit profiles retain their previous behavior.
The user confirmed IC and LB editing work on Black. KG round trips remain
unverified.

## Telemetry and execution

CONFIRMED (source): the pinned kirby remote decodes AA 81/2B live telemetry,
phase at payload 2, set at 3 and big-endian rep at 4–5. SDK
79f6c55297500e6a81c31af78eb85fedd13c2b61 defines frame offsets 22/24/26 for
force (tenths LB), position (mm), velocity (signed mm/s), equivalent to payload
11/13/15. Only the complete 45-byte subtype is accepted. AA 82/3B return-phase
statistics (61-byte payload, phase 2) trigger completed reps for Stairs; pull
statistics and in-progress counters do not trigger reductions.

Motion and completed reps have separate timestamps/revisions. Settings polls
cannot advance a hold. Motion freshness is 500 ms; completed-rep freshness is
2 seconds. Counts must be sequential in the same set; duplicates do nothing,
while gaps/resets stop without catch-up. Observation starts with actual motion.

RP policy: movement first; then speed at most 20 mm/s, extension at least 20 mm
beyond reported cable offset (offset register is cm), tension at least 2 LB,
and cumulative drift at most 10 mm for the requested hold. These are initial
tunable detection policies, not device interlocks or physically calibrated
thresholds. One continuous hold permits one drop; movement rearms the next.

Apply arms the local recipe instead of silently storing enabled=false. Failed
Apply stays in the editor. No recipe intent survives disconnect/reboot. Stale
state, missing motion, Twin/context change, manual weight changes or failed
confirmation stop execution. Disabling cancels unsent drop settings. Another
completed rep during confirmation stops execution instead of silently skipping
it. A confirmed change increments the count once. Unload remains accessible.

## Loaded-write boundary

CONFIRMED (source): the pinned kirby capture identifies motor 1 as already
loaded and supports changing its base register. It separately reports motor 5
changing to 1 after an active-set edit and reasserts load command 5. This remote
does not copy its reassertion, load retries or bypass.

INFERRED physical profile: fresh standalone modern control registers plus motor
1 permit a base-only loaded write. No curve, amount, unit or motor write belongs
to that transaction. Unit, limit and accessory context must remain unchanged.
Motor 5 and Auto Load transitions are excluded. Actual physical effect still
needs the user's test: readback proves the setting, not measured resistance.
Arming drops never requests a load. The separate explicit-load readiness gate
was opened only after the mounting-state check described below.

CONFIRMED (user/old-build observation): Black was manually loaded at 10 LB with
modifiers off. Reps and interrupting Auto Deload retained 10 LB; the completed
countdown unloaded it. Serial reports showed motor 1, then 0, and changing cable
position. The old build did not log stream fields; this does not validate the
new decoder. Development tools never load or exercise a physical motor.

## Verification

Native and both firmware builds pass. Existing reference validators pass
(203 SDK packets, 593 modifier packets); these are not automation tests. Existing
360 px Magic/RP render paths were inspected. No new test code was created.
Physical checks remain: compare live stream/reps/holds to Black, then
user-initiated Stairs/RP trials including Auto Deload interruption.

First installed capture: Black accepted 21865=0 and Magic 16007=4 LB, derived
21722=4000, at 10 LB base; it then accepted 21865=1. Return statistics correctly
reported set 2 reps 1–3 and set 3 reps 1–4. Workout statuses 0→1→2, 3 at rest,
and 4→0 on unload were observed. Continuous 81/2B samples were absent, so RP
detection is not yet physically validated. The follow-up explicitly sets the
fitness stream rate 20866 to the catalogued default 10 Hz (Android's disconnect
path resets it to 0); the existing mask alone was insufficient. Loading/unloading
status now vetoes live writes immediately, before the next motor poll.

The user confirmed IC and LB controls work, reported the completed-statistics
counter lagging the device, and wanted return to Main at set end. The live screen
now follows device workout status 2/3 rather than waiting for physical unload.
The continuous stream supplies the in-progress display counter; completed-return
statistics remain the Stairs trigger. A confirmed external load clears a stale
load-refusal message.

Later physical confirmation: remote-tap loading and return to Main work. The
in-progress counter still missed the first rep. A bounded header capture found
AA94/9B (157-byte payload) with an all-fields custom notification mask, not AA81.
The Android mask originally used came from Custom Curves. SDK telemetry setters
are explicitly experimental; readback of their values is not proof of the frame
layout. Restoring the catalogued zero subscription, with 10 Hz, restored AA81/2B
on Black. CONFIRMED: the capture shows set 6, live reps 0→1→2, signed motion,
position 148–901 mm and about 10 LB tension. The user confirmed the first-rep
issue is fixed. The default subscription is required for this decoded layout;
do not substitute a Custom Curves or all-fields mask.

AA85/5F set-close decoding now reads repCount at payload 15 and retains the
set index actually observed in the same connection. Black reported set 5 / 4
reps, and the user confirmed the summary appears. Register 21377 stayed zero
through this check; it is not treated as an elapsed timer. The user notes Black
internally times inactivity and prefers synchronization, but explicitly permits
a local fallback when elapsed time cannot be read. TrainingApplication now
prefers a reported elapsed value, otherwise measures from receipt of AA85's
confirmed set-close. The local clock is never written into TrainingReport.
Repeated closes for the same set do not restart it; new actual movement ends it.
Stale/disconnected state hides/clears it; reconnect, settings and load toggles
never start it. Device summary debounce and BLE delivery limit synchronization
accuracy; it does not claim to mirror Black's private Auto Deload countdown.
Guition's board adapter has no implemented buzzer/haptic driver. Rest alerts
currently wake the screen and show visual feedback for two seconds, once per
rest period; the hardware hook remains available for a validated future driver.

CONFIRMED (user, follow-up): the rest timer appears after a set and disappears
when another starts. The user then observed 1–4 LB jumping on the remote while
Black displayed the selected 10 LB. The capture shows actual idle AA81 samples:
set 8 / rep 3, phase 0, position 148 mm, velocity about -1 to +2 mm/s, and force
0–36 tenths LB. These are genuine sensor readings, not locally generated weights
or a base-setting change. Live previously displayed every fresh force sample
until the later set-close/status event switched to Main. Presentation now uses
measured force only for fresh phases 1–3 in a loaded active set, and uses the
confirmed base for idle/unknown/stale rep phase or absent force. Unknown core
state still displays unavailable. Raw telemetry, drops, load commands and set/
rep/rest tracking are unchanged. Physical display observation of this fix remains
pending.

CONFIRMED (Black, physical action): the user unloaded, released the mounting
lock and secured it again. Register 20642 changed 2→1→2; the user then confirmed
it was secured. Companion 21224 remained 6, as during successful manual loading.
The explicit load gate requires fresh mount=2, this exact companion profile=6,
motor=0, Weight Training, standalone state, child-lock=0, OTA=0, Auto Load
status=0, known unit/limit/modifiers, activation, and cable position/offset.
A normal tap additionally requires zero cable within integer-cm quantization
(1 cm tolerance, source capture 37/38 cm at rest), and base at least 5 LB.
This is a narrow observed profile, not a general decoding of void-lock bits.
Any unknown/change fails closed. Only a current explicit tap writes motor 5,
once, without retry or fallback. Hardware-interlock bypasses are never written.

Existing Auto Load long-press triggering remains one AA12 request using its
enabled safety preference; no setting changes. Source-documented motor 35 with
status 11–13 is armed/in progress, 14 loaded. These Auto Load enum paths remain
unverified on Black and cannot authorize loaded drops (which require motor 1).

## Loaded set transition and coarse dial follow-up

CONFIRMED (Black capture and user): the first Stairs trial stopped with
"Rep sequence changed". The preceding completed statistics were set 10 / rep 2;
Black then reported status 2, set 11 / rep 0 with actual movement, followed by
completed set 11 / rep 1. Its base stayed 10 LB. The controller had copied the
previous set's completion baseline when drops were armed while still loaded at
rest, and treated the legitimate next set as a sequence failure.

The controller now waits between confirmed sets, then establishes a zero baseline
only on a fresh, moving, zero-rep sample for a different reported set in active
workout state. This changes local automation bookkeeping, not device counters.
The subsequent completed-return event still triggers the first drop. Missing
start evidence, within-set gaps/resets, stale telemetry, or a completed rep while
a change remains unconfirmed still stop automation; no missed drops are replayed.
RP movement/hold state rearms with actual movement. Its local drop quota belongs
to the armed loaded sequence and persists across reported statistics sets until
unload or explicit rearming.

CONFIRMED (next Black capture and user): the first drop changed 10 to 8 LB after
completed set 12 / rep 1. Black emitted a set-12 summary while workout status
remained 2, returned base=8, then streamed moving set 13 / rep 0. The controller
stopped with "New set before drop confirmation" solely because its local
dropTarget still existed, before checking whether confirmation had finished.
The trace does not identify exactly when the settled readback completed; the
user and force samples around 8 LB confirm the first physical change occurred.

That premature rejection is removed. Observing a new set start only updates
local sequence tracking; it does not acknowledge the weight write or send one.
The existing pending/failed/mismatched confirmation checks still run, and the
next distinct completed rep can trigger another change only after the preceding
one is confirmed. RP's quota is no longer reset by each device set transition,
which would otherwise replenish it after weight-induced segment changes. Further
consecutive physical reductions and RP behavior remain pending user verification.

CONFIRMED (source): the dial's former intermediate 2-tick grid, plus immediately
falling back to fine steps while slowing, could end a fast stroke off 5/10 marks.
The intermediate gear now moves one 5-unit grid mark per two detents, followed
by 5 then 10 per detent at higher speeds. It retains at least the gentle 5-unit
gear until a 300 ms pause or direction reversal; touch/focus changes also reset
it. No timer sends a final rounding command. The first coarse change aligns in
the travel direction; device minimum/maximum clamps remain authoritative. Fine
precision remains 1 LB / 0.5 KG / 1%, and absolute coarse grids use whole units.
The prior 120 ms/three-detent ramp and 160 ms rate filter remain tunable policy.

Future regression cases (new tests deferred): selector rejection/reversion;
EO across unit conversion; mixed presets; duplicate/reset/gapped reps; first
motion and expiry; slow drift; one drop per hold; movement rearm; busy trigger;
external modifier/unit/Twin changes; unload mid-request; no reconnect replay;
zero drops/percentage; KG/floor; mode 5 refusal with no automatic reload.

Sources:
- https://github.com/kirby6365/voltra_remote/blob/8e1ea1cb752acd3108c61f1a502b63978a172832/docs/PROTOCOL.md
- https://github.com/HJewkes/voltra-node-sdk/tree/79f6c55297500e6a81c31af78eb85fedd13c2b61
