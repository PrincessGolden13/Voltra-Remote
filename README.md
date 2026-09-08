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

The Magic, EO and Dropset controls below the weight toggle on a short tap.
Hold a control to edit it. Turn the wheel in the editor, then Apply & Back.
In RP, tap Drops, Drop % or Hold to choose which value the wheel changes.
Device-reported settings choose LB/KG and percentage/absolute modifiers.
The UI follows the supplied mockups with cyan circular controls, pink EO
accents, a four-tile device picker and three simultaneously visible RP dials.
Use the picker's touch arrows for additional devices. The outlined RP dial
selects rotary focus; all three RP parameters remain active.

## Offline scenarios

| Key | Action |
| --- | --- |
| F1–F4 | Open Garage with unloaded, loaded, stale, or unknown Twin state |
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
