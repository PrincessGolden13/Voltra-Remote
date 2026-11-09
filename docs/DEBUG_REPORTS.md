# Diagnostic reports (v0.4.3)

Hold a stationary finger anywhere on an awake screen for six seconds. The remote
asks **Send last events for debugging?** Choose YES to save and send, or CLOSE / NO
to dismiss without transmitting. Waking a sleeping screen consumes the first
touch; lift and hold again. Ordinary 650 ms hold actions now execute on release
before six seconds, so the debugging gesture cannot first trigger Auto Load.
Movement cancels the gesture. Unload remains available on the dialog when connected.

The last 20 bounded semantic events are kept in RAM, replacing the oldest. They
include user actions, connection outcomes, relevant state changes, protocol parse
failures and displayed errors. Each message is limited to 256 bytes. No raw BLE
payloads, bootstrap identities, Wi-Fi credentials, Bluetooth addresses or paired
device identifiers are included. A snapshot also records firmware, screen, numeric
readiness fields and their ages. The paired ID is represented only by presence.

YES persists one approved snapshot in the separate `voltra-debug/pending` NVS key.
It survives a restart or OTA. Unapproved ring history is RAM-only and resets at
boot, avoiding continuous flash writes. An existing queued report is retained;
another YES retries it instead of overwriting it. Upload waits for saved Wi-Fi
and no VOLTRA connection, pending control or firmware update. It never disconnects
a VOLTRA, unloads, or changes resistance automatically. Close the dialog and tap
the VOLTRA name to release BLE when ready. Upload failure retains the snapshot;
hold again and select YES to retry. Simulator delivery is disabled.

On receipt, a short reference code appears. Include that code and a description
of the symptom in the bug report. Delivery does not send an email. The inbox is
hosted with Sites at `https://voltra-remote-diagnostics.darkheath.chatgpt.site`;
its deployment/access verification is recorded in the November 9 change notes.

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

Limits are 12 KiB/request, 20 events, 256 characters/event, 2 KiB context,
10 ticket requests/hour per address, and 1,000 retained reports/tickets. Rate
buckets store an hourly address hash, not a raw IP. Reports older than 90 days,
expired tickets and expired rate buckets are deleted when another ticket is
requested; there is no background deletion scheduler. Hosting access logs are
separate from application report contents. Treat submitted text as untrusted data.

The report API/schema sources are in `services/debug-inbox`. The hosted Vinext
source, database migration and deployment build are kept in the Site's source
repository. Never place a private Sites bypass token in the public firmware.

## Pending physical checks / future regression coverage

New test source remains deferred by the project brief. Existing builds/validators
do not replace these observations: 650 ms hold/release vs six seconds; holding
the weight must never load before debug confirmation; blank-screen holds, finger
drift, cancellation, wake-only touch, overlay unload, queued upload after manual
disconnect, reboot while queued, failed/retried uploads, and settings retention.
No autonomous physical motor tests are allowed.
