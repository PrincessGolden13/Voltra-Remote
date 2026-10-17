# Weight units and limit evidence — October 17, 2026

`protocol/WeightLimits.h` resolves effective limits before they reach the UI.
The UI/controller use display ticks: one tick is 1 LB or 0.5 KG. Percentages
remain integer percentage points. These ticks are not BLE parameter units.

## Precedence

1. Require known single/Twin state. Use a fresh, scoped device report for that
   state and the requested display unit, including a lower configured limit.
2. When only the other unit is reported, convert and round down to a tick.
3. With no applicable report, use the documented VOLTRA I firmware family table.
   Twin fallback additionally requires known combined-weight semantics and
   matching firmware identities for both endpoints. A single-unit report is
   never substituted for a combined report or multiplied by the UI.
4. Unknown model, version, Twin state or required peer evidence gives no limit.
   Future firmware families do not inherit a guessed ceiling. A zero/invalid
   report does not fall through to a larger firmware default.

| VOLTRA I fallback | Single LB | Single KG | Twin LB | Twin KG |
| --- | ---: | ---: | ---: | ---: |
| 1.x | 200 | 90 | 400 | 180 |
| 2.x | 230 | 104 | 460 | 208 |

Native KG ceilings are explicit. Converting the advertised LB ceiling and
rounding to a nearby KG value would be a different policy. Firmware fallbacks
are INFERRED nominal capabilities; they do not prove readiness, configuration,
entitlements, routing or physical compatibility. Device reports take precedence.

## Evidence and scope

- CONFIRMED (official announcement, accessed October 17): Firmware 2.0 raises
  the nominal single-unit ceiling to 230 LB / 104 KG.
  [Beyond Power firmware FAQ](https://help.beyond-power.com/en/articles/17131084-faq-for-2026-3rd-anniversary-campaign).
- CONFIRMED (user, October 17): 2.x supports 230 LB each / 460 LB in Twin.
  This supersedes the incorrect 230 LB combined simulator profile and using
  the older 400 LB example as a universal Twin maximum.
- CONFIRMED (official legacy manual): PDF page 3, printed page 2, specifies
  200 LB / 90 KG. The controls page does not specify KG adjustment increments.
  [Official manual index](https://help.beyond-power.com/en/articles/13926869-product-manuals).
- CONFIRMED (official Japanese product description): approximately 0.5 KG
  adjustments on the unit/app. This supports the requested 0.5 KG UI policy;
  it does not establish exact wire encoding.
  [Beyond Power product description](https://jp.beyond-power.com/blogs/news/meet-voltra-i-the-200lb-portable-cable-trainer-powerhouse).
- INFERRED: Twin native-KG fallback ceilings of 180 and 208 from the native
  per-unit ceilings and combined-resistance requirement. Physical KG Twin
  readback/routing remains UNVERIFIED; a verified effective report overrides this.
- UNVERIFIED physically: Normal diagnostics now read catalog candidate 21268;
  candidate 21534 remains research evidence, not a universal maximum.
  Neither is a combined limit without scope evidence. See VOLTRA_COMPATIBILITY.md.

## Implementation boundaries

The mock supplies explicit 230 LB / 104 KG single and 460 LB / 208 KG combined
reports. Red and Black omit a max report to exercise firmware fallback through
the same resolver. Names are presentation only; limits use version/model
metadata. Black's 1.8 identity is synthetic, not a device observation.

The physical ControlSession now performs connection/read-only diagnostics.
Its unknown Twin mapping leaves effective limits and training writes disabled.
It must supply fresh scoped capabilities and independently validate mode,
readiness, routing and peer health before enabling training commands. See
[physical implementation status](NORMAL_MODE_2026_10_17.md).

Recipes convert absolute values/stair steps to the nearest tick; envelope
ceilings and RP reductions round down. Integer-KG recipes migrate once to half-KG
ticks (desktop format 3, board NVS format 2). Zero-valued legacy modifiers become
disabled. Default stairs remain -2 KG (-4 ticks); reduction floor is one tick.
