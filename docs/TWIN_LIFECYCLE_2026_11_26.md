# Twin workout transitions and rep timing

Report `00886881` on v0.4.23 records `VOLTRA left Weight Training; reconnect`
before a later successful Twin connection. This matches the notice in the
owner's photo. The earlier code closed an accepted connection as soon as the
device reported an idle exit from Weight Training. This could happen before
the endpoint reported its new role. There is no raw packet from the earlier
exit in the bounded report.

An idle exit to the home mode from a connected single now gets an eight-second
read-only check with at most six topology requests. Pending controls and input
from the old workout are cleared. An own-matched,
fresh primary keeps the current link. A follower can hand off to its exact
reported controller after a fresh direct local-idle observation. The new
endpoint still has to confirm the reciprocal pair. A normal exit without
verified Twin evidence keeps the existing connection-ended notice once local
idle is confirmed. An explicit standalone return to Weight Training can keep
the link after fresh device confirmation. If the verified primary is still at
home, any one-time training entry uses the current idle and normal application
guards. No resistance request is restored or sent by recovery.

## Enter and leave Live during training

After a regular Twin load, stay on the weight screen until fresh machine rep
activity reports the first movement. An active workout status alone is also
present immediately after loading and does not start Live. After an observed
set, the reported workout rest or end returns to the weight screen even if
the motor remains loaded. Idle rep phase alone occurs within sets and cannot
be used to end one. The user confirmed that set and rep counts are accurate.
The report shows 57 decoded extended AA81 frames and no B4 frames. These
display observations remain separate from automatic weight or load permission.
The paired AA81 rep phase and AA80 workout status have independent meanings.
The AA80 parser requires the exact previously supported Weight Training body
and records received and parsed shapes so Twin delivery can be checked.
[Steven's captured workout telemetry](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L95-L116)

## Combined live weight

The reviewed sources do not establish a field that matches the changing
combined weight from the Twin controller LCD. A B4 word between its motion
channels is unverified and did not appear in this connection. The extended
AA81 suffix may be follower-scoped under one reference's interpretation.
The catalogued signed motor target `20859` is expressed in tenths of a pound
but its relationship to the combined display is unverified. Sample it only
in up to three isolated read-only diagnostic requests per accepted Twin link.
Keep the raw numeric suffix observations bounded and separate from UI weight.
Comparing known pull and return values on the controller during the same
set with a report can distinguish those fields. Never use base, force or a
multiplier as the real Live target.
[Albert's captured B4 and target scope](https://github.com/albertzhang86/voltra-remote/tree/c4f2884798941babd83788cf79609620e767bc85)
· [Android parameter catalog](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv#L869)

## Auto Load refusal

The report captured fresh `21272=0` (Auto Load enable) and `21389=0` (Direct
Load Safety Check enable) from the verified Twin controller. Both conditions
block the remote's trigger. Native long press can use the machine's optional
Safety Check configuration. The remote keeps the project's requirement for
both reported flags, with fresh same-scope values. The hold and refusal
now name a known disabled setting. Unknown reports remain unavailable.
This does not configure either VOLTRA or bypass a safety check.
[Beyond Power Safety Basics](https://help.beyond-power.com/en/articles/14167229-safety-basics)

See [validation](TESTING.md) for mock scenarios and build checks. Physical
handoff, workout status delivery and exact Twin target remain to be verified.
