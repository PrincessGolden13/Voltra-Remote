# Pre-authentication timeout — report b47444b1

Received 2026-11-22T18:39:27Z, firmware 0.4.13. This is the newest report by receipt
timestamp, not row order. The owner identifies the first attempted unit as the
primary.

## Confirmed observations

- Candidate1:9656ms elapsed, no protocol acceptance; transport stage3/Radio,
  final code13/host timeout, two attempts,9518ms radio elapsed.
- Requested address type0 and actual type0 match. `typeObserved=1` confirms the
  current boot's learned-type lookup was used. It does not prove current radio
  availability or an address-type correction in this attempt.
- Candidate2:2477ms elapsed, accepted session and fresh full own-matched A7 with
  marker1/raw22, distinct other identity and the secondary profile. No primary
  control promotion, weight write or load operation is reached in this report.
- Visible candidate2 events show native establishment failure574 after239ms,
  backoff500ms, then successful connection/authentication. Client handoff and the
  establishment retry therefore work in this sequence.
- The original primary timeout remains the final user-facing error. Missing
  candidate2 motor/settings and activation0 are not evidence of another failure:
  selection deliberately ends once the secondary points back to the failed primary.

Radio stage means no usable link was announced to the controller, so discovery
and VOLTRA authentication never started for candidate1. The summary does not
exclude a fleeting lower-stack link event before failure; its individual attempt
events are unavailable. Two attempts do not prove both ended with13: only the
final failure code is retained in this summary.

## What this report cannot establish

The event list starts at439408ms, the second candidate's start. The primary's
earlier per-attempt detail has fallen out of the20-event ring. Its persistent
snapshot retains stage/code/attempt count/types, but not advertisement age, RSSI,
connectability or earlier link-up evidence.

The visible42815ms observation age belongs to candidate2, not candidate1. Unknown
RSSI/connectability means the cached signal exceeded the15-second freshness policy;
it does not mean a unit was nonconnectable. The body reader also truncates long
values around2KiB, but a later tail cannot restore events already evicted before
the displayed first event.

No evidence establishes that another app is connected, that the primary is out of
range, that its advertisement stopped, or that a particular remote code defect
caused this timeout. The postconnection Twin control corrections are not exercised
by candidate1's pre-authentication failure.

## Source comparison and next discriminating observation

Current public heads were rechecked: Albert knobc4f2884, Albert Android8f93880,
Steven1c14860. Steven's captured paired controller stopped advertising while the
secondary remained accessible. Its once-observed approximately50-second recovery
was cumulative10-second attempts with increasing backoff, not one long timeout;
current README warns recovery can take a minute. Albert separately documents
type0/HS13 when paired while standalone worked. These are firmware-specific
observations, not proof of this owner's current radio condition.

No checked source supplies a read-only secondary command that opens primary access
or relays pair controls. Both working remotes stop scanning before connection.
An explicitly started, cancellable primary recovery session could reproduce the
documented cumulative retry strategy, but is best-effort rather than a confirmed
repair for this failure. Existing explicit Retry already permits repeated sessions.

Whether Beyond+ can connect specifically to the same primary while the pair remains
active and the remote is disconnected is a useful physical discriminator. It does
not require changing pair membership or resistance. Successful app access would
narrow the investigation to remote-specific connection behavior; the same failure
would support a trainer access/availability limitation. Neither outcome alone
identifies every possible RF or controller cause.

## Confirmed Wi-Fi/Bluetooth ordering defect and v0.4.14 correction

`UiController::tick()` previously polled the application before pausing Wi-Fi.
Manual selection, Retry and automatic selection queue the native connection for
that next poll. Its first GAP attempt could therefore start before the same
frame's `WifiSetup::tick(false)` call. This violates the documented policy in
TRAINING_WIFI_2026_10_31.md that Wi-Fi pauses during VOLTRA connection attempts.

Pause Wi-Fi before polling an existing or queued connection. Sample the UI and
application clock after that potentially blocking shutdown. Retain the post-poll
Wi-Fi handoff when the application becomes idle, so debug uploads and saved-network
update checks can resume immediately. Do not repeat the pause twice in a frame
that remains connected/connecting.

Pinned Arduino-ESP32 2.0.17 `WiFi.mode(WIFI_OFF)` calls stop/deinit synchronously.
The existing Wi-Fi adapter does not propagate a failed shutdown result, so this
correction guarantees request order, not proof the physical radio is off. Wi-Fi/
BLE coexistence is supported by the platform; the report does not prove overlap
caused this particular timeout.

CONFIRMED offline MOCK differential checks reproduce the old order and verify
the corrected manual, actual Retry and automatic paths. A simulated 37 ms
shutdown verifies fresh application timestamps. Explicit pending unload, first
idle-frame report handoff and saved-Wi-Fi update checks pass. Existing update
previews have identical output/pixels in 39 cases. See TESTING.md for scope.

The effect on Black's physical connection remains UNVERIFIED. Connection timers,
retry budgets and Twin protocol/control routes are unchanged by this correction.
No A8, physical connection, resistance operation or settings reset was performed.
