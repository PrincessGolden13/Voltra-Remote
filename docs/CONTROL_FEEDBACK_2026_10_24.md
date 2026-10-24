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
