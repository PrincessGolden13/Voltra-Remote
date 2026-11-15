# November 15: debug dialog unload and Bluetooth handoff

Baseline: v0.4.4 / main 2e76d66526f2a9446a1c4547682be921cebad29d.

CONFIRMED (inbox): the owner's photographed report is `e354d15a`, firmware 0.4.4.
Its context has empty last-closed-session and last-failure snapshots. Its eleven
events show touch-register ACK/readback (0xFB=0, 0xFC=0, 0xFE=1), startup, a dial
event on Devices, opening Settings and choosing Send debug log. It contains no
connected session, unload request or control refusal. It proves delivery, not the
cause of the earlier RP or loading failure. The old snapshot was captured when
the dialog opened, so later dialog interactions could not appear in that report.
No resistance/protocol correction is justified by this report alone.

CONFIRMED (source): rebuilding an obscured page called wait-release and discarded
buffered input even while the debug overlay remained open. Overlay controls also
inherited the hidden page's stricter picker drift threshold, and unload had no
visible result/status in the dialog. These are concrete input/feedback defects;
which caused the owner's particular failed unload remains UNVERIFIED.

The overlay now preserves its input through underlying rebuilds and uses the
ordinary control drift threshold. Its larger UNLOAD target acts on initial press,
so a drifting/lost release cannot prevent an explicit unload. Dial press in this
dialog is also unload-only. Pending, reported-unloaded, unknown and failure states
are visible; numeric unload results enter the ring, and a rejected request retains
its control snapshot. No unloaded state is invented from a successful BLE write.

The owner now explicitly requests automatic Bluetooth release when sending.
SEND explains the disconnect/Wi-Fi handoff rather than asking another question.
After a successful save/requeue, the UI waits for any pending control to finish
or time out, leaves the connection, disarms local drops and returns the underlying
page to Devices. It sends no automatic unload or load. New non-unload actions are
blocked while the handoff is pending. Closing/reopening the dialog cannot cancel
an approved handoff. Save failure retains the link and offers RETRY; upload failure
retains the original queued report and offers RETRY. Reboot restoration alone
never authorizes a Bluetooth disconnect. Normal controls still work offline.

Snapshots now capture on SEND, before leaving the session, including any earlier
dialog unload actions. Existing queued reports retain their original contents.
Wi-Fi is the path to the online inbox; BLE connects to the machine. This firmware
pauses Wi-Fi during control to prioritize the shared radio. This does not establish
that simultaneous Wi-Fi/BLE is impossible on the hardware.

New test source remains deferred by the project brief. Physical touch, unload,
Bluetooth release, Wi-Fi delivery and OTA retention require owner observation;
no autonomous motor test or USB flashing is authorized or performed here.
Build and existing-validator evidence is recorded in TESTING.md after execution.
