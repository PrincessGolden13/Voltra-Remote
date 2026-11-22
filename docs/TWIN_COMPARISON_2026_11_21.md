# Native Twin comparison and control implementation — November 21–22, 2026

The owner requested a full comparison before another physical retest. The earlier
timeout releases left every physical Twin command blocked. That was an implementation
gap: retrying a connection alone could not make the pair usable.

## Sources checked at their current public heads

| Source | Commit | Relevant evidence |
| --- | --- | --- |
| [Albert knob](https://github.com/albertzhang86/voltra-remote/tree/c4f2884798941babd83788cf79609620e767bc85) | c4f2884 | Own-matched A7 pair, one controller, per-unit settings, reported limit, explicit unload, current address-type refresh. |
| [Albert Android](https://github.com/albertzhang86/android-voltra/tree/8f9388063d684f73cbc02831a339919951ac2b1c) | 8f93880 | Native pair owned by one ready controller; second member's presentation derives from that session, without an independent live safety query. |
| [Steven remote](https://github.com/stevenpoint/voltra_remote/tree/1c14860b53b805a789b8bed2095e63443e355d7d) | 1c14860 | Captured Twin0101 load/0100 unload, Auto2003/status15; connection retention and longer supervision. |
| [Ryan remote](https://github.com/RyanMarkoff-eaton/voltra-diy-remote/tree/dff6227ddbc9bb1767698898e166b1ff019ad18f) | dff6227 | Connects the freshly scanned peripheral, preserving its actual radio address type; no Twin implementation. |
| [Node SDK](https://github.com/HJewkes/voltra-node-sdk/tree/c76a523ecb4a5062f3aa60de1b4be98a152723f7) | c76a523 | No native Twin routing/role decoder. Its motor mask does not classify A003 or0100 safely. |
| [Dylan Android](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/tree/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb) | da514d2 | Parameter catalogue and independent protocol captures, not a working native Twin control path. |
| [Home Assistant](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant/tree/93b0819745e665ab4d35f4cc501d48c6296c81f7) | 93b0819 | No native Twin routing decoder. |

Other current remote heads/branches checked: Kirby/Philip8e1ea1c, Robert4afe8824,
Hagestedt8289454a, Ring20946aa7, Nano16405e01, Omar1ec6bded and Marv67f7278e.
No newer applicable native Twin implementation was found. Independent two-device
fan-out projects were excluded: duplicating commands does not control an existing
native pair. [Official Cortex Twin](https://cortex.beyond-power.com/docs/cortex/twin)
also exposes one primary session rather than an independent secondary live session.

## Concrete defects found in our implementation

1. The app persisted a radio `address/type` as logical identity. A newer advertisement
   for the same address with another type could coexist with the old entry; selection
   and first-map peer resolution could reuse the stale type. Albert refreshes it by
   canonical address, and Ryan uses the scanned peripheral itself.
2. Role/A7 expiry disconnected an already established primary before polling could
   process queued refreshes. This is particularly damaging when a paired controller
   stops advertising. TTL expiry is a readiness problem, not proof the link changed.
3. Physical probe promotion, context/readiness, command methods, report revision,
   load state and limits each independently required standalone. All Twin operation
   was impossible even with perfect status.
4. Removing those guards alone would have been wrong: settings writes used combined
   UI quantities directly as per-unit wire values, and motor writes always used5/4.
5. Standalone Auto completion14 and runtime packet layouts do not cover native Twin
   Auto completion15 or the captured two-channel B4 messages.
6. Generic command/sequence de-duplication also covered A7 observations. Reused
   status sequences could suppress a real pair change or freshness renewal. A7 now
   processes every complete valid observation, with bounded logging. Ordinary
   settings/authentication frames also require the expected source endpoint.

## Source-backed control profile

Current full own-matched A7 state12 with distinct second identity establishes the
primary route. State22 remains the bounded secondary profile inferred from the
owner's reports; no secondary receives a control write. Names, colors, scan order,
historical LAST_ROLE and motor high bits never decide the route.

Optional20844/21023 fields are not independently decoded primary-role/absence
proof. In particular their default zeros cannot override a fresh full paired A7.
Standalone's existing relationship guards remain separate. Explicit A7 alone,
secondary, changed own identity or changed other identity revokes the established
route. A stale or transitional same-identity report cancels pending intents and
blocks new edits/loads while retaining the connection and explicit unload route.
Retained route knowledge alone never authorizes a new load or resistance change.

Settings/load readiness still requires current supported topology, known exact
motor state, Weight Training, fresh settings, cable-zero evidence and the existing
mount/child/update/safety checks. The primary's current A7 represents its coordinated
pair; no unsupported independent secondary-readiness query is invented. This follows
the one-controller implementations and the brief's instruction to include secondary
fields only when evidenced. No safety lock/mask, pairing or role-setting write is added.

The native paired command profile uses0101/0100 for exact0100/0101 or paired0/1
readbacks; Steven captures a refused native load settling0100→0 while still paired.
Legacy paired4/5 uses5/4. Unknown A003 is not masked into permission to load. A known
primary route retains explicit unload independently of load readiness, using the
documented stop family and awaiting device confirmation. These exact profiles are
source-backed; owner-hardware compatibility remains unverified until installation.

Auto Load sends one existing AA12 trigger, as Steven's implementation does, without
altering its settings or bypass fields. Native2003 and completion15 have an explicit
paired decoder, rather than sharing standalone35/completion14 accidentally. There
is no automatic load on connection, freshness recovery, mode change or role transition.

## Weight scope, limits and units

Albert's current `voltra_link.cpp:805–822` writes settings once to the controller in
per-unit wire quantities. Its `VALIDATION.md:176–188` confirms combined presentation
without changing those payloads. The owner's earlier65/130 screenshot agrees with
that scope. The brief explicitly permits verified per-unit capability plus verified
Twin behavior as the second priority for an effective limit.

Use only the current primary's fresh21268 report after the current topology epoch
as its per-unit envelope. Convert the UI total to that scope before validating and
writing base/absolute modifiers; percentages remain unchanged. Confirm raw per-unit
readbacks. Derive combined presentation from that same scope, with no fixed200/230/
250 fallback or fixed460/500 total. Never use a cached secondary limit.

`DeviceWeightProfile.h` isolates Albert's native2.2LB/KG display grid from general
SI recipe conversion. Round each unit before summing the pair; combined LB steps
are2 and combined KG steps are two half-KG ticks. The base5LB→2KG minimum is separate
from ordinary modifier rounding. Inverse conversion clamps to the reported wire
bound and must roundtrip exactly, including duplicate display buckets; e.g. a16LB
maximum cannot become17LB merely because both map near7.5KG. Captured179LB maps to
81.5KG per unit/163KG combined. Native ceiling-display edge cases at other firmware
capacities remain a compatibility inference; wire requests never exceed the report.

## Connection and telemetry boundaries

Logical identities/preferences remain stable. The adapter caches the latest observed
type for each canonical address during this boot and resolves each new GAP attempt
through that cache. Availability/RSSI freshness expires separately; losing a fresh
advertisement cannot restore an older type. No address or alternate type is guessed.
Generation/terminal/handle/GAP ownership barriers and bounded retries remain intact.
UUID-filtered service discovery avoids enumerating unrelated services before the
existing handshake. Notification/authentication order remains intact. Supervision
is10s, following Steven's established paired-link policy.

Steven captured controllers which stop advertising after pairing, and one recovery
after approximately50s. Albert's reversed-role scans did not correlate visibility
with role. Those are firmware-specific observations: our logs do not prove which
radio cause produced every HS13. The fresh-type and TTL-disconnect defects are
confirmed code defects, not a claim that all physical radio failures are solved.

`TwinTelemetry` admits exact captured B4 sources10/11/12 and complete two-channel
samples. Set/rep counters come from source12, with its actual crossed source mapping.
Sparse counts persist across source10/11 motion updates; missing optional counters
do not erase an earlier actual count. No rep-completion phase is invented.
Measured force is never the target resistance displayed by VOLTRA. Neither current
Albert project nor Cortex supplies a proven combined target byte layout; Twin Live
therefore shows unavailable target instead of base, doubled local target or summed
measured force. Manual loaded base edits have a separate capability: Albert's
`voltra_link.h:184–188` allows loaded base-only edits without cable activity, and
Steven's setting path explicitly skips load reassertion for a native pair. Our
profile requires fresh stationary samples from both channels, known loaded state
and unchanged modifiers/curve/units. It sends one bounded base write and confirms
readback, with no motor reassertion. Moving/stale samples block it. General live
modifier editing and automatic drops remain unsupported; the manual capability
cannot admit presets, recipes or an automatic weight-change path.

## Validation

Existing build/validator, capture, UI and release receipts are recorded in TESTING.md
after execution. No physical connection/motor operation or new tracked test source
was performed. Synthetic UI/I/O exercises are labelled as mocks, not device captures.
