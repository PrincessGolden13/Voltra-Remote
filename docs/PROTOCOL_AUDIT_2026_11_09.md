# November 9 source audit: Twin and safety status

The user requested a wider source audit before arranging another physical
diagnostic session. References were cloned outside this repository. The source
baseline remains public v0.4.1; ignored build/pre-nov9-bugfix contains its binaries.

## Reviewed revisions

| Reference | Default-branch revision | Finding in that revision |
| --- | --- | --- |
| [VOLTRA Node SDK](https://github.com/HJewkes/voltra-node-sdk) | 79f6c55297500e6a81c31af78eb85fedd13c2b61 | Independent sessions, settings and telemetry; no Twin/coworker decoder or void-lock decoder |
| [VOLTRA Android](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android) | da514d2db1a16ebb4ded3156bc8fd200bed1e6eb | Parameter catalog and analysis describe Twin; live role/status decoding is not implemented |
| [Home Assistant](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant) | 93b0819745e665ab4d35f4cc501d48c6296c81f7 | Independent BLE device controls; no Twin/coworker implementation |
| [Ryan's remote](https://github.com/RyanMarkoff-eaton/voltra-diy-remote) | dff6227ddbc9bb1767698898e166b1ff019ad18f | Single-device captured commands; no Twin/coworker implementation |
| [Kirby's remote](https://github.com/kirby6365/voltra_remote) | 8e1ea1cb752acd3108c61f1a502b63978a172832 | Single-device settings and load commands; no Twin decoder |
| [Prompt Knob](https://github.com/andrew-malitchuk/prompt-knob-firmware) | b738e065b7dec7a13501cdd81c8d3b511850bce4 | Hardware/application reference; no VOLTRA implementation |
| [Waveshare / Guition board reference](https://github.com/nkinnan/Waveshare-ESP32-S3-Knob-Touch-LCD-1.8_and_Guition-K5-Knob-Series-JC3636K518) | a7401232f562fba87eb23d8c9c675861e2eeb30a | Hardware reference; no VOLTRA implementation |

CONFIRMED within this source scope: searched tracked implementation files,
protocol notes, catalogs and tests. Also fetched SDK and Android histories/tags
(172 and 24 commits reachable from their respective default-branch HEADs).
History searches for coworker/void-lock implementations did not reveal a decoder.
This does not claim access to unpublished branches, private binaries, or all
third-party VOLTRA projects.

## What is documented

The Android catalog does document Twin, including:

- 20844 COWORKER_CONN_ST, uint8; enum values are not provided.
- 21023 EP_COWORKER_REMOTE_ST, uint32; combines connection, battery and lock
  state, but bit positions are not provided.
- 21605 COWORKER_PAIRED_EP_DEV, uint64; relationship information, not peer
  readiness or a documented advertisement format.
- 21560 LAST_COWORKER_ROLE: 0 none, 1 main, 2 follower. It explicitly describes
  the **last** role; historical role cannot establish live topology.
- 21559 / 21561 last peer address/name; 21569 slave Auto Load status;
  21435..21437 peer force, cable position and speed; 21427..21429 main/sub/total
  force. The latter three fields have unspecified units.

[ANDROID_PORT_ANALYSIS.md](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/ANDROID_PORT_ANALYSIS.md)
describes paired-device concepts and explicitly recommends deferring Twin until
the BLE core is proven. Its referenced private analysis artifacts do not supply
a public, working Twin implementation in this checkout.

[Official Cortex Twin documentation](https://cortex.beyond-power.com/docs/cortex/twin)
does establish that an active Twin is controlled through its master. It does not
publish the raw BLE role/status enum, advertisement encoding or peer lock bits.
Thus master routing is documented at the API level; mapping it into this direct
BLE remote remains UNVERIFIED. Do not infer two connections, duplicate commands,
halve weights, or equate two nearby devices with one active pair.

## Why the current remote fails in Twin

CONFIRMED in our code: PhysicalSession only recognizes the previously observed
standalone zero/zero coworker state. Physical discovery has no Twin advertisement
decoder. The Twin icon/grouped names already exist in the simulator, but physical
Twin routing, peer safety and combined readback were never implemented.

This is an implementation gap, not proof that Twin is impossible or wholly
undocumented. v0.4.3 does not claim to fix or enable Twin control. Its reports
capture the two raw coworker status fields and paired-ID presence without storing
the identifier. Future evidence must establish live role, peer readiness/freshness,
combined command/readback units and loss-of-peer behavior before enabling control.

## Safety error

CONFIRMED in our code: EP_VOID_LOCK (21224) is accepted only when its fresh value
equals 6, the value observed with Black during the October 24 session. The catalog
calls it a uint32 lock-type field, without bit definitions. EP_VOID_LOCK_MASK
(21227) is a configuration mask, and EP_VOID_LOCK_CHASTITY (21371) records history;
neither supplies the missing meaning of the current bits.

INFERRED: a different legitimate value or a stale report can explain the user's
failure. It is not established which occurred. v0.4.3 distinguishes missing/stale
status from an unrecognized numeric value and captures state values/ages in the
approved report. It does **not** change the accepted lock bits, write a lock mask,
retry loading or bypass mount/cable/peer readiness. The reported loading defect
remains unresolved pending a supported decoder or observed status evidence.

## Existing validators

Ryan's check_packets.py passed: 203 packets, weight builder 5..230 and 593 modifier
packets. check_upstream.py passed against its required SDK commit
a53804e46c87a9064bc776d0e9ecedc2e66867fe in a separate detached reference worktree.
These validate upstream captures only, not physical compatibility or Twin support.
No new test code or physical resistance commands were introduced or executed.
