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

[Steven's physical capture](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L241)
recovered an established pair by connecting to the known controller address
after roughly 50 seconds. [Albert's client](https://github.com/albertzhang86/voltra-remote/blob/c4f2884798941babd83788cf79609620e767bc85/firmware/VoltraKnob/src/link/voltra_link.cpp#L863)
also retries a known selected address. Neither source provides an existing pair
wake command, follower relay, or Bluetooth takeover. The longer policy is a
documented best effort, not physical proof of a particular recovery time.
