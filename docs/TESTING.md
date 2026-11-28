# Validation status and pending tests

## November 28 v0.4.29 standalone Live cadence

Matched mock runs compared both versions using the same changing 59–89 lb
device target. With 25–60 ms replies and a 90 ms pacing grid, 33 seconds
showed 73 accepted samples on v0.4.28 and 122 on v0.4.29. Median spacing fell
from 360 to 180 ms and median step from 12 to 6 lb. The 630 ms maximum was
unchanged. At 110–240 ms reply latency with a fast host loop, the sample
count rose from 73 to 97. With a deliberately slower 110 ms host loop and
70–190 ms reply latency, both sampled 75 times. In that case median spacing
improved from 440 to 330 ms and the maximum widened from 660 to 770 ms.

The six required query groups had the same maximum spacing in both versions,
at most 1,350 ms. Background activation polls ran and explicit load and unload
used the same commands and confirmation timing. Four pacing profiles, 14
standalone continuity mocks, 31 protocol scenarios and the core/packet inspector
build passed. Physical matching remains unverified. [Video and cadence evidence](LIVE_CADENCE_2026_11_28.md)

[PR CI46099373563](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46099373563)
at d5624f96 and [tag CI46099772569](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46099772569)
at b0a4b918 passed. Artifact IDs 15245205926 and 15245221689. Both tagged
images match the reviewed PR byte for byte. Normal has 2,098,896 bytes,
SHA-256 `348d8a9f9ad0f2cbc76e6b08e243de9eca301ca7b95da87b0db49e5414a2d015`.
Descriptors, manifest, USB layout, backend isolation, NVS and rollback hooks,
release notes and all four assets passed independent checks.

Release 461254127 became latest at 2026-11-28 02:51:40 UTC. The public
manifest and pinned image match the tag bytes. Receipts are in ignored
`build/review-v0429`, `build/review-v0429-tag` and
`build/published-v0429/verification.json`.

## November 28 v0.4.28 Auto Load guidance, Live continuity and loaded dial

The Auto Load hold, reported phases and Cancel passed 222 cases and 2,778
assertions in actual LVGL at 360, 466 and 480 pixels. The mocks cover single
and Twin guidance, countdown freshness, pending or rejected requests, Direct Load and the
one-unload paths. The reference videos establish which instructions appear
on the native screen. See [video and protocol evidence](LIVE_GUIDANCE_2026_11_28.md).

The standalone Live fix passed 14 focused continuity cases and 72 B4 replay
assertions. The replay preserves exact target values across short gaps and
clears them when the readout context changes. The 31 physical-session mock
scenarios and five loaded dial protocol cases passed, including raw accessory
preservation, write-time invalidation, the native limits and existing mode
and lock gates. The 53-scenario Stairs endpoint suite passed. The preference
passed 19 UI assertions each at 360 and 466 px. Mock K5 storage checks kept
the legacy configuration blob byte-identical, and desktop config v1, v2 and
v3, reboot persistence and unreadable preference checks passed. Physical
device behavior and Twin Live target availability remain unverified.

[PR CI46093894877](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46093894877)
at 7dd8c9e5 and [tag CI46094371910](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46094371910)
at 7de33861 passed. Artifact IDs 15242436804 and 15242239645. The exact
tagged Normal and Simulator images match the reviewed PR. Normal has
2,098,880 bytes, SHA-256
`36403b20e88835c2c5165e387e445dac9b3c697f1d4ea3fbb009bb216a7a2753`.
Descriptors, manifest, USB layout, backend isolation, NVS and rollback hooks,
release notes and the four assets passed independent package checks.

Release 461222807 was published as latest at 2026-11-28 01:46:43 UTC.
The public manifest and pinned image match the tag bytes. Receipts are in
ignored `build/review-v0428`, `build/review-v0428-tag` and
`build/published-v0428/verification.json`.

## November 27 v0.4.27 Updates, standalone Live and Direct Load

The Updates UI passed 48 actual LVGL scenes and 429 assertions at 360,
466 and 480 pixels. Opening the screen starts or adopts one check. The
checks cover saved Wi-Fi reconnect, no check while connected to VOLTRA,
current/update/error screens, reentry, and explicit installation.

Standalone Live passed six focused protocol cases covering request scope,
freshness, bounded misses, target range and B4 preference. All four real
B4 capture replays and 31 prior physical-session mock cases passed. The
fallback only reads the target. Its response on Grey remains a physical check.

The exact Stairs counter interleaving reproduced a failed endpoint on
v0.4.26. The updated protocol passed 11 cases and 39 assertions, including
one base-only decrement at the stationary endpoint, no earlier dispatch
and all loaded mode and lock gates. Previous protocol and app Stairs
fixtures passed 53 and 12 cases respectively. The original Grey report
did not contain its endpoint or loaded write-gate snapshot.

Five extended-cable PhysicalSession mock scenarios checked positive
extension, fresh flags, unavailable and unknown state, single scope, no
unguarded ordinary load, and the existing one-shot countdown and cancel.
The integrated app and LVGL matrix passed 147 cases and 1,479 assertions at
360, 466 and 480 px. These include 45 Direct Load consent, rejection and
progress cases. Changes applied during the BLE poll invalidated a queued
confirmation before a motor request could run. No physical operations.
See [the report and video evidence](SINGLE_LIVE_2026_11_27.md).

[PR CI46078393931](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46078393931)
at 9b7152ac and [tag CI46079024665](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46079024665)
at 0a22d4cb passed. Artifact IDs 15235459420 and 15236014449.
Both tagged images match the reviewed PR bytes. Normal has 2,095,232 bytes,
SHA-256 `5135c033060bb92565f6692a0d1f3cd6cb70b4bc0e8c0a0eba071c4b3345f66d`.
The image descriptors, manifest, USB layout, backend isolation, NVS and
rollback hooks, release notes and four assets passed. Receipts in ignored
`build/review-v0427` and `build/review-v0427-tag`.

Release 461135346 was published as latest at 2026-11-27 22:55:00 UTC.
The public manifest and pinned asset match the reviewed tag. Receipt in
ignored `build/published-v0427/verification.json`.

## November 27 v0.4.26 Twin recovery and Live

Actual LVGL tests passed 72 idle-loss cases and 654 assertions at 360, 466
and 480 px. The cases distinguish idle loss, Live loss, a failed selection,
confirmed mount-lock release, and explicit exit. The idle notice reproduced
on v0.4.25. The new cases verify retained diagnostics and no implicit controls.

The Twin Live idle fallback passed eight physical-session mock cases and 61
assertions, plus 69 LVGL scenes and 1,134 assertions across the three display
sizes. Confirmed rest remains immediate. Long gaps, stale context, motion,
unknown packets and Auto Load calibration do not supply a completed idle
interval. The fallback changes the page only. No new tracked tests or
physical radio or motor operation. See [report evidence](TWIN_RECOVERY_2026_11_27.md).

Five foreground selection scenarios and five native retry scenarios passed.
A verified controller was retained when it alone advertised. Exact follower
handoff, absent and exhausted endpoints, and ordinary two-attempt limits
passed. Existing application handoff and native GATT fixtures passed 14 and
16 scenarios respectively. The final physical-session regression passed all
31 scenarios, including standalone load and unload. The offline core and
packet inspector built.

The physical-session tracing fixture emitted all 11 rows within 1,473 encoded
bytes. The trace retained the Twin workout through a read-only discovery,
kept AA82 phases separate, and supplied neither a command nor a Twin live
display value. Nine host serializer and inbox cases passed. The largest
trace fixture fit the inbox preview and the report size and entry limits.

[PR CI46074970625](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46074970625)
at 1827954b and [tag CI46075581207](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/46075581207)
at dc2b5cfb passed. Artifact IDs 15233624565 and 15235355268.
Both tagged images matched the reviewed PR. Normal has 2,087,536 bytes with
SHA-256 `6de63c69c6a736f29da747ecd32acb4b886d557484c1ac218772f31df018d562`.
Product descriptors, manifest, USB bundle, partition map, backend isolation,
NVS and rollback hooks, four draft assets and notes passed. Receipts are in
ignored `build/review-v0426` and `build/review-v0426-tag`.

Release 461114157 became latest at 2026-11-27 22:20:37 UTC. The public
manifest and pinned asset matched the tag bytes. Receipt in ignored
`build/published-v0426/verification.json`. Physical behavior is unverified.

## November 27 v0.4.25 Twin Drops and BLE trace

Actual LVGL tests for 360, 466 and 480 px passed 27 Drops cases, 420 RP
assertions and 105 trace-toggle assertions. Back discards changes, Twin Save
retains a disabled recipe, and the loaded and stale-state touch targets send
an explicit unload. Single Apply keeps its existing behavior. A full Hold
arc is visually distinct from selection. Trace starts off, switches without
settings or BLE writes, clears on disable and is captured only on SEND.
Receipts are under `/workspace/preview-tools/twin-drops-v0425` and
`/workspace/preview-tools/rp-selection-v0425/current`.

All 31 physical-session regression scenarios and two focused Auto scenarios
passed with a simulated transport. Ready single and Twin requests send one
trigger with fresh matching flags. An unsupported or stale request sends none.
The hold/input matrix passed 129 scenarios and 1,386 assertions. The offline
core and packet inspector builds pass. No physical radio or motor operation.

The production report serializer was compiled with host stubs. Nine
fixtures passed the inbox handler with a simulated database. Ordinary
reports matched the previous JSON byte for byte. All ten production trace rows
fit in the 2,048-byte inbox preview. The physical-session trace fixture checked
the three routes, CRC rejection, explicit enable and clear, preserved samples
through disconnect, and allowlisted payloads. The fixture sent no commands
from tracing and supplied no Twin Live target. No reports were uploaded.

[PR CI45916596703](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45916596703)
at d7b9bc53 and [tag CI45917135716](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45917135716)
at 4b951def both passed. Artifact IDs 15162766802 and 15163007093.
The tag's Normal and Simulator images match the reviewed PR bytes. Normal has
2,084,752 bytes and SHA-256
`69252557a1a08b4978283caabb45dcd03e0c5caabf18e02f5ec84bad094e8dd0`.
The version descriptors, manifest, USB hashes and offsets, partition map,
backend isolation, NVS and rollback hooks, four assets and tagged notes pass.
Receipts are in ignored `build/review-v0425` and `build/review-v0425-tag`.

Release 460032338 became latest at 2026-11-27 00:35:25 UTC. The public latest
manifest and pinned image match the checked tag. Receipt in ignored
`build/published-v0425/verification.json`. Twin hardware behavior remains open.

## November 26 v0.4.24 Twin workout recovery and rep timing

Report evidence and limits are in [Twin workout transitions and rep timing](TWIN_LIFECYCLE_2026_11_26.md).
The v0.4.23 protocol reproduced immediate teardown on idle mode exit from
both a direct response and a parameter push. The corrected actual protocol
passed 11 scenarios, including both mode/role arrival orders, the six-query
eight-second bound, current idle and role requirements, maintenance and unload,
and standalone manual resume. Actual application/controller mocks passed 14
scenarios and 164 checks. Receipt `workout-exit-v0424/validation.json`.

Extended Twin lifecycle parsing and diagnostic separation passed six protocol
scenarios and 43 checks. Actual LVGL and application passed 66 Live and battery
scenes and 1,089 checks, and 129 hold/input scenes and 1,350 checks at 360, 466
and 480 px. The latter included old touch, rotary preset, and editor scope
changes through a workout transition, exact configuration reasons, continued
explicit unload, and six-second debug. Receipts under `/workspace/preview-tools`
include `twin-lifecycle-v0424/integrated-final-validation.json` and
`workout-input-v0424/current/validation.json`.

Four focused physical-session mocks passed exact configuration, stale refusal,
and bounded isolated target reads, including ignored foreign, push, duplicate
and late responses. All 31 prior protocol scenarios and the core/inspector build
pass. No physical radio or motor operation. No new tracked test source.

[PR CI45912658413](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45912658413)
at 3411830f passed both firmware builds, core and packaging. Artifact 15160269404
SHA-256 `944694969837ddcfd4d17e419191d67a700d01104d44939de1a550132c74ff38`.
Merge 4922297d has the exact reviewed tree. [Tag CI45913287976](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45913287976)
artifact 15161560561 SHA-256 `7964be9d1fde95a05d03bec92e02d21a7f54d587f48bd2fbd88bac161c29dbb5`.
Both tagged images match the reviewed PR bytes. Normal has 2,073,536 bytes and
SHA-256 `78112399d83c85bb88192dc18873f5439fad32b327cabe6bf313cff000cc39a6`.
Simulator has 1,059,744 bytes and SHA-256
`d81be0a90ca5b5cb9948ac1ece31d12896618be9525d6cfa2674801e103c0365`.
Version, manifest, USB hashes and offsets, partition map, backend isolation,
NVS and rollback hooks, and all four draft assets and notes pass review.
Receipts in ignored `build/review-v0424` and `build/review-v0424-tag`.

Release 460010579 became latest at 2026-11-26 23:55:21 UTC. The public latest
manifest and pinned image match the reviewed tag bytes. Receipt in ignored
`build/published-v0424/verification.json`. Physical recovery and Twin workout
status delivery still need checking. Combined display target remains unverified.

## November 26 v0.4.23 Twin handoff and Live

Source and report evidence is recorded in [Twin handoff and Live](TWIN_HANDOFF_LIVE_2026_11_26.md).
The unchanged v0.4.22 application reproduced the reported connection-ended
message when the accepted endpoint became primary. The fixed application and
controller passed 11 synthetic scenarios with 124 checks for in-place adoption,
idle follower recovery, reciprocal validation, stale or conflicting evidence,
cleared intents, and unload access.

The actual protocol session passed five local-idle scenarios, 11 prompt-refresh
scenarios and 31 prior regression scenarios with synthetic radio I/O. In the
660 ms replay of the brief confirmed-load window, v0.4.22 sent no refresh. The
fix sent one at 100, 260 or 305 ms depending on the tested cadence. The existing
activation matrix passed 15 scenarios and 13,437 assertions, accounting for
v0.4.20's earlier read-only A7 query. Offline core and packet inspector build.

The reviewed LVGL and application sources passed 102 hold and input-scope scenes
with 1,059 assertions, and 63 Live scenes with 990 assertions at 360, 466 and
480 px. Old-scope clicks, delayed editor changes and queued dial inputs could
not issue controls after adoption. Unload and debug priority passed. External
receipts under `/workspace/preview-tools` include `twin-handoff-v0423`,
`twin-recovery-v0423`, `twin-direct-refresh-v0423`, `auto-hold-v0423/final` and
`twin-live-v0423/release-4210c84-validation.json`.

[PR CI45907398106](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45907398106)
at 4210c842 passed both public images, core and packaging. Artifact 15158208902
SHA-256 `39c40ad66478488fd157691e074677ba54ebdf580a6c620c1b19933714088d26`.
Main merge 7e7004eb has the exact reviewed tree. [Tag CI45907964336](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45907964336)
artifact 15159386333 SHA-256 `7ad94fe63ec9f9449adb432e5cae3dafd77ef6bb20a4d272169fbe14ca5a5222`.
Both tagged images match the reviewed PR bytes. Normal has 2,066,704 bytes and
SHA-256 `ffaddf33214732daf4a230995a33a8fe60152040a7c39029dfe2a4e1ac0bc118`.
Simulator has 1,059,312 bytes and SHA-256
`131c6abf20ed09e00e8d3956028af18d263c5cde638e30e29e49857567d85d99`.
Version, channel, layout, manifest, USB hashes and offsets, backend isolation,
strong NVS and rollback hooks, and all four draft asset digests and notes pass.
Receipts are in ignored `build/review-v0423` and `build/review-v0423-tag`.

Release 459980562 became latest at 2026-11-26 23:02:09 UTC. The public latest
manifest and pinned image match the verified tag bytes. Receipt is in ignored
`build/published-v0423/verification.json`. The checks used no physical radio or
motor operation. Physical handoff, stream recovery and Twin target remain unverified.

November 23 v0.4.18: [Stairs boundary correction](STAIRS_BOUNDARY_2026_11_23.md).
CONFIRMED source baseline: actual PhysicalSession dispatches a base edit on
AA82 return-start during movement and lets an idle-queued edit survive a later
moving sample. Actual TrainingApplication also requests the premature drop.
External receipts: `/workspace/preview-tools/stairs-boundary-v0418`.
PASS final actual PhysicalSession source-shaped MOCK:53 scenarios/210 assertions,
including slow return, same-batch movement and stop, idle-queued/new-pull rejection,
duplicates/reused sequence, gap/unknown phase, rest/summary, rejected topology,
temporary changed/restored control context and already-issued write confirmation.
Four additional push cases cover changed mount/unknown motor with a reused
sequence, duplicate pushes not confirming settings before0x0F and push-only motor
data not renewing expired direct-read evidence. Physical cpp
SHA`b0ddc36e53b3459917d8c7c0042c2b19de17ce6061129949a862afee46a0fe5b`.
PASS actual TrainingApplication:12 scenarios/120 assertions. Sequential valid
Auto/manual Stairs, duplicate/no catch-up, absent/moving/active/unknown/stale/gap/
wrong-set windows and independent RP hold behavior pass. App cpp
SHA`54a459941e1b1b036d6a2faf8b2a3daa89be80cc421d2b5a05a617d1ad392ba5`.
PASS final combined PhysicalSession activation15/13,436 plus31 prior regressions,
standalone refresh22/121 and unchanged captured B4 target4/72. PASS fresh actual
LVGL/application rebuilds:Single Auto51/795 and quiet-dial57/306 at360/466/480.
Quiet-dial drop fixtures explicitly supply a qualified stationary endpoint;
the Single Auto fixtures are unchanged. Source hashes match each receipt.
PASS actual MockTransport workflow14 checks: Auto completion, two endpoint
Stairs changes and independent RP hold behavior. Its simplified40 ms settings
queue does not implement the PhysicalSession cancellation guard; these are
synthetic workflow checks, not physical or dispatch-parity evidence.
Receipts: `stairs-boundary-v0418/{physical_matrix,app_matrix,target_replay,refresh_followup}-validation.json`
under `/workspace/preview-tools`, plus `quiet-dial-v0418/validation.json` and
`single-auto-live-v0418/validation.json` there; simulator workflow receipt is
`stairs-boundary-v0418/simulator_matrix-validation.json` and final activation
outputs are `stairs-boundary-v0418/activation-current-*.txt`.
Existing core/packet-inspector and diff whitespace checks pass.
No physical operation or new tracked automated test source.

PASS [current PR CI45265879282](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45265879282)
at6603e4d5a752ec9270ffdd29d5fe71d9b47755d0: both public images, core and package.
Artifact14862881979 SHA-256
`bce0d8e7665f9a6ed14d123af17d1844daaa7e223d0ce73db0e9657da1c9bbd3`.
Normal2,044,240bytes SHA-256
`ee1fbd6ff37d7a90fb1fcc9b7f0041276ef140db1a2170c65c6a17b947fcdd2e`;
Simulator1,048,768bytes SHA-256
`1c391a7b892ab84ca700d8b1044e4baf2a7ffa6daf29e149f4ce01185903c084`.
Explicit0.4.18 descriptors, manifest, all USB hashes/offset/map and USBNormal=OTA,
physical-only Normal/mock-only Simulator and strong GLOBAL NVS/rollback hooks
pass. Superseded run45264709040/artifact14861213799 is not the release source.
Merge d1fa73b379e0b38e783adfff477267bdf280b36a exactly matches the reviewed tree.
Receipts: ignored `build/review-v0418/{validation,source-review}.json`.

PASS [tag CI45266887723](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45266887723)
atv0.4.18/d1fa73b3: artifact14863162889 SHA-256
`88f553af42b76eb99508fc8d9931e75bed8292d92c4aceca6156f9d2e64336f9`.
Full descriptor/manifest/USB/map/backend/protection checks pass. Both images
match reviewed PR bytes exactly. All four draft456006007 assets and notes match
the tag, confirmed by independent metadata review. Receipts: ignored
`build/review-v0418-tag/{validation,draft-validation}.json`.

PASS publication: release456006007 published as latest ordinaryv0.4.18 at
2026-11-23T21:17:16Z. The unchanged bare latest-feed verifier passes version,
schema/board/channel/layout/descriptor, length and SHA-256. Public pinned Normal
matches reviewed tag bytes exactly. Receipt: ignored
`build/published-v0418/verification.json`. v0.4.17 remains unpublished. These
delivery/build/MOCK checks do not establish physical ROM or Auto feed recovery.

November 23 v0.4.17: [load readiness and Auto Load handoff](LOAD_READINESS_2026_11_23.md).
Publication HOLD: the subsequent Stairs report is addressed in v0.4.18 instead.
PR CI45260726519 at836fe791 and tag CI45261755158 ata6bb9d4b pass both public
images/core/package. Tag artifact14860886764 SHA-256
`55834dfd38891f6613242a90a4f1d335b555cd3a6fd31b0d50f298280cfdac37`
matches the PR images exactly. Full descriptors/manifest/USB hashes/offset/map,
backend isolation and strong NVS/rollback hooks pass. All four draft455975057
assets and notes match the tag; it remains unpublished. Receipts: ignored
`build/review-v0417/validation.json` and
`build/review-v0417-tag/{validation,draft-validation}.json`.
PASS actual PhysicalSession offline MOCK:15 scenarios/13,436 assertions and
31 existing regressions. Prioritized AB01, exact0/1, malformed/error/foreign/CRC
guards, same-sequence revocation/recovery, three paced attempts, background late
recovery, no intent replay, reconnect reset and explicit unload pass. Synthetic
15.4-second/70-intent differential sends0 activation queries before and3 after,
with zero motor/Auto controls. Receipt:
`/workspace/preview-tools/activation-v0417/validation.json`.
PASS actual LVGL/TrainingApplication observers at360/466/480:51 Auto-to-Live
scenes/795 assertions, plus57 quiet-dial cases/306 checks against the combined UI.
The old executable reproduces the phase14 admission failure. Confirmed completion
without reps,35-to-loaded without14, retained old rest/end/active, new boundaries,
TTL/unknown consumption, same-device reconnect, fresh readback, unload/rearm,
normal tap behavior, reported target changes/no fallback and real errors pass.
Expected pre-first-rep silence adds no rep warning when target telemetry is valid.
Renders fit all three round sizes. Receipts:
`/workspace/preview-tools/single-auto-live-v0417/validation.json` and
`/workspace/preview-tools/quiet-dial-v0417/combined-validation.json`.
Core/packet-inspector build and diff whitespace check pass. No new tracked
automated test source or physical
Bluetooth/motor operation. Four real standalone B4 fixtures pass72 actual-session
MOCK assertions across Auto11/12/13/14,35-to-1/5 invalidation/recovery, missing
target clearing and no force/base fallback. Receipt:
`/workspace/preview-tools/single-auto-live-v0417/target-replay-validation.json`.
The standalone post-Auto stream sequence omission is reproduced at100/260 ms
cadence. PASS final actual-session MOCK follow-up matrix:22 scenarios/121
assertions. Exact AA12/read/AA13 order sends one trigger and zero motor-setting
writes; refresh follows at200/520 ms for the two cadences. Failed trigger/read/
refresh, cancellation, unload, disconnect, topology/workout loss, boot/manual/
preexisting Auto and unchanged Twin behavior pass. Activation15/13,436 and the
existing31 regressions also pass on final combined PhysicalSession source
SHA`3da78bc78a06b26d48afb499d8f9403b992cd9bcaf1951a6b9973e9f5fe2bf14`.
Physical Auto
Load target-feed recovery remains unverified pending a report from that state.

November 22 v0.4.16: [ten-repo GATT review](TWIN_GATT_REVIEW_2026_11_22.md).
PASS independent current-head/source comparison, actual HELLO byte comparison
against Steven and literal official capture, existing core/packet-inspector build
and diff whitespace check. The HELLO payload/header/CRC is unchanged.
PASS actual Native adapter compiled against a synthetic SDK: 16 scenarios /
126 assertions on cpp SHAcf4b96ef12a4578cfd5cfed502a24eaba4a2c87facabc77f68a229fea87e0357.
One full snapshot, callback/pointer lifetime, reconnect invalidation, absent/empty
channels, queued531 during discovery/subscription/write, no recursive callback
delivery or queue consumption, stale-owner/wrong-peer/cancelled events, overflow
and live-only failure clearing pass. UUID case/short/base aliases share one
snapshot/callback lifetime, including first-use SDK-prefixed short UUIDs;
invalid-length UUIDs fail without a query. ASan/UBSan pass; LeakSanitizer was disabled
because this environment cannot run it under ptrace. This is the actual adapter
with mocked SDK behavior, not a real NimBLE host/ATT or physical-radio check.
Receipt: /workspace/preview-tools/native-gatt-v0416/validation.json.
PASS actual-controller/PhysicalSession offline MOCK validation: 20 scenarios /
1301 assertions, plus31/31 existing PhysicalSession regressions. Authentication,
readback and fresh-primary readiness work through each documented response route.
Missing/non-notifiable/failed55CA can use notify-capableA010; failedA010 can retain
55CA. No response or telemetry alone refuses before HELLO. Dead query/subscription/
clear cannot complete; real531 is retained. Optional telemetry, separate fragment
parsers, no preaccept controls, reconnect cleanup and original sequence0 HELLO pass.
Controller cpp SHA241e225b9f142bb6da9171400b8fb80968e61bf743050912de262d93fd4279ef.
Receipt: /workspace/preview-tools/response-routes-v0416/validation.json.
PASS unchanged LVGL/application regression:72 cases /1566 assertions across
360/466/480. Discovery, membership changes, visible-member selection, both icon
halves, automatic/manual Retry, async scans, cancellation and Wi-Fi/report/update
handoffs pass with zero resistance intents/GAP overlaps in these MOCK cases.
69 renders match v0.4.15 byte-for-byte; three Updates renders differ only in the
version fixture. Receipt: /workspace/preview-tools/twin-discovery-v0416/validation.json.
No new tracked automated test source or physical radio/pairing/motor operation.
These checks do not prove error531's cause or physical Twin recovery.
PASS [CI45078070078](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45078070078)
at95cc2de7d42327a21ea73c926842664543755c65: public Normal/isolated Simulator,
offline core/packet inspector and packaging. Artifact14776605308 digest matches
`9a3439bc6f0ccf0b75d2dce5d86e7a17fa63892d3ea0fbaeefc1ce03f9ee45d4`.
Normal2,038,912bytes SHA-256
`05cabae14b360f58e29d7b376f5eba1ab3d6e7c7af5e3a324b50e16ce04e8654`;
Simulator1,047,584bytes SHA-256
`cefacc2830d52887a64649c393362c7f9aee21461fb7d4c8c2bb5fc04f252204`.
Explicit0.4.16 descriptors, manifest, every USB hash/offset/partition map and USB
Normal=OTA pass. Defined-symbol checks confirm physical-only Normal/mock-only
Simulator and strong GLOBAL erase-protection/verifyRollbackLater hooks in both.
Merge02aa9672d4cc940fc536bd55d7961a01ab0163ce has the exact reviewed tree.
Receipt: ignored build/review-v0416/validation.json.
PASS [tag CI45078626045](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45078626045)
atv0.4.16/02aa9672d4cc940fc536bd55d7961a01ab0163ce: build/package/draft creation.
Artifact14775914768 digest matches
`1e190b8a43557b2fe4277527deb371195f8df546c20fe48e62339f6818405b4c`.
Normal/Simulator exactly match the reviewed PR bytes. Full descriptor, manifest,
USB hash/offset/map, backend isolation and strong protection-hook checks pass.
All four draft asset sizes/digests and notes match the tag. Receipts: ignored
build/review-v0416-tag/validation.json and draft-validation.json.
PASS publication: release454802614 published as latest ordinaryv0.4.16 at
2026-11-22T22:41:28Z. Initial bare latest redirect returned0.4.15 with an older
Date, while APIlatest and the pinned0.4.16 manifest were current. Revalidation
with no-cache headers returned0.4.16; the subsequent unchanged bare-URL verifier
also passes. Public latest manifest and pinned Normal image pass schema, board,
channel/layout/version, descriptor, length and SHA-256, matching tag bytes.
Receipt: ignored build/published-v0416/verification.json. No physical installation,
radio connection, pairing or motor operation was performed; recovery remains
unverified. The transient response does not establish where it was cached.

November 22 v0.4.15: [Twin refresh and connection setup](TWIN_DISCOVERY_2026_11_22.md).
PASS independent source review and existing core/packet-inspector build.
PASS external actual-controller/PhysicalSession offline MOCK validation: 11
scenarios / 111 assertions, plus all 31 existing PhysicalSession regressions.
Targeted write -> exactly one original HELLO -> notification setup; no early
acceptance, readiness, load or unload. Fresh post-GATT pacing/deadline, every
service/write/query/subscription failure, explicit reconnect, idempotent prepare,
discarded premature acceptance, optional telemetry and legacy fallback pass.
PhysicalSession cpp SHA aee111b3feb04dd8ce76f967fa5e58ac0778b07f3313f0c0757f83c135cb02bb;
controller cpp SHA ba4bc43236a07b364a4d831922e7773533cb218cd877fd192634ff9f927e1d23.
Receipt: /workspace/preview-tools/early-hello-v0415/validation.json. Both executables
were rebuilt with the final generic transport navigation API/header.
No new tracked automated test source or physical radio/motor operation. These
checks do not establish error 531's cause, current roles or physical recovery.
PASS actual LVGL/TrainingApplication matrix: 72 cases / 1566 assertions, 24 scenes
at 360/466/480 on UI SHA697ab492cbb1d5766b169b38684a12e38268f07221cdbf9173ed3869366f12c1
and app SHA30015e91aa4ca3f5532769449fae1defbddf4bd8d38cd292d138c50c225e09d3.
Periodic paired/Alone/partner changes over five scan windows, old catalog Alone,
unknown invisible partner/bare identity, both icon halves selecting only the
visible entry, automatic interruption, pre/post-GATT budgets, pending native-style
async scan completion and actual LVGL Retry after changed visibility all pass.
Settings, Updates, debug SEND, first-idle report handoff, battery alert, pending/
held touch and Wi-Fi pause-before-GAP pass. Zero resistance intents or Wi-Fi/GAP
overlaps, including a 37 ms simulated shutdown clock. Visual samples at all three
sizes fit the round display with no redundant Twin labels. Receipt:
/workspace/preview-tools/twin-discovery-v0415/validation.json. Synthetic MOCK I/O
and real LVGL, not physical Bluetooth compatibility or trainer-role proof.
PASS [CI45071575628](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45071575628)
at31ab87dc696c253110ebd98c685da1357fc2de31: public Normal/isolated Simulator,
offline core/packet inspector and release packaging. Artifact14772873798 digest
matches `8d10dd855226086be7344072ef6db945b4ec39e6c2474adb3cf87211aa167c96`.
Normal 2,034,976 bytes SHA-256
`f9dd1b1129af6c62c18c3b5bd79a6b2aea82f2574757f81a2de8e73765936bb2`;
Simulator 1,046,304 bytes SHA-256
`3a4c48f5141958f9f8d6699203b977743bfa0db57d0ae445869793707e6832ff`.
Explicit0.4.15 descriptors, manifest, every USB hash/offset/partition map and USB
Normal=OTA pass. Defined-symbol isolation confirms physical-only Normal/mock-only
Simulator and strong GLOBAL erase-protection/verifyRollbackLater hooks in both.
Merge fcf39ba1c142d74bd79c496012d97b6d75d28977 has the exact reviewed tree.
Receipt: ignored build/review-v0415/validation.json.
PASS [tag CI45072079593](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45072079593)
at v0.4.15/fcf39ba1c142d74bd79c496012d97b6d75d28977: build, package and draft
creation. Artifact14773947135 digest matches
`ea1e45a4fc4ac6f01cab3414c1ae7b5bdf018dfe8d6f4207aca994dec3738bab`.
Normal and Simulator exactly match the reviewed PR bytes. Full descriptor,
manifest, USB hash/offset/map, backend isolation and strong protection-hook checks
pass. All four draft asset sizes/digests and notes match tagged files. Receipts:
ignored build/review-v0415-tag/validation.json and draft-validation.json.
PASS publication: release454764162 published as latest ordinary v0.4.15 at
2026-11-22T21:31:21Z. Public latest manifest and pinned Normal image pass schema,
board/channel/layout/version, descriptor, length and SHA-256, matching tag bytes.
Receipt: ignored build/published-v0415/verification.json. No physical installation,
radio connection, pairing or resistance operation was performed. Error531 recovery
and the official app's actual current Red-to-pair route remain unverified.

November 22 v0.4.14: [primary timeout and Wi-Fi ordering](TWIN_CONNECTION_REPORT_2026_11_22.md).
PASS independent source review and external offline MOCK differential validation
against 20b99e0. Eight scenarios / 83 assertions reproduce GAP starting with fake
Wi-Fi powered in the old manual, actual Retry and automatic paths. Corrected UI
SHA b030d84989c399b59bddc9dc95f1eec02766f83e62dea55ab80ce99976be6d70 pauses
before all three, with fresh native-poll timestamps after a 37 ms fake shutdown.
Active frames pause once. Failure -> idle resumes Wi-Fi in that frame. Pending
explicit unload delays debug BLE release; the first released frame resumes
Wi-Fi/report, with one unload and no load. Leave -> saved-Wi-Fi update check passes.
Existing update-reconnect renderer: 13 scenes at 360/466/480; baseline/fixed
stdout and pixels are byte-identical in all 39 cases (unchanged version fixture).
Receipt: /workspace/preview-tools/wifi-ble-handoff-v0414/validation.json.
PASS existing offline core/packet-inspector build and diff whitespace check.
No new tracked automated test source, physical radio connection or motor operation.
This verifies software ordering, not the cause of b47444b1 or physical recovery.
PASS [CI45058145716](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45058145716)
at 0aa1fe4e8dfa88c0b0d70d4136b81d503dee5ccf: Normal/isolated Simulator, offline
core/packet inspector and packaging. Artifact14767984048 digest matches
`f8fa3490687fa806510aa238b767b2aafd1afe10ab9e83f44ff2843e743ca5d9`.
Normal 2,027,120 bytes SHA-256
`f6d869d397b9c00e27b9d53d36e88e0ffbe55f3293b4289563552182452f0fab`;
Simulator 1,040,848 bytes SHA-256
`0190f9dc353a3efb088f2b5cca8fb14c27a74e38cb96518abe1e12b4cc55a00f`.
Both explicit 0.4.14 descriptors, manifest identity/size/hash, every USB file
hash/offset/partition map and USB Normal=OTA pass. Defined-symbol checks confirm
physical-only Normal/mock-only Simulator and strong GLOBAL erase-protection and
verifyRollbackLater hooks in both images. Merge aa223a5a24518d4e86131fd32fe411e69fbfa4b2
has the exact reviewed tree. Receipt: ignored build/review-v0414/validation.json.
PASS [tag CI45058676409](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/45058676409)
at v0.4.14/aa223a5a24518d4e86131fd32fe411e69fbfa4b2: build, package and draft
creation. Artifact14769125174 digest matches
`ddf67052251db743de8c3286e7cc2e00a16bb7770de3dc4946ade4eebbf028f6`.
Both firmware images exactly match the reviewed PR above. Full descriptor,
manifest, USB hash/offset/map, backend isolation and strong protection-hook checks
pass. All four draft asset sizes/digests and release notes match tagged files.
Receipts: ignored build/review-v0414-tag/validation.json and draft-validation.json.
PASS publication: release454681385 published as latest ordinary v0.4.14 at
2026-11-22T19:09:48Z. Public latest manifest and pinned Normal image pass schema,
board/channel/layout/version, descriptor, length and SHA-256, matching tag bytes.
Receipt: ignored build/published-v0414/verification.json. No physical installation,
radio connection or motor operation was performed; timeout causality and physical
recovery remain unverified.

November21–22 v0.4.13: [complete native Twin comparison](TWIN_COMPARISON_2026_11_21.md).
Current public heads and source/capture scopes were checked, including the separate
Albert Android implementation. PASS new scoped weight helper syntax and manual
arithmetic:93,357 bounded cases plus published179LB→163KG paired and duplicate-bucket
max16 inverse safety. PASS existing core/inspector with new Twin B4 decoder and
manual external decode of all seven published real Twin captures; strict envelope,
length/source and signed velocity/count checks pass, no target inference. Full
integration, UI, firmware/package and publication receipts follow below.
No new tracked test source or physical motor operation. Unknown/secondary loading,
unproven paired target, loaded modifier edits and automatic drops remain unsupported.
PASS actual PhysicalSession public-API offline MOCK I/O:31 scenarios,0 failures on
frozen cppSHA51a02321c35a045bf77735ddcdb0a2dbc82231bd9b88dbe9d7d89347b913f079.
Checks include Primary promotion/Secondary refusal, native/legacy motor words,
one AA12 trigger with2003/status14armed/15complete, unknownA003 unload-only,
130total→65one-wire-write and settled confirmation, modifier/percent scope,
fresh reported460/500 limits, minimum/grid/envelope and changed-cap cancellation.
TTL/unknown-same-pair retains GATT/unload while cancelling intent; explicit changed
peer/role/alone permanently revokes that connection's control route. Reused-sequence
A7 observations renew or revoke correctly. Sparse counters from real source12 B4
persist across actual source10/11 samples; no target/held/completion phase is added.
Foreign-source acceptance/activation/parameter/unload-ACK frames are rejected.
Manual loaded base uses both stationary channels, one base write/no motor or
accessory reassertion; moving/stale/Auto/accessory/curve/unit/FFFF-field cases refuse,
while legitimate signed velocity-1 remains valid. Native radio timing is not exercised.
Standalone regression keeps130LB as130 wire and preserves its5/4 load/unload profile.
PASS app/Mock native observations: canonical typed aliases preserve stable identity,
names and pair references; same-name different addresses stay distinct. Stale and
transitional routes retain Main/probe-off and explicit unload without edit/load/Auto
calls or intent replay. Fresh contradictions revoke. Manual50→52 uses one settings
call with no load/Auto/automatic-live calls; new motion or stale data cancels pending
intent. Receipts remain external under /workspace/preview-tools, not tracked tests.
PASS LVGL implementation matrix:28 scenes at360/466/480, covering canonical aliases/
Retry, primary control admission, LB/KG combined step and reported limits, pending/
confirmed load/unload, hold/Auto11/12/14/15, Live entry with target unavailable,
stale/transitional unload retention, explicit contradiction revocation and guarded
manual loaded base edits. The observed stale Waiting footer was fixed in fd2ad585.
PASS final focused recovery/feedback matrix:24 combinations across all three sizes
on fd2ad5852514bd6a803bd0565b7c84ae2422bb3b. Own Waiting text clears without intent
replay, unload feedback is preserved and Auto13 Engaging fits. The eight actual
protocol source hashes still match the31/31 receipt. External receipts:
/workspace/preview-tools/physical-integration-v0413/validation.json and
/workspace/preview-tools/final-focused-v0413/validation.json. All are offline MOCK
checks, not physical compatibility evidence. Working tree was clean at final head.
PASS [CI44965504418](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44965504418)
at fd2ad5852514bd6a803bd0565b7c84ae2422bb3b: Normal/isolated Simulator, offline
core/inspector and packaging. Artifact14730718768 digest matches
`385859a385b49e43f8314995a06e644f3d4d4e81679c03bdb35c5af482377565`.
Normal2,027,072bytes SHA-256
`a273ca75439f14e74922b5f2db416c551c17854056752c9c64d1f1d0e5c54ccc`;
Simulator1,040,800bytes SHA-256
`65ca08d1ef7ab3e73304e44f2a326acffeba6b42ec4df5bb93806c6fa54cb77e`.
Both explicit0.4.13 descriptors, manifest, all USB hashes/offsets/partition map and
USB Normal=OTA pass. Defined-symbol isolation confirms no mock backend in Normal
and no PhysicalSession/NimBLE/updater/reports functions in Simulator. Both images
have strong GLOBAL erase-protection and verifyRollbackLater hooks. Merge127c8fb
has the exact reviewed tree. Receipt: ignored build/review-v0413/validation.json.
PASS [tag CI44965962167](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44965962167)
at v0.4.13/127c8fb7bfb85d1ee2c23d7a8f9631bb423738fd: build/package and draft
creation. Artifact14731925608 digest matches
`c966189f790aa51d32ed8bfc90cee60abba0aac5177144551d541906cc351485`.
Normal and Simulator bytes exactly match the reviewed PR above. Full descriptor,
manifest, USB hashes/offsets/map, backend isolation and strong NVS/rollback hooks
pass. All four draft assets' sizes/digests and release notes match tagged files.
Receipt: ignored build/review-v0413-tag/validation.json.
PASS publication: release454082364 published as latest ordinary v0.4.13 at
2026-11-22T00:23:59Z. Downloaded public latest manifest and pinned Normal image pass
schema/board/channel/layout/version, descriptor, length and SHA-256; bytes match
the checked tag. Receipt: ignored build/published-v0413/verification.json.
No physical installation, radio connection or motor operation was performed.

November21 v0.4.12: [host timeout retry](TWIN_TIMEOUT_2026_11_21.md).
Report58926a2a confirms physical v0.4.11 follower recognition and preserves the
previously hidden first-endpoint code13/10.14s timeout. PASS independent source
review: exact Failed13 retry once,4500ms native attempts, shared10000ms radio
budget plus existing1000ms MTU grace, terminal/queue/handle/GAP reuse barriers,
and no application third visit. Typed first failure wins over generic role errors;
fresh topology contradictions win over prior failures, including at the deadline.
Read-only/role/Retry/command permissions are unchanged. Compact diagnostics retain
stage/code/timeout/attempts/radio time without duplicate closed-session context.
PASS existing core/packet-inspector build and diff checks. Physical retry recovery remains unverified; Twin
resistance controls remain unavailable. No new automated test source.
PASS real LVGL at360/466/480: exact report timing of first preacceptance timeout
(10141ms/code13), then accepted Follower(2940ms), retains the original timeout in
the notice and only two automatic candidates. Actual Retry resolves the updated
Master directly and enters read-only Main with probe1/controlCalls0. Fresh
contradictory identity overrides the earlier failure, including a separate clock
jump onto the30s global deadline. Layouts fit all three display sizes. These
previews validate application/UI behavior, not native radio recovery.
PASS [CI44960382285](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44960382285)
at e1de93d31a5b28efafafdc901f3aae87939f1bda: Normal/isolated Simulator, offline
core/inspector and packaging. Artifact14729287754 digest matches
`cede7fcf1c41bb8b03331e52ca20675ec8d5e4056930009c72e005d05346917f`.
Normal2,015,776bytes SHA-256
`57065610796199155c7454902c85faf4ff8d05d23bddcc9775f6af45735d3a4d`;
Simulator1,034,288bytes SHA-256
`44122ffff23e003e4c42d86af36905abec8581efee066009841ae50e527c4606`.
Both explicit0.4.12 descriptors, manifest identity/size/hash, all USB file hashes,
offsets and partition map, and USB Normal=OTA pass. Defined-symbol checks confirm
physical backend only in Normal, mock only in Simulator and strong GLOBAL
__wrap_esp_partition_erase_range and verifyRollbackLater hooks in both images.
Merge43651822df543ed26a0d4da194d6d20dc664c1ce has the checked tree. Receipt:
ignored build/review-v0412/validation.json.
PASS [tag CI44960798145](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44960798145)
at v0.4.12/43651822df543ed26a0d4da194d6d20dc664c1ce: build, packaging and draft
creation. Artifact14729378523 digest matches
`8699ae4ea2d8080b935afd23dd7d62d8b58b41fc6080b3ba24ec504a6e8a0b80`.
Both firmware images exactly match the reviewed PR bytes above. Descriptor,
manifest, all USB hashes/offsets/partition map, backend isolation and strong
NVS/rollback checks pass. All four draft assets' sizes/digests and release notes
match the tagged files. Receipt: ignored build/review-v0412-tag/validation.json.
PASS publication: release454053504 published as latest ordinary v0.4.12 at
2026-11-21T23:20:17Z. Downloaded public latest-feed manifest and pinned Normal image
pass schema/board/channel/layout/version, descriptor, length and SHA-256; binary
bytes match the reviewed tag. Receipt: ignored build/published-v0412/verification.json.
No physical installation or connection/motor operation was performed. Real radio
recovery from the new policy remains unverified; physical Twin controls unavailable.

November21 v0.4.11: [connection handoff and Retry](TWIN_HANDOFF_2026_11_21.md).
PASS independent source review: bounded exact12/22 read-only role inference,
own-match/current-field/epoch/freshness requirements, reciprocal second candidate,
role-derived navigation hint only and retained physical probe. Every app manual
command and automatic recipe is blocked in the read-only view. Standalone stale
unload remains accessible. BLE reuse requires matching terminal evidence and queue
drain, not a time-only fallback. Unsupported Bluetooth-occupied wording is removed.
PASS existing core/packet-inspector builds and PhysicalSession native syntax check.
PASS real LVGL360/466/480 pointer flows: generic renamed follower->master, direct
master, Retry after stage/code error, Back and explicit Send debug log consent.
Master view remains probe1/readOnlyView1; dial/tap preserves base/load and produces
zero backend control calls. No new test source or physical operation. Physical radio
improvement and Twin resistance control remain unverified/unsupported respectively.
PASS independent Wi-Fi source review: queue the manual check before reconnect,
wait the existing discovery window, issue one saved join, preserve an ongoing join's
deadline and adopt a running startup check. Cancel pending work on navigation,
VOLTRA use, setup, upload or install. No credential writes/reset, portal or
automatic installation; existing install save/readback checks remain in place.
PASS real LVGL Wi-Fi pointer previews at360/466/480: pending/checking/join failure,
bounded25s wait, duplicate taps, leave/control cancellation, async scan wait and
fast Current/Error completion. One join/check as appropriate, no join overlapping
the previous scan and no hidden deferred check after cancellation. No setup/install
calls. Final ASCII status and full cleanup/restart advice fit all three displays.
PASS [CI44956073824](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44956073824)
at674e6da91c8428a1aeba8f90eed7b39d28596759: public Normal/isolated Simulator,
offline core/inspector and packaging. Artifact14727611985 digest matches
`9ae31f177e940aad0e5c0da9155c5ab5dd63c876c664be7d4b23a4b5d8389ec3`.
Normal2,011,296bytes SHA-256
`03febdd2e33008f04999ee66e2237deeacd8caa5bda498f8242f52b6acab262e`;
Simulator1,032,352bytes SHA-256
`dfbf2f0f3bc68e68dabffd97e240dc992bc0d759519071820cdba16cefff9b5d`.
Both descriptors, manifest, all USB hashes/offsets/partition map and USB/OTA equality
pass. Defined-symbol checks confirm physical backend only in Normal, mock only in
Simulator and strong GLOBAL __wrap_esp_partition_erase_range and verifyRollbackLater
in both. Merge712870549e84f1c10fd99b9b082d76c7e075cc31 has the exact checked tree.
Receipt: ignored build/review-v0411/validation.json.
PASS [tag CI44956593104](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44956593104)
atv0.4.11/712870549e84f1c10fd99b9b082d76c7e075cc31 builds, packages and creates
the draft. Artifact14727717702 digest matches
`3b43b37e4bf4afd48574418f9a9bd71ce04cedb0e95527314b84427ec2c69b67`.
Both firmware images are byte-identical to the PR images above. Full descriptor,
manifest, USB/hash/partition, backend and strong NVS/rollback checks pass; all four
draft assets' sizes/digests and release notes match the tagged files. Receipt:
ignored build/review-v0411-tag/validation.json.
PASS publication: release454028353 published as latest ordinary v0.4.11 at
2026-11-21T22:30:00Z. Downloaded public latest-feed manifest and pinned Normal image
pass schema/board/channel/layout/version, descriptor, length and SHA-256; bytes
match the reviewed tag image. Receipt: ignored build/published-v0411/verification.json.
No physical installation, network-profile reset or motor operation was performed.

November21 v0.4.10 diagnostic update: [report357833b6](TWIN_STATUS_REPORT_2026_11_21.md).
PASS existing core/packet-inspector builds and five full published A7 payloads;
the published short/truncated payload remains rejected. Identity/metadata
separation does not supply role or authorize commands. No new test source.
PASS static review: sanitized headers/both-slot comparisons contain no address
bytes; latest detail is always reported while accepted/rejected transitions share
a30-second log budget. Fresh own-matched identity can nominate a read-only
candidate while semantic state remains unknown; final promotion still requires
paired12/currentMaster/reciprocal identity. Physical role remains Unknown.
PASS real LVGL previews360/466/480: known pair names and unknown second names
show no placeholder/separator; no normal-screen "peer" wording remains. Failure
screen Send debug log opens existing consent with zero automatic submissions.
Fresh identity with unknown semantics probes both resolvable endpoints read-only,
then fails without training entry, promotion or control calls. Slot1-only matches
remain diagnostic-only, so two-endpoint inspection is not guaranteed.
Required public build/artifact results follow. The release is now diagnostic,
superseding the historical hold recorded below; Twin controls are still blocked.
PASS diagnostic release source: [CI44952181058](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44952181058)
at55c513968fb3da5d687f069359eea9f1b14d5589 builds public Normal, isolated
Simulator, offline core and packaging. Artifact14725878221 digest matches
`33cd62076a5823336f751889a3a9b8b933286d1988b1e10f2b1859396671fae5`.
Normal2,001,824bytes SHA-256
`79581c62b3682b065f95f0be769e4509d193636e57ea5c4ab314f05739e597e9`;
Simulator1,027,248bytes SHA-256
`26cecb7a1483a27bf31a8cd5d0a5d2358cd57a6c9043b8eda91bacd5a264a2f4`.
Descriptors, manifest, USB hashes/offsets/partition map and USB/OTA equality pass.
Defined-symbol checks confirm physical backend only in Normal, mock only in
Simulator and both strong GLOBAL __wrap_esp_partition_erase_range and
verifyRollbackLater hooks. Mergea238b7f has the exact checked tree. Receipt:
ignored build/review-v0410-nov21. Tagged publication checks follow separately.
PASS publication: [tag CI44952693885](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44952693885)
atv0.4.10/934b070f180649ebb1798cd3389fe108824b8408 passes builds, packaging and
draft creation. Artifact14726226806 digest matches
`463ef9a4bcd5b5d4c72494fe6142aec4f2af06f00a6f1a018736055ee40e3761`.
Normal and Simulator are byte-identical to the reviewed PR images above. Full
manifest, USB/partition/hash, backend and strong NVS/rollback checks pass; all four
release asset hashes/sizes match. Receipt: ignored build/review-v0410-nov21-tag.
The diagnostic release was published as latest at2026-11-21T21:45:22Z. Downloaded
public latest-feed manifest and pinned image pass schema/board/channel/layout/
version/descriptor/size/SHA-256 checks against the reviewed Normal image.
No physical installation or motor operation was performed. Twin resistance
controls remain unavailable; the next owner report supplies missing status fields.

November 20 v0.4.10 addition: [remote battery alert/estimate](REMOTE_BATTERY_2026_11_20.md).
PASS static review: source is the remote hardware's valid optional reading only,
not VOLTRA/peer battery. Queue at <=10%, defer Live/pending-Live/known active sets,
other modals, gestures, connection resolution and critical work. OK acknowledges
one episode; known >=15% rearms. Unknown/invalid readings defer without losing a
pending warning. Modal input cannot reach underlying weight/load controls; explicit
UNLOAD remains available. Board-owned ADC cache uses no I2C or power writes.
PASS real LVGL previews at360/466/480: threshold10 alert, deferred Live then
set-over display and explicit unload confirmation. Extended360 previews cover
active set on Settings, dismiss/no jitter nag,15->10 rearm, unknown/255, held touch
and encoder-input consumption.
Separate360/466/480 loaded-rest previews retain loaded=true and repsStarted=true
while changing the fresh setActive report to false: Live returns to Main and the
warning appears with OK/UNLOAD, with zero control calls. An unload is not required
to receive the deferred warning after a confirmed set end.
No new automated test source or physical battery/motor operation. Physical rail
values, estimate calibration and exact threshold timing on the owner's unit remain
unverified. Public firmware/build results follow; existing Twin publication hold
remains in effect.
PASS held-draft battery build: [CI44770074824](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44770074824)
at5ea859f2cb9fa9e2d44a04202839da159dc73486 passes public Normal, isolated
Simulator, offline core and packaging. Artifact14647696114 digest matches
`7242819911b60573ec06750d8aa138fe5da064125eda65ff4c95c3cd381698c6`.
Normal1,997,520bytes SHA-256
`2fe2136f16cbb41421bbef85da803cb6b168876b51285a9835397591cbaaff96`;
Simulator1,027,072bytes SHA-256
`b8dd56ad5ab4c2a123d0bb33b198c88ad637d891f2a137a3f3d3b58c62f6d3a6`.
Descriptors, manifest identity/size/hash, all USB hashes/offsets/partition map,
USB/OTA Normal equality, defined-symbol backend isolation and strong NVS/rollback
hooks pass. Receipt: ignored build/review-v0410-battery. No merge, tag or release
publication; physical ADC calibration and alert timing remain unverified.

November 20 v0.4.10: [one paired control and quieter screens](TWIN_PRESENTATION_2026_11_20.md)
and [reported target-resistance correction](LIVE_TARGET_SOURCE_2026_11_20.md).
UNRELEASED: the owner requires actual current-master resolution for every pair.
The physical decoder remains unavailable; PR10 stays draft and publication is held.
The later generic router is checked with explicit synthetic roles only.
PASS: existing offline core/packet inspector build and pinned Ryan validators
(203 SDK packets, weight5..230,593 modifier packets, SDK identity consistency).
Four published CRC-valid independent B4 captures decode a6LB reported target;
the published Twin-source frame is rejected by the independent target decoder.
No new automated test source or physical motor command is added.
PASS: actual LVGL Main/Devices/unknown-motor renders at360/466/480 show names-only
headers, no caption/banner/dial overlap and matched availability. Synthetic
pointer previews on both paired circles at every size open the same endpoint.
Live renders at all three sizes cover reported28 versusBASE25, zero, absent,
stale-- andKG. These are offline visual/source checks, not physical compatibility.
Independent static review checks pair navigation versus current authorization,
absolute two-pound grids, target context/freshness and rollback-readable storage.
PASS: actual DesktopDeviceMemory preview across separate processes restores one
pair, with connection/readback/freshness/readiness absent; either side selects the
same candidate. Subsequent role-aware LVGL previews use Orchid/Slate aliases with
no pair catalog: follower-first probes follower then master read-only, master-first
probes master, Unknown probes each endpoint once and fails, and Cancel disconnects.
At360/466/480 only fresh explicit synthetic Master reaches training. These previews
do not establish a physical role decoder or authorize any physical Twin command.
PASS: independent static review of the later role-aware framework checks bounded
reciprocal probes, dynamic readiness, cancellation, the physical query allowlist,
no automatic workout entry during discovery, and the mock's role/identity guard
before queued mutation. Physical role stays Unknown and cannot promote a Twin.
PASS baseline only: [CI44763541029](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44763541029)
at47f5f911e649db300825581e7919c5e7be0b756d builds both public images and packaging.
Artifact14643255564 digest matches
`c8e9ede972fcd9ed88b0a087ffd45a6b66b73a141b690716a54075c773105ae1`;
Normal1,977,088bytes SHA-256
`9bcaccceb3445dcaa3aa97858a1dbacb20cf92f781a62763f176f2a93962bf65`,
Simulator1,006,144bytes SHA-256
`77d65850155b7341205b3a20058f09f48954b562a739016db3b3e380e8522e80`.
Descriptors, manifest, USB hashes/offsets/partition map, USB/OTA equality, backend
isolation and strong NVS/rollback hooks pass. This artifact predates the subsequent
role-aware router/probe changes and is not approved as master autodetection.
PASS held-draft framework: [CI44766603773](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44766603773)
at ea1552bb28e0e7d0cdf36e02de36fa318e2e5379 builds public Normal, isolated
Simulator, offline core and packages. Artifact14644108166 digest matches
`b802a17b4cea580ebefe4c9fb216fcc662e389f378d87402a7ba93873b57a27c`.
Normal1,983,376bytes SHA-256
`00f34c8251dedbf6799ebf1f8c1a4651174a2a90ec98a39eaf5ab525e6a3b9fd`;
Simulator1,012,576bytes SHA-256
`266af4f2aa05aaedea809c49de4a791b94c2e8c294ad5d06c785b03bf352c1bf`.
Both descriptors, manifest identity/length/hash, all USB hashes/offsets/partition
map and USB/OTA Normal equality pass. ELF defined-symbol checks confirm physical
backend only in Normal, mock only in Simulator and both strong NVS erase-range
protection/verifyRollbackLater hooks. Receipt: ignored build/review-v0410-role.
These checks approve the draft's build/artifacts, not physical master detection.
PR10 stays draft; no merge, release tag or publication was performed.
Physical Twin adjustment/load/unload remains unresolved. The latest stored Twin
report still predatesv0.4.9, and A003, peer safety and a scoped combined maximum
remain unknown. Device-reported B4 target compatibility and exact agreement with
the owner's changing VOLTRA LCD still require a physical comparison.

November 20 v0.4.9: [Twin/readback, reconnect and Live changes](TWIN_LIVE_READBACK_2026_11_20.md).
PASS: changed offline core/packet inspector with CMake 3.31.6/GNU 14.2.0, existing
Ryan packet validator (203 SDK packets, weight 5..230, 593 modifier packets) and
pinned SDK consistency validator. The new strict A7 inspector accepts five full
published source fixtures and rejects the published short fixture without identity.
These are source/decoder checks, not physical compatibility evidence.
PASS: existing stevenpoint watch206 native protocol suite 37/37, including its A7
fixtures/Twin mode constants. This validates that upstream source, not our state
machine. Native syntax checks cover PhysicalSession and UI; embedded compilation
is validated separately by CI. No new automated test source is added.
PASS: actual LVGL headless renders at 360/466/480 inspect Twin/current/unknown/stale,
long peer names, Black/Red Venn selection, 130/150/50 combined readback with unknown
motor and unknown maximum, and Live 28 versus BASE 25, zero, missing/stale -- and KG.
Independent static review corrected stale topology recovery, contradictory reports,
own-identity mismatch quarantine and scope expiry between callbacks.
Physical checks remain: A7 compatibility/name matching, quick reselection, direct
16003 agreement with VOLTRA's changing LCD and report delivery. Physical Twin
role/peer safety/limits/A003 are unresolved; this release does not enable controls
or claim the user's Twin adjustment failure is fully repaired.
PASS: [CI44755705290](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44755705290)
at d29a5780bb82761d8f85b93a50afdae1901c1f87 builds public Normal, isolated
Simulator, offline core and packaging. Downloaded artifact digest matches GitHub:
`df0451f0fd5b4eb2e50f53ae406cff313fbc8108e08717b2c01f56ddb57a61ad`.
Normal is 1,968,944 bytes, SHA-256
`ac20394991e42991c3bfcc9c27f9e0d1e220f6cb60660b6c4b0ca670fe4d4df5`;
Simulator is 999,856 bytes, SHA-256
`016369d5c84d4237d4b97fc027dd509a3edefbecd35d1e142f1d2b9674986357`.
Both descriptors, manifest identity/size/hash, USB images/hashes/partition map and
USB/OTA Normal consistency pass. ELF defined symbols confirm backend isolation
and strong NVS protection/deferred rollback hooks. Merge325bf0a matches the checked
tree. Artifact receipts are kept in ignored build/review-v049.
PASS publication: [tag build44756585032](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44756585032)
at v0.4.9 /d84fb4b18c12520e6ec8dcac57b0164593bb783e passes public Normal,
isolated Simulator, offline core and packaging. Downloaded artifact digest matches
GitHub: `1437d4b2fa391cae72053f3bb92fa5df24819cfba5aeb1c83f00e2c319f5241d`.
Both descriptors, manifest, USB images/partition map and USB/OTA consistency pass;
Normal and Simulator images exactly match the reviewed build above. Tag receipts
are in ignored build/review-v049-tag. Release v0.4.9 was published as the latest
ordinary release on November 20 at 14:51:17 UTC under the owner's standing request.
The downloaded public latest-feed manifest and pinned v0.4.9 image pass board,
channel, layout, version, descriptor, length and SHA-256 checks; the firmware
exactly matches the reviewed tagged image. No physical installation or motor
trial was performed. The physical Twin/control and Live limits above still apply.

November 18 v0.4.8 follow-up: [Auto Load visual feedback](AUTO_LOAD_FEEDBACK_2026_11_18.md).
The owner confirms an actual Auto Load trial succeeds but remote progress/armed
feedback is missing. The new UI and read-only countdown changes were built and
checked separately; the earlier 7e13632 artifact predates them.
No new automated test source or physical motor operation is performed.
PASS changed offline core/packet inspector and existing pinned Ryan/SDK validators.
Real LVGL9.3 headless renders at360/466/480 inspect half/full touch hold, Starting,
armed guidance, received device countdown, release, cancellation and loaded return.
The temporary Linux adapter delegates to the existing shared SimulatorApp and
DesktopHardwarePlatform with synthetic pointer/time input; it does not establish
a graphical Linux desktop host or physical behavior. No clipping/overlap remains
in these scenes; cancel/loaded return restores modifiers and scale labels.
PASS: [CI44330719778](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44330719778)
at aae14dce22b6f38ff16fff3df1b23e4d06d156c7 builds public Normal, isolated
Simulator, offline core and release packaging. Reviewed archive digest matches
GitHub: `0e1282c2b67380f7a3bbe00d5db0c5abcc2ddbe17d61f17dab2ecd7bafa0e277`.
Normal is 1,951,136 bytes, SHA-256
`12d896fe75842b4e13a14f78ee2778d76c8ad905395d30f2f6e140d42c72ce61`;
Simulator is 987,488 bytes. Both descriptors, manifest length/hash, all USB bundle
hashes/partition map and USB/OTA Normal consistency pass. ELF inspection confirms
physical implementations only in Normal and mocks only in Simulator; both retain
strong NVS and deferred rollback hooks. Merge cfd23d2 has the identical tree.
These are offline source/build/artifact checks, not new physical verification.
PASS publication: [tag build44342205239](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44342205239)
at v0.4.8 /89d1dcac168cac133baa1b5e0152e73447c254da passes public Normal,
Simulator, offline core and packaging. Downloaded artifact digest matches GitHub:
`e72b01f99d31f566ea66c069c35fab76ddcab7d8467a324509cb4c0c0af76556`.
Both descriptors, manifest, USB/OTA consistency, backend isolation and strong
NVS/rollback hooks pass; Normal and Simulator hashes match the reviewed build.
The owner approved publication in chat. Release v0.4.8 is published as latest;
the public latest-feed manifest and version-pinned image return0.4.8 with matching
identity, length and SHA-256. No physical installation or motor trial was performed.

November 18 v0.4.8: [unload/reset report 5a65d590](UNLOAD_RESET_2026_11_18.md).
PASS baseline: offline core/packet inspector with CMake 3.31.6 and GNU 14.2.0;
existing inbound base-weight fixture accepts its valid CRC and rejects corruption;
pinned Ryan packet validator passes 203 SDK packets, weights 5..230 and 593
modifier packets; upstream validator against SDK a53804e passes. New test source
remains deferred. Final public firmware/package results are recorded above.
Physical unload/reset timing and subsequent Load/Auto Load remain unverified;
no physical motor command or USB operation is performed.

November 18: [hold duration, debug hint and report bb697e8f](HOLD_LOAD_2026_11_18.md).
PASS: local offline core/packet-inspector build, existing inbound fixture and
pinned Ryan validators (203 SDK packets, weights 5..230, 593 modifier packets,
SDK a53804e consistency). New test source remains deferred. Public Normal,
isolated Simulator, package and image results follow.
PASS: [CI 44321393881](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44321393881)
at 7948fdc28ad16c28af636b0a8d9a43569fb58ec6: public Normal, isolated Simulator,
offline core and release package. Downloaded artifact digest matches GitHub:
`2fab58410ea6dadb302c20b83b2b488dc530b763db79cbd1e10bc0f53d798710`.
Normal is 1,945,184 bytes, SHA-256
`389e00dc38527d4c838182778e5cfe69eb2e765f58bb5c6a7a5c8c2af5c72174`;
Simulator is 982,720 bytes. Image descriptors, manifest length/hash and USB/OTA
Normal consistency pass. ELF inspection confirms physical BLE/protocol/network
implementations only in Normal and mock transport only in Simulator; both retain
strong NVS protection and deferred rollback hooks. This is build/artifact evidence,
not a behavioral test of the new status predicate, touch logic or physical motor.
PASS publication: [v0.4.7 tag build 44322395772](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44322395772)
at c0ba428206ec36c328fbf6d010fa5e846f5ddfd7. Tagged asset descriptors, manifest
length/hash and USB/OTA Normal consistency pass; Normal hash matches above.
The release is published as latest. Public latest-feed and version-pinned firmware
downloads were verified as v0.4.7 with correct identity, length and SHA-256.
No physical installation/load/hold trial was part of publication.
These checks do not establish physical load safety/compatibility. Required owner
observations: status 0 under otherwise-ready conditions, status 6 unchanged,
missing/stale/other status blocked, hold/release vs tap, delayed release processing,
preset override, wake/movement/stale cancellation, no hint before 3 s, hint after 3 s,
and a six-second debug hold without a preceding load. Live-write status policy
and active-mode 5 restrictions are unchanged. No physical motor or USB test ran.

November 16: [settings retention and Wi-Fi completion](SETTINGS_RETENTION_2026_11_16.md).
PASS final configured-network overview/ping/reset source:
[CI 43890790858](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43890790858)
at e1c1e637840d9b51588e36bbf2a514e9dee18b76: public Normal/Simulator, offline
core and release packaging. Downloaded artifact matches GitHub digest
`d95183fadee1a35b7bbf0a47c10bff4810ce5fefb142eef2524e2a97d53f7388`.
Normal is 1,944,336 bytes, SHA-256
`ee9ee18540014a36ede0db8661b8fc1cf8eaa738248bad19be552ef0bb622d68`;
Simulator is 981,904 bytes. Both image identities, manifest size/hash and USB/OTA
Normal consistency pass. Both ELFs retain the strong NVS/rollback hooks; only
Normal links esp_ping_new_session/start/stop/delete_session/get_profile and
physical BLE/OTA/debug networking. Simulator retains mock transport.
These are compiled artifact checks, not live ICMP or UI-interaction validation.
PASS publication: [v0.4.6 tag build 43891852006](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43891852006)
at 7cce754a82fcbe88bcddf8f959fa7d92599b0710; reviewed tagged assets retain the
same Normal hash above and pass identity/manifest/USB checks. The ordinary release
is published as latest; public manifest and pinned OTA image download were
verified with matching version, descriptor, length and SHA-256. No physical OTA
or live ping trial was performed.
Pending physical checks: current vs saved/disconnected SSID, opening before
reconnect, ICMP reply/no reply/start failure, leaving during a check, BLE pause,
reset during a check, completed setup showing only OK, and cancellation/failed
replacement keeping the prior network. The historical build below predates this
follow-up.
PASS: local offline core/packet-inspector build, existing inbound fixture, pinned
Ryan packet/upstream validators (203 SDK packets, weights 5..230, 593 modifier
packets and SDK a53804e). No new test source was added.
PASS: [CI 43885326367](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43885326367)
at 3b702fa49fb903c443d6e9ab6f614fb997b0c772: public Normal, isolated Simulator,
offline core and release packaging. Downloaded artifact matches GitHub digest
`e4afe8884262c4811839aa6fa46c6cf2a097a329879515a33f073735f6b03579`.
Normal is 1,937,744 bytes, SHA-256
`d2663a251a68cd874f490b5ddfb2306202d40744dec39db1038207d1991b4940`;
Simulator is 980,400 bytes. OTA/USB Normal images match; independent descriptors
and manifest size/hash pass. Both ELF files contain GLOBAL (not weak) functions
`__wrap_esp_partition_erase_range` and `verifyRollbackLater`; Normal/Simulator
backend isolation is retained. These checks establish compiled wiring and image
delivery artifacts, not execution of storage-fault recovery or UI interactions.
Physical settings survival, Wi-Fi OK navigation, failed saves and NVS fault/
rollback behavior remain unverified; no physical motor tests are performed.

November 15 debug flow: [changes and evidence](DEBUG_FLOW_2026_11_15.md).
PASS: baseline and changed offline core/packet inspector builds with CMake 3.31.6
and GNU 14.2.0; pinned Ryan packet validator (203 SDK vectors, generated weights
5..230 and 593 modifier packets) and upstream validator against SDK a53804e;
the existing SDK chains-report fixture passes packet inspection and a one-byte
CRC corruption is rejected. The repository's existing inbound base-weight fixture
also passes, and its corrupted CRC is rejected. `git diff --check` passes.
PASS: [GitHub Actions 43762944735](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43762944735)
at 3a71e948185eb4c5453bdc220f529cb32dfa5bb5: public Normal, isolated Simulator,
offline core and release packaging. Downloaded artifact digest matches GitHub:
`5a9d344ca4fae3c051cdfe186230f62896865c200d4b6b669b695e3cf9d52c74`.
Normal is 1,935,008 bytes, SHA-256
`21c2386b9c133ec1479dc21abe1fdfbc0c5b6348c175846e74b3f0f07dc97847`;
Simulator is 977,232 bytes. Both independent image identities match v0.4.5 and
their channels; Normal length/hash match the OTA manifest. ELF symbol inspection
confirms physical BLE/OTA/debug upload/certificate code only in Normal and
MockTransport only in Simulator. These are build/artifact checks, not execution
of the new interaction flow. No Linux visual/interactive pass is
claimed: the repository's desktop adapters require Windows/macOS. No new test
source or physical motor tests were added. Owner checks still needed: UNLOAD on
press/dial with pending or stale controls; confirmed vs failed/unconfirmed
feedback; modal input during underlying page transitions; SEND save failure,
pending-control handoff, close/reopen, missing Wi-Fi, upload failure/RETRY and
rebooted queue without a new disconnect; OTA preferences retention.

Publication follow-through: [v0.4.5 tag build 43766736376](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43766736376)
passed at merged source 2e71053471b92d6e314e55388ec58e7437362672. Downloaded
draft assets pass Normal manifest hash/length, Normal/Simulator identities and
USB/OTA Normal consistency; the tagged Normal hash matches the reviewed image
above. The ordinary release is published as latest. The public update feed and
version-pinned firmware download were verified as v0.4.5 with matching image
identity, size and hash. This is delivery evidence, not a physical OTA/unload trial.

November 14 follow-up: [changes and evidence](BUG_FIXES_2026_11_14.md).
Future cases (new test code still deferred): Settings debug access; visible hold
countdown, 6+ second blank/control holds, drift independent of tap rejection,
controller 0xFB/0xFC ACK/readback, cancellation and wake-only input; no action
on release after a debug hold; preserved refusal snapshot after disconnect;
optional-parameter rejection must not erase required polls; direct motor 5 vs
bulk echo 1, stale direct motor and immediate single-field unload/unknown state;
no automatic reload after RP, and weight writes/readbacks distinct from force.
Keep load refusal and Unload readable on 360/466/480 displays.

November 9 bug follow-up: [source audit](PROTOCOL_AUDIT_2026_11_09.md),
[implementation/validation record](BUG_FIXES_2026_11_09.md) and
[diagnostic gesture/report cases](DEBUG_REPORTS.md). New test source remains
deferred. Physical Twin and the unrecognized void-lock state remain unresolved;
no hardware or resistance tests were performed for these changes.

November 9 rest completion follow-up: [behavior and hardware evidence](REST_ALERT_2026_11_09.md).
The factory timer's ability to vibrate is owner-confirmed. New DRV2605/2605L
hardware driving remains unverified on this remote. Check 20-second automatic
stop, early acknowledgment, persistent Rest over across navigation, no hidden
unload control, wake/dim/sleep, no boot buzz, Off defaults/reset, saved choices,
no repeat for one rest, later-set rearming and temporary stale telemetry.
CONFIRMED (cloud build): PlatformIO 6.1.19 / espressif32 6.12.0 builds of
both guition_k5_release and guition_k5_offline pass for v0.4.2. CMake 3.31.6
builds the offline core and packet inspector; the existing source-reported
base-weight fixture passes framing/CRC inspection. package_release.py validates
v0.4.2 (Normal image 1,899,568 bytes) and packages OTA, USB and relink assets.
git diff --check passes. The baseline offline core at 3bf2b14 was also built.
These checks do not execute the UI or prove motor output. Linux has no supported
graphical simulator adapter in this repository; no visual pass is claimed.
No new automated test code was added, per the master brief.

November 9 live-update follow-up cases (new test code deferred): signed GitHub
asset redirects longer than the IDF default send buffer; delayed DNS/TLS;
connection reset with manual retry; startup Wi-Fi before BLE discovery;
manual device selection while discovery is paused; stored bootstrap
profile loaded from a public update without embedding private fields.

November 9 update cases (new test code remains deferred): check once per boot;
offline boot and unavailable SNTP/TLS; interruption by manual BLE connection;
duplicate install taps; no install/load from startup, a reconnect or a check alone;
installed/same/older/newer versions; bad/truncated/oversized manifests; wrong board,
channel, layout and image descriptor; HTTP redirects to unapproved/non-HTTPS hosts;
size/hash mismatches; download cancellation/timeouts; power loss during flash;
startup rollback; Normal A/B to Simulator return; local bootstrap/preferences
survival; no radio/update symbols in Simulator; and no secrets in public artifacts.
Source checks/builds cannot establish actual OTA or physical interoperability.

October 24 idle-force regression (new test code deferred): with confirmed base
10 LB, fresh idle phase 0 and force samples 0–36 tenths LB at a stationary
148 mm cable, Live must retain 10. Active phases 1–3 use fresh reported force;
missing/stale phase or force returns to confirmed base, never a fabricated force.
Unknown core state remains unavailable. Check modifiers during active reps,
half-KG formatting, delayed set-close, unchanged drops/raw telemetry, first rep,
set summary and rest timer. The user confirmed the preceding rest-timer update.

October 24: [units, guarded loading and telemetry](DROPSETS_UNITS_2026_10_24.md)
supersedes the older unvalidated-training status below. The user confirmed
%/LB controls, remote-tap loading, return to Main, post-set counts and first-rep
tracking. Stairs/RP reductions, KG and Auto Load triggering remain unverified.
No new test code was created. Future regression cases: subscription masks must
not silently change the expected stream layout; sparse summaries cannot freshen
motion; duplicate set closes do not restart rest; no timer from load toggles,
settings, reconnect or old summaries; stale-state cancellation; new-set motion
ends rest; reported elapsed time takes precedence; alert fires once and wakes
the display; local elapsed time never becomes machine-confirmed state.

October 18: see [physical discovery observations](LIVE_CONNECTION_2026_10_18.md).
Discovery, GATT subscriptions, acceptance after user approval and read-only state
are physically established with the local diagnostic profile at ten feet.
Training is unvalidated. Queue the cases below; no new test code was added.

October 17 Normal-mode work: [implementation and blockers](NORMAL_MODE_2026_10_17.md).
New test code remains deferred. Queue future cases for zero/one/multiple completed
scans, late power-on, accepted-only persistence, failed GATT/approval, stale scan
refresh during a hold, forget/persistence failure, retained offline devices,
mode gesture versus page swipes, simulator-to-normal reboot selection, per-image
link isolation, notification overflow/generation, truncated read groups and
unknown Twin/readiness. Physical training is not validated.
Also cover immediate invalidation on unknown units/motor/workout mode, unload
from that state, and monotonic report revisions after valid state returns.

October 17: see [feedback changes and pending regression cases](UI_FIXES_2026_10_17.md).
Existing pinned Ryan packet and SDK-consistency validators were rerun successfully.
New UI/state tests remain deferred; builds and render checks are not interaction
or physical-resistance validation.

The explicit instruction under REQUIRED TEST CASES defers new test code for now.
Existing upstream validators are run; no artificial coverage or passing CTest
placeholder is supplied. This is not a tested production control implementation.

## October 18 connection checks

Normal, isolated Simulator and Windows native builds pass after the asynchronous
connection change; existing offline renders open Devices/Main/Twin successfully.
Physical captures reproduce error 574 and show a later successful radio link/MTU
negotiation. They exposed cached-only GATT enumeration; the correction now reaches
three services, three subscriptions, acceptance and live state. No autonomous
resistance command or new test code was used. Queue fresh empty GATT caches, bounded 574-only
retries, cancellation during connect/backoff/MTU, late callbacks before a different
target, no pre-subscription bootstrap, failed service/subscription feedback, and
missed-scan availability grace versus unchanged machine-state freshness. Also
cover missing CCCDs, independent three-channel fragmentation, bootstrap timing
after blocking discovery/write, approved versus refused/expired sessions, 0/1
motor readbacks versus 4/5 commands, core-read prioritization, and module tags
that must not be interpreted as installed app firmware. Local identity/profile
comparison is a physical diagnostic, not a validated provisioning algorithm.

Unloaded LB write cases queued: explicit input only; fresh zero/zero standalone
reports; nonzero/stale/unknown peer fields; optional paired field omitted versus
previously present then stale; lock/OTA/unit/motor changes between inputs; modifier
freshness; no settings or load on reconnect; accepted write without readback;
readback mismatch/clamping; rapid explicit edits; unload priority; KG and modifier
edits rejected. Auto Load enabled versus actually armed must remain distinct.

## Existing upstream results

- PASS: Ryan `tools/check_packets.py`: 203 SDK packets, generated weight range
  5–230, all 593 pinned modifier packets. This checks bytes, not safe resistance.
- PASS: Ryan `tools/check_upstream.py <SDK checkout>`: exact pinned commit,
  identifiers, customized identity, init/STOP/SETUP/GO, weight table and guided
  capture consistency. Starter pins the current audited SDK commit.
- PASS: Node SDK `vitest run src/voltra/protocol/__tests__ --maxWorkers=2`:
  **229 tests in 10 files**, Vitest 4.1.11, Node 24.19.0. No radio was used.
  The bundled runtime lacked npm, so pnpm 11.19.0 resolved upstream package
  ranges with scripts disabled in the external checkout; this was not a frozen
  npm-lock install. The resulting pnpm lock remains with that checkout.

Android tests were inspected but not executed; no Android/JDK toolchain was
installed. HA was inspected rather than installed as a runtime service. SDK
hardware scripts and automated motor tests were not run.

Build/render results are recorded in BASELINE.md after execution.

## Five-screen update

Native build succeeds with the expanded controller, mock and shared UI.
Offscreen LVGL renders exercise all five screens and both dropset editors.
These are layout checks, not passing interaction or safety tests.

Windows Computer Use was attempted for native interaction checks. Window
capture failed twice with `SetIsBorderRequired failed: No such interface
supported (0x80004002)`. The custom LVGL surface exposes only a pane to UI
Automation, so accessibility does not provide an alternative interaction oracle.
New test code remains deferred under the original instruction.

Manual checks still required: saved-device list across relaunch, five-second
availability changes/offline no-op, short/long-press exclusivity, all screen
navigation, mode value transfer, unit/envelope edge cases, zero selection,
Auto Load cancellation and ignored responses, duplicate/gapped reps, RP hold
rearming, confirmed drop counts, and stale/disconnect/unload cancellation.
F7–F11 provide offline inputs for these checks; no physical motor is involved.

## Future regression matrix (pending, not implemented)

| Area | Required cases |
| --- | --- |
| Framing | Known captured vectors; both CRC failures; ordinary/extended length; every fragment split; coalesced frames; noise; truncation; buffer bound; partial expiry |
| Parameters | Signed eccentric; unknown widths/IDs; bad count; invalid battery; no partial merge; actual read/ACK envelopes |
| State | Write success never confirms; wrong/late readback; stale individual fields; disconnect/reconnect generation; failed/time-out intent |
| Safety | Every missing/unknown/false load guard; boot/reconnect/wake/mode/Twin never loads; no force path; Auto Load/Unload writes absent |
| Unload | Available while loaded/pending/stale/error; priority queue; confirm physical report; disconnected feedback |
| Limits | Verified effective maximum; reduce overload modifiers first while increasing base; stop at valid envelope; unknown max disables control |
| Capabilities | Old/stable/beta/unknown reports; unsupported options hidden; no guessed MTN version cutoff; read-only feature discovery |
| Twin | Current role vs historical role; peer stale/lost/incompatible; combined max/routing; no half/double/duplicate assumption |
| UI/hardware | 360/466/480 layout; circle margins; touch targets; positive clockwise events; press debounce; board capability rejection |
| Sessions | Slow/rejected/missing ACK; multiple independent devices; cross-device notification rejection; reconnect cancels intent |
| Telemetry | Reps/sets/force/velocity/position units; ordering; boundaries; separate peer freshness; malformed packets |

Reuse simulator mocks; introduce a deterministic MockClock for lifecycle tests.
CI and simulator targets must not link a physical BLE backend. Protocol/safety/
state/Twin/reconnect/hardware bugs need regression cases as implementation proceeds.

## October 11 review

Windows build and responsive offscreen renders pass (see BASELINE.md). These
are not interaction/safety test results. No new test code was added. Add future
coverage for mixed-unit envelope limits, preset unit migration/readback mismatch,
queued explicit edits versus unsolicited reports, and wake-input consumption.

Follow-up regression cases queued (new test code still deferred): continuous
rotary input beyond two seconds must not starve mock reports; every theme at
and below maximum; confirmed vs ignored maximum requests; modifier amount tap
with pending edits; no X controls; exact preset replacement at high base,
70% IC + 20% EO / max 200 -> base 105; mixed absolute/percentage recipes;
unsupported recipe rejection; knob input during preset confirmation; mismatch
must not activate dropsets or the preset indicator. No new automated pass claimed.

## October 18 single-device follow-up

No new test code was created. Existing Ryan packet and pinned SDK validators
passed again (203 SDK packets / 593 modifier packets); this is reference-data
validation, not proof of new transaction behavior or physical readiness.

Future regression cases: modern percentage values overriding misleading legacy
zeros; uint8 inverse selector and sport-mode field 21007; settings ACK mismatch;
ten-second transaction vs three-second per-write timeout; continued critical polls
during rapid explicit input; units/interpretation/maximum/Twin/motor changes during
a batch; unload cancels all queued steps; expired intent is never replayed;
unrelated reports cannot erase a failure; half-KG round trips and wire envelopes;
disabled modifiers write zero while enabled values stay positive; no redundant
style/alignment invalidation or hidden scale-label rewrite on idle frames;
sparse rep events are not motion freshness; no synthetic physical rest timer.

## October 24 feedback

Existing packet/upstream validators pass again; no new test code was created.
Future cases: one automatic idle-to-Weight-Training request per accepted connection;
mode already selected; refusal/timeout/stale/loaded/armed/other-workout context;
manual exit is not reversed; unload cancels setup; no load command during setup.
One-modifier changes must never write the other parameter; all reduction/increase
orderings must remain within the envelope. Repeated taps use latest requested
state; coalesced edits preserve both independent fields. Continuous rotary input
must retain fresh reports while waiting for the 100 ms quiet interval. Requested
weight is labelled, discarded on failure/disconnect and never authorizes loading.
Missing standalone Twin status must not create a Twin banner. Synthetic pair
limits are 500 LB / 226 KG; ordinary 2.x fallback stays 230 LB / 104 KG.
Disabled physical reports contain zero, so UI editor recall must retain the last
confirmed positive Magic/EO amounts independently per device. Future cases:
60% IC -> disable -> reopen at 60%; EO unaffected; report-driven changes update
memory; no writes on editor entry/reconnect; lower headroom clamps the draft;
absolute-unit conversion; unsupported unit interpretations are not restored;
forget/reset clears the local editor memory. This memory is session-local and
does not override machine-confirmed enabled state or preset matching.

The user confirmed IC disable/reopen retains its value. Further October 24 cases
are queued for progressive rotary input: start/reversal/300 ms pause use fine
steps; noisy batches cannot jump directly from level 0 to level 3; slowdown
brakes immediately; 73 -> 75 -> 80 clockwise and 73 -> 70 counterclockwise at
the 5-unit level; 0.5 KG precision, whole-unit coarse grids, signed stairs across
zero, pending-intent accumulation and final maximum/minimum clamps. No new
automated test pass is claimed for these cases.

Mountain cases: fresh valid 0x556F and accessory-display reports; omitted/stale/
invalid values; both inverse/Mountain switches set; clear prior special curve
before enabling the new one; readback required at every step; EO unchanged;
changing the device's display interpretation cancels percentage edits. Dropset
preview/save on an unsupported connection must not arm it, write resistance,
activate a preset or imply automatic execution is available.

Later October 24 cases (new test code still deferred): arm Stairs while loaded
at rest with completed set 10 / rep 2, observe active moving set 11 / rep 0,
then completed rep 1 and request exactly one reduction. Repeat without unloading;
reject a new set without its observed start, a gap/reset within the same set,
and a completed rep while a prior drop is unconfirmed. A fresh new-set start
may precede weight readback; observing it must neither stop drops nor confirm
the write. Duplicate completions do not drop twice. Confirm RP requires actual
movement to rearm and retains its quota across weight-induced set boundaries.
For rotary input, the later policy supersedes immediate return to fine steps:
slowing retains a 5-unit grid until pause/reversal/touch reset. Check the gentle
two-detent cadence, batched clicks, both directions, negative stairs, KG ticks,
pending previews and off-grid device limits. No delayed write on release.

Next captured ordering to cover: set 12 completed rep 1 -> request 8 LB ->
set-close while workout remains active -> base=8 -> set 13 moving rep 0 ->
settled readback -> set 13 completed rep 1 -> request 6 LB. Also cover readback
before the new start, readback with the next completed rep in one controller
tick, failed/mismatched/late confirmation, and no quota reset for RP segments.

Latest October 24 cases (new test code deferred): Live remains 10 LB while
sensor force changes 9/10/11, and while force or rep-phase reports are missing
but core state stays fresh. Stale/disconnected core state still shows unknown.
Stairs 10 -> 8 -> 6 -> 5 must confirm the final 5, then issue no more writes;
NEXT and MIN labels must agree. RP floors, signed ascending Stairs, manual
rotary limits, below-floor incoming state, invalid/unknown minimum, presets with
insufficient base headroom, and explicit mock Twin/native-KG minima are queued.
Never reinterpret an unexpected mismatch as confirmation or automatic unloading.

Dropset presentation/recovery cases queued (new test code still deferred):
15/13/11/9 LB with one reported rep per machine segment displays one logical
set / four reps; multiple reps within each RP segment accumulate into one set;
duplicate and late completions do not add twice; continuation at 5 LB keeps
counting with no further automation writes. Confirmed unload preserves the last
total and starts rest; a second loaded dropset increments the logical set once.
Weight-induced AA85 closes while active must not start rest. Disable/stop drops
mid-set without discarding existing totals. Missing/stale telemetry, revisited
set IDs, disconnect/reconnect and ordinary training outside drops must not
invent counts. Final minimum readback must settle before exhaustion; at the
floor/quota, later rep gaps/motion loss must not restart automation. Recovery
requires fresh non-training mode plus an equally recent unloaded motor; stale,
unknown or loaded state must retain unload access. Preserve the recovery reason,
clear pending intent, and never replay resistance commands after reconnect.
Manual adjustment after exhaustion disables the recipe; it must not silently
resume drops or leave an active NEXT preview for a dormant executor.

RP/rest cases queued (new test code deferred): status 3 after returning the cable
ends a grouped set without unload; next moving zero-rep start increments SET and
resets REP/RP quota while retaining the old summary during rest. Status 2 plus
AA85 from a base edit does not end it. Preserve failed-unload cancellation across
rest. A 500-2000 ms motion gap discards partial hold time and requires new motion;
a longer loss stops automation. An unchanged extended hold cannot consume two
drops, and a pending drop cannot trigger another. Two confirmed RP holds produce
15 -> 7 -> 5 LB; the floor produces no further write. Strict sequence guards
still apply to Stairs. Verify only base is sent with percent or absolute Magic/EO,
including delayed/mismatched accessory reports. Loaded drops skip rotary quiet
time but preserve readback/deadline checks. Cue occurs once per settled target,
never on GATT success, mismatched target, timeout or reconnect. It must not imply
measured force has already changed. Hardware sound/vibration remains unverified.

October 31 motor-report cases queued (new test code deferred): preserve raw
16009=0x0100 while interpreting its mode as unloaded; read fresh base/modifiers
without a false connection error. Check 0x0101 / 0x0105 loaded, 0x0104 unloaded
command echo, and prefixed Auto Load states against the same additional status
guards as their plain values. Unknown upper bits (e.g. 0x0200), unknown low-byte
modes, and stale reports must not establish readiness or acknowledge a command.
Load/unload acknowledgements use the normalized mode but all outgoing command
values remain unchanged. Reconnect, mode entry, base edits and decoder recovery
must never issue a load. A low-byte-only match cannot bypass locks, cable zero,
activation, standalone context, or current explicit user intent.

Pending-update cases queued (new test code deferred): fresh OTA_STATUS 0 and
the captured Black update-ready value 0x0105 allow ordinary control only when
all other guards pass. Missing/stale status and unrecognized nonzero values
(including nearby 0x0104/0x0106) must block new intents and cancel a pending
transaction without retry, replay, load, or rollback. Update-block feedback
must identify update status rather than require unload. An update-available
report alone must not create a load or settings command. Preserve explicit
unload access regardless of update state. Verify unloaded dial changes and
mode entry with an update waiting, without accepting/installing the update.

October 31 Auto Load/drops and touch cases queued (new test code deferred):
preserve an enabled Stairs/RP recipe through 35/11 -> 12 -> 13 -> 14, with no
drop during waiting/countdown/engagement. At 14 require fresh motion and the
normal completed-rep/qualified-hold trigger, write base only, preserve Magic/EO,
then await settled readback. Verify multiple drops, floor/quota, cancel/unload,
stale or unknown Auto Load status, unsupported motor variants, locks and Twin
loss; never reassert load or retry a drop. A new explicit Auto Load must clear
old hold/rep/quota state while retaining the recipe. Disconnect must still disarm.
During countdown the tile shows recipe values in amber, can be disabled by tap,
and must not falsely claim OFF. An unavailable new recipe still cannot be armed.
At 360 px, control taps drifting 10-18 px remain taps; >18 px cancels. Device/
preset pickers retain >9 px cancellation and >45 px horizontal paging. Verify
long press issues one action, release does not toggle, wake tap only wakes,
adjacent modifier targets stay separate, and caption/circle gap taps register.
Touch diagnostics now distinguish recognized actions from drag cancellation;
responsiveness on the physical panel remains a human observation, not a build
or packet-validator result.

Buffered-touch cases queued (new test code deferred): a complete 80 ms tap
inside a 150-250 ms transport stall delivers down then up and exactly one action;
retain intermediate swipe excursions even if the finger returns to its start.
Stationary long holds remain holds, not repeated taps; queued short taps never
become long presses. A screen change discards old input and a held release does
not act on the new screen. Stale (>350 ms) input, overflow, invalid coordinates
and an I2C error while pressed cancel without a CLICKED action; fresh input only
resumes after a real release. Check wake-only touches, both picker swipe guards,
Apply refusal feedback, unload during pending BLE work, and initialization
failure if the input task cannot start. Verify single taps on Drops Apply,
Magic/EO, device selection and presets on the physical panel. The desktop
window screenshot helper timed out during the pre-change comparison; no native
interactive pass is claimed from that attempt.

RP countdown/Auto Unload cases queued (new test code deferred): no bar before
250 ms; 1 s hold shows 75% at 250 ms, half at 500 ms; duration scales to 2..9 s.
Movement/drift resets elapsed time; stale motion, disconnect, rest/unload,
pending target, quota/floor, 0%/0 drops and Stairs hide the bar. Continuous hold
triggers once; new movement rearms. A bar reaching zero cannot send or confirm
a command. Check 360/466/480 round bounds and the primary unload touch target.
Unknown/off Auto Unload preserves max 9; 2 -> max 1, 3 -> max 2, 10 -> max 9;
1 -> no valid RP pause. Reject out-of-range/stale parameter reports. Editor,
Apply, presets, toggles and externally shortened timers share the same policy;
no automatic recipe shortening, rearm, resistance load or Auto Unload write.
Correlate raw parameter 21525 against Black's displayed holding-time setting.

Later October 31 cases queued (new test code deferred): Stairs and RP restore
the original base only at actual workout rest/unload, after settled unloaded
state; segment closes must not restore. Unload once, base once, never load.
Verify next set's base and grouped counters, unchanged accessories, floor/quota,
external unload, failed unload, stale state, different units/device, disconnect,
manual override, incompatible envelope and cancellation/timeout without replay.
Loaded amount writes must preserve base and the other accessory, accept only
current/in-flight/requested amounts, and never reassert motor mode. Reject curve
and unit switching while loaded, mode 5, unknown locks/Twin and stale controls.
Persist one global rest-alert setting across device changes, reboot and mode
switch; ±detents advance 5 s on-grid, bounded at off and 59:55.

Wi-Fi cases: protected AP, captive/manual page, hidden/duplicate/long/escaped
SSIDs, open/WPA2 networks, bad password and join timeout, candidate-save failure,
power loss before/after save, saved reconnect, AP expiry, browser partial bodies,
duplicate/oversized fields, invalid token, non-AP client refusal and no secret
logs. Wi-Fi/HTTP must stop during VOLTRA connection, without a resistance action.
Physical phone join and network persistence require human observation. Review
360/466/480 setup/Apply layouts. Diagnose occasional Apply with press/release,
cancel reason and app refusal rather than claiming every lost touch is fixed.

November 2 Wi-Fi regression cases queued (new test code remains deferred):
IPv4 and ::ffff:IPv4 peer/local sockets, truncated sockaddr, native IPv6,
getpeername/getsockname failure and non-AP interface/subnet. Root must return
HTML rather than {}, and a refused page must explain how to join setup Wi-Fi.
Captive probe redirect and direct 192.168.4.1 must both reach the form. The
open AP has no Wi-Fi password; each setup gets a four-digit PIN shown only on
the remote, not prefilled in HTML or logged. Reject absent/wrong/expired PIN,
duplicate fields and stale token without changing stored credentials. Verify
phone join, correct-PIN submission, failed home-network password, success/save,
reboot persistence and timeouts. Check 360/466/480 remote PIN instructions.

Later November 2 supersedes PIN cases: no PIN/password on setup AP/page.
One Settings -> Wi-Fi tap starts the portal. Back/Stop/expiry close it; restart
is explicit. A select control lists unique scanned SSIDs with a manual hidden
network fallback. Refresh reports scanning/failure/empty results and retains
the selection without rebuilding unchanged options. Check duplicate/escaped
names, failed initial scan, refresh while scanning, expired token, and submission
during refresh: joining must wait, with no false "Connected and saved" from a
previous saved-network report. No phone credential harvesting or mock physical
scan pass. Native/browser previews use synthetic names and never join a network.
