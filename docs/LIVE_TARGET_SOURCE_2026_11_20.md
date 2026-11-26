# November 20: distinguish target resistance from runtime wire weight

The additional public source
[albertzhang86/voltra-remote](https://github.com/albertzhang86/voltra-remote/tree/c4f2884798941babd83788cf79609620e767bc85)
at c4f2884798941babd83788cf79609620e767bc85 supplies evidence that supersedes
v0.4.9's use of runtime register 16003 as the Live primary number.

CONFIRMED within that source: `test/test_protocol.cpp` lines137–149 contains
four real independent B4 captures. Their first payload word is measured cable
force of 1.0–1.2 LB; their final word is target resistance of 6 LB. The separately
captured register 16003/0x3E83 readback at lines276–279 is 1 LB while the configured
base was 6 LB. Runtime weight therefore cannot substitute for target resistance.
The source README explicitly leaves dynamic target-to-primary-LCD agreement
unverified; its physical Twin force verification concerns measured force, not
target resistance or a combined target.

`src/protocol/voltra_protocol.cpp` lines243–280 decodes independent B4 source
variants0/1/2, with local payload lengths6/10/10 bytes and an optional trailing
uint16 little-endian target in tenths of LB. A missing target or sentinel0xffff
clears the target. Source occupies the usual frame protocol field and differs
from ordinary command protocol0x0020. Native Twin variants0x10–0x12 have different
scope and must not be accepted as independent target reports.

The application now decodes only those exact independent source/type/direction/
length combinations after framing and CRC verification. Live uses the fresh
device target with known loaded standalone state and displays whole pounds by
numeric rounding. Unit conversion is presentation only; no base, modifier,
phase, runtime-register or cable-force fallback supplies a missing value. Target
freshness is independent of settings polls. Twin targets remain unavailable;
no local target is doubled into a pair total. This read-only telemetry change
does not enable resistance commands, alter subscription rates or establish
physical compatibility with the owner's VOLTRAs.

## Twin Live counters in v0.4.22

The Live display also reads the captured Twin `AA 81 42` subtype documented in
[Steven's protocol notes](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L95-L102).
It uses the controller's set byte and big-endian rep count. B4 source `0x12`
takes priority while fresh. Both sources require a current loaded primary and
expire independently. This path supplies display counts only. The owner's
Twin Auto Load flow still needs a physical counter check. The matching live
target for a Twin is still UNVERIFIED.
