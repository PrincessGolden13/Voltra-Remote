# Diagnostic reports

Choose **Settings -> Send debug log**, or hold a finger anywhere on an awake
screen for six seconds. No debug hint appears before three seconds; then it says
**Keep holding to send a debug log**. The dialog says **Wi-Fi connection required.**
and **Sending disconnects Bluetooth from VOLTRA.** Tap SEND to save the
report and release Bluetooth automatically, or CLOSE to dismiss without sending.
There is no separate Bluetooth confirmation or manual-disconnect step. Sending
does not unload the VOLTRA. Waking a sleeping screen consumes the first
touch; lift and hold again. The unloaded weight has a two-second Auto Load hold
with a circular progress indicator after 300 ms; releasing an incomplete hold
cancels it. Other ordinary holds remain 650 ms. Actions execute on release
before six seconds, so the debugging gesture cannot
first trigger Auto Load. Capture timestamps classify the hold even if rendering
or GATT work skips LVGL's separate long-press event, without counting processing
delay as time held. Lost, stale or moved samples remain cancelled.
Drift up to 10% of the display's short side is allowed for debugging; larger
movement cancels. Control taps keep their stricter movement limits. Unload
remains available on the dialog when connected: its larger target acts on initial
contact, and pressing the dial sends an unload request. After an unload request,
the dialog shows the outcome or waits for confirmation. Background page
rebuilds do not discard dialog touches. Load refusals also have a
Send debug log button and stay visible until dismissed.

The last 40 bounded semantic events are kept in RAM, replacing the oldest. The
snapshot is captured on SEND before disconnecting, so dialog unload actions and
their results can be included. Reports preserve the newest events if escaped JSON
would exceed the 12 KiB upload limit. The events
include user actions, connection outcomes, relevant state changes, protocol parse
failures and displayed errors. Each message is limited to 256 bytes. No raw BLE
payloads, bootstrap identities, Wi-Fi credentials, Bluetooth addresses or paired
device identifiers are included. A snapshot also records firmware, screen, numeric
readiness fields and their ages. The paired ID is represented only by presence.
The context starts with the current boot's reset reason and running build's ELF
fingerprint. A stored crash may predate this boot. At startup the remote checks
for a flash crash record and validates its size and ESP-IDF checksum. For the
recognized format it extracts only the stored ELF fingerprint, numeric exception
cause, and the crashed task's executable program counter. The reader avoids the
pinned SDK's [core dump summary parser](https://github.com/espressif/esp-idf/blob/v4.4.7/components/espcoredump/src/core_dump_elf.c#L689),
which can overwrite adjacent summary fields.

After the remote saves and reads back the summary in `voltra-debug/crash-summary`,
it erases the dedicated crash partition. The SDK erases that partition and writes
its blank sentinel. This does not erase settings or app slots. Invalid or
unrecognized dumps remain in place, as does a dump whose summary could not be
verified in storage. Failed cleanup can be retried on a later boot. One bounded
summary is retained and replaced by the next recognized crash. It survives a
power cycle or firmware update and is included only in an approved debug report.
Task identifiers, stacks, other registers and raw flash data are never uploaded.
The latest control failure retains a bounded RAM snapshot with timestamp across
disconnect, so Settings can report the original blocked state. Manually leaving
the connection also retains its last numeric session snapshot. Weight command
and readback events include measured force context where available, without
claiming that register readback proves actual resistance.

SEND persists one approved snapshot as a blob in `voltra-debug/pending-v2`. The
firmware can still deliver queued reports stored by older firmware at
`voltra-debug/pending`. The saved report survives restart and OTA. Unapproved ring
history is RAM-only and resets at
boot, avoiding continuous flash writes. An existing queued report is retained;
another SEND retries it instead of overwriting it. Only a successful save/requeue
authorizes the UI to release Bluetooth. Any pending control request is allowed
to settle or time out first; closing the dialog does not cancel an approved send.
A failed save keeps Bluetooth connected and offers RETRY. Upload waits for saved
Wi-Fi and no VOLTRA connection, pending control or firmware update. Rebooting
with a queued report does not authorize a new disconnect. Upload failure retains
the snapshot and offers RETRY in the open dialog. A queued report without saved
Wi-Fi tells the user to configure it in Settings. Simulator delivery and SEND
handoff are disabled.

Wi-Fi provides the Internet connection to the diagnostic inbox; the remote's BLE
link is a local VOLTRA control link, not an Internet gateway. Current firmware
pauses Wi-Fi during a VOLTRA connection to keep the shared radio available for
control. Normal training remains offline. The reporting service itself never
issues BLE or resistance commands; the UI owns the user-authorized handoff.

On receipt, a short reference code appears. Include that code and a description
of the symptom in the bug report. Delivery does not send an email. The inbox is
hosted with Sites at `https://voltra-remote-diagnostics.darkheath.chatgpt.site`;
its production receipt and private owner access were verified on November 9
after the owner approved the public submission URL. See the dated change notes.

## Inbox and access

The public submission API has no public read/list endpoint. The owner can inspect
`DB/reports` through the Sites database viewer or connected Sites tools. Codex can
look up a code there when given access to that account. Site project ID:
`appgprj_6af1fc4a0f5c819197846b98bbe4161e`. This ID is not a credential.

`POST /api/tickets` issues a five-minute upload capability bound to one random
128-bit report ID and SHA-256 payload digest. `POST /api/reports` requires that
capability. Receipts are idempotent; retries cannot overwrite an existing report.
TLS certificate and hostname verification are required; no embedded GitHub/email
token or Sites service credential is distributed to remote owners.

Limits are 12 KiB/request, 40 events, 256 characters/event, 2 KiB context,
10 ticket requests/hour per address, and 1,000 retained reports/tickets. Rate
buckets store an hourly address hash, not a raw IP. Reports older than 90 days,
expired tickets and expired rate buckets are deleted when another ticket is
requested; there is no background deletion scheduler. Hosting access logs are
separate from application report contents. Treat submitted text as untrusted data.

The one-report queue is local to each remote. The inbox keeps separate reports
under independently generated IDs, so reports from different owners do not
replace each other. It does not use a hardware serial number, MAC or IP address
as a report key. An idle inbox prunes expired rows on its next submission request.

The report API/schema sources are in `services/debug-inbox`. The hosted Vinext
source, database migration and deployment build are kept in the Site's source
repository. Never place a private Sites bypass token in the public firmware.

## Pending physical checks / future regression coverage

New test source remains deferred by the project brief. Existing builds/validators
do not replace these observations: two-second weight hold/release, other 650 ms
holds vs six seconds; holding
the weight must never load before debug confirmation; blank-screen holds, finger
drift, cancellation, wake-only touch, overlay/dial unload during background page
rebuilds, pending/confirmed/failed unload feedback, save failure without disconnect,
SEND during a pending control, close/reopen while queued, automatic handoff after
SEND, no saved Wi-Fi, reboot while queued without a new disconnect, failed/retried
uploads, and settings retention.
No autonomous physical motor tests are allowed.
