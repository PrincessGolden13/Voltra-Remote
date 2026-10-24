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
