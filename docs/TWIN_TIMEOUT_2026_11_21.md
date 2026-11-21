# Connection timeout report58926a2a — November 21, 2026

## Confirmed report evidence

The newest report by received timestamp is58926a2ae2097eac7d6baa733c23b103,
2026-11-21T23:01:29Z, running v0.4.11. Candidate1 takes10,141ms and fails before
protocol acceptance or any A7/readback. Its preserved failure is Bluetooth radio
connection failed(code13). Candidate2 takes2,940ms, accepts the session and returns
fresh own-matched full A7 marker1/raw0x22, current20844=2 and nonzero21023. The
decoder correctly reports Follower. Its identity points back to the first candidate;
otherwise the resolver would have reported a changed pair. No master session is
established in this report. The final displayed error is the generic “Could not
identify the paired controller,” masking the preceding radio failure.

The reader truncates long body values around2KiB. The visible context provides
these facts; the missing event tail cannot establish why the first unit did not
answer. Wi-Fi delivered this report and v0.4.11 is installed; that does not prove
the saved network record's current contents or explain the radio timeout.

## Pinned stack evidence

CONFIRMED source, NimBLE-Arduino2.3.6: ble_hs.h defines BLE_HS_ETIMEOUT as decimal13.
This is a host timeout, not HCI reason0x0D and not proof that a phone is connected.
ble_gap_master_connect_cancelled extracts/clears the GAP master state, then emits
CONNECT with BLE_HS_ETIMEOUT when its timed procedure was not explicitly cancelled.
NimBLEClient invalidates the client handle before calling onConnectFail for this
status. It is terminal evidence, subject to the existing callback/queue/handle/GAP
handoff barriers.

CONFIRMED source defect: v0.4.11 only retries wrapped HCI connection-establishment
failure574. Host timeout13 exits immediately. The application subsequently accepts
a fresh follower, finds its other endpoint already attempted, and emits a generic
role-identification failure despite preserving the original timeout in diagnostics.

## Bounded recovery

v0.4.12 retries in the generic BLE transport only. Shorter native connection
attempts, one timeout retry, the existing establishment-attempt cap and a shared
ten-second radio budget prevent retries from extending the previous radio window.
The existing one-second MTU grace remains for an observed link. Client reuse still
requires terminal evidence, drained events, a cleared handle and idle GAP. Missing
terminal evidence cannot be replaced by a time-only assumption. No separate scan
is needed: GAP initiation already listens for the target. No application third
endpoint visit or retry layer is added.

The application preserves the actual earlier connection failure when a fresh
follower points back to that failed endpoint, and for a generic overall deadline.
Changed or contradictory topology remains the higher-priority error. Retry still
revalidates the saved navigation candidate; cached data never supplies a role.
Failed pre-acceptance traces are compact, and failed-selection context omits a
duplicate closed-session snapshot. Accepted endpoint status remains available.

INFERRED benefit: a second initiation can recover a transient missed advertisement
or recently closed connection without making the previous maximum wait longer.
The report cannot distinguish those causes. Physical recovery remains UNVERIFIED.
All Twin role evidence remains limited to read-only presentation; no Twin
resistance command, combined limit or motor state is inferred.

Existing build, source review, UI and release receipts follow in TESTING.md. No
physical connection or motor operation was performed, and no new automated test
source was added.
