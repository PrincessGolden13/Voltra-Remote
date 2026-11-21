# November 21: report 357833b6 and Twin status compatibility

Report 357833b66af37b17bc9f32cbe50f2e59 arrived November 21 at21:14:02 UTC from
published v0.4.9. It confirms acceptance/activation and fresh motor0, workout1,
base65, LB, local maximum250, coworker2, remote status0x02005801, historical
role1 and peer Auto Load raw0. Full-length22/23-byte A7 messages are rejected
with shape0. The dependent ownMatch0 does not establish an address mismatch.

This capture no longer has the earlier unknown A003 motor state. It does not
establish current master ownership, combined limits or required remote-unit
safety semantics. A recently read historical role remains historical.

## Supported correction and missing evidence

The old parser required valid batteries, marker1 and a narrow state whitelist
to retain the entire identity record. Albert c4f2884 keeps identity with optional
batteries and uninterpreted marker; its physical validation records the transition
11 -> 01 -> 12. These support separating structural identity from interpretation,
not a claim of which check rejected this report. v0.4.9 did not retain the rejected
header fields; another report from that version cannot identify the failed check.

The new decoder retains only the documented full envelope, exact address slots,
nonzero/distinct identities and successful direct-response prefix. Unknown
batteries are unavailable. Marker/raw state remain observations; only consistent
00/no-second-address or12/distinct-second-address establish known topology.
Unknown/transitional state cannot authorize combined totals or commands.
Physical current role remains Unknown.

New reports include rejection reason, frame type/direction/protocol, scalar
battery/marker/state bytes, identity-presence/distinctness and selected-address
matches against both slots. These Twin status fields contain no address bytes
or device names; the standard report still includes the last20 events.
Slot order does not establish role. Reviewed public A7 captures are from the
controller; a follower capture is still needed to verify slot interpretation.

Accepted/rejected pushes/replies share a30-second event-log budget so alternating
22/23-byte messages do not consume all20 events. Latest detail always remains in
diagnostic state. Bounded read-only probes put snapshots first within the2048-byte
context. A fresh matched identity can nominate an exact saved/scanned candidate
even when state is unknown; this is not role proof. Automatic inspection of both
units depends on resolving that identity. A match only in the second slot is
reported but cannot reverse the address interpretation or authorize controls.

## Diagnostic publication

The previous hold was on claiming a completed Twin-control fix. v0.4.10 is now
published as an explicitly diagnostic update so the owner can obtain fields missing
from v0.4.9 through the remote's update workflow. It includes the checked screen
and battery work. Physical Twin resistance commands remain unavailable. An
unresolved master stops selection and offers Send debug log using existing consent.

Additional source checks cover current Albert, Steven watch206, Kirby Ring20946aa7,
Ring Nano16405e01, Robert4afe8824, Philip/Kirby8e1ea1c, Hagestedt8289454a,
SDKc76a523e and Androidda514d2. None supplied a verified current-role/remote-lock/
combined-limit decoder. Official Cortex exposes role after managed pairing but
publishes no raw direct-BLE decoder. Findings are bounded to reviewed sources.

No physical motor, pairing, role-change or power operation was performed. No new
automated test source was added. Existing published fixtures, builds, LVGL previews
and firmware/artifact checks are recorded in TESTING.md.
