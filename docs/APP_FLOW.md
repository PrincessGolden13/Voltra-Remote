# Revised application flow — September 7, 2026

This records the user's later five-screen specification. It supersedes the
single-screen foundation UI. Initial physical-board validation and offline-only
restrictions remain in place. Auto Load's explicit trigger is now authorized;
editing Auto Load/Unload configuration is still outside this app.

## 1. Devices

- Start here without connecting or loading automatically.
- Discover available VOLTRAs; remember the last four successful connections
  across launches. Keep their names/opaque IDs, not resistance or load intent.
- Show online devices bright and remembered offline devices dim. Offline taps
  do nothing; online taps connect and open the weight screen.
- Refresh availability every five seconds. Rotary rotation and press do nothing.
- Show four circular tiles per page, with touch arrows for additional discoveries;
  discovering a device does not evict a saved connection. Online tiles glow cyan;
  offline tiles, their link icons and status badges are dim.

## 2. Weight

- Connected name above the main number; tap the name to return to Devices.
- Show confirmed Twin status, using reported combined limits with no doubling
  or duplicated commands. Unknown Twin state blocks loading.
- Rotary adjusts weight from zero to the reported maximum. The colored outer
  arc under the markers shows the reported selection.
- A short tap toggles load/unload. A 650 ms hold requests existing Auto Load
  once; releasing a long press must not also send a short-tap load.
- Loaded weight is bright; unloaded weight is dark. Zero can be selected, but
  loading requires at least one display unit in this simulator. Unload before
  selecting zero; a loaded device never silently becomes zero resistance.
- Show device-returned values, even when a requested value is not applied.
  No retries, automatic correction, reconciliation writes or force loading.
- Below the weight: Magic, EO, and a stairs/dropset icon. A short tap toggles
  each saved setting. A long press opens its editor. This detailed gesture rule
  takes precedence over the earlier sentence saying a push opens the editor.
- Magic shows IC/C/MTN, with the selected active mode bright and others dim,
  its value and an arc. It can be off. Unsupported modes cannot be selected.
- EO shows its value and arc and is dim when off. It works alongside Magic.
- Dropsets show Stairs and its negative weight, or RP and drops/%/hold seconds.
  All displays reflect actual modifier reports or active local dropset settings.

## 3. Magic

- Shared circular editor: rotary changes the central value and colored arc.
- IC, C and MTN selectors transfer the same value between supported modes.
- Device selects percentage vs absolute units; the remote has no toggle for it.
- Percentage ceiling 300%, further restricted by the combined weight envelope.
- Absolute limits account for base + active magic + active eccentric weight.
- Apply & Back sends the setting and enables it; main-screen activation waits
  for a report. No report means no invented activation.

## 4. Eccentric overload

Same editor, unit policy, envelope constraints and Apply & Back behavior as
Magic, without the IC/C/MTN selector.

## 5. Dropsets

- Select Stairs or Rest Pause; switching keeps each mode's draft settings.
- Stairs: zero or negative integer weight, default -2 in the displayed unit.
  Subtract it after each distinct completed-rep report. Example 30,28,26...
  Clamp reductions to one; never use dropsets to command zero.
- RP: defaults two drops (three sets), 50% reduction and one-second hold.
  Drops are 0–9; percentage 0–80; hold 1–9 seconds. Single-digit ceilings follow
  the requested single-digit fields. Zero drops or 0% produce no reduction.
- Tap the RP field to assign the rotary dial to it. The center shows that value.
- All three mandatory RP settings stay visible and active together (e.g. Drops
  2, Drop % 50%, Hold 1s). An outlined field indicates rotary editing focus;
  the other fields are not disabled or deselected.
- The supplied visual mockup places these values in three large circular dials
  across the center. The outer tick scale and caption follow the editing focus.
- Require movement before a qualifying stationary hold. One uninterrupted hold
  produces one drop, not repeated drops. Movement rearms the next set.
- RP reductions compound from the last reported weight, rounded down to one
  display-unit step, with a floor of one.
- Dropsets run in the controller, independent of which editor is visible.
- Stop reductions after disconnect/device switching, stale/invalid telemetry,
  rep counter resets/gaps, or an unconfirmed reduction. Never catch up missed
  reps, replay a drop, retry, or reapply a requested weight.
- Returning to Devices disarms local dropsets; saved parameter values remain
  available during the current process. No dropset intent survives app restart.

## Limits and implementation choices

All display values use integer steps in the device-reported LB or KG unit.
The initial -2 step is consequently -2 LB or -2 KG. No unit selector is exposed.
The envelope uses base * (1 + active percentage modifiers / 100), or base plus
active absolute modifiers. Limits round down, never up. Increasing base weight
trims active EO first, then Magic, before capping base at the device maximum,
as required by the original brief's maximum-weight test case.

Modifier changes are sent while unloaded. Weight changes while loaded and
remote dropsets require an explicit device capability. Unknown capability means
unavailable; the mock is the only implementation that currently supplies it.
Encoder press remains an unload/load shortcut outside Devices; a small weight
status target provides unload/cancel on editor screens.

## Evidence boundary

The user's five supplied visual mockups guide the native LVGL presentation:
cyan rings and active ticks, white active numbers, dark inactive numbers,
pink EO accents, circular device/mode selectors, and outlined Apply buttons.
Scale endpoints and midpoint are computed from the current reported limit;
the illustrative 230/300 maxima in artwork are not hard-coded capabilities.
Remote battery comes through HardwarePlatform. Desktop explicitly simulates
75% and labels the footer SIM; hardware without a reading shows unknown.

CONFIRMED (official UI documentation): long press on the weight activates Auto
Load; the existing device behavior uses a stable cable hold.
[Beyond Power reference](https://help.beyond-power.com/en/articles/14167829-advanced-mode-quick-reference).

CONFIRMED (this source only): desktop scenarios implement all five screens and
synthetic capabilities, modifier reports, rep/hold telemetry and Auto Load.
The new TrainingReport adapter is not a physical protocol decoder.

UNVERIFIED on hardware: Auto Load trigger/response mapping, MTN wire semantics,
device-selected percentage representation, combined envelope and Twin routing,
rep/hold detection thresholds and safe live weight changes. No BLE resistance
commands or physical motor tests have been added.
