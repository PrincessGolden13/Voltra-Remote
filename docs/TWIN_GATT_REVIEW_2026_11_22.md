# VOLTRA GATT/handshake comparison, November 22, 2026

Source-only review for report `1cb1cbf1` on remote v0.4.15. No product edits,
physical BLE operations or motor operations were performed by this audit.

## Current public heads rechecked

GitHub default-branch latest commits were reread during this review:

| Repository | Current commit |
| --- | --- |
| albertzhang86/voltra-remote | c4f2884798941babd83788cf79609620e767bc85 |
| albertzhang86/android-voltra | 8f9388063d684f73cbc02831a339919951ac2b1c |
| stevenpoint/voltra_remote | 1c14860b53b805a789b8bed2095e63443e355d7d |
| RyanMarkoff-eaton/voltra-diy-remote | dff6227ddbc9bb1767698898e166b1ff019ad18f |
| HJewkes/voltra-node-sdk | c76a523ecb4a5062f3aa60de1b4be98a152723f7 |
| dylanmaniatakes/Beyond-Power-Voltra-Android | da514d2db1a16ebb4ded3156bc8fd200bed1e6eb |
| dylanmaniatakes/Beyond-Power-HomeAssistant | 93b0819745e665ab4d35f4cc501d48c6296c81f7 |
| omarshahine/voltra-knob | 1ec6bded08783d695f1cb9b7cb3ec7a1f3722e5b |
| marvaments/voltra-knob-controller | 67f7278e3c26354567d84a002156adb1b059dd59 |
| kirby6365/voltra_remote | 8e1ea1cb752acd3108c61f1a502b63978a172832 |

These heads are unchanged from the prior current-head inventory. Some full local
clones are older than these heads; current Ryan main.cpp and Albert Android
transport and current Node adapter files were additionally fetched at the exact
commits above rather than relying on those older local clones.

All inspected implementations agree on the service and these known roles:

- Service: e4dada34-0867-8783-9f70-2ca29216c7e4.
- Write/transport: a010891d-f50f-44f0-901f-9a2421a9e050.
- Command response/notification: 55ca1e52-7354-25de-6afc-b7df1e8816ac.
- Additional notification: ca94658c-0525-5046-e78b-5391b65f47ad where exposed.

## Source matrix

| Source | Required/optional notification setup | Opening order and evidence |
| --- | --- | --- |
| Steven 1c14860 | Queries all three known channels. Subscribes each only if present and canNotify; subscription failures are logged, not individually fatal. | Native doConnect: targeted service/write, HELLO, then three channel queries/subscriptions, bootstrap with 90 ms pacing. src/voltra/voltra_client.cpp:1112-1155. Same pattern in joinFollower:324-353. |
| Albert knob c4f2884 | Requires write a010 and notify 55ca. No third-channel requirement. | Discover/write+notify, subscribe55ca, auth identity, 3 s wait, INIT with 25 ms pacing. voltra_link.cpp:300-315. |
| Albert Android 8f93880 | Requires55ca PROPERTY_NOTIFY and CCCD. | Descriptor acknowledgement yields transport READY before DirectController sends identity, waits3s, sends init frames25ms apart and protocol subscription. AndroidVoltraTransport.kt:232-240,271-273; DirectVoltraController.kt:104-123. |
| Omar 1ec6bded | Requires55ca; write a010. | Subscribe55ca before auth; 3s auth wait and init pacing. firmware/VoltraKnob/src/link/voltra_link.cpp:182-200. |
| Marvaments 67f7278e | Requires55ca; write a010. | Subscribe55ca before runHandshake. src/voltra_ble.cpp:304-324. |
| Ryan DIY dff6227 | Requires55ca. Additionally subscribes a010 if canNotify; its failure is fatal there. | Both subscriptions before auth, then3s and INIT0/INIT1 at20ms. Current src/main.cpp:372-386, fetched at exact public head. |
| Dylan Android da514d2 | Subscribes available known-role characteristics supporting NOTIFY or INDICATE; no universal per-channel requirement in its diagnostic transport. | Descriptors before captured read-only bootstrap. AndroidVoltraClient.kt:3373-3435,3470. |
| Official iPhone PacketLogger evidence recorded by Dylan | Captured all three CCCDs before framed application writes. Captured responses on55ca; test device55ca properties include READ/WRITE/NOTIFY/INDICATE. | PROTOCOL_NOTES.md:19-32,49-55,121-123. This is capture evidence, not a public native iPhone implementation. |
| Home Assistant93b0819 | Attempts all three start_notify calls, catches/logs each individual failure, proceeds to bootstrap. Confirmed protocol-response channels are55ca and a010; ca946 alone cannot set protocol_validated. | custom_components/voltra/client.py:1128-1148; const.py:19-34; protocol-validation branch client.py:1295-1306. Descriptors precede bootstrap with90ms pacing. |
| Node SDK c76a523, exact current files fetched | Noble/Web require55ca and a010; subscribe a010 when it can notify; no ca946 requirement. Native permits immediate auth before monitors. | Noble/Web descriptors before auth. Authentication waits3s, INIT20ms. node-noble.ts:572-608; web-bluetooth-base.ts:131-145; native.ts:351-417; voltra-client.ts:570-578,2151-2167. |
| sdk_nov21 current data subset | Config specifies55ca and a010. | Parameter/frame data, not a separate independent BLE implementation. |
| Kirby/family native forks | Same three-channel roles and source lineage as Steven. | Not independent evidence for a different firmware capability contract. |

## Exact independent response-quorum evidence

[Home Assistant const.py at93b0819](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant/blob/93b0819745e665ab4d35f4cc501d48c6296c81f7/custom_components/voltra/const.py#L19):

```python
VOLTRA_COMMAND_CHARACTERISTIC_UUID = "55ca1e52-7354-25de-6afc-b7df1e8816ac"
VOLTRA_NOTIFY_CHARACTERISTIC_UUID = "ca94658c-0525-5046-e78b-5391b65f47ad"
VOLTRA_TRANSPORT_CHARACTERISTIC_UUID = "a010891d-f50f-44f0-901f-9a2421a9e050"

CONFIRMED_RESPONSE_CHARACTERISTIC_UUIDS: tuple[str, ...] = (
    VOLTRA_COMMAND_CHARACTERISTIC_UUID,
    VOLTRA_TRANSPORT_CHARACTERISTIC_UUID,
)
```

At client.py:1295, it tests this list before setting protocol_validated from a
decoded state change. Other notifications can update availability/state but not
that validation flag. This is source evidence for the two known response routes;
it is not proof of the latest physical trainer's exact GATT properties.

## Findings and recommended correction boundary

1. UUID disagreement was not found. Early HELLO is one working native strategy,
   not a universal official ordering rule; multiple working/captured implementations
   subscribe before auth.
2. Latest remote stage0/generic subscription error does not distinguish a missing
   characteristic, non-notify properties, an ATT discovery error or an already
   queued disconnect. It does not prove55ca is physically absent.
3. The later EventLog531 timestamp is poll time, not necessarily HCI arrival time.
   Blocking GATT can returnnull after a remote disconnect, with cleanup logged
   before the queued531 callback is drained. Do not infer local cleanup caused it.
4. A compatibility quorum of successfully subscribed55ca OR notify-capablea010
   is independently supported by Steven/HA. CA946 alone must not satisfy it.
   Full protocol acceptance, fresh readback, role, capability and resistance
   readiness gates must remain unchanged. No guessed channel or control profile.
5. Caching a single complete attribute snapshot before HELLO can avoid repeated
   discovery after the opening message without adding guessed characteristics.
   Preserve actual query errors/properties and deferred disconnect reasons. The
   actual pinned SDK operation and on-air equivalence require the transport audit.

## Ordering judgment for the correction

Use the single cached GATT snapshot, establish the available documented response
CCCDs (55ca/a010, at least one successful subscription), then send HELLO immediately.
Do not retain HELLO-before-any-response-subscription merely because Steven uses
that order. The official capture, Albert, Dylan, Ryan, Omar, Marvaments and Node
all establish response notifications before application frames.

Exact official capture also subscribes the optional ca946 channel before HELLO.
Deferring that optional stream until after the response quorum and HELLO is a
compatible latency optimization, not an exact reproduction of that capture.
It must not delay the time-sensitive bootstrap or qualify as a response route.

## Independent HELLO wire comparison

PASS: compiled the actual Steven 1c14860 `voltra_protocol.cpp` under a renamed
namespace alongside our actual `PhysicalSession.cpp` and FrameBuilder. Compared
both builders with this literal official iPad capture from Dylan's notes:

```text
552904c90110000020004f69506164000000000000000000000000000000000084ab1a5f292001ea4f
```

Both builders exactly reproduce that capture for identical iPad name/identity.
The actual PhysicalSession HELLO exactly equals Steven for the same `VOLTRA Remote`
name and captured identity, and also matches Steven's generic frame builder with
a separate synthetic retained identity. This checks 41-byte length, type04,
sender01, receiver10, sequence0, protocol0020, command4f, 21-byte zero-padded name,
6-byte identity, final01 and both CRCs against independent code/captured bytes.
No actual owner's identity was read or tested. Different authorized names/identity
bytes intentionally produce different payload/CRC and must not be overwritten
with a captured phone identity.

External manual comparison harness: `/tmp/hello_cross_source.cpp`, executed PASS.
Its SHA-256 is ef3e4c1905e9899aee5ec489d8ae8f30b7e902cbad35faa4c0d38030794e26a1.
FrameBuilder SHA-256 is
8e587d668409c7d36e3d3ecf20d8fac3b6c30a729aa230d2608150df269dcfd7;
Steven protocol cpp SHA-256 is
ee5422b07c81204d416543cd83700d2097ca65114e7cd0bd42cf762652fac6fd.

No source comparison proves that this correction alone repairs native Twin
connection or establishes a Red-to-Black relay. Firmware compatibility remains
scoped to each implementation/capture; report1cb1cbf1 does not include trainer
firmware/GATT metadata sufficient for a universal claim.

## v0.4.16 implementation and latest report

Latest report1cb1cbf1e810181a1fd7732cfd80fc99, received2026-11-22T22:08:18Z,
identifies runningv0.4.15. It records a usable link and prepared HELLO, then
"VOLTRA subscription failed" with transport stageNone. Native subscription
failures set stageSubscription, so this generic text arose before that call: the
required-channel lookup returned absent or non-notifiable. This does not prove
the characteristic was physically absent; a disconnect can interrupt discovery.

The latest attempt's GATT setup events have the pre-blocking application
timestamp714688ms. The later531 entry is callback-drain time715155ms. Neither
supplies the wall-clock ordering of physical termination and local cleanup, or
the full wall time of each GATT operation. Candidate elapsed746ms is
also sampled application time, not a measured complete connection duration.

Pinned NimBLE-Arduino2.3.6 uses ATT ReadByType0x2803 for both all-characteristic
discovery and by-UUID discovery. The latter filters locally and caches matches
only. v0.4.15 therefore rescanned the service for writer, response and telemetry.
Use one complete immutable characteristic snapshot per service/connection before
returning the first write channel. Further lookups use that cache. Do not refresh
SDK objects after installing callbacks, or retain adapter pointers across an SDK
refresh that deletes the underlying objects.

Subscribe available documented response channels before HELLO; require at least
one successful55ca/a010 response subscription. Send the original HELLO once,
then configure optionalca946 telemetry. This follows the official response-before-
frame invariant, without treating Steven's alternate pre-CCCD order as universal.
CA946 alone cannot admit a session. Full VOLTRA acceptance, fresh state, capability
and role checks still authorize commands separately. No motor/role/relay profile
or physical pairing behavior is changed.

When blocking GATT fails after a current-owner terminal callback is queued, retain
that actual disconnect cause before local cleanup. Do not consume or retag the
callback, reuse a live client, or infer a reason from stale lastError. Separate
response parsers prevent interleaved channels from joining unrelated fragments.

Source review supports these setup corrections; it does not prove that they fix
the physical Twin connection. No physical connection, pairing, reset or resistance
operation was performed during development. Executed checks are recorded in
TESTING.md after completion.
