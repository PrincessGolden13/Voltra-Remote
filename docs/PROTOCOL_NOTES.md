# Protocol evidence and implementation limits

October 17: [PhysicalSession and source review](NORMAL_MODE_2026_10_17.md) add a
Normal-only frame builder, acceptance bootstrap and bounded read-only decoder.
This does not establish physical safety, advanced modes or Twin routing. Explicit
unload is the only physical control write enabled in that diagnostic profile.

Source revisions are fixed in [UPSTREAM_AUDIT.md](UPSTREAM_AUDIT.md).
October 18: [physical connection audit](LIVE_CONNECTION_2026_10_18.md) confirms
GATT discovery, application acceptance after user approval, and read-only state
at approximately ten feet with the local diagnostic profile. Training remains
unvalidated. EP1.0 is a module tag, not proof of installed app firmware 1.x.

## September 20 SDK follow-up

See the [0.15.0 review](SDK_REVIEW_2026_09_20.md). The release supports retaining
our strict framing, signed eccentric decoding, reconnect reset and device-report
confirmation policies. Physical acceptance/refusal plus a post-acceptance read
remain mandatory before a real session can become ready. IC selector/value and
width, Auto Load phases and device limits remain unverified here and unresolved
in the upstream review. Heartbeat per-rep means must not be used as live motion
observations for rest-pause hold detection.

## GATT and connection

CONFIRMED (cross-source): service `E4DADA34-0867-8783-9F70-2CA29216C7E4`,
write/transport `A010891D-F50F-44F0-901F-9A2421A9E050`, and observed response
channel `55CA1E52-7354-25DE-6AFC-B7DF1E8816AC` agree with the starter/SDK and
Android's official-app capture notes. Android also identifies
`CA94658C-0525-5046-E78B-5391B65F47AD` and write-without-response
`19DE84ED-0A69-482C-A8A6-C75CB5BB4389`. Discover properties; do not hard-code
ATT handles or subscribe to every candidate without investigating the profile.

CONFIRMED (Android reported captures): GATT-only connections expired at roughly
five seconds before app-level bootstrap was implemented. The notes describe
0x4F hello, 0xFF connect request and 0x27 finish/check, followed by read commands.
Subscription and successful writes are not handshake confirmation. A production
bootstrap state machine, identities, timing and response predicates remain
UNVERIFIED here. No captured device key, serial or BLE address is copied into
this repository; no handshake packet replay is enabled.

## Framing

CONFIRMED (source cross-check): minimum 13 bytes. Byte 0 is 0x55; byte 1 is
length low byte; byte 2 distinguishes ordinary 0x04/0x08 and extended 0x05/0x09
frames. The extended forms add 256 to the length. Byte 3 covers the first three
bytes with CRC8. Bytes 4/5 are sender/receiver; 6/7 are treated as little-endian
sequence by current SDK (older notes called byte 7 channel); 8/9 are protocol
0x0020; byte 10 command; remaining payload precedes little-endian CRC16.

CONFIRMED (source): reflected CRC8 uses initial 0x77, polynomial 0x8C; reflected
CRC16 uses initial 0x3692, polynomial 0x8408. HA's reflected-at-boundaries
0xEE/0x31 and 0x496C/0x1021 formulation is equivalent. The project's implementation
is a small bitwise implementation, not a copy of upstream tables.

The new parser rejects wrong sizes, unknown frame types and either checksum
failure; reassembly is bounded to 511 bytes and reset on reconnect. Per-channel
buffers are required when more than the initial response channel is implemented.
Timeout-based partial-frame expiry and replay/out-of-order rejection remain
pending before a real session can be marked validated.

## Parameters and state

CONFIRMED (Android capture notes and HA source): 0x10 updates contain a little-
endian uint16 count, then uint16 parameter ID and catalog-width value per entry.
0x0F read responses require their own envelope/return-code handling; they are not
decoded by the initial application. Unknown widths cannot safely be skipped.
Our small decoder rejects the whole update on an unknown ID/truncation/trailing
data, so an unparsed tail cannot freshen part of the machine's safety state.

| ID | Meaning / representation | Foundation |
| --- | --- | --- |
| 0x3E86 | Base weight, uint16 lb | Read notification |
| 0x3E87 | Chains, uint16 lb | Read notification |
| 0x3E88 | Eccentric, **signed** int16 lb | Read notification; CSV uint16 label conflicts with negative captured values |
| 0x3E89 | Fitness mode, uint16 | Read raw; 4/5 unloaded/loaded only in Weight Training context |
| 0x4FB0 | Workout/session state, uint8 | Read raw; 1 Weight Training |
| 0x4E2D / 0x1B5D | Battery percentage, uint8 | Read, reject >100 |
| 0x521F | Coworker remote status, uint32 | Preserve raw; no bit interpretation |

Use canonical numeric IDs above. Some SDK factories store IDs in an already
byte-swapped representation and write their high byte first. Compare actual
wire bytes, not literal numeric constants alone.

CONFIRMED (source): 0x11 writes and matching 0x10 reports exist for base weight,
chains, eccentric and fitness mode. A generic ACK cannot prove applied value.
Session/workout state is not synonymous with motor state. No 0x11 builder,
load/unload builder, mode change, Auto Load/Unload setter or generic application
raw-write escape hatch is supplied in the foundation.

## Telemetry and beta evidence

CONFIRMED (Android reports): 0xAA/0x812B samples include phase and rep observations;
SDK tests cover telemetry, cmd07/cmd10 decoding, rowing and set summaries.
Old common-state battery offsets were corrected upstream. Force/velocity/
position units, cadence, dropped-packet handling, set boundaries and Twin
aggregation require fixture comparison before implementation.

UNVERIFIED: Mountain/MTN selector and captured behavior. Targeted search did not
find a validated Mountain implementation in these sources. This is not proof
that firmware lacks the feature; the UI must leave it unavailable.
October 24 update: the additional kirby6365 reference documents the separate
0x556F selector; see [new source and physical evidence](CONTROL_FEEDBACK_2026_10_24.md)
for the implementation that supersedes this earlier search result.

## Before physical read-only work

Implement response-validated bootstrap, read envelope decoding, channel-specific
assembly, malformed/replay diagnostics, session generation and freshness checks.
Audit every bootstrap write for settings side effects. Record device/module
versions and reported capabilities without persisting unnecessary identifiers.
No resistance testing belongs in this stage.

## October 24 connection and modifier follow-up

CONFIRMED (source): pinned Android commit
da514d2db1a16ebb4ded3156bc8fd200bed1e6eb, VoltraControlFrames.kt,
enterWeightTrainingPayload writes parameter 20400 as uint8 value 1. Following
the user's explicit request, connection setup now attempts this once from fresh
idle/unloaded standalone state, with child-lock and OTA reports clear. An already
active Weight Training session needs no write. Other workouts, loaded/armed or
unknown state do not authorize automatic mode switching. Mode readback, not the
BLE write result, confirms completion. No motor command is part of setup.

CONFIRMED (Black, October 24): 20400 changed from 0 to 1 after that setup write;
motor 16009 remained 0 and selected base remained 57 LB. A later reconnect found
20400 already 1 and 60 LB unloaded. These observations do not validate loading.

The previous settings planner temporarily cleared both modifier registers for
any modifier edit. It now compares each requested effective value to its fresh
report and writes only changed parameters. Base/modifier reductions precede
increases; IC direction changes do not clear EO. The 100 ms quiet interval
coalesces unsent explicit edits, with fresh reports and bounded confirmation
still required. Physical tap/rotary behavior is user-tested, not an automated
motor test. See LIVE_CONNECTION_2026_10_18.md for observed scope and limitations.

## October 31 motor report variant on connection

CONFIRMED (Black capture): after reconnect, GATT and application acceptance
succeeded, Weight Training was 1, base was 42 LB, and motor parameter 16009
consistently returned 256 (0x0100). IC and EO each reported 2500 / 25%, with
maximum 230 LB and standalone state. The old whole-word motor comparison
rejected 256, cleared TrainingReport, and displayed "Unknown motor state / tap
to unload" despite the working connection. This observed failure was not the
mode-exit recovery path and did not require a new mode-entry or load command.

CONFIRMED (source): pinned Android da514d2db1a16ebb4ded3156bc8fd200bed1e6eb,
VoltraControlFrames.kt normalizedFitnessMode masks the low byte; pinned kirby
8e1ea1cb752acd3108c61f1a502b63978a172832, voltra_protocol.h uses the same low-byte
interpretation for loaded and Auto Load states. Raw reports remain retained.
The physical session now normalizes the known high-byte 0/1 family consistently
for presentation, readiness, command confirmation, mode entry and exit recovery.
It still rejects unknown low-byte modes and any other upper bits. Outgoing
motor commands remain the documented plain command values, never echoed raw
reports. Freshness, locks, cable-zero, explicit-load and standalone guards remain.

INFERRED: the low-byte interpretation applies to Black's 0x0100 unloaded report,
based on those independent implementations and the captured context. UNVERIFIED:
the meaning of the upper byte and physical loading with the flagged variants.
No automatic motor operation is used to validate this change. The source-backed
interpretation is not a claim that every possible upper-byte value is supported.

CONFIRMED (post-install capture): Black again returned raw motor=256; the revised
session reported connected/accepted/fresh, 42 LB and unloaded, then cleared its
status after the remaining context reads. The check sent no mode, base, modifier
or motor control command. This verifies decoding/connection recovery, not the
physical load/unload cycle for prefixed reports.

## October 31 pending firmware update blocking adjustments

CONFIRMED (user): the connection/display error was cleared by the preceding
build. A subsequent dial attempt showed "Unload / wait for fresh control state"
and did not change Black's weight. The user confirmed Black had an update ready,
not an installation in progress, and requires normal training while it is pending.

CONFIRMED (capture): reconnect succeeded and reported 42 LB, motor=256,
Weight Training=1, 25% IC/EO, child lock=0, standalone state and OTA_STATUS=261
(0x0105). The other previously checked controls stayed consistent. The remote's
`fresh(otaState)==0` predicate was the blocker. This was a control-state rejection,
not a stalled Bluetooth connection; no speculative radio-state changes were made.

INFERRED (Black / current firmware scope): 0x0105 identifies the update-ready
state observed and described by the user. The physical profile now permits only
fresh 0 (idle) or this exact captured value. All other nonzero values and missing/
stale status still block control; do not mask arbitrary OTA bits or treat every
nonzero value as harmless. No firmware-update command or preference is written.
Blocked or missing OTA evidence now produces update-specific feedback instead
of incorrectly asking an already unloaded user to unload. Loading still needs
every other readiness predicate and a current explicit user action. Automatic
mode entry and pending-setting validation use the same update-state policy.

CONFIRMED (user follow-up, build 857d4c6): slow unloaded dial changes and the
return adjustment followed on both Black and the remote without the message,
while the update remained pending. This validates unloaded base editing in the
observed pending-update state; it does not establish additional OTA enum values
or verify physical loading/RP.

## October 31 Auto Load with dropsets

CONFIRMED (source): kirby6365/voltra_remote at
8e1ea1cb752acd3108c61f1a502b63978a172832, docs/PROTOCOL.md Auto load section,
captures motor 35 persisting after loading; 21447=11 waits for the cable,
12 counts down, 13 engages, 14 is loaded. Its voltra_client.cpp sends a base
register edit in this state; the separate motor reassertion branch applies to
mode 5, not mode 35. Our profile already decoded 35/14 as loaded but incorrectly
required motor 1 for all live edits. The UI also displayed OFF whenever this
temporary readiness predicate was false, irrespective of the local recipe.

The loaded base-only predicate now also accepts fresh motor 35 plus status 14.
Modes 35/11-13 remain in progress and cannot run drops. Existing context, locks,
standalone, limits, accessory preservation, actual motion and readback guards
remain in force. No extra motor write, safety bypass or Auto Load setting is
added. A successfully requested Auto Load resets old execution counters/holds
without clearing the selected recipe. The tile represents local selection
independently from live-write readiness. This supersedes the earlier exclusion
of all Auto Load states from drops; Black's completed Auto Load drop behavior
still requires a human-initiated physical trial.

CONFIRMED (user follow-up, b0d08b5): enabling drops, using Auto Load, doing a
rep and unloading worked. This verifies that interaction on Black; no capture
of the entire trial was retained, so do not claim additional status enum values
or broader firmware/Twin compatibility from this observation.

## October 31 Auto Unload holding-time read

Read-only parameter 21525 / 0x5415 (Android da514d2 uint8, 0..10) is polled
with existing control state. Official Cortex documents the holding-time setting
in seconds, zero disabled. Fresh in-range values constrain RP pause locally;
unknown values retain the user's requested fallback. There is no write path for
Auto Unload configuration. CONFIRMED (Black, October 31): fresh read 21525=2
matches the user's 2 s holding-time setting. Other values/firmware remain
unverified. Full source links and evidence boundaries are in
[RP countdown notes](RP_COUNTDOWN_2026_10_31.md).

## October 31 loaded amounts and dropset completion

CONFIRMED (source): the pinned kirby6365 client at
8e1ea1cb752acd3108c61f1a502b63978a172832 writes loaded accessory amounts through
the ordinary chain/EO registers. This profile now permits those transactions
in its already-supported motor 1 / completed Auto Load 35/14 context. Its
motor-mode-5 reassertion is not implemented. Curve/display-unit changes remain
unloaded-only; decreases precede increases and each register requires readback.
Readback validation allows the confirmed pre-edit, intended or current in-flight
amount while preserving curve/unit/context invariants. No new packet format.

CONFIRMED (Black capture scope): workout phase 4 precedes actual unload at set
end. Expose it as optional setEnding alongside phase 3/rest; only a fresh report
can finish the local grouped dropset. AA85 while phase 2 remains active is a
weight segment, not permission to restore. End-of-set restoration issues at
most one unload, then an unloaded base transaction after fresh confirmation.
See TRAINING_WIFI_2026_10_31.md for evidence links and physical trial limits.
