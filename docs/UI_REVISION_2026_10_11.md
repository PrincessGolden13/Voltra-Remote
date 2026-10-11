Yes. I would give Codex a **hierarchy-of-authority rule first**, because at this point the screenshots are mostly UI references, while you have already taught the codebase some implementation details that the screenshots cannot represent correctly.

You can copy/paste the following as one specification:

---

# VOLTRA Remote — Revised UI / Interaction Specification

These instructions describe the UI and interaction changes represented by the latest mockups.

## IMPORTANT: EXISTING CODE AND PRIOR EXPLICIT INSTRUCTIONS MUST BE PRESERVED

The screenshots and the requirements below are **not permission to rewrite working logic that already exists in this project**.

I have already given explicit implementation instructions and made code changes for things that may not be visible in these mockups, including behavior such as:

- maximum-weight handling;
- minimum-weight handling;
- rounding;
- limits derived from the connected VOLTRA;
- unsupported-feature handling;
- BLE/device-confirmation behavior;
- safety behavior;
- calculations;
- edge cases;
- device capability detection;
- and potentially other implementation details.

Preserve that existing behavior unless these new instructions **explicitly and unambiguously supersede it**.

### Conflict rule

Use this priority order:

1. **Existing explicit implementation instructions I previously gave you and intentional existing code behavior.**
2. **The written requirements in this message.**
3. **The screenshots/mockups as visual references.**

If you identify a **direct conflict** between an existing explicit instruction/code behavior and something in these new requirements or screenshots:

**Do not silently choose one. Ask me which behavior I want.**

For example, if a mockup shows `300%` but existing code calculates a lower permissible maximum because of the connected VOLTRA's actual weight limits, **do not replace the calculation with 300% just because the screenshot says 300**.

Likewise, do not modify existing rounding logic merely because a screenshot displays a convenient round number.

The screenshots are examples of visual state, not test fixtures for business logic.

Do not infer technical behavior purely from numbers shown in mockups.

---

# GLOBAL UI PRINCIPLES

The display is a very small round touchscreen surrounded by a physical rotary encoder.

The UI must be readable and usable at approximately 1.8 inches.

Prefer:

- large values;
- minimal wording;
- large touch areas;
- strong visual hierarchy;
- one obvious rotary-control target at a time.

The physical rotary knob is the primary adjustment method.

Do not add `+` and `-` controls for values that are adjusted with the knob.

Use touch primarily for selecting what the knob controls, activating functions, or entering configuration states.

---

# COLOR / THEMING

Do not permanently assign separate colors to Magic, EO, Dropsets, Presets, etc.

The entire interface uses **one user-selectable theme/accent color**.

Examples of available themes:

`red / green / blue / yellow / purple / pink / orange / teal / gray`

The cyan shown in the mockups is merely an example theme.

Active/selected controls use the theme color.

Primary text can remain white.

Inactive/unavailable values should use dim neutral text or a subdued version of the theme.

---

# MAIN WEIGHT-TRAINING SCREEN

The latest main-screen mockup is the general visual target.

The base-weight section should remain the dominant part of the screen, but the four controls beneath it need enough space to remain readable on the real device.

Do not shrink the weight section as aggressively as one of the earlier drafts did.

Use the width of the round screen efficiently rather than leaving unnecessary dead space at the left and right.

---

## Connected VOLTRA

Show the connected VOLTRA name near the top.

Example:

`VOLTRA Alpha`

If Twin mode is active, show the Twin indicator beneath it.

The VOLTRA name/header should be tappable.

**Tap the VOLTRA name/header → return to the VOLTRA selection screen.**

---

# BASE WEIGHT

Show the base weight as a large number.

Example:

`50`

Do **not** show `LB` or `KG` beside the base weight on the main setup screen.

Do **not** put a separate circular ring immediately around the large weight value.

The large outside dial already represents this setting.

---

## Base-weight outer dial

The outside gauge represents the valid base-weight range.

Use a reasonably dense series of tick marks.

Show useful landmark values based on the range.

For an example maximum of 230 lb:

`0 / 50 / 100 / 150 / 200 / 230`

The highlighted portion must represent the selected value relative to the actual maximum.

Do not treat this as decorative lighting.

The actual limits must continue to come from existing device/capability logic.

---

## Base-weight controls

When base weight is the currently selected adjustment:

**Rotate knob → change base weight.**

The intended touch behavior is:

- Tap base weight when not focused → focus/select base weight.
- Tap base weight when already focused → load/unload the VOLTRA.
- Hold base weight → Autoload.

Do not change existing device-safety or load-confirmation logic in order to implement this UI.

---

# FOUR MAIN-SCREEN CONTROLS

Show four modifier/preset controls underneath the main weight:

1. Magic
2. EO
3. Dropset
4. Preset

The current visual layout uses a row of four reasonably large controls.

They should be readable without overwhelming the main weight.

---

# MAGIC — MAIN SCREEN

Do not show all three Magic modes on the main screen.

Only show the currently selected mode and its value.

Examples:

`IC`
`70%`

or:

`MTN`
`25 lb`

or:

`C`
`12 kg`

Use the actual unit mode currently configured.

---

## Magic mini gauge

Magic may use an arc/ring because it represents a scalar adjustable value.

The illuminated amount must correspond to the value relative to its **actual valid maximum**, not simply the printed percentage.

Example:

If the selected value is `70%` and the valid maximum is `300%`, the mini gauge should be approximately:

`70 / 300 = 23.3%`

filled.

Continue using any existing code that dynamically limits the maximum based on VOLTRA capabilities or combined-load constraints.

---

# ECCENTRIC OVERLOAD — MAIN SCREEN

Show:

`EO`
`20%`

or:

`EO`
`20 lb`

or:

`EO`
`9 kg`

EO can have a mini gauge because it also represents a scalar adjustable value.

As with Magic, the arc represents the selected value relative to the actual permitted maximum.

Do not hard-code the screenshot's visual fraction if existing limit calculations produce something different.

---

# DROPSET — MAIN SCREEN

The Dropset control may have a circular outline for visual consistency.

However:

**The outline must NOT act like a progress/value gauge.**

Do not partially fill the circumference based on:

- drop percentage;
- number of drops;
- stair decrement;
- or any other Dropset parameter.

A Dropset is a collection of settings, not one normalized scalar.

Use the border only to communicate things such as:

- enabled;
- disabled;
- selected/focused.

---

## Stairs icon

When the selected Dropset mode is Stairs, use a clear **descending staircase** icon.

Stairs means weight decreases after each rep.

Example:

`40 → 38 → 36 → 34`

if decrement is `-2`.

---

## Rest-Pause icon

Do **not** use the earlier circular-arrow/repeat icon. It was not intuitive enough.

For Rest-Pause on the **main setup screen**, use a simple composite symbol based on:

**pause bars + downward/drop indication**

Conceptually:

`Ⅱ ↓`

It should visually suggest:

**pause, then reduce the weight.**

Do not surround it with additional confusing arrows.

Under the icon show the configured RP values compactly.

Example:

```text
2
50%
1s
```

Meaning:

- 2 drops;
- 50% reduction per drop;
- 1-second hold.

Do not write `DROP` or `RP` inside the normal main-screen tile if the icon and values are sufficient.

---

# DROPSET MAIN-SCREEN BORDER

Unlike Magic and EO:

**Dropset does not get a percentage-progress arc.**

If its circular border is active, use a consistent full-state treatment, such as:

- dim outline = disabled;
- theme-color outline = enabled;
- brighter outline = focused.

Do not partially fill it.

---

# PRESET — MAIN SCREEN

The Preset control may also use a circular outline for visual consistency.

It must **not** contain a partially filled progress/value arc.

A preset number has no normalized magnitude.

Show only something like:

bookmark icon

`P3`

Do not try to fit the preset's full generated description into the main-screen tile.

The border can indicate:

- inactive;
- preset currently matches settings;
- focused.

But it must not visually imply a percentage.

---

# PRESET ACTIVE STATE

Immediately after applying a preset, highlight the Preset control.

Example:

`P3` bright/white with an active theme-color border.

This means:

**the current modifier configuration exactly matches P3.**

If the user then manually changes anything contained in the preset, including:

- Magic mode;
- Magic value;
- Magic unit;
- Magic enabled state;
- EO value/unit/enabled state;
- Dropset mode;
- Dropset parameters;
- Dropset enabled state;

then automatically clear the active-preset state.

The user is no longer exactly on P3.

Do not clear it merely because base weight changes; base weight is not part of presets.

---

# MAIN-SCREEN BOTTOM STATUS ROW

At the bottom show available workout status compactly.

Example:

`5 | 9      [battery] 75%      02:14`

`5 | 9` means:

- 5 sets completed;
- 9 reps in the last completed set.

The timer is the post-set rest duration.

Do not add labels or icons around the set/rep pair or timer.

---

## Source of set/rep/rest information

Where the VOLTRA exposes this information, the VOLTRA should be the source of truth.

Do not create competing local tracking merely to populate the UI unless required by the existing implementation.

If a value genuinely is not available from the device/protocol:

**omit that UI field.**

Do not invent a value or show a fake `0`.

And importantly:

If the current code already contains deliberate tracking logic that I previously asked for, do not replace it merely because this specification says to prefer device data. Treat that as a possible conflict and ask me first.

---

# MAGIC ADJUSTMENT UI

Do not title this screen:

`Magic Mode`

or:

`Magic Modes`

Those are development terms, not required user-facing UI.

The selected Magic amount remains the dominant value.

Example:

`70%`

---

## Magic outer gauge

Use the outer gauge for the amount.

For percent mode the conceptual range may be up to `300%`, subject to existing device-limit logic.

Useful labels:

`0 / 50 / 100 / 150 / 200 / 250 / 300`

If existing calculations produce a lower permissible maximum, continue using that calculated maximum.

In absolute mode use the correct device/unit range.

---

# MAGIC MODE PICKER

The mode order must ALWAYS be:

**IC → MTN → C**

Do not use:

`MTN → IC → C`

Do not use a vertical wheel.

Do not put circles around each individual mode.

Use a **horizontal iOS Picker View / wheel-picker style presentation**.

Example when IC is selected:

```text
IC        MTN        C
```

But visually:

- `IC` = substantially larger, brighter text;
- `MTN` = smaller/dimmer;
- `C` = smaller/dimmer.

The selected value should be unmistakably larger than the unselected choices.

It should look as though the values belong to one horizontal rotary picker.

As the physical knob rotates:

`IC → MTN → C`

the newly selected value becomes the large central/emphasized text and the previous value becomes smaller.

Do not give every option its own button/circle.

A shared container is acceptable.

---

# MAGIC UNIT PICKER

Use the same design principle as the mode selector.

Do not display the word:

`UNIT`

Do not place vertical bars/separators around the `%`.

Do not create uneven blank cells.

Do not leave an unexplained blank option.

For a pounds-based VOLTRA:

```text
%          LB
```

For a kilograms-based VOLTRA:

```text
%          KG
```

The currently selected value is:

- noticeably larger;
- brighter;
- centered/emphasized.

The other value is:

- smaller;
- dimmer.

Rotate the knob while the unit picker is focused to change between them.

The visual spacing should be balanced and symmetrical.

---

## Older firmware without percentage support

If the connected VOLTRA/firmware does not support percentage mode:

Do not show `%` as an available option.

Do not show a blank placeholder where `%` would have been.

Simply present the supported absolute unit cleanly.

Do not remove existing capability-detection logic.

---

# MAGIC DISABLE

Show an X while editing Magic.

The X means:

**disable Magic.**

Do not add a checkmark merely to apply ordinary value changes.

If implementation requires a separate navigation/back mechanism, do not make the X ambiguously perform both Back and Disable.

---

# EO ADJUSTMENT

Use the same general amount/unit interaction as Magic, except there is no IC/MTN/C selector.

The user chooses:

- amount;
- `%` or `LB/KG`, where supported.

Show an X to disable EO.

No separate Apply button is required for normal editing unless existing architecture requires one.

---

# PRESET CONTENT

Presets save modifier configuration, not base weight.

A preset contains the relevant state for:

**Magic**
- on/off
- IC / MTN / C
- value
- unit

**EO**
- on/off
- value
- unit

**Dropsets**
- on/off
- Stairs or Rest-Pause
- relevant Dropset parameters

Do not include base exercise weight.

---

# PRESET NUMBERING

Support up to:

`P01 ... P99`

No keyboard-based naming is required.

A compact generated description may be shown in the preset browser.

Examples:

`IC70 EO20`

`MTN20 EO20`

`IC20 RP2-50-1`

Do not rely on that text as the unique identifier; the preset number is primary.

---

# PRESET BROWSER — EXISTING PRESETS

Use a horizontal carousel.

Example:

```text
P02       P03       P04
```

The current preset is centered, larger and bright.

Adjacent presets are smaller/dimmer.

Rotate the physical knob to browse.

Do not put a 1–99 numerical gauge around the outside.

---

## Applying an existing preset

When an existing preset is centered:

**Tap it → apply it and return to the main screen.**

The main Preset widget should then show that preset as active.

---

# PRESET — EMPTY / NEW SLOT

Always expose exactly one additional unsaved preset after the highest saved preset, unless P99 already exists.

Example:

If saved presets are:

`P01 P02 P03 P04`

the browser also exposes:

`P05 NEW`

P05 is the final available carousel item.

### IMPORTANT

When the user is on P05 NEW:

- there is **nothing to the right**;
- do not show P06;
- do not show a right-arrow suggesting another item;
- the user can only rotate/swipe **back to the left** toward P04.

This must visually communicate that P05 is the end of the current list.

---

# SAVE NEW PRESET SCREEN

When the user chooses the empty P05 slot, show:

`P05`

`NEW`

and the current settings that would be stored.

Example:

```text
IC 70%
EO 20%
RP
2
50%
1s
```

There should be only one primary bottom action:

**SAVE**

Do not also put a second Cancel button at the bottom.

Use the top X to cancel.

The broader carousel/navigation design should make it clear that before entering the save confirmation, the user could simply rotate back to existing presets.

---

# PRESET OVERRIDE

Holding an existing preset invokes Override.

Example:

`OVERRIDE P03?`

Show:

**OLD**

versus:

**NEW**

side-by-side.

Display the existing stored configuration against the current configuration.

Provide:

**SAVE**

and an obvious safe cancel action.

The top X can also cancel.

Do not overwrite the preset until the user deliberately confirms.

---

# LIVE WORKOUT — NORMAL

When an active set starts, change automatically to a simplified live-workout display.

Show the current force prominently.

Example:

`61 LB`

Below it:

```text
SET       REP
 2         8
```

Do not use circular dials around SET or REP.

Do not show:

- `LIVE`;
- waveform icons;
- concentric/eccentric phase labels;
- rest timer.

The screen should be very clean.

---

# LIVE WORKOUT — STAIRS

Stairs should use the **same overall visual language** as other live workout screens.

No Stairs icon is necessary on the live screen.

Show:

```text
START 40

   36

NEXT 34
```

Where:

- Start = starting resistance;
- large middle number = current effective resistance;
- Next = next resistance after the next completed rep/drop.

Do not label the large number `CURRENT`.

Use the same typography and color treatment for `START` and `NEXT`.

Under that show ONLY:

```text
SET       REP
 2         3
```

Do not insert an empty third column between SET and REP.

Do not reserve space for an icon.

SET and REP should be balanced across the available width.

---

# LIVE WORKOUT — REST-PAUSE

The Rest-Pause live screen should use the **same visual design for START / current / NEXT as the Stairs screen**.

Do not use different colors or font sizes for the START/NEXT labels between the two modes.

Example:

```text
START 80

   80

NEXT 40
```

After the first 50% drop it might become:

```text
START 80

   40

NEXT 20
```

subject to existing rounding and minimum-weight logic.

Do not change existing rounding behavior merely to match these examples.

---

## Rest-Pause lower statistics

Show:

```text
SET      REP      DROPS
 1        3        0/2
```

Where:

`0/2`

means:

- 0 drops have happened;
- 2 total drops were configured.

After the first drop:

`1/2`

After the second:

`2/2`

Do not show an RP icon on this live screen.

The presence of the DROPS column already differentiates this state.

---

# STAIRS VS REST-PAUSE LIVE VISUAL CONSISTENCY

Both should use the same:

- VOLTRA header;
- Twin indicator;
- START treatment;
- current-weight typography;
- NEXT treatment;
- separators;
- SET/REP typography;
- battery treatment.

The only necessary lower-information difference is:

**Stairs**

```text
SET      REP
```

**Rest-Pause**

```text
SET      REP      DROPS
```

Do not add decorative mode icons that make one mode look structurally different from the other.

---

# REST TIMER

The rest timer belongs on the main/setup screen **after a set is complete**.

It does not belong on the active live screen.

When the set finishes and the UI returns to the main screen, show the available post-set rest value in the bottom status row.

Continue using whatever trusted/device-derived timing implementation already exists.

---

# SETTINGS ENTRY

The VOLTRA selection screen should contain a settings gear.

That opens remote settings.

---

# SETTINGS — THEME

Offer:

- Red
- Green
- Blue
- Yellow
- Purple
- Pink
- Orange
- Teal
- Gray

The selected theme controls the accent color across the UI.

---

# SETTINGS — REST ALERT

Allow:

- Off
- or a configured `minutes + seconds` threshold.

When the configured rest time is reached, trigger the remote's haptic alert.

Approximately two seconds of noticeable feedback is desired.

This may be implemented as a pulse pattern rather than a literal continuous two-second motor activation.

Retain any existing hardware-specific haptic implementation unless there is a reason to change it.

---

# SETTINGS — DIM / SLEEP

Provide independent inactivity settings.

Default target behavior:

**Dim:** 15 minutes

**Sleep:** 20 minutes

Both should be configurable and capable of being turned off / set to Never.

Do not overwrite better existing power-management logic without discussing the conflict first.

---

# SETTINGS — WI-FI

Do not try to type Wi-Fi credentials using the rotary display.

Preferred flow:

remote creates a temporary access point → user joins it with phone → captive portal → user chooses Wi-Fi and enters password using phone keyboard → remote saves credentials and connects.

This is an architectural direction, not an instruction to copy another repository's implementation.

---

# SETTINGS — UPDATES

Provide:

`Check for updates`

The remote can eventually:

- connect through Wi-Fi;
- query a current firmware version;
- compare against installed version;
- download an update;
- verify integrity;
- install;
- reboot.

Do not install firmware while the VOLTRA is actively loaded/in use.

Preserve any update architecture already added to the code unless there is an actual conflict.

---

# SETTINGS — ABOUT

Show useful remote information such as:

- firmware version;
- build/version information;
- hardware info where useful.

---

# SETTINGS — FACTORY RESET

Factory Reset must require deliberate confirmation.

Do not execute immediately.

The safe/default action should be:

**Cancel / No**

The destructive action should be visually secondary and unmistakably intentional.

Reset the remote's own persisted configuration according to the existing reset implementation.

Do not invent additional erased data if the current code intentionally preserves something.

---

# EXTERNAL REPOSITORIES / PROTOCOL REFERENCES

The external VOLTRA-related GitHub repositories discussed during development are **reference sources only**.

They are not my repositories.

Do not describe them as ours.

Do not wholesale-copy their implementations.

Use them to understand VOLTRA protocol behavior where appropriate and implement the needed behavior independently in this project.

---

# FINAL IMPLEMENTATION RULE

Before changing existing behavior to match one of these mockups, ask:

**Is this actually a UI/layout change, or would it alter existing device logic?**

If it is only visual:

Implement it.

If it changes behavior that I have already explicitly specified or that the existing code intentionally implements:

**preserve the existing behavior.**

If these new requirements genuinely contradict that existing behavior:

**stop and ask me to resolve the conflict rather than silently overwriting it.**

For ambiguous screenshot details, treat the screenshots as visual examples only.

For numerical edge cases, device constraints, rounding, maximums, minimums, unsupported modes, safety checks, protocol interpretation, and other technical behavior, prefer the existing explicit implementation unless I specifically tell you to replace it.