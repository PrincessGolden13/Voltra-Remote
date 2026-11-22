# Device-screen Twin refresh and connection setup

## Follow-up report version requires confirmation

Report `049f991fb40fe58932000fcd24d55d72`, received 2026-11-22T21:50:23Z,
identifies firmware 0.4.14 in both the JSON header and captured context. It shows
the old first-member timeout followed by establishment retries and link error
531 before acceptance on the second member. It does not establish a v0.4.15 trial.

The currently published latest manifest and Normal image were rechecked as
v0.4.15, matching the reviewed tag's length, descriptor and SHA-256. Report,
context, Installed label and descriptor share one version macro; the checked
Normal binary contains their 0.4.15 strings and none of the corresponding
0.4.14 strings.

A pending report is stored as complete JSON in NVS, restored unchanged after
reboot/update, and reused by submit until successful delivery/clear. Thus this
snapshot's version does not prove the currently running image. Receipt time is
upload time. The current Settings -> Updates -> Installed value is required to
distinguish an old running/rolled-back image from a queued old report before
classifying this as a v0.4.15 connection regression. No new firmware change.

## New physical evidence

The owner reports that the Beyond+ iPhone list showed only Red. Connecting
to Red discovered its existing relationship with Black and allowed Twin operation.
This is evidence that the official app can reach the pair through that entry;
it does not reveal whether it retains Red, opens Black, or uses another route.
It also does not establish the current endpoint roles after that visit.

Latest report `27aee9ac268c9ae8faa53cfadbab5936`, received
2026-11-22T21:05:47Z on v0.4.14:

- First candidate: 9794 ms, Radio/stage 3, final host code 13, two attempts,
  9521 ms radio elapsed. Both visible failures are code 13. Learned type 0 is
  used; the observations are approximately 129–134 seconds old.
- Second candidate: usable link/MTU 247 at 257368 ms, then disconnect at
  258028 ms, 660 ms later. No accepted VOLTRA session or fresh A7 is recorded.
  Final Link/stage 7, code 531, one attempt, 874 ms radio elapsed.
- Pinned NimBLE defines 531 as HCI base 0x200 plus remote-user termination
  0x13. This reports the peripheral ending the established link; it neither
  identifies the reason for that decision nor proves another app is connected.

Earlier raw-0x22 observations cannot establish the second endpoint's current
role. Its secondary interpretation remains a scoped inference from previous
owner-identified endpoints. No new relay or resistance profile is invented.

## Confirmed source corrections

The old selector always started at remembered primary navigation, including
when only the other member advertised. It then opened a second read-only session
to resolve the pair. This can impose an unnecessary timeout before trying an
advertising member.
Start with a member from the latest completed scan, retaining normal fresh-role
control admission afterward. Both circles remain one logical selection. Neither
saved names nor this discovery entry choose the physical control role.

The old controller discovered/subscribed notification channels before the first
hello. Steven's native implementation sends hello after targeted write-channel
discovery and before descriptors, explicitly documenting the VOLTRA's short
hello window (`stevenpoint/voltra_remote` 1c14860, voltra_client.cpp:1124).
Use the same early order, with a protocol-owned hello-only preparation hook.
Preparation sends exactly one original 0x4f frame; it cannot accept the session
or authorize a control. Complete notification setup, then continue bootstrap
with fresh clock/pacing. Every failure/disconnect clears preparation. This
corrects a supported timing difference; the report does not prove it caused 531.

The Failed selection phase previously returned before discovery refresh. Keep
scanning after failure so Retry can use newer observations. Generic connection
text replaces the changing per-device Checking messages.

## Background topology policy

Advertisements have no documented Twin membership/role field. While idle on
Devices, use one bounded read-only session after a completed scan to request
current machine status. Keep tiles visible; a tap or navigation cancels the probe.
No training state, selected identity, load indication, automatic mode entry or
control permission escapes the background session.

Application policy constants, not protocol facts:

- Existing scan cadence: 5 seconds, 2-second native scan windows.
- Per-endpoint probe cooldown: 15 seconds, rotating currently advertised members.
  One fresh pair reply covers both identities for this cooldown.
- Pre-link window: 4.5 seconds from queueing, including handoff and Wi-Fi pause.
  Existing fast establishment retries may fit; no application retry loop is added.
- After GATT setup: a 3-second application window for an accepted fresh topology
  observation. Loop deadlines cannot interrupt blocking GATT operations; the
  pre-link deadline is also checked between polls, not during such an operation.
- Presentation TTL: 30 seconds. Control freshness remains the separate 2-second
  policy. A failed/unknown/stale reading never means the pair has been unpaired.

Fresh Alone removes reciprocal navigation, including old bootstrap metadata.
Fresh paired identity updates grouping. An unseen partner may retain a bare
adapter-owned address identity and generic name; no radio type is guessed.
Connecting that bare identity requires an observed type. Later advertising names
and types merge with the same logical identity. Stored metadata remains navigation
only and does not reconfigure either VOLTRA.

Wi-Fi pauses before a queued probe reaches GAP. Radio ownership is separate from
the user connection flag, and is included in report/update/Wi-Fi handoff guards.
Background work stops for pending touch, overlays, settings, startup/update work,
report submission or display sleep. Explicit training unload retains its route.

## Crowded discovery

Both native references use NimBLE callback-only scan storage. The previous
64-result SDK limit discarded later advertisers before callbacks; our own result
map also admitted unrelated neighbours before the application service filter.
Use callback-only mode and a generic profile-configured service filter before
bounded result/type collection. The application supplies the VOLTRA UUID; generic
BLE contains no VOLTRA UUID. Both maps remain bounded to 64 matching identities.

## Validation scope

Independent source review and existing core/packet-inspector build pass.
External actual-controller/PhysicalSession offline MOCK checks pass 11 scenarios /
111 assertions and all 31 existing PhysicalSession regressions. These verify
early-hello order, exact wire bytes, failure/reconnect cleanup, fresh timers,
unaccepted isolation and existing control guards. See TESTING.md for receipts.
Actual LVGL/application checks pass 72 cases / 1566 assertions at 360/466/480,
including pending async scans, periodic membership changes, Retry, cancellation
and radio handoffs. These are synthetic observations, not physical BLE trials.
Physical authentication compatibility, current roles and the actual Red-to-pair
route remain unverified. No pairing, physical connection or resistance operation
was performed during development.
