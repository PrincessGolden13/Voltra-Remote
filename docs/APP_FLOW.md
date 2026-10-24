> October 24 feedback supersedes conflicting behavior below:
> - Between reps and after a set, show the machine-confirmed selected weight,
>   not its fluctuating idle cable tension. Normal Live shows measured force
>   only during a fresh machine-reported active rep phase. Keep raw force and
>   motion available to the controller; this is a presentation rule.
> - The user confirmed first-rep tracking and post-set counts after restoring
>   the default Weight Training telemetry subscription. Prefer a published
>   elapsed rest value; if unavailable, the user now authorizes a remote clock
>   anchored to the machine's confirmed set-close event. Never start it from a
>   load/unload tap. Keep the local clock separate from machine-confirmed state.
> - The latest follow-up requires % / machine-unit editing and real local
>   dropsets. See [unit and dropset implementation](DROPSETS_UNITS_2026_10_24.md)
>   for shared-unit behavior, source evidence, loaded-write guards and pending
>   physical verification. The older preview-only notes below are historical.
> - On accepted connection, open Weight Training automatically from confirmed
>   idle/unloaded state, once; wait for mode readback. Never load resistance.
> - Show Twin information only for a detected pair. Missing status alone is not
>   evidence of Twin; retain internal safety guards without the spurious banner.
> - Coalesce physical settings until rotary input pauses for 100 ms. Show local
>   requested weight immediately with a pending label; confirmation stays separate.
> - Magic and EO writes are independent. Send only changed fields, with reductions
>   before increases when applying a combined-limit adjustment or preset.
> - Disabling Magic/EO retains its last confirmed positive amount as a local
>   editor default, independently per device. Reopening the editor recalls that
>   amount within current limits; it does not reactivate the modifier by itself.
> - Later October 24 feedback confirms modifier recall works. Rotary acceleration
>   should ramp gradually and faster changes should land on round 5/10 values.
>   Keep the dropset tile/editor visible when execution is unavailable; saved
>   local recipes must remain disabled until the device supports execution.
>   Mountain is available on Black and must use its separate confirmed selector.
> - Synthetic Yellow/Pink have explicit Overdrive limits of 250 LB each and
>   500 LB combined (113 / 226 KG). Do not assign 250 LB to all real 2.x devices.
>
> October 18 testing priority: finish the single available VOLTRA first. The user
> has other units but explicitly deferred real Twin work until later. Keep unknown
> or active physical Twin states blocked; this does not change simulator limits.
> Base selection now stops at one display tick (1 LB / 0.5 KG), superseding the
> older zero-selection text below. Loading has its own device readiness minimum.
>
> October 18 clarification: Auto Load **enabled in settings** is distinct from
> Auto Load **armed by the explicit long press**. Enabling that preference must
> not block ordinary unloaded weight adjustment. Leave its configuration alone;
> determine active/armed state from the machine's current reports.
>
> October 17 Normal-mode request supersedes the earlier offline-only milestone:
> boot Normal, hide Simulator behind rotary press + three left clicks, auto-connect
> one discovered VOLTRA, retain remembered devices, hold/confirm to forget, and
> continue discovery while disconnected. All real Weight Training functionality
> is requested; its incomplete implementation and protocol blockers are recorded
> in [Normal mode status](NORMAL_MODE_2026_10_17.md). Existing load-safety invariants
> and the prohibition on autonomous physical motor testing still apply.
>
> October 17 feedback supersedes conflicting rules below: see
> [all sixteen fixes](UI_FIXES_2026_10_17.md) and [unit/limit policy](WEIGHT_LIMITS.md).
> Devices support arrows, swipe and rotary paging. Off modifier tiles open their
> editors; active tiles disable on tap, hold edits. Save returns to Main.
> Magic/EO amount taps apply/return without a Back button. Weight uses 1 LB or
> 0.5 KG steps, with speed-dependent acceleration. 2.x Twin is 460 LB, with
> reported effective limits preferred over firmware and native-unit fallbacks.
>
> October 11 follow-up supersedes older interactions below: mode/unit
> selection is touch-only, rotary always edits the main amount, and percent/absolute
> toggles convert with nearest-integer rounding and remember unit-only roundtrips.
> Main load stays on setup until confirmed rep-start telemetry. Presets are global,
> convert absolute units, and require an OK notice for firmware incompatibility.
> Stairs supports signed steps, zero at the top, no central circle, and keeps an
> Apply button per the user's final clarification (center tap also applies).

> Follow-up: tapping the large Magic/EO value returns to setup; its outer scale
> selects amount editing. X controls are removed. Exact preset modifiers replace
> current settings, lowering base only as needed (explicitly approved by user).
> See UI_IMPLEMENTATION_2026_10_11.md for current maximum-feedback behavior.

> October 11 presentation extension: see [implementation status](UI_IMPLEMENTATION_2026_10_11.md)
> and [supplied revision](UI_REVISION_2026_10_11.md). Limits, safety, dropset
> calculations remain authoritative. The user explicitly approved the new
> selectable modifier units and immediate editing on October 11, superseding
> the older unit/Apply rules below.

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

UNVERIFIED on hardware: IC selector/value mapping and width, Auto Load trigger/response mapping, MTN wire semantics,
device-selected percentage representation, combined envelope and Twin routing,
rep/hold detection thresholds and safe live weight changes. No BLE resistance
commands or physical motor tests have been added.

The [September 20 SDK review](SDK_REVIEW_2026_09_20.md) reinforces that the
upstream heartbeat contains per-rep means, not a live stationary-cable signal.
Physical RP requires a verified live motion source; the simulator's explicit
hold/movement events do not establish that mapping.
