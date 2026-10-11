# Protocol evidence and implementation limits

Source revisions are fixed in [UPSTREAM_AUDIT.md](UPSTREAM_AUDIT.md).
Physical validation in this project: **none**.

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

## Before physical read-only work

Implement response-validated bootstrap, read envelope decoding, channel-specific
assembly, malformed/replay diagnostics, session generation and freshness checks.
Audit every bootstrap write for settings side effects. Record device/module
versions and reported capabilities without persisting unnecessary identifiers.
No resistance testing belongs in this stage.


## September 26 unit increments review

CONFIRMED (official documentation scope): single VOLTRA I base resistance is
adjustable in 1 lb increments:
https://help.beyond-power.com/en/articles/14167505-what-is-voltra-i
The official Japanese article describes approximately 0.5 kg increments:
https://jp.beyond-power.com/blogs/news/meet-voltra-i-the-200lb-portable-cable-trainer-powerhouse
This supports fractional KG display, but does not establish exact KG protocol
quantization or every firmware's modifier behavior.

CONFIRMED (Cortex command documentation): integer percentages are a distinct
force interpretation. Absolute versus percentage commands change that
interpretation; display-unit preferences alone do not convert command input:
https://cortex.beyond-power.com/docs/cortex/training-control
Consequently a fractional arithmetic equivalent of an integer percentage is
not evidence that an absolute fractional-LB command is supported. Our local
Magic view retains the percentage and shows a whole-pound equivalent rounded
down with ~ when inexact. Switching views is still not a device setting write.

Reference SDK inspected outside this tree at /tmp/voltra-unit-sdk, commit
600028a33e664c8258e801047f0181a6f71f0d4b. Its client setWeight/setChains interfaces
are expressed in lbs; this targeted inspection did not establish Twin KG steps.
No upstream validators were run in this review; no physical commands were sent.


September 27 supersession: the display-only conversion described above has
been removed under the latest user requirement. The mock accepts an independent
value/unit flag per modifier, including mixed absolute/percentage settings.
This is an offline product behavior, not a verified wire model; the official
percentage-force switch may have different coupling on physical devices.
TrainingReport.modifierUnitsSelectable defaults false until established.


## October 4 — units, shared modifier interpretation and dropsets

CONFIRMED (official documentation, not physical compatibility): Cortex describes
`set-weight-unit` as a global device display preference, distinct from command
input conversion. `set-chains` and `set-eccentric` select the device percentage
force interpretation with `--percent`; absolute commands switch it back.
https://cortex.beyond-power.com/docs/cortex/training-control

CONFIRMED (catalog only): Android reference commit
`da514d2db1a16ebb4ded3156bc8fd200bed1e6eb`,
`app/src/main/assets/voltra/paramInfo.csv` row FITNESS_UNITS_WEIGHT identifies
register 20590 (0x506E), uint32 / four bytes, described as lbs/kg. This establishes
that the protocol catalog has a unit preference; it does not establish enum
values, notification/readback cadence, or native KG command quantization. Source:
https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv
The remote UI now consumes TrainingReport.unit with no unit-toggle command or
local conversion. Physical TrainingReport decoding remains unimplemented and
must populate this from verified reports before physical control is enabled.

RP: Cortex documents Drop Set configuration in cloud session templates, but
this is not proof of a single native BLE command that executes the requested
pause-triggered sequence on the device:
https://cortex.beyond-power.com/docs/cortex/sessions
The public SDK catalog at `1d47cb29a90912284c383fabb684981b508f5f54`
(`src/voltra/protocol/data/protocol-data.generated.ts`, decoded outside this
source tree) and the Android catalog above did not expose a verified native
RP configuration matching drops, percentage, and hold time in this inspection.
Absence in these catalogs does not prove the device lacks such a facility.
UNVERIFIED: native RP execution, hold telemetry mapping, and physical live writes.

The offline simulator therefore continues controller-managed RP: movement arms
one hold, a continuous qualifying hold produces one reduction, then movement
must rearm it. Two requested drops produce two reductions/three sets; no further
reductions occur. Stairs is controller-managed on distinct completed-rep events,
with signed native-unit steps. Neither runs without explicit local activation,
fresh valid telemetry, confirmed load and live-weight capability. Reports of
per-rep average velocity must not be treated as live hold observations. The
physical adapter stays disabled; no motor operations were performed.

Twin: official guidance says adjusting one paired unit affects both, but does
not establish BLE routing or automatic limit doubling:
https://help.beyond-power.com/en/articles/10124771-how-do-i-pair-two-voltra-i-devices-in-twin-mode
B/D are explicitly configured as a shared synthetic 460 LB session. The linked
circles are this project's compact visualization, not a verified copy of the
current VOLTRA UI. Catalog/source snapshots inspected under /tmp only; no
reference repositories added here and no upstream validators run this turn.
