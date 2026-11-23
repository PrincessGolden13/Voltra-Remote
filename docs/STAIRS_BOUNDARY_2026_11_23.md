# Stairs return boundary and queued writes — November 23, 2026

## Evidence

CONFIRMED by the owner: after Auto Load, Stairs changes the weight, but during
the repetition rather than at its end. No standalone capture of that movement
was available when the source defect was found.

CONFIRMED in the already-pinned SDK: AA82/3B is emitted at pull START and return
START, not full-return completion. See
[`79f6c55` event contract](https://github.com/HJewkes/voltra-node-sdk/blob/79f6c55297500e6a81c31af78eb85fedd13c2b61/src/sdk/types.ts#L208).
The remote incorrectly promoted return-start to completedRep and immediately
used it to trigger Stairs. That interpretation was wrong even before this
follow-up; the live target-weight feed is a separate stream.

AA81 has a different phase field: 0 idle, 1 pull, 2 transition, 3 return. See
[Steven `1c14860` captured phases](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L99).
Its phase 2 is not AA82's phase 2. Neither reference supplies a universally
verified full-return event or a conditional "write at ROM end" command.

CONFIRMED source-shaped MOCK reproductions using the actual PhysicalSession:
return-start during moving Auto35/14 allows and dispatches a base change;
an edit queued while idle still dispatches after a fresh moving pull arrives.
The actual TrainingApplication separately reproduces the premature request.
These reproduce source mechanisms, not the owner's unseen packet sequence.

## Conservative endpoint policy

Keep an AA82 return-start counter as a candidate. Qualify that reported counter
only after a matching, continuously fresh AA81 return3-to-idle0 observation with
stationary velocity. Do not derive completion from the target-weight feed,
force, elapsed time, counter increment, ROM extrema or phase2. A slow return
can last longer than two seconds while its motion stream remains fresh.

Use the existing 500 ms motion freshness and 20 mm/s stationary threshold.
The completed window ends on new movement/phase, different set or rep, more
than 10 mm drift, missing/unknown motion, rest/finish, unload or changed control
context. The drift bound is a named remote policy, consistent with the existing
RP hold-drift policy; it is not a device safety interlock or ROM measurement.
Unknown and missed windows cannot authorize a catch-up change on a later rep.

Stairs requires the qualified event and its still-current stationary window.
The protocol backend also captures an automatic edit's motion context and
checks it through the actual base write. It cancels an unsent edit if movement,
stream loss, a new phase/set/rep, a control-context change, rest, set-close or a
topology contradiction arrives first, including within one notification batch.
Once the base write is issued, later movement does not cancel its confirmation
or pretend that command was withdrawn. No retry, motor command or load reassertion
is added. Explicit unload remains available.

Vendor telemetry and parameter pushes are observations, so reused sequence numbers cannot
hide motion from these guards. Counter/transition checks prevent duplicate
completion; a repeated boundary may conservatively cancel an unsent edit.
CRC/source/envelope checks and atomic parameter decoding still precede updates;
parameter query-response (0x0F) deduplication and settled settings confirmation
are unchanged.
Hardware sequence reuse has
not been established as the cause of this report.

The selected policy waits for the return to finish, consistent with the existing
"after each completed rep" brief. Physical agreement with the owner's ROM and
availability of an idle packet between continuous reps remain UNVERIFIED.
Without that observation, Stairs must wait or stop rather than change midrep.

## Release status and validation

The previously prepared v0.4.17 images and four draft assets passed all checks,
but publication was held on receiving this new Stairs report. Draft455975057
remains unpublished; its tag and assets are not replaced. v0.4.18 includes the
activation, single Auto screen/refresh and quiet-dial changes from that tree.

Validation uses external source-shaped MOCK fixtures and the actual application,
protocol and simulator implementations. Protocol53/210 and application12/120 pass;
prior activation, refresh, target and LVGL observers pass on the combined source.
Detailed results and artifact checks are recorded in TESTING.md. The simplified
Simulator's 40 ms settings queue does not model every PhysicalSession dispatch
guard; its workflow checks are not evidence for real BLE timing.
No new tracked automated test source or physical
radio/motor operation is introduced. Physical Auto target-stream recovery and
Stairs timing still need the owner's observation.
