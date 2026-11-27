# Grey VOLTRA Live and Stairs follow-up

The v0.4.26 report `98abc5dd` identifies the connection as standalone
Weight Training in pounds at a selected 36 lb. The remote received 1,169
Bluetooth notifications. None had a B4 header or a complete B4 frame. It
received 281 AA81 motion and 12 AA82 phase-start notifications, so the rep
stream remained available. The read-back fitness rate was 10 and mask was 0.

The AA82 pull and return examples both carry 360 tenths at payload offset 8.
The SDK identifies this field as base weight in Weight Training. AA84's
offset 38 carries a pull-volume field in the reviewed schema. The current
B4 decoder is still available when the device provides its live target.
[SDK event contract](https://github.com/HJewkes/voltra-node-sdk/blob/a53804e46c87a9064bc776d0e9ecedc2e66867fe/src/sdk/types.ts#L194)

The parameter catalog identifies read-only 20859 as `MC_TARGET_WEIGHT_LBS`,
signed tenths of a pound. This is the single motor's target. The standalone
fallback asks for that value during a fresh loaded Weight Training session
when the B4 target is missing. It does not alter the fitness mask, rep
stream, selected weight or control readiness. The Twin display remains
unverified because a motor target does not establish the combined reading.
[VOLTRA parameter catalog](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv#L869)

## Stairs timing

The captured AA82 pull starts with rep count 5, and the next AA82 return
starts with count 6. AA82's return marker is not a complete rep. Stairs
needs a matching fresh AA81 stationary return-to-idle boundary before it
can request a reduction. The report includes no corresponding standalone
AA81 payload window or fresh loaded write-gate snapshot. It confirms normal
load/readback and a visible set. It does not establish which required
condition blocked Stairs on this set.

The decoder now preserves the verified AA82 return candidate through an
intervening AA81 transition that still carries the preceding count from the
same pull. The reduction still requires exact matching AA81 return and
stationary idle frames with the new count, plus current standalone write
permission. Track one completed counter at a time. The trace retains the
standalone motion and loaded write-gate context for a physical retest.

## Direct Load with the cable extended

The first video shows the remote opening a full-screen cable-retraction
notice. The other shows the VOLTRA with its cable already extended and
the selected resistance unloaded. Its own tap opens a confirmation with
Cancel and Load Weight. Confirming starts Safety Check and then Progressive
Loading, seen at 10 lb on the way to the selected 70 lb.

The protocol capture documents the existing AA12 guided operation from an
extended-cable state and the returned countdown, engaging and completion
phases. The remote can present its own confirmation for a fresh eligible
standalone connection and use the same guarded action. Confirmation rechecks
the connection, current settings, unloaded readiness and both reported Direct
Load flags. Settings and safety checks remain on the VOLTRA.
[Steven's physical capture](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L135-L163)
