# Existing Twin pairs

October 17 correction: 2.x is 230 LB per unit / 460 LB combined, as confirmed
by the user. The older 400 LB example is not universal. Effective combined
reports take precedence over the unit-aware [firmware fallback](WEIGHT_LIMITS.md).
The simulator uses 460 LB / nominal 208 KG and one selected synthetic session;
this does not establish physical routing or command encoding.

Pairing, unpairing, role changes, automatic reconnect configuration and peer
configuration writes are excluded. Independent VOLTRA sessions are not Twin.

## Evidence

CONFIRMED (Android parameter catalog at da514d2):

| Decimal ID | Catalog meaning | Current interpretation |
| --- | --- | --- |
| 21023 / 0x521F | Remote coworker status: connection, battery, lock | Raw uint32 only; bit packing unknown |
| 20844 | COWORKER_CONN_ST | Enum unknown |
| 21560 | LAST_COWORKER_ROLE: 0 none, 1 main, 2 follower | **Historical** role; not proof of current role |
| 21559 / 21561 | Last coworker address/name | Avoid persistence; not proof of an active pair |
| 21605 | Paired EP device | Relationship evidence candidate; not live readiness |
| 21569 | Slave Auto Load status | Read-only; enum unknown |
| 21437 | Peer speed, int16 mm/s | Catalog evidence; capture validation pending |
| 21436 | Peer cable length, uint16 mm | Catalog evidence; capture validation pending |
| 21435 | Peer force, uint16, 0.1 lb | Catalog evidence; capture validation pending |
| 21427 / 21428 / 21429 | Main/sub/total force, uint16 | Units listed '-' — do not assume scale |

CONFIRMED (public static-analysis report): Twin-related master/slave addresses,
model/firmware compatibility checks, pairing failures/timeouts, lock validation
and coworker summaries are described. These are source-level clues; referenced
private binary/capture artifacts were not independently inspected here.

CONFIRMED (Android notes): 0x521F=0 occurs alongside mode metadata in a reported
capture. That value alone is not a verified single-device/Twin-state decoder.

## Working hypothesis and unresolved questions

INFERRED (user's preferred model, not yet corroborated): send a combined user-
facing resistance to the appropriate connected VOLTRA, likely the master, and
let the pair coordinate internally. Do not halve values, duplicate commands,
assume two BLE links, or double the limit.

UNVERIFIED: current-role detection, live connection/status enum, peer readiness,
peer lock/battery bit layout, separate freshness, command routing, total-weight
confirmation, failure behavior, and an explicit combined maximum. No reviewed
capture establishes the example 300 lb combined write or its distribution.

October 18: the user confirmed standalone/unloaded while 20844 and 21023 both
reported zero. The limited unloaded-LB profile recognizes that fresh zero/zero
combination; all nonzero/unknown/stale current values block it. Optional paired
endpoint 21605 was omitted by this firmware. If reported elsewhere, its fresh
absence is additionally required. Historical role is never substituted.

This is CONFIRMED for the observed standalone state, INFERRED as a narrow profile;
transitions/lost-peer and other firmware remain UNVERIFIED. Physical loading stays
disabled. No active-Twin roles, synthesized peer readiness, combined limits or
command distribution are implemented. This does not establish Twin support.

## Validation needed

After single-device read-only connection is validated, obtain human-provided
observations of an already-paired healthy pair, both roles, and a disconnected
peer while unloaded. Compare reports with device screens, module/model versions,
and displayed total resistance. Do not alter pairing from the remote.

Before any Twin load support, establish maximum/routing semantics with evidence,
cover stale/unknown/incompatible/lost-peer cases in mocks, then require explicit
human initiation for physical resistance testing. Peer freshness must be
independent from unrelated local telemetry. Unknown required peer state blocks
load; unload remains a priority request without a readiness gate.
