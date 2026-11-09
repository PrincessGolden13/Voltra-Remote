# VOLTRA Remote

An open-source touchscreen and rotary remote for Beyond Power VOLTRA Weight
Training, with shared application code for physical hardware and offline desktop
simulators. Independent community project; not affiliated with Beyond Power.

**Development software.** The current physical target is the Guition JC3636K518
(ESP32-S3, 360 x 360 round display, 16 MB flash / 8 MB OPI PSRAM). Do not install
its firmware on a different board, including a 480 x 480 Elecrow rotary display.

**Pairing on a fresh remote still needs work.** Physical training observations
come from the owner's local diagnostic bootstrap profile, which is excluded from
public code and binaries. Generated identities did not establish pairing in those
trials. Other owners can download, build, run the simulator and improve the code;
public firmware is not yet a validated plug-and-play remote.
See [connection evidence](docs/LIVE_CONNECTION_2026_10_18.md).

## Download and contribute

- [Download source](https://github.com/PrincessGolden13/Voltra-Remote/archive/refs/heads/main.zip)
- [Firmware releases](https://github.com/PrincessGolden13/Voltra-Remote/releases)
- [Report a bug or request a feature](https://github.com/PrincessGolden13/Voltra-Remote/issues)
- [Contribution guide](CONTRIBUTING.md)

Original code is [MIT licensed](LICENSE): use, modify and share it, keeping the
license notice. [Third-party portions](THIRD_PARTY_NOTICES.md) retain their own terms.

## What it does

The physical remote operates directly over BLE; a phone, server and Internet
connection are unnecessary for normal training control. Implemented controls
include base weight, Magic (IC / Chains / Mountain), eccentric overload, presets,
Stairs and Rest-Pause dropsets, load/unload and an explicit Auto Load long press.
Availability depends on confirmed device capabilities and current safety state.
Read the dated [application flow](docs/APP_FLOW.md) and
[validation record](docs/TESTING.md) for the distinction between implementation,
simulator behavior and actual hardware observations. Real Twin work is deferred.

Turn the dial to edit weight. Tap the main number to load/unload; hold for 650 ms
to request the device's existing Auto Load behavior. An unloaded number is darker;
confirmed loaded weight is bright. Pending intent stays distinct from device
confirmation. Boot, reconnect, presets and updates never load resistance.

Tap an off modifier tile to open its editor; tap an active tile to disable it,
or hold to edit. The device name returns to the picker. The gear opens settings.
Wi-Fi setup uses a temporary phone portal under Settings -> Wi-Fi.

## Firmware updates

After the [initial USB installation](docs/USB_INSTALL.md), the remote checks
GitHub at power-on using saved Wi-Fi. A newer release shows an installation prompt.
Settings -> Updates -> CHECK FOR UPDATES checks again; INSTALL & RESTART downloads
and installs the firmware. Leave the VOLTRA connection and keep power connected
during installation. Offline operation remains available without Wi-Fi.

Updates use published firmware releases for this board, with HTTPS, size and
SHA-256 verification. The old flash layout requires one USB migration; subsequent
updates write the inactive Normal slot and preserve the separate Simulator.
See [update design, build/release process and unverified cases](docs/FIRMWARE_UPDATES.md).

## Build and run

Clone or download this repository. On Windows, use a real Python 3 installation:

```powershell
./tools/Setup-Development.ps1 -Python C:/path/to/python.exe
./tools/Build-Desktop.ps1
./build/native/voltra_simulator.exe
```

On macOS, install Apple's command-line tools and CMake, then:

```sh
bash tools/Setup-Development.sh
bash tools/Build-Desktop.sh
bash tools/Run-Simulator.sh
```

The simulator uses mouse/touch, mouse wheel or Left/Right, and Enter/Space for
encoder press. F1–F4 select synthetic unloaded, loaded, stale and Twin scenarios;
F5 disconnects, F8 completes a rep, F9 holds cable still, F10 moves it and F11
ignores the next command. Synthetic devices are not hardware compatibility evidence.
Simulator builds cannot link physical BLE or OTA implementations.

Build both public embedded images on Windows:

```powershell
./tools/Build-Guition.ps1 -Target Release
./.venv/Scripts/python.exe tools/package_release.py
```

Linux/macOS firmware builds use PlatformIO; commands and pinned versions are in
[firmware updates](docs/FIRMWARE_UPDATES.md). The default `prototype_21` environment
is intentionally blocked. Select an explicit supported environment.

## Engineering documentation

[Phone and Codex Cloud workflow](docs/CLOUD_DEVELOPMENT.md) explains how to report
a bug, prepare a fix, publish firmware and update the remote without a running PC.

[Architecture](docs/ARCHITECTURE.md), [hardware compatibility](docs/HARDWARE_COMPATIBILITY.md),
[safety](docs/SAFETY.md), [protocol notes](docs/PROTOCOL_NOTES.md),
[Twin evidence](docs/TWIN_MODE.md), [upstream audit](docs/UPSTREAM_AUDIT.md),
[Windows development](docs/DEVELOPMENT.md), [Mac setup](docs/MAC_SIMULATOR.md),
[build baselines](docs/BASELINE.md) and [standing requirements](AGENTS.md).

The documentation retains dated development history. Later dated findings and
the precedence notes in APP_FLOW.md supersede older milestone descriptions.
