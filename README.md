# VOLTRA Remote

Standalone round touchscreen/rotary Weight Training remote, with a shared LVGL
application and offline Windows and macOS simulators.

**Physical BLE control is not enabled.** All current devices, capabilities,
weight changes, Auto Load and telemetry are synthetic. The unidentified
prototype board remains blocked.

## Run

On Windows, open `build/native/voltra_simulator.exe`.
On Mac, follow [Mac setup](docs/MAC_SIMULATOR.md), then run
`bash tools/Run-Simulator.sh` or open `build/mac/voltra_simulator.app`.
Both start at the device picker and share the same screens and controls.
Tap a VOLTRA; turn the mouse wheel or use Left/Right to adjust weight. Tap the
weight to load/unload; hold it for 650 ms to request Auto Load. Tap the name to
return to the picker. Drag the top rim to move the window; Escape closes it.

The setup screen has one theme color, a device-scaled outer weight dial, and
four controls: Magic, EO, Dropsets and Presets. Tap the weight to focus it;
when already focused, tap to load/unload. Unload remains available immediately
while loaded, pending or stale. Hold the weight for Auto Load. At the confirmed device maximum, the number and
dial use a contrasting color paired with the selected theme, and show MAX.

Magic, EO and Dropsets retain their earlier short-tap toggle / hold-to-edit
behavior. The Magic picker is ordered IC / MTN / C; touch a mode then turn the
knob to browse supported modes, or touch the outer scale to adjust the amount. Tap the large amount to return
to setup.
Modifier editors now apply changes without an Apply button. Touch the outer scale,
mode or unit picker to choose the knob's target. Supported devices offer % and
LB/KG independently for Magic and EO; older profiles show only their supported
absolute unit. Consecutive knob events settle for 150 ms before one request;
reported values remain authoritative, with no confirmation-driven retries.
There are no X controls. Disable modifiers with their main-screen tiles;
Back arrows navigate without disabling. Dropsets retain Apply.

Tap Presets to browse P01–P99 with the knob. Tap an existing preset to request
its modifiers; hold it to compare OLD / NEW before saving an override. The
single NEW slot at the end saves the current modifier configuration. Presets
exclude base weight, never load resistance, and become active only when the
machine report matches. Unsupported units/features or intrinsically invalid recipes
are rejected instead of silently converted. When the exact preset modifiers
need more headroom, selecting it reduces base weight to the highest safe integer
value (never increases it). The stored modifier values are not trimmed. Presets and remote preferences
persist in `remote-settings.txt` beside the Windows executable (or in the Mac
application support directory). Restoring saved data never applies a preset.

Confirmed loading opens the simplified workout screen. F8/F9/F10 provide
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
| F1â€“F4 | Open Garage with unloaded, loaded, stale, or unknown Twin state |
| F5 | Disconnect and return to Devices |
| F6 | Scan now (normal background interval is 5 seconds) |
| F7 | Turn the Studio device off/on; connect once first to see its remembered offline row |
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

Five simulated VOLTRAs provide different scenarios. Studio reports KG and
absolute modifiers; Twin reports its own combined limit; Travel omits MTN.
Recent connections are stored in `recent-voltras.txt` beside the Windows executable
or in `~/Library/Application Support/VOLTRA Remote/` on Mac
(maximum four, most recent first). No load intent is stored.

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

Next physical step: identify the exact remote PCB and revision, then implement
and validate read-only BLE observations before enabling physical commands.
