# Twin handoff, Auto Load feedback and direct Live entry

Report `24b52042` was captured on v0.4.22. It records an earlier selection
failure reading “VOLTRA connection changed; reconnect.” The current connection
was an accepted, own-matched Twin controller. The log records a confirmed load
(motor readback 257, then 1) followed by confirmed unload (0). About 660 ms
elapsed from the first load readback to the unload request. The report
recorded zero B4 and AA81 frames and two
telemetry setup steps. It does not establish why the device sent no visible
stream or whether it would have sent counters during a longer set.

The same report shows fresh `21272=0` and `21389=0`. These are the Direct Load
enable and Safety Check enable fields in the pinned [parameter catalog](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv).
The remote requires both to be enabled before sending Auto Load. The old hold
captured that capability only at touch-down. It could go straight from no
feedback to the six-second debug gesture. The hold now follows current support
and shows a short unavailable message if the capability is absent. The
two-second release still goes through normal readiness. Holding for six
seconds still opens debug without sending Load or Auto Load.

## Adopt an existing pair while connected

The previous Ready single selection treated a new Twin indication as a reason
to close the connection. It now waits on the accepted link with standalone
controls stopped. If the endpoint reports a current own-matched primary role,
the remote keeps that link and adopts the combined readback after it settles.
If it reports a follower role, the remote waits for an idle motor and reset
Auto Load status in the same fresh direct reply, strictly after the new role.
That observation only permits leaving the follower. It does not describe the
pair's resistance state or authorize a command.

The remote then uses the existing bounded address recovery and verifies the
controller's reciprocal relationship before promoting it. Unknown or stale
role, wrong identity, armed or loaded local state, and unknown motor state
cannot authorize this handoff. Prior settings, load requests and automatic
drop recipes are discarded. A hold or tap begun in the previous scope cannot
load the new pair. Unsent editor changes and unread gestures are also dropped
when the scope changes. The confirmed primary unload route stays
available while the same connection receives new readback. This flow changes
the remote's connection to an existing pair. It sends no pairing operation.

## Live after direct Twin load

The confirmed load opens Live without waiting for the first rep or its
counters. This avoids trapping the display on Adjust if the stream is absent
or delayed. Live retains explicit unload and shows the two names plus actual
set and rep reports when received. Its twin target value remains unavailable
until a verified device field can reproduce the controller display.

On the first confirmed direct Twin load, reserve one paced state-refresh
turn ahead of routine reads. The same `AA13 01` observation request already
runs periodically when the controller is loaded, as captured in [Steven's
protocol notes](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L264).
Five 1300 ms polling groups can fill every paced turn at 260 ms per turn,
delaying the old refresh path. The new turn is bounded to one per verified
direct load and waits for any pending control. No telemetry mask, sample
rate, motor command or live-weight formula is added. The diagnostic event
records a successful transport send. Physical stream recovery remains
unverified.

See [validation](TESTING.md) for the checked scenarios. All automation uses
mock I/O. No physical strength command or radio connection was attempted.
