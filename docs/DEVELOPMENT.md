# Development environment

Windows 10 x64; existing bundled Python 3.12.14 and Node 24.19.0 used.
Project-local ignored `.venv` contains the pins in tools/requirements-dev.txt.
No VS Code, Visual Studio, Arduino IDE, Android toolchain, Cortex or Home
Assistant installation is needed for the foundation.

## Native build

Run `tools/Build-Desktop.ps1` from PowerShell. It uses native CMake, Ninja and
Zig's bundled Clang/Windows libraries. Compiler/cache paths are derived from the
project location, not embedded in source files. The initial sandbox blocked
Ninja subprocesses; agent execution may need sandbox approval.

Outputs: `build/native/voltra_simulator.exe` and `packet_inspector.exe`.
Run the simulator with `--size 360`, `--size 466` or `--size 480` (default).
Mouse click is touch, wheel/left/right is rotary, Enter/Space is press.
F1 unloaded, F2 loaded, F3 stale after the observation ages, F4 raw coworker
status, F5 disconnect, F6 scan. F7 toggles Studio availability. F8 emits one rep,
F9 holds the cable, F10 moves it, F11 ignores the next command. All are synthetic.
See README for the five-screen touch gestures. The picker ignores rotary input.

`--render build/preview.bmp --size 480 --screen 1 --profile 1` renders without opening a
window. Profiles 0–3 correspond to F1–F4. Screens 0–4 are Devices, Main, Magic,
EO, Drops. Use `--drop-mode 1` for RP, and `--device 1` for the KG/absolute
scenario. Device indices 0–4 follow the initial picker order. Offscreen renders
do not read or write recent-device memory. No Bluetooth, network or vendor SDK is
linked into this executable. The mock supports weight changes from zero to its reported limit
and explicit load/unload with delayed simulated confirmation (350 ms). These
limits and readiness signals are synthetic, not evidence of physical support.
Requests time out after 3 seconds without confirmation; no retries or corrective
writes occur. Main-screen values always come from reports. Tap the large weight
to load/unload, or press Enter/Space. A second tap during loading cancels with
an unload request. Numbers stay bright until unloading is confirmed. Stale or
unknown load state is amber and explicitly labeled; it is not shown as unloaded.
The native window is circular: drag its top area to move, Escape to close.
Recent IDs/names are stored in `recent-voltras.txt` beside the executable with
an atomic replace. Only successful connections update the four-entry MRU.

## Recreate tools

`tools/Setup-Development.ps1 -Python <real-python-executable>` creates the virtual
environment and downloads the pinned LVGL archive, verifying SHA256 before use.
It requires Python with pip and network access. Never commit `.venv` or `.tools`.
LVGL is a normal build dependency; the four research repositories stay external.

## Starter baseline

External checkout: `%TEMP%/voltra-reference-audit/voltra-diy-remote`, exact revision
in UPSTREAM_AUDIT.md. Its ignored LocalConfig.h uses the template's dummy address
for compilation only. Do not upload this reference artifact to the unknown board.

The normal absolute PlatformIO core path produced compiler commands of 32,674
characters and a Windows CreateProcess error. A temporary drive alias shortened
the toolchain path. To reproduce, reserve an unused drive letter (verify first),
map it to the project, set PLATFORMIO_CORE_DIR to that drive's `.tools/platformio`,
build `elecrow21_ui`, then remove only the alias. No source change is needed.

Firmware dependencies: espressif32 6.12.0, Arduino framework
3.20017.241212+sha.dcc1105b, Xtensa/RISC-V 8.4.0+2021r2-patch5,
NimBLE-Arduino 2.3.6, Arduino GFX 1.4.7. All resolved packages are under ignored
`.tools/platformio`. The unused PlatformIO MinGW 5.1 package was an unsuccessful
native compiler candidate; Zig is the selected C++17 compiler.

## Git

Local branch: codex/foundation. No remote existed; no GitHub repository was
created or changed. With no configured author, initial commits explicitly use
`Codex <codex@local.invalid>` via per-command config, without changing user/global
Git identity. Keep requirements, research and implementation commits separate.

No release/license choice for this project's own code has been made. Before
commercial distribution, choose it deliberately and retain dependency notices.
