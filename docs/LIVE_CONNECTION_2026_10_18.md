# Physical connection audit — October 18, 2026

The user supplied a VOLTRA running 2.x, restarted it, and required operation at
approximately ten feet. Discovery alone does not prove a working connection;
RSSI alone does not establish the cause of connection failure.

## Confirmed on this remote and VOLTRA

- Normal boots, initializes display/touch/encoder and scans repeatedly.
- Initial failures were NimBLE 574 (HCI 0x3e, establishment failure), before GATT.
  A bounded retry later established a radio link and negotiated MTU 247.
- Correcting cached-only enumeration exposed three services and four expected
  VOLTRA characteristics. All three notification channels have CCCDs and subscribe.
- With the corrected bootstrap, VOLTRA returned HELLO/routing replies, a 27 ACK,
  and the 27/820100 acceptance response. The user confirmed a connection approval
  appeared on VOLTRA and approved it. Do not assume approval is never required.
- The accepted session returned base 30, motor 0, Weight Training 1, unit 0,
  Chains/EO amounts 6/6, cable position/offset 14/14, coworker status 0,
  child lock/OTA 0, battery 63 and max-force register 21268 = 230 LB.
  These are observed raw values; not all enumerations or scope are established.
- Connection/readback worked at the user's ten-foot distance throughout the
  55-second capture. This is not yet a long-duration/range robustness result.
- The user observed an intermittent missing weight and diagnostic messages.
  Motor 0 was rejected by our old decoder; a full read cycle also exceeded the
  two-second core-state freshness window. Both were application defects.

No autonomous resistance commands were sent. The user explicitly tapped the
weight target while it was unknown; that target can request unload. No physical
load, settings change, Auto Load or dropset was exercised by the agent.

## Reference audit

External checkouts remain outside this source tree. Current GitHub source and
remote revisions were checked on October 18; source evidence does not prove
physical compatibility.

| Source | Revision and evidence |
| --- | --- |
| [Ryan ESP32 remote](https://github.com/RyanMarkoff-eaton/voltra-diy-remote) | Pinned validator baseline f0dffbb5ba15eeaa64c8c713a1d7291dfeb75a1a; current dff6227ddbc9bb1767698898e166b1ff019ad18f fetched without changing that checkout. Connection sequence unchanged; newer code distinguishes readback 0/1 from motor commands 4/5 and describes a one-byte IC selector. |
| [Node SDK](https://github.com/HJewkes/voltra-node-sdk) | Current 79f6c55297500e6a81c31af78eb85fedd13c2b61; dual subscriptions, framing, sequence zero, acceptance gating and small reads. |
| [Android protocol research](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/PROTOCOL_NOTES.md) | Current da514d2db1a16ebb4ded3156bc8fd200bed1e6eb; official-app captures establish three channels, fragmented responses and an approximately five-second connection-request deadline. |
| [Home Assistant](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant/tree/93b0819745e665ab4d35f4cc501d48c6296c81f7) | Reviewed 93b0819745e665ab4d35f4cc501d48c6296c81f7; independent framing and 90 ms bootstrap pacing. |
| Installed NimBLE-Arduino 2.3.6 | Collection discovery defaults to cached-only. subscribe() can return true without a CCCD. Both require explicit checks. |

## Corrections

1. Preserve the union of advertised names/service UUIDs during each completed
   scan. Later advertisements can replace scan-response data in NimBLE.
2. Keep discoveries for 15 seconds in a bounded 64-device cache. One missed scan
   no longer removes an unsaved tile. This never supplies machine state/readiness.
3. Establish links asynchronously with visible progress/cancellation. Retry only
   574, at most three times, 500 ms apart, each with a 10-second timeout. Observe
   MTU negotiation with a one-second fallback. Drain cancelled attempts before
   client reuse. Never retry protocol or resistance commands.
4. Call getServices(true)/getCharacteristics(true). Empty fresh caches previously
   caused app rejection without actual discovery.
5. Subscribe to 55CA, notify-capable A010, and CA946 when present, requiring CCCDs.
   Maintain independent bounded frame parsers per channel.
6. Use sequence zero for all three bootstrap frames; increment accepted-session
   requests separately. BLE write success never establishes application acceptance.
7. Pace bootstrap at 90 ms per HA/Android. Testing Ryan's 3000 ms HELLO delay
   after full GATT discovery sent the connection request more than five seconds
   after link-up. Start bootstrap on a fresh loop after discovery and settle
   after each synchronous write completes; do not use backdated loop time.
8. Decode readbacks 0/1 as unloaded/loaded per current Ryan evidence, retaining
   recognized legacy values. Unknown states still invalidate the report. A motor
   value is not a readiness authorization; training controls remain disabled.
9. Prioritize base/motor/workout/unit reads between small background groups;
   information reads must not starve the two-second freshness budget.
10. Stop identifying EP1.0 as the installed app release. That module tag appeared
    on this user-reported 2.x machine. Log bounded module tags separately and
    leave the app version unknown until its actual mapping is established.
11. Preserve failure reasons and use a compact read-only session status.
12. Explicit unload command 4 can be confirmed by a subsequent unloaded readback
    0. Waiting for literal 4 alone left a false pending-command message. Only a
    machine report confirms this; a write acknowledgement still does not.

## Identity comparison

The generated local identity was an unverified interoperability choice. A local
profile uses public reference HELLO/finish fields from ignored config/LocalConfig.h.
No identity values or keys enter Git. This is not a production provisioning design.

- Generated fields with earlier handshake defects: no notification replies.
- Reference fields with accurate 3000 ms delay: failed before finish; inconclusive.
- Reference fields with 90 ms pacing: VOLTRA approval and accepted session above.
- Generated fields with corrected 90 ms pacing: all writes acknowledged, no
  notification bytes, remote termination 531 about eight seconds after link-up.
- Restored reference fields for continued read-only operation and display checks.

The comparison supports an identity/profile incompatibility, but does not isolate
which field or define a valid per-remote provisioning scheme. Normal builds without
the ignored local profile still use the generated NVS identity and are NOT a
physically validated connection configuration. Do not distribute them as ready.

Bootstrap never loads or restores settings. Diagnostics contain bounded command
IDs, headers, byte counts, timestamps, GATT UUIDs and normalized parameter values;
no raw identity/key payloads. Raw logs/images remain ignored under
build/voltra-live/2026-10-18 and build/guition/2026-10-18-*.

## Validation and remaining limits

Existing Ryan validators passed: 203 SDK packets, weights 5–230, 593 modifier
packets and pinned SDK/bootstrap consistency. These validate reference data,
not this remote's physical safety. The packet inspector accepts the captured FF
reference frame. Native build and existing Devices/Main/Twin 360 px renders pass,
including synthetic 230/460 LB limits. No new test code was added.

Normal/Simulator builds and bounded flashing pass with esptool hash verification.
The accepted-session image and SHA256 manifest were preserved before decoder
changes. Final build sizes/results are recorded in BASELINE.md.

Still UNVERIFIED: app-version encoding, full readiness/Auto Load semantics,
current Twin role/peer health/combined limits, resistance controls, modifier
selectors/units, rep/motion telemetry and long-duration link robustness.
EP_MAX_ALLOWED_FORCE=230 is a real single-device observation, not proof of combined
Twin scope. Unknown Twin state leaves the effective control limit unavailable.
The all-functions request remains incomplete; see NORMAL_MODE_2026_10_17.md.

Future cases are tracked in TESTING.md. Existing boot coredump and first-use NVS
warnings did not prevent startup; they are not handshake-failure evidence.

User follow-up: the corrected display shows a steady 30 after a manual connection;
reconnection did not require another approval. Rotary weight changes still did
nothing because PhysicalSession::setTraining remains Unsupported. Subsequent
boots still encountered intermittent 574 before GATT, so reliable startup is not
yet established. Final readback image is preserved in build/guition/2026-10-18-readback.

## Unloaded LB weight follow-up

The user confirmed the device is unloaded and standalone, with Auto Load enabled
as a preference (not armed). That preference is not a reason to block ordinary
unloaded edits and is never changed by this remote.

Fresh current fields 20844 and 21023 both returned zero. The newer paired-EP field
21605 was omitted from a successful read response; it is not a required supported
field on this device. The narrow observed standalone profile accepts current
zero/zero, with stale/nonzero/unknown current reports blocked. If a paired endpoint
is reported, require fresh absence too; do not let its expiry erase a relationship.
Active Twin roles, lost-peer transitions and other firmware are still unverified;
physical loading remains disabled.

The new explicit-input path writes only unloaded LB base weight (16006, uint16).
It requires fresh motor/mode/units/maximum/modifier/lock/OTA evidence, preserves
modifiers and Auto Load settings, and confirms only from a subsequent weight report.
KG, modifier edits/automatic trimming, load and Auto Load trigger are unsupported.
A serial c command connects exactly one available device for debugging; it cannot
write resistance. Bootstrap/reconnect never restores weight or load intent.

CONFIRMED after flashing: automatic link on attempt two, remembered acceptance
without a prompt, sustained fresh 30 LB/unloaded readback, all required standalone
fields and the "Unloaded weight adjustment" status. The user then turned the dial
and confirmed both screens changed. Captured base readbacks: 30 -> 29 -> 32 -> 33
-> 37 -> 43 -> 45, all with unloaded state 0. No resistance was loaded, and no
Auto Load preference was changed. Images and SHA256 manifest:
build/guition/2026-10-18-unloaded-weight.

Remaining observed issue: a rapid input burst briefly showed "Device state
unavailable" while a required background field expired; later inputs and fresh
reports recovered. Freshness checks were not relaxed. Poll scheduling/input
coalescing needs further work for continuous fast rotation. Unsolicited state
packets containing uncatalogued 21007 are rejected atomically; explicit small
read groups still provide weight confirmation. Do not claim perfect responsiveness.
