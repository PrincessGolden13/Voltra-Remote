# VOLTRA Remote

Standalone round touchscreen/rotary Weight Training remote, with a shared LVGL
application and offline Windows and macOS simulators.

The Guition remote now has Normal Bluetooth firmware and a separate offline
Simulator image. **Full real training control is not ready:** connection and
unloaded LB dial changes are physically confirmed with the local diagnostic profile;
explicit unload is implemented. Loading, modifier/KG/Twin control and connection
robustness remain incomplete. Desktop devices are synthetic.
See [current implementation and blockers](docs/NORMAL_MODE_2026_10_17.md).

The remote boots Normal. On Devices, press the rotary button, then turn left
three logical clicks within three seconds to enter/leave Simulator. Normal mode
rescans every five seconds, attempts a sole available VOLTRA once, and keeps saved
devices visible offline. Hold a saved device and confirm REMOVE to forget it.

## Run

On Windows, open `build/native/voltra_simulator.exe`.
On Mac, follow [Mac setup](docs/MAC_SIMULATOR.md), then run
`bash tools/Run-Simulator.sh` or open `build/mac/voltra_simulator.app`.
Both start at the device picker and share the same screens and controls.
Tap a VOLTRA; turn the mouse wheel or use Left/Right to adjust weight. Tap the
weight to load/unload; hold it for 650 ms to request Auto Load. Tap the name to
return to the picker. Drag the top rim to move the window; Escape closes it.

The setup screen has one theme color, a device-scaled outer weight dial, and
four controls: Magic, EO, Dropsets and Presets. Tap the weight to load/unload.
Unload remains available immediately
while loaded, pending or stale. Hold the weight for Auto Load. At the confirmed device maximum, the number and
dial use a contrasting color paired with the selected theme, and show MAX.

Tap an off Magic, EO or Dropset tile to edit it; tap an active tile to disable it.
Hold either state to edit. The Magic picker is ordered IC / MTN / C and is
touch-only, as is the unit picker. Rotary always adjusts the amount. Tap the
large amount to apply/return to setup. There is no Back button in Magic or EO.
Modifier editors apply changes without an Apply button. Supported devices offer % and
LB/KG independently for Magic and EO; older profiles show only their supported
absolute unit. Consecutive knob events settle for 150 ms before one request;
reported values remain authoritative, with no confirmation-driven retries.
There are no X controls. Disable modifiers with their main-screen tiles;
Dropsets retain Apply.

Slow rotation changes 1 LB or 0.5 KG; faster rotation accelerates. Device paging
supports arrows, horizontal swipes and rotary (positive rotation goes right).
Twin pairs use overlapping circles. See [the October 17 fixes](docs/UI_FIXES_2026_10_17.md).

Tap Presets to browse P01–P99 with the knob. Tap an existing preset to request
its modifiers; hold it to compare OLD / NEW before saving an override. The
single NEW slot at the end saves the current modifier configuration. Presets
exclude base weight, never load resistance, and become active only when the
machine report matches. Unsupported units/features or intrinsically invalid recipes
are rejected instead of silently converted. When the exact preset modifiers
need more headroom, selecting it reduces base weight to the highest safe integer
display tick (never increases it). Save returns to Main and matching confirmed
settings automatically light the preset indicator. The stored modifier values
are not trimmed. Presets and remote preferences
persist in `remote-settings.txt` beside the Windows executable (or in the Mac
application support directory). Restoring saved data never applies a preset.

Confirmed rep-start telemetry opens the workout screen. F8/F9/F10 provide
synthetic workout telemetry; unloading returns to setup with mock-device
completed-set, rep and rest fields. Missing telemetry is omitted.

The device picker's gear opens theme, rest-alert, dim and sleep settings.
Touch a setting to select it; rotate to change it. Rest alert adjusts in seconds
(0 is Off); dim/sleep adjust in minutes (0 is Never), defaulting to 15/20.
The desktop simulates display dim/sleep and shows a two-second rest alert;
physical haptics require a board adapter. Loaded, armed, pending or unknown
connected state keeps unload visible. The first wake input only wakes.
Wi-Fi and Updates currently explain their unavailable platform services.
Factory reset requires an explicit confirmation and preserves recent devices.

## Offline scenarios

| Key | Action |
| --- | --- |
| F1–F4 | Open Garage with unloaded, loaded, stale, or unknown Twin state |
| F5 | Disconnect and return to Devices |
| F6 | Scan now (normal background interval is 5 seconds) |
| F7 | Toggle White OFF availability (offline at startup) |
| F8 | Complete one simulated rep while loaded |
| F9 | Simulate holding the cable steady |
| F10 | Simulate cable movement; rearms the next RP hold |
| F11 | Ignore the next command; subsequent reports remain the source of truth |
| Enter / Space | Load/unload shortcut; inert on Devices |

For Stairs: enable Dropsets, load, then press F8 for each rep.
For RP: hold Dropsets, choose RP, Apply & Back, load, then use F10 followed by
F9 for each set's pause. Defaults are two drops, 50%, one second.
For Auto Load: hold the unloaded weight, then F9; the **mock device** loads
after a three-second hold. Tap the weight to cancel.

Six simulated VOLTRAs provide different firmware, unit, availability and Twin scenarios.
Recent connections are stored in `recent-voltras.txt` beside the Windows executable
or in `~/Library/Application Support/VOLTRA Remote/` on Mac
(maximum 64, most recent first; no automatic eviction). No load intent is stored.

## Build and documentation

Build Windows with `tools/Build-Desktop.ps1`; build Mac with
`bash tools/Build-Desktop.sh`. See [Mac setup and validation status](docs/MAC_SIMULATOR.md).
[Revised five-screen specification](docs/APP_FLOW.md),
[development/render options](docs/DEVELOPMENT.md),
[architecture](docs/ARCHITECTURE.md),
[validation status](docs/TESTING.md),
[original requirements](docs/PROJECT_REQUIREMENTS.md),
[source audit](docs/UPSTREAM_AUDIT.md),
[hardware](docs/HARDWARE_COMPATIBILITY.md),
[safety](docs/SAFETY.md) and [Twin evidence](docs/TWIN_MODE.md).

October 18 physical results and remaining work are recorded in the
[connection and weight audit](docs/LIVE_CONNECTION_2026_10_18.md).

### October 11 simulator profiles

The two device pages contain Green 2.0 (230 LB, MTN and percentages), Yellow TW
2.0 and Pink TW 2.0 (paired, reported 460 LB / nominal 208 KG limit), Red 2.0 KG
(104 KG ceiling, 0.5 KG steps), Black 1.x (200 LB, no MTN/percentages), and White
OFF (unavailable). Device reports take precedence over unit-specific firmware
fallbacks; see [limit evidence](docs/WEIGHT_LIMITS.md).
These are synthetic profiles, not claims of verified hardware compatibility.
Loading stays on setup with a white number. F8 reports a rep and opens Live.
Presets are shared across devices; absolute values convert to the device unit.
Unsupported recipes require an OK acknowledgement.

For large-number layout previews, add `--modifier-value 125` to a `--render`
command on screen 2 or 3. This initializes synthetic settings only and cannot
be used in the interactive simulator.
For unit-specific layout previews, `--weight-unit 0` selects LB or
`--weight-unit 1` selects KG, also only with `--render`.
`--weight-ticks N` previews a base amount (1 LB / 0.5 KG per tick), clamped to
the profile ceiling. For example, screen 2 with 127 ticks on Green has an 81%
modifier ceiling, useful for inspecting adjacent scale labels.

### Physical Guition K5 bring-up

The connected JC3636K518 working target has a dedicated offline build sharing
this UI and the six mock VOLTRAs. See [hardware bring-up](docs/GUITION_K5.md).
`tools/Build-Guition.ps1` builds it; `tools/Flash-Guition.ps1` requires the verified
local recovery image and checks the generated partition map before upload.
This firmware does not connect to or control a physical VOLTRA.
