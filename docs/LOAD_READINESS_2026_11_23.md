# Load readiness and Auto Load screen handoff — November 23, 2026

## Observed behavior and limits

CONFIRMED by the owner: v0.4.16 eventually connected to the paired primary and
weight changes reached the VOLTRA. Load and Auto Load were refused. Report
7fff983a, received at 20:08:07 UTC, records the refused action at uptime 30736 ms
with accepted=1, activated=0 and fresh own-matched primary status. Its reason is
"Waiting for VOLTRA activation status". At submission activated=1; that later
observation does not prove that another action after activation was refused.

CONFIRMED source defect: the activation query AB01 was behind the ordinary
background information sweep. Critical polls, settings and optional discovery
could delay it even when weight editing was already available. Weight editing
does not require the activation fact; Load and Auto Load do. A synthetic paced
loop reproduced zero activation reads during 70 coalesced weight intents over
15.4 seconds. This is a scheduler reproduction, not measured physical timing.

The earlier 9cc53ede report records a real pre-link radio timeout to the second
candidate. It does not establish another app occupying Bluetooth. The current
change does not alter connection attempts, roles or paired command routing.

CONFIRMED by the owner: on a standalone VOLTRA, normal tap-load supplies the
Live weight correctly, while Auto Load opens Live late and its weight feed is
missing. No standalone Auto Load Live report was available during this change.
The latest received report above concerns Twin. The feed cause remains
UNVERIFIED; no subscription-mask, rate, target decoder or motor change is made
on the basis of that different report.

CONFIRMED independent source difference: Steven's
[Auto sequence](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/src/voltra/voltra_client.cpp#L1369)
and Dylan Android's
[Direct Load queue](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/device/ble/src/main/kotlin/com/technogizguy/voltra/controller/ble/AndroidVoltraClient.kt#L2608)
both follow AA12 with a status read and AA13 01 vendor-stream refresh, including
standalone operation. Our standalone trigger omitted that prompt refresh.
The synthetic actual-session baseline sends one AA12 but no AA13 during the
first two seconds at either 100 or 260 ms loop cadence. This establishes an
omitted sequence, not the physical cause of the owner's missing target stream.

## Changes

Independent reference: Steven
[`1c14860` bootstrap AB01](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/src/voltra/voltra_protocol.cpp#L123),
[`queueBootstrap`](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/src/voltra/voltra_client.cpp#L962)
and [exact 0/1 reply parser](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/src/voltra/voltra_protocol.cpp#L349).
These support initial read order and response meaning, not an upstream claim
that delayed activation caused this owner's physical failure. Existing
[Auto Load evidence](AUTO_LOAD_FEEDBACK_2026_11_18.md) and Steven's
[captured states](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L118)
document the standalone 11/12/13/14 sequence; this change retains completion14.

Request AB01 on the first accepted paced tick, before ordinary polling. While
unknown, allow three prioritized reads at the existing 1300 ms interval. Keep
ordinary background queries after that budget so a late reply can recover.
Only a valid same-session success response with exact flag 0 or 1 establishes
inactive/active status. An unknown successful flag clears authorization;
truncated/error responses cannot grant it. Reused response sequences may update
this observation. Reconnect clears it. There is no new activation TTL, activation
write, resistance retry or replay of a previously refused action.

After a successful explicit standalone Auto trigger, schedule one status read
and one AA13 01 refresh at paced ticks ahead of recurring polling. These two
bounded observations cannot be starved by a saturated critical-poll sweep.
Consume each step before sending, with no retry; clear on cancellation, unload,
disconnect or topology change and require current standalone training context.
The existing Twin refresh schedule and telemetry mask/rate remain unchanged.
This matches the source-backed sequence; physical restoration of the reported
Auto Load weight feed remains UNVERIFIED.

For standalone Auto Load, navigate from Adjust to Live on fresh, known loaded
completion (phase 14), or after a fresh armed observation followed by known
loaded/nonarmed state. Rep telemetry is no longer a prerequisite for that
presentation transition. Preserve Live while awaiting the first set. Snapshot
retained workout status and set-end revision so an old rest/end does not bounce
back to Adjust. New rest/end, normal set completion or unload ends that view;
a retained completion field cannot reopen it repeatedly. Reset on a confirmed
unload/rearm or connection/selection change. This adds no control permission.

Ordinary dial queue/readback waits are quiet on Adjust and Live. Filter only
"Awaiting device" when a manual weight preview exists and no automatic drop is
pending; remove the Adjust "Setting weight..." fallback. Keep confirmations,
actual errors, drop feedback, Auto Load and unload behavior. Expected absence of
rep data before the first Auto-loaded rep no longer adds a warning when an
actual live target is available. Missing live target still shows unavailable.

## Validation

CONFIRMED offline MOCK: actual PhysicalSession passes 15 focused scenarios /
13,436 assertions and 31 existing regressions. Baseline/fixed differential sends
0/3 activation queries under the synthetic edit workload, with no motor or Auto
control in either run. Checks cover malformed/foreign/CRC-invalid status,
same-sequence updates, bounded retries, late background recovery, reconnect,
explicit unload and no intent replay. Independent source review passes.

CONFIRMED final actual-session MOCK refresh matrix:22 scenarios/121 assertions.
At100/260 ms cadence, the exact AA12/read/AA13 sequence emits one trigger and
zero motor-setting writes, with refresh at200/520 ms. Failed trigger/read/refresh,
cancellation, unload, disconnect, topology/workout loss and no boot/manual/
preexisting-Auto follow-up pass. Existing Twin behavior and activation/prior
regressions pass on the combined source. These are synthetic wire observations.

Actual LVGL and TrainingApplication observers exercise both UI changes at
360/466/480. The old executable reproduces the phase-14 admission failure.
External receipts are under `/workspace/preview-tools/activation-v0417`,
`quiet-dial-v0417` and `single-auto-live-v0417`. These use synthetic device I/O;
they do not establish physical Auto Load timing or restoration of its missing
weight stream. No new tracked automated test source or physical operation.

Four unchanged real standalone B4 packets replayed through the actual session
pass 72 MOCK assertions: Auto11/12/13 reject target,35/14 accepts its reported
trailer,35-to-1/5 clears the old context and the next sample restores it. Missing
target clears the value; neither force nor base substitutes for it. This rules
out permanent target rejection for those supplied packets, not different
delivery or payloads on the owner's Auto path.

The existing whole-pound Live display continues to use only fresh device target
telemetry. Force, measured wire weight, base and percentage calculations are
not replacements for the VOLTRA display.
