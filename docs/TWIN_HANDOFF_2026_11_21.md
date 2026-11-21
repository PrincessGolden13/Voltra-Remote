# Twin handoff and misleading Bluetooth error — November 21, 2026

## Report evidence

CONFIRMED, report2ab11a8e at21:52:25UTC on v0.4.10: the first candidate completes
authentication and returns full own-matched A7 identity, marker1/raw state0x22,
distinct second address, current20844=2 and nonzero21023. The owner identifies this
member as the follower. The next candidate never opens a protocol session. The
report reader truncates the event tail; the exact radio failure code is unavailable.
Neither the report nor the generic error proves another phone holds Bluetooth.

CONFIRMED source cause of the extra wait: raw0x22 was identity-known but had unknown
state/role, so discovery exhausted its six-second read window before moving on.
CONFIRMED source cause of the message: an unconditional controller fallback said
“Bluetooth may be in use.” No ownership evidence accompanied that claim.

## Bounded read-only routing profile

INFERRED, read-only scope: exact marker1/raw0x12 means master; marker1/raw0x22 means
follower. Master captures are in stevenpoint1c14860b53b805a789b8bed2095e63443e355d7d
and albertc4f2884798941babd83788cf79609620e767bc85. The follower capture is the
owner-identified first member above. This is not a generalized high-nibble mask.
Other markers/states stay role-unknown. Historical LAST_ROLE is not used.

PhysicalSession additionally requires accepted fresh own-matched full A7 identity,
a distinct paired address and fresh current20844=2/nonzero21023 from the same
topology epoch. The A7 role has its own timestamp; refreshing another register
cannot extend it. A marker/raw-state/identity change invalidates the epoch.

A fresh follower immediately routes to its exact other address; the second member
must return fresh reciprocal identity and its own master evidence. Persist only a
navigation candidate based on fresh roles. Cached names, colors, order and hints
never establish a live role or authorize commands. Every connection revalidates.

The physical master may open a read-only Main view without clearing the backend
probe flag. All manual actions and automatic recipe execution remain blocked,
with “Use VOLTRA controls” only on an attempted action. Physical Twin resistance
commands, safety state and combined limits remain unsupported.

## Radio ownership and recovery

INFERRED contributor to inconsistent handoff: one mutable untagged callback could
belong to the previous client procedure while a new target was queued. The new
event queue carries physical generation, source address, kind and reason. Merely
queueing a target does not advance the radio owner. Reuse requires matching terminal
evidence, an empty event queue, cleared client handle and idle GAP, followed by a
100ms settling policy. NimBLE2.3.6 calls terminal callbacks before clearing handles.
No time-only cancellation fallback permits ambiguous reuse; missing terminal
evidence produces a bounded cleanup timeout. Retries remain radio-only.

Failures preserve their actual stage/code when available. Retry resolves the saved
logical selection again; no automatic debug submission or command replay occurs.
Reports retain the first connection failure and per-candidate elapsed times.
If terminal evidence never arrives, the cleanup timeout advises restarting the
remote; blindly retrying cannot safely reclaim that client. Ordinary radio errors
retain Retry without this restart advice.

## Validation and limits

The owner also supplied a v0.4.10 Updates photo saying “Connect Wi-Fi in Settings
first.” CONFIRMED source comparison v0.4.9->v0.4.10: Wi-Fi credential storage,
networking and updater code are unchanged. Wi-Fi is intentionally powered down
during VOLTRA use, without erasing the application credential record; it attempts
the saved network after release. The old update-check message only tested live
connectivity and cannot establish deleted settings. Actual saved-profile presence
on the owner's board is UNVERIFIED. v0.4.11 adds an explicit saved-network retry
before a manual update check, without setup/reset or automatic installation.

PASS independent source review, existing offline core/packet-inspector builds,
PhysicalSession syntax check and real LVGL pointer previews at360/466/480. Generic
renamed pairs exercise follower handoff, direct master, Retry, Back and explicit
debug consent. Read-only dial/tap produces zero backend control calls and leaves
weight/load unchanged; backend probe remains enabled. No new test source or
physical motor operation. Public firmware/package checks are recorded in TESTING.md.

UNVERIFIED: physical handoff/reconnection improvement and wider-firmware role
compatibility. This release must not be described as completed Twin control.
