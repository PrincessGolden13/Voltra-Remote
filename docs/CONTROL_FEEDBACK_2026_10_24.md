# October 24 control follow-up

The user confirmed the remembered modifier amount works and reported abrupt
acceleration, a hidden dropset tile and missing Mountain support on Black.

## Rotary policy

`src/ui/RotaryAdjustment.h` owns the shared policy in logical detents. Slow input
is one display tick (1 LB, 0.5 KG or 1%). The intermediate level uses even
two-tick marks. Brisk/very fast levels use whole-unit 5/10 grids, including percent
and KG. The first coarse step goes to the next mark in the direction of rotation,
so 73 goes to 75 clockwise or 70 counterclockwise on the five-unit grid.

Named tunable policies: enter at 10/18/32 detents per second; leave at 8/14/26;
160 ms speed filter; at least 120 ms and three additional detents before climbing
one level. A 300 ms pause, reversal, navigation or touch selection starts fine.
Slower input brakes immediately instead of waiting for the filter to decay.
All values still pass through existing controller limits and protocol guards.
These are UI tuning choices, not device specifications; physical feel needs the
user's feedback. New test code remains deferred.

## Dropset presentation

The local dropset editor remains accessible on real connections. An unavailable
execution capability shows an OFF tile and a preview-only status; applying saves
the local recipe with enabled=false. It never starts automation or sends a
resistance command. Loaded-edit readiness and motion telemetry remain separate,
unresolved physical milestones. Simulator execution behavior is unchanged.

## Mountain evidence

CONFIRMED (source): kirby6365/voltra_remote commit
8e1ea1cb752acd3108c61f1a502b63978a172832 documents 0x556F (uint8) as the Mountain
switch: 1 Mountain, 0 ordinary chains. It is not pushed asynchronously. The
amount remains the existing chains amount, 21722 for percentage input. It also
documents 0x5569 (uint8: 1 percent, 0 pounds) as the accessory display selector;
21446 is adaptation behavior and must not stand in for that selector.

CONFIRMED (user/read-only observation): Black was manually set to 25% Mountain,
and the existing poll returned inverse 21424=0, chains percent 21722=2500 and
absolute chains 16007=15 at a base of 60 LB. Those fields alone cannot distinguish
Mountain from standard chains. The new poll includes both separate switches.

The implementation requires fresh valid switch reports, a non-conflicting curve,
percentage display and the existing unloaded safety context. It clears the old
special curve before enabling the next; all steps require readback. EO is not
touched by a curve change. Absolute editing stays disabled rather than guessing
from 21446. Device compatibility and physical readback results are recorded below
after installation.

## Installed build and observations

CONFIRMED on Black after installation: first radio attempt connected, remembered
acceptance completed, and the new poll returned 21871=1 (Mountain), 21865=1
(percent display), inverse=0 and chains-percent=2500. Base was 60 LB, motor 0,
EO-percent=300 (3%) and maximum 230 LB. This matches the user's 25% Mountain
observation and verifies the read path. Remote-driven curve changes and the new
dial feel await the user's check; no autonomous resistance command was sent.

Normal and isolated Simulator firmware builds passed, as did the Windows native
build and existing reference packet/upstream validators (203 SDK / 593 modifier
packets). Existing 360 px Main, Stairs and RP renders were inspected; the final
Main BMP is byte-identical to the reviewed render. These are layout/reference
checks, not a new interaction test suite. No new test code was added.

Bounded flashing passed recovery/partition and esptool hash verification. Logs
remain ignored under build/voltra-live/2026-10-24; images/manifests are in
build/guition/2026-10-24-mountain-controls. Loading, Auto Load triggering, loaded
changes and automatic real drops remain gated. Real Twin testing stays deferred.

## Later dropset counting and session-exit follow-up

This section supersedes the earlier dropset gating milestone above; subsequent
user-authorized live work enabled guarded base-only drops on standalone Black.

CONFIRMED (Black capture and user observation): Stairs progressed through
15 -> 13 -> 11 -> 9 -> 7 -> 5 LB. Each base change closed an AA85 statistics
segment while workout status remained active (2), and the next moving stream
used a new set ID with its rep count reset. Showing those raw IDs directly caused
the remote to lose the cumulative dropset rep count.

The user's explicit dropset exception now has a local presentation model,
`DropSetProgress`, separate from machine-confirmed `TrainingReport` and from
the completed-rep events that authorize automatic reductions. It sums the maximum
reported rep count for each observed moving-stream segment. Duplicate/late
completion reports can amend an observed segment but cannot create a segment.
Stairs and all RP drops in one loaded sequence share one logical set; confirmed
unload finishes that set. RP rest phases preserve the group. The Main summary
and Live counters use the same totals, which continue at the floor or after
automation is stopped. Counts reset on disconnect/leave, and ordinary training
uses the device counters. Segment storage is bounded to 256; an ambiguous
revisited ID or invalid count hides the local totals instead of inventing reps.

Weight-induced statistics closes do not start the grouped rest clock. An actual
device rest/non-active phase or confirmed unload can do so; active movement ends
rest. This remains presentation state and never authorizes a resistance command.

After the last target is confirmed, reaching the profile minimum or RP quota
latches automatic execution as finished until unload/rearming. The controller
does not evaluate further reduction triggers, resend the minimum, reassert load,
or automatically unload. The next-target clamp remains a second guard. A final
target that is still pending must still receive its normal confirmation.
An explicit manual dial adjustment also disables an exhausted recipe, so its
NEXT preview cannot imply another automatic drop will occur.

CONFIRMED (capture): after the first rep at 5 LB, Black reported unloading/off,
workout summary 6 sets / 6 reps, an unrecognized workout status 5, motor 0, and
mode 0. GATT still appeared connected, so the old UI remained on an invalid
training screen. The user did not retain the device's error message. UNVERIFIED:
the cause/error code, and whether any later radio failure accompanied it. The
capture does not establish an extra resistance command at the floor.

Recovery now cancels queued intent and disconnects an established training
session when a fresh non-training mode and unloaded motor are confirmed together
(the motor observation must be at least as new as the mode observation). Device
selection retains the reason and permits an explicit reconnect. Unknown/loaded
state retains the unload target. Unknown workout statuses no longer preserve
the previous active status. Recovery never automatically loads or re-enters
training; a new explicit connection follows the existing one-shot setup rules.
Numeric control-write ID/value/timestamp diagnostics were added for future
investigation, without logging handshake identities. Physical verification of
these changes is pending; no device fault was intentionally reproduced.

## RP timing and actual rest boundary

CONFIRMED (user): the grouped dropset counters and minimum continuation worked.
The next set incorrectly continued the same count after the cable was returned
and left idle. CONFIRMED (capture): after segment 6 / rep 3, Black sent a set
summary and workout status 3 (rest), remaining loaded, before active set 7 began.
This supersedes the previous loaded-sequence-only boundary: the physical profile
now exposes recognized between-set rest separately from an active statistics
close, and both the presentation counter and drop executor reset for a new real
set. Weight-change closes at status 2 still preserve the group and RP quota.

CONFIRMED (capture): RP sent base=7 during the extended stationary hold, at
device log times 287816 ms and 312811 ms in two trials. Black reported base=7
before subsequent movement, but force stayed near 15 LB into the following rep.
An eventual motion-freshness failure stopped RP before the second hold, despite
later motion samples resuming. The code's 500 ms motion limit previously made
that cancellation permanent on the first missed freshness deadline.

RP now pauses through a short motion gap, clears all accumulated hold duration,
and requires fresh movement followed by an entire new hold. A loss over 2000 ms
still disarms it. RP is driven by that live motion/hold evidence, not completed
rep sequence continuity; Stairs retains its strict rep sequence checks. An
unconfirmed target still prevents any next drop, and timeouts never retry it.

A dedicated loaded base-only operation copies current confirmed Magic/EO
settings, retains context/envelope validation, and dispatches before background
polls without the 100 ms manual-input quiet period. The physical loaded planner
can write only parameter 16006, never accessory/unit/load parameters. Critical
polling resumes immediately afterward. Percentage modifiers retain their
percentage, so their derived pound contribution follows the new base naturally;
absolute modifiers retain their configured amount. Physical modifier-on drops
remain pending user verification.

After settled target readback, Live shows a two-second accented TARGET value.
It calls the optional hardware drop-alert hook, but the present K5 adapter has
no verified sound/vibration output; the shipped cue is visual. No guessed GPIO
or a different knob model's audio/haptic wiring has been adopted.

UNVERIFIED: a safe command that makes Black physically apply a reduced target
immediately during an unfinished rep. The pinned SDK 79f6c55 and Android
da514d2 setters write the same base parameter. The kirby 8e1ea1c reference uses
a load-mode reassertion in a different active-state path; this project does not
adopt load retries/reassertions. The official
[training command reference](https://cortex.beyond-power.com/docs/cortex/training-control)
documents set-weight as a target; its
[training guidance](https://cortex.beyond-power.com/docs/skills/voltra-device-training)
describes apply_force_immediately under the force-load override, which is outside
this project's no-bypass loading rules. The new cue therefore confirms a target,
not an instantaneous change in measured cable force. This timing limitation is
not represented as fixed by the shorter request queue.
