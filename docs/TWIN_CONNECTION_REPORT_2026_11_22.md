# Pre-authentication timeout — report b47444b1

## Follow-up: v0.4.14 report fc71b3c6 still fails

Received 2026-11-22T19:24:21Z, full report ID
fc71b3c63b65b4d44a02449044b7c58a. Firmware 0.4.14 therefore does not resolve
the owner's primary connection failure.

- First candidate: 9894 ms elapsed, accepted=0, Radio/stage 3, final code 13,
  timedOut=1, two attempts and 9519 ms radio elapsed. Requested/actual type 0,
  typeObserved=0. The retained final primary failure at 187566 ms reports unknown
  observation age, RSSI and connectability. No VOLTRA authentication was reached.
- Second candidate: 2361 ms, accepted=1, own-matched fresh A7 marker 1/raw 0x22,
  interpreted as secondary, with distinct partner and age 8 ms. The preserved
  first failure means its reported partner matched the first endpoint; otherwise
  runSelection would have failed with the explicit changed-pair error.
- Secondary observation age 58570 ms belongs to that second candidate, not the
  primary. Its first attempt fails 574, then a retry connects.

`typeObserved=0` means the transport found no matching learned entry when it
launched the attempt. It is not proof the primary never advertised: entries can
be evicted, and the scanner has a separate visibility limit described below.
The previous v0.4.13 report did use observed type 0 and still failed with 13,
so a stale type alone does not explain both reports.

Independent reviews found no wrong-MAC conversion or hardcoded primary route.
The A7 byte order matches both native references. NimBLE 2.3.6's async connect
flags are correct and use the same GAP initiation procedure as Steven's blocking
call. Final code 13 is expiry of that procedure, before GATT/hello/control code.
Neither the log nor those checks identify the physical cause.

The reference's verified flow connects the primary before pairing and retains
that connection. Reacquiring an already-paired primary is explicitly best effort.
No checked implementation supplies a read-only secondary relay or reopen command.
Official Cortex's `twin connect` is a PAIR alias, not such a recovery command.
Another timer change or unrelated discovery fix cannot be claimed as a proven
repair for this timeout. No new product change/release is justified by this review.

The required distinguishing observation remains: with the remote disconnected,
fully close/reopen Beyond+ and freshly connect specifically to the same primary
while the existing Twin pair remains unchanged. Successful fresh access would
narrow investigation to remote interoperability; failure would support a paired
endpoint availability problem. Merely keeping an app connection from before
pairing does not answer this question. No load or pair change is needed.

### Separate discovery limitations found, not established timeout causes

Our scanner's `setMaxResults(64)` makes pinned NimBLEScan discard subsequent new
advertisers before callbacks, whereas both native references use callback-only
`setMaxResults(0)`. Our own advertisement map also caps arbitrary neighbours before
the application service filter. These limits can hide a VOLTRA in a crowded scan;
fixing only the stack cap would leave the second limit. The direct connect procedure
does its own radio initiation and does not require that result map, so these are
not proof of the observed host timeout.

The Failed selection phase also returns from the application tick before discovery
completion/refresh. Retry immediately reuses the saved candidate. This can explain
old observations while a notice stays open, but not establish why a previously
observed correct address also failed. Track these as discovery/Retry coverage gaps;
do not publish them as an overall Twin recovery claim.

No product code edit, physical connection, resistance operation, pairing or reset
was performed in this follow-up review.

## Original v0.4.13 review

Received 2026-11-22T18:39:27Z, firmware 0.4.13. This is the newest report by receipt
timestamp at the time of that review, not row order. The owner identifies the first attempted unit as the
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
