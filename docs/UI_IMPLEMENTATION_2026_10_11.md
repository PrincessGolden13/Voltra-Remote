# October 11 UI implementation

The supplied ChatGPT prose is a design specification, not a replacement for
prior explicit device/safety requirements. Screenshot numbers are illustrations.

## Mockup mapping

1. Rest-Pause live workout: START/current/NEXT, SET/REP/DROPS.
2. Stairs live workout: START/current/NEXT, SET/REP.
3. Main setup: weight tick gauge and four modifier/preset tiles.
4. Existing preset carousel.
5. Explicit preset override comparison.
6. Normal live workout: reported force, SET/REP.
7. New preset save confirmation.
8. Magic editor: amount gauge and horizontal mode/unit presentation.

## Implemented

- One selectable accent (nine themes), responsive vector LVGL layouts.
- Main weight without an inner ring or unit suffix; dynamic tick landmarks.
- Scalar Magic/EO gauges use calculated permitted maxima. Dropset/preset borders
  represent state only. Descending steps and pause/down glyphs identify dropsets.
- Presets contain modifier enabled states, mode, values, reported units and
  dropset parameters, never base weight or a load request. The store is versioned,
  validated and atomically replaced. P01–P99; one terminal NEW slot, no wrapping.
- Apply sends one guarded request. Dropset settings activate only after matching
  modifier readback. Pending, rejected, unsupported, incompatible-unit and
  out-of-envelope presets do not become active. Manual modifier changes clear
  active identity; a base change alone does not (an actual modifier trim does).
- Explicit save and override; safe cancel; restart only restores recipes.
- Simplified live screens and optional device workout data. The mock produces
  synthetic force/set/rest data; no physical decoder or live-force claim added.
- Settings/presets persist separately from device history. Theme/rest-alert/dim/
  sleep, version/about and confirmed reset. Reset preserves recent-device history.
- Optional hardware power/haptic interface; desktop dim/sleep and visible rest
  alert. Loaded/armed/pending/stale-connected states inhibit inactivity blanking.
- Wi-Fi/captive portal and update entries accurately describe unavailable
  services. No provisioning backend, release manifest, installer or physical
  board implementation exists. These are platform integration work, not simulated
  successful operations; no network is linked into the desktop simulator.

## Resolved interaction decision

The user explicitly chose the new unit picker and immediate edits, superseding
the earlier APP_FLOW device-selected representation and Apply rules. Magic and EO
now each carry their own percent/absolute representation. Unknown capabilities
still disable changes; old simulated firmware exposes only LB. The legacy
percentage ceiling and combined envelope/floor rules are unchanged. Mixed-unit
envelopes sum each modifier in its own representation, and base increases still
trim EO first, then Magic with the existing conservative rounding rule.

Editing amount, mode or unit creates one explicit input intent. Rotary events
settle for 150 ms before sending. An edit made while a request is pending waits
for that request to finish; reports never create new intents or retries. The
confirmed report replaces the editor draft afterward unless another explicit
input is queued. The bottom chevron navigates back; X controls have been removed. Unit
switches now convert and remember roundtrips as described below. The
store reads version-1 recipes and writes version 2 with both modifier units.

## Evidence and validation

CONFIRMED in this source/build only: Windows native build and offscreen renders.
No physical BLE backend is linked. Existing limits, floors, rounding, Twin
readiness, load guards and no-retry dropset logic remain in the controller.

UNVERIFIED: native click interactions. Windows Computer Use capture failed twice
with `SetIsBorderRequired failed: No such interface supported (0x80004002)`.
Mac compilation and physical compatibility remain unverified.

No new automated test code was created, as explicitly requested. Future cases:
preset persistence/99 limit/terminal slot; delayed/mismatched preset confirmation;
manual edits/base-only edits; unsupported units; reset cancel; sleep/wake input
consumption; optional telemetry; stale live display; rest alert once per rest.

Original build baseline: commit 3977431. The pre-edit executable was preserved
in ignored build/native/voltra_simulator-before-oct11.exe. This checkout has no
Git remote configured; all changes and builds are directly in the user's folder.

## Follow-up: maximum feedback, editor navigation and exact preset replacement

User feedback supersedes the earlier X-disable and amount-focus gestures:
tapping the large Magic/EO amount now returns to setup. The outer tick scale
selects amount editing. X controls are removed throughout; preset/settings
pages use Back arrows and existing cancel actions. Modifier toggling remains
on the main tiles. Pending explicit edits are submitted before returning.

Maximum feedback is derived from fresh confirmed base weight, persists while
at the device maximum, and clears below it. Each theme has a paired contrasting
limit color; the unloaded number remains darker than the loaded number. MAX
text makes the indication independent of color. Modifier editors use the same
limit palette at their calculated maximum. Stale state remains a distinct
safety condition, not a maximum indication.

CONFIRMED in source: the mock previously moved its confirmation deadline on
every knob event, allowing continuous scrolling to starve reports. Replacing
a pending mock setting now retains its original 350 ms deadline. Real freshness
and timeout policies were not relaxed. Repeated already-confirmed maximum
requests no longer generate redundant writes.

The user explicitly approved lowering base weight to fit an exact preset.
Applying a preset replaces Magic, EO and dropset configuration. Base is lowered
only when required by the recipe's envelope, using the existing integer-floor
calculation. E.g. 70% IC + 20% EO on a 200-unit device caps base at 105. Presets
still contain no stored base/load command; nothing loads on preset selection.
Weight edits wait during preset confirmation so they cannot overwrite its
pending modifier request. Unsupported/intrinsically invalid recipes still fail.
Failures are now visible in the preset browser. Activation still requires readback.

## Firmware profiles and editor follow-up

CONFIRMED in source: six simulator entries now match the user's names and
capabilities. Yellow/Pink share their synthetic workout state via the selected
session and show the peer name. The original 230 LB combined maximum was wrong:
the October 17 correction is 460 LB / nominal 208 KG with report-first limit
resolution (see WEIGHT_LIMITS.md). Red has a native 104 KG ceiling and now permits
0.5 KG steps. Black is labeled
1.x because no specific older firmware version was supplied. White starts offline.
Firmware/peer/rep-start fields are report data, not UI inference.

Main weight taps always request load/unload; confirmed loaded numbers are white.
Live opens only with fresh, valid loaded rep-start telemetry (F8 in the simulator).
Large values use content-sized labels, fitted transforms and transparent parents.
Mode/unit selectors align font centers and are touch-only. Rotary edits the value.
Percent/absolute conversion uses nearest integer and caches both representations
per device/editor/base unit/base weight. Amount or mode edits invalidate the
alternate representation; unit-only toggles retain it. 25% on 10 LB becomes 3 LB
and returns to 25%; a direct 3 LB edit becomes 30%. Unsupported conversions remain
capability-gated. Caches are session presentation data, not persisted presets.

Presets remain one global configuration vector. Applying a cross-unit recipe
converts absolute Magic/EO and signed stairs values before envelope validation.
Percentages remain unchanged. Unsupported firmware recipes show the requested
version-specific acknowledgement screen; OK dismisses it. No request is sent on
incompatibility. Readback still gates activation, using the converted recipe.

Stairs spans negative-left to positive-right with zero at twelve o'clock. Positive
steps preserve modifiers and cap at the safe base maximum; negative steps stop at
one display unit. Both directions retain fresh telemetry, sequence and confirmation
guards. Apply remains visible, and tapping the center also applies. The Dropset
heading is separated from the Stairs/RP controls.

Validation: Windows build and existing offscreen-render path. Native input capture
remains unavailable as documented above; no new test code was introduced. Future
regressions: unit roundtrip and cache invalidation; firmware modal dismissal;
cross-unit preset acknowledgement; signed stairs near limits; load versus first
rep navigation; Twin switching while loaded; three-digit values at 360/466/480.

Final verification: native Windows rebuild passed. Existing render previews were
produced for Magic/EO/RP at 360, 466 and 480 pixels; reviewed 125% values after
widening the parent container, plus Twin, KG, legacy-firmware, loaded-setup and
Live layouts. The first 125% render reproduced the crop; the corrected renders
show the complete value. `git diff --check` passed. Interactive unit/preset/rep
flows still require simulator input verification; no physical commands were used.
