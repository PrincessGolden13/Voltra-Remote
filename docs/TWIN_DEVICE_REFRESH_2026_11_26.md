# Twin discovery and connection recovery

CONFIRMED (owner and v0.4.19 reports): the phone still held its connection
during the earlier failed remote test. A reachable member reported a fresh
own-matched follower response with raw state 0x22. The controller attempt
timed out during Bluetooth connection establishment. The error code alone
does not identify who holds a connection.

The owner's later test reported several Twin reps. Report `865acc9d` contains
accepted base weight readbacks before the connection ended. Its headline
reason was generic; the report cannot independently attribute the failure
to the mounting detection the owner saw on the VOLTRA.

Devices requests A7 early in the read-only probe. A confirmed single can be
checked on the next eligible scan; a fresh paired result can group both
members immediately even if just one advertises. Unknown or transitional
data does not manufacture a pair or split the existing group. Probes remain
short, one device at a time, and give way to explicit selection.

A tap or Retry that reaches a fresh, own-matched follower can try its exact
reported controller with a longer radio budget (up to 55 s). After a first
controller radio failure, live evidence from the follower may justify one
such return to that exact address. An unpublished address type is never
inferred from the A7 reply. The existing Cancel button tears down the queued
or active attempt. A fresh reciprocal primary observation is required to
promote the resulting link; the normal safety and readiness checks still
apply. Startup selection and discovery probes use the standard radio budget.

CONFIRMED (v0.4.21 saved report): the first endpoint accepted the session and
reported a fresh own-matched follower with its paired controller identity. Each
of four controller GAP attempts used a saved address type with no matching
advertisement observation for this boot, then timed out before GATT. The report
does not establish why the controller could not be reached.

v0.4.22 scans for the exact reported address after follower teardown and during
existing recovery backoff windows. A matching advertisement refreshes its radio
type even without a VOLTRA service in the advertisement. The scan cannot select
another endpoint or alter the Devices list. If a saved type is available, the
direct radio attempt remains available. With no known type or matching
advertisement, the transport refuses to guess a type. Pairing is outside this
flow. Physical recovery with this change is UNVERIFIED.

[Steven's physical capture](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L241)
recovered an established pair by connecting to the known controller address
after roughly 50 seconds. [Albert's client](https://github.com/albertzhang86/voltra-remote/blob/c4f2884798941babd83788cf79609620e767bc85/firmware/VoltraKnob/src/link/voltra_link.cpp#L863)
also retries a known selected address. Neither source provides an existing pair
wake command, follower relay, or Bluetooth takeover. The longer policy is a
documented best effort, not physical proof of a particular recovery time.

## A connection that ends during use

The reviewed BLE sources expose numeric status, with no documented message
carrying the exact mounting-error text from the VOLTRA screen.
[The parameter catalog](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv#L1085)
identifies 20642 as the mounting-lock state. The owner's earlier Black capture
verified locked → released → locked as 2 → 1 → 2. A fresh matching transition
reported by the connected endpoint can be shown as a released mounting lock.
That observation does not identify the state of the other unit or prove the
cause of the disconnect. Without a specific observed condition, use a short
connection-ended notice. Only peer termination or confirmed Weight Training
exit supports attributing the event to VOLTRA.

The mid-session notice offers Back and Retry where available. Diagnostic
sending stays available in Settings. Capture the last available numeric state
and transport reason before clearing the connection, for reports that the owner
chooses to send. Cancel pending controls on loss and require a new explicit
load action after reconnecting.
