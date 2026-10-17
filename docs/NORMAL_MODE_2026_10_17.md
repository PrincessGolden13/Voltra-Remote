# Normal mode and physical connection work — October 17, 2026

## User request and remaining work

The user authorized real discovery, connections and all Weight Training functions,
superseding the earlier offline-only development milestone. The requested real
training functionality is **not complete**. Discovery, GATT connection, protocol
bootstrap and read-only observations are implemented. No physical VOLTRA has been
available for validation; the user will make one available later.

Normal firmware currently permits an explicit unload request only. Weight edits,
loading, Magic/EO changes, Auto Load and dropsets remain disabled. Current public
sources do not establish this project's required Twin role/peer-health semantics,
all safety predicates, Mountain/IC selectors, percentage/absolute selectors,
fractional-KG command encoding, or RP hold telemetry. Do not describe a successful
build or connection as a working training release. The session deliberately
returns Unsupported for those operations until a supported physical profile exists.

## Remote behavior

- Power-on/reset starts Normal. On Devices, **press the rotary button, then turn
  left three logical clicks within three seconds** to switch modes. The same
  gesture returns from Simulator. A right turn, touch or navigation cancels it.
  The button arm prevents ordinary leftward page navigation from switching modes.
- Switching restarts the remote. Entering Simulator immediately restores Normal
  as the next boot target, so a later reset does not silently return to simulation.
- Normal scans for two seconds every five seconds while disconnected. Only the
  VOLTRA service qualifies. A completed scan with exactly one online VOLTRA gets
  one automatic connection attempt; zero keeps scanning, multiple leave selection
  to the user. Manual navigation/disconnection does not trigger another auto-select.
- Previously accepted devices remain visible offline. Hold a remembered tile for
  650 ms and confirm REMOVE to delete its local identity/name. An online forgotten
  device can still be discovered and selected again. Failure to persist deletion
  keeps the saved entry. No automatic eviction; up to 64 entries, with an explicit
  memory-full error. Real and synthetic device lists use separate NVS namespaces.
- No load state, base resistance, pending command or dropset activation is restored.
  Global presets/preferences are shared; restoring them never sends a preset.
- VOLTRA application acceptance is distinct from BLE connection. The UI can show
  the connected screen while awaiting an approval on VOLTRA. A timeout/refusal
  returns to Devices with the reason. Only acceptance saves a new device.

## Build isolation and recovery

`guition_k5_normal` links NimBLE 2.3.6 and PhysicalSession, with no mock backend.
`guition_k5_offline` links MockTransport/MockVoltra and no physical BLE adapter.
BootMode is a board adapter; application screens receive only RuntimeMode.
Desktop simulators remain permanently offline.

The two applications occupy OTA0 at 0x10000 and OTA1 at 0x210000, each 0x1d0000
bytes. NVS, boot metadata and coredump offsets are unchanged. Existing first-2-MB
factory recovery was preserved. The original bytes for 0x210000–0x3dffff were
recovered from the previous sequential factory read and **verified against this
remote with esptool verify_flash** before any new writes. Both recovery files and
SHA256 manifests stay in ignored build/hardware-backups. This is not a full-flash
backup. Flash tooling validates both recovery hashes and both generated partition
tables before writing the bounded images. No whole-chip erase or eFuse change.

Build both with `tools/Build-Guition.ps1`; flash both with
`tools/Flash-Guition.ps1 -Port COM3`. Boot selection is initialized to Normal.

## Protocol evidence and limits

CONFIRMED in external source checkouts, not on a physical VOLTRA here:

| Source | Reviewed commit | Relevant evidence |
| --- | --- | --- |
| [Node SDK](https://github.com/HJewkes/voltra-node-sdk/tree/79f6c55297500e6a81c31af78eb85fedd13c2b61) | 79f6c55297500e6a81c31af78eb85fedd13c2b61 | Framing, acceptance payload, 30-second timeout, small bootstrap reads; wide cold query can cause disconnect |
| [Android](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/tree/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb) | da514d2db1a16ebb4ded3156bc8fd200bed1e6eb | GATT/handshake observations, 0x3e89 unload=4, parameter widths, EP firmware strings, max-force catalog |
| [Home Assistant](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant/tree/93b0819745e665ab4d35f4cc501d48c6296c81f7) | 93b0819745e665ab4d35f4cc501d48c6296c81f7 | Independent framing and readiness cross-check |

Checkouts are outside the source tree, in `%TEMP%/voltra-live-oct17/`. Existing
pinned validator checkouts remain separate. No captured client identity or device
key was copied. Normal firmware generates and persists a local UUID; acceptance
of that identity is UNVERIFIED. Bootstrap sends HELLO, routing initialization and
connection identity, never load or configuration. Accepted sessions request small
read groups with 90 ms spacing. Reads include units, base/motor/workout state,
modifiers, cable position/offset, raw Twin/lock/OTA status, max force and firmware.
The parser rejects unknown widths, truncation and trailing bytes atomically.
Disconnect/queue overflow invalidates the session. A BLE write is never confirmation.

The catalog gives 21268 (EP_MAX_ALLOWED_FORCE) as uint16 LB. It is a single-unit
candidate, not proof of a combined Twin ceiling. Unit enum 0=LB/1=KG is INFERRED.
Firmware extraction uses EP tags, not BMS/MC versions. Current Twin status 20844
has no verified enum/peer-health mapping here, so `twinKnown` stays false and the
effective limit stays unavailable. The shared resolver still prefers fresh scoped
reports, then unit-specific firmware fallbacks: **2.x 230 LB / 104 KG single,
460 LB / inferred 208 KG Twin**. See [weight evidence](WEIGHT_LIMITS.md).

[Official Cortex training behavior](https://cortex.beyond-power.com/docs/cortex/training-control)
and [device training skills](https://cortex.beyond-power.com/docs/skills/voltra-device-training)
describe cable/readiness checks, unloaded mode transitions, selectors and Twin
master routing. They do not supply the missing BLE encodings. Cortex is not a
runtime dependency; no Cortex API key/CLI is available in this environment.

## Next physical observation (user deferred)

Keep the USB remote connected; turn on one unloaded VOLTRA with other controllers
disconnected. Observe its firmware version and any connection approval. The first
session records normalized read-only values and connection outcome, without
automatic resistance actions. Resolve current-role/readiness and unit mappings
against these observations before enabling any training profile. Full Twin and
advanced-mode support needs the additional confirmed mappings listed above.

Physical discovery/acceptance, saved-device persistence on this board, mode-switch
gesture feel, all resistance controls and real telemetry remain UNVERIFIED.
No new automated test code was added; validation results go in BASELINE.md.
