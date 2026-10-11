# macOS simulator

The Mac simulator builds a native round AppKit window around the same LVGL UI,
controller, offline devices and scenario controls as Windows. The source targets
macOS 12 or newer on Apple Silicon and Intel; the default build uses the Mac's
native architecture. Runtime use is offline and needs no Bluetooth permissions.

## First setup

Copy or clone the project folder onto the Mac. Do not copy Windows `.venv`,
`build`, or `.tools/zig-cache` folders; they contain machine-specific tools and
build caches. A verified LVGL source/archive in `.tools` can be reused.

1. Open Terminal and install Apple's compiler tools with `xcode-select --install`.
   Finish the installer before continuing. An existing Xcode installation also works.
2. Install [CMake](https://cmake.org/download/) 3.24 or newer. If using CMake.app, run
   `export PATH="/Applications/CMake.app/Contents/bin:$PATH"` in Terminal.
3. In Terminal, change to your project folder and run:

```bash
cd "/path/to/Voltra-Remote"
bash tools/Setup-Development.sh
bash tools/Build-Desktop.sh
bash tools/Run-Simulator.sh
```

Setup downloads the same SHA256-verified LVGL 9.3.0 archive used on Windows.
It requires Internet access only when that archive is missing. Build uses Apple
Clang and Make, with output isolated in `build/mac`. After building, you can
also double-click `build/mac/voltra_simulator.app` in Finder.

After editing source, rerun `bash tools/Build-Desktop.sh` and launch again.
Run only one simulator instance per machine so its recent-device file has one writer.

## Controls

Click for touch, hold for 650 ms for long press, scroll or use Left/Right for
the rotary dial, and Enter/Space for its press. Drag the top rim to move the
window. Escape or Command-Q closes it. A trackpad uses ten scroll points per
step and ignores momentum after finger release; arrow keys give exact steps.

F1–F11 use the same scenarios listed in the project README. On Mac keyboards,
hold Fn/Globe when using function keys if macOS assigns them system actions.
Recent connections live in
`~/Library/Application Support/VOLTRA Remote/recent-voltras.txt` (four devices,
names and IDs only). They survive rebuilds; no load intent is saved.

## Render without opening a window

```bash
bash tools/Run-Simulator.sh --render build/mac-rp.bmp --size 480 --screen 4 --drop-mode 1
```

All Windows command-line options also work on Mac: sizes 360/466/480,
screens 0–4, profiles 0–3, devices 0–4, drops 0/1. Offscreen rendering does
not read or write saved devices. See DEVELOPMENT.md for scenario details.

For a universal Apple Silicon/Intel app, build with:

```bash
bash tools/Build-Desktop.sh '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
```

This is a local development app; signing/notarization and distribution packaging
are not configured.

## Validation status

CONFIRMED (September 26, 2026, Apple Silicon Mac): dependency setup verified
the pinned LVGL archive; CMake 4.4.3 and Apple Clang 17 successfully built the
simulator and packet inspector. All five screens rendered offline at 480 pixels,
and the RP editor rendered at 360 pixels. The device-picker render was visually
inspected. The native app was launched with macOS `open`.

UNVERIFIED: interactive Retina rendering, pointer/trackpad input, function keys,
window closing and recent-device persistence. These build/render checks do not
establish physical hardware compatibility or validate resistance control.


October 4 profiles: device indices 0=A (230 LB, MTN), 1=B (paired with D,
460 LB combined), 2=C (90 KG, no MTN, base edits never confirm), 3=D (same
pair), 4=E (offline). Start without arguments to see all names and the pair.
F7 toggles E availability. F8 simulates a completed rep, F9 a hold, and F10
movement; use F10 between RP holds. These inputs only affect the mock.
