# November 20: Twin readback, connection handling and live load

Later v0.4.10 corrections supersede the separate member selection and Live
runtime-weight source below: [paired navigation/presentation](TWIN_PRESENTATION_2026_11_20.md)
and [device-reported target resistance](LIVE_TARGET_SOURCE_2026_11_20.md).

Baseline: published v0.4.8; main c0f3f95. The owner reports Black paired with Red,
missing Twin indication, a failed first connection, halved weights and refused
adjustment. They also request device-reported live load during Magic/eccentric
reps and clarify that LB display increments are whole pounds.

## What the reports and photographs establish

The two newest stored reports by receipt time are 4ac675b8 (November 20,
14:13:08 UTC, v0.4.8) and 5a65d590 (November 18, 14:38:38 UTC, v0.4.7).
Only one November 20 report is present; do not identify the older report as
today's first connection failure.

CONFIRMED in 4ac675b8: the BLE link connects at uptime 27752 ms with MTU 247,
VOLTRA accepts the session at 29057 ms, and core state arrives at 29308 ms.
Motor 16009=40963/0xA003 is rejected by the old decoder, which discards the
TrainingReport. A later dial action therefore fails before a settings write.
The relevant fresh fields include workout1, base 65, unit LB, local max 230,
mount 2, safety 6, child lock 0, OTA 0, Auto Load status 14, coworker 20844=2 and
remote word 21023=33572609/0x02004701. Peer identifier 21605 is absent.

CONFIRMED (owner/photos): the VOLTRA's screen says Total 130 lb while the remote
shows 65. Its absolute modifier values are also per-unit: remote IC 75/EO 25
versus VOLTRA 150/50. These observations establish the display mismatch for this
pair; they do not decode current host/follower role, peer readiness or motor flags.

## New source evidence and strict boundary

Reviewed [stevenpoint/voltra_remote watch206](https://github.com/stevenpoint/voltra_remote/tree/1c14860b53b805a789b8bed2095e63443e355d7d)
at 1c14860b53b805a789b8bed2095e63443e355d7d, outside this application.
Its PROTOCOL.md Twin section, protocol parser, UI and existing native fixtures
document a read-only 0xA7 query with an empty payload:

- Full push: 22 bytes, `battery 01 state own[6] peer[6] peerBattery tail[6]`.
- Full successful reply: 23 bytes, the same payload preceded by 00.
- States 00 alone, 11 joining, 12 paired. Addresses are in printed order.
- The tail has no established role/lock meaning. The peer name query 0xA9/01
  lacks a published response decoder; peer names are resolved by exact address
  against existing saved/scanned IDs instead.

The same source explicitly confirms per-unit wire setting values with combined
display ×2, including absolute modifiers; percentages remain percentages. Its
one-pound wire step corresponds to two pounds combined. No fractional wire or
one-pound combined write is inferred here.

CONFIRMED within the reviewed source scope: host control is different from
follower control. Captured host load/unload are 0101/0100, and its Auto Load mode
is 2003 with completed status 15. No reviewed source defines Black's A003 or its
additional 8000 flag. Both members can report paired and the same remote word;
that word and connection status 2 cannot prove current role. The source's host
guess based on a nonzero high byte is deliberately not adopted.
[Official Cortex Twin documentation](https://cortex.beyond-power.com/docs/cortex/twin)
supports host-owned control but does not publish these raw BLE flags.

Marvaments 67f7278 ports the same Twin constants/parser and supplies no independent
missing role/lock mapping. Current Kirby remains 8e1ea1c; Android da514d2, SDK 79f6c55
and Omar's implementation provide no definition of A003 in the reviewed sources.

## Implementation

Topology and setting readback have independent validity and clocks from motor/
control state. An unknown motor cannot erase a known pair or manufacture a valid
control report. A7 is validated for full shape, marker/state, address consistency
and own-address match against the selected transport endpoint. Addresses stay in
RAM and never enter reports. Pair identity must precede fresh base/unit readback
before combined values are displayed. Stale, conflicting or invalid topology
cannot be used to double a reading or become standalone authorization.

Devices uses overlapping named circles, deduplicated only by verified peer IDs.
Each member retains its own selection target. A peer that stops advertising is
not silently treated as absent from a connected pair. After disconnect the RAM
relationship is marked historical/unknown, not live. A remembered device can be
selected explicitly by saved endpoint even without a current advertisement;
automatic connection still requires a completed live scan.

Current Twin status remains insufficient to enable resistance changes. Paired
limits/minimums remain unknown; no maximum is doubled, no guessed host selected,
no unknown motor masked, and no load/settings/Auto Load command is enabled.
An unverified pair Unload route reports Use VOLTRA to unload Twin rather than
sending a single-unit command that could affect just one member. Standalone
Unload remains unconditional. Diagnostics include bounded state/shape/match/age
information, not addresses. Failed dial and connection reasons survive retries.

The generic BLE adapter also waits for both cancellation and the old connection
handle to drain before client reuse, with the existing 11-second connect+MTU
bound. NimBLE 2.3.6 invokes its disconnect callback before clearing that handle.
This is a source-backed quick-reselection race correction, not proof of the
unreceived first-failure cause. Existing bounded link retries are unchanged.

## Live load

The owner's November 20 request supersedes the older base-only Live policy.
Register 16003/0x3E83 is documented in Android's parameter catalog as signed
whole-pound runtime wire weight and decoded as forceLb by the Android client.
It is distinct from the selected base and the AA81 tenths-of-pound sensor stream.
The optional register is probed alone before being added to supported loaded
monitor reads. Its observation timestamp stays independent; UI data older than
500 ms is unavailable, not a reason to fall back to selected/calculated weight.

Live shows this direct whole-LB reading with BASE separately labeled. Only the
existing unit conversion is applied for KG. No percentage, eccentric phase,
or predicted rounding calculation supplies the large number. Paired live-force
scope is still unverified, so per-unit actual force is not doubled into a total.
Equality between 16003 and the VOLTRA's changing primary LCD label still requires
a synchronized physical comparison; the source name alone does not prove it.

## Verification and next physical evidence

No physical BLE commands or new automated test source. Existing upstream native
validators, packet inspection, core/firmware builds, artifacts and actual LVGL
render results are recorded in TESTING.md. A source/fixture pass is not a device
compatibility pass.

Remaining owner observations: fresh A7 topology and raw motor values while both
units are unloaded, Black/Red name matching, connection/reselection behavior,
unknown/stale pair presentation, and a side-by-side standalone Live comparison
with Magic/eccentric. Current host/follower and peer safety/combined-limit evidence
remain required before enabling physical Twin resistance control.
