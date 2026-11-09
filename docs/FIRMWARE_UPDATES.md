# GitHub publication and firmware updates — November 9, 2026

Public source: https://github.com/PrincessGolden13/Voltra-Remote
License: MIT for original code; third-party terms remain in THIRD_PARTY_NOTICES.md.

The owner's requested behavior is a check at power-on and a prompt before
installation. Settings -> Updates -> CHECK FOR UPDATES checks the same feed;
INSTALL & RESTART downloads the matching compiled release without a computer.
The ESP32 runs firmware, not source code. Merging code into GitHub does not
publish an update until a maintainer releases a version.

## Feed and installation policy

- `VERSION` defines numeric major.minor.patch. `config/release.json` defines the
  repository and the supported board/layout. Fork owners change the repository
  before compiling to use their own releases.
- Feed: `/releases/latest/download/guition-k5.json`; firmware is pinned to that
  manifest's `/releases/download/vVERSION/voltra-remote-guition-k5.bin`.
  GitHub's latest published non-prerelease release is used; unreviewed branch
  commits, drafts and prereleases are not installed. Only strictly newer numeric
  versions are offered. A failed install requires a new explicit action.
- The manifest checks schema, board, Normal channel, flash layout, version,
  length and SHA-256. The image contains an independent board/channel/version
  descriptor, validated before selecting it for boot. There are no GitHub tokens.
- TLS uses ESP-IDF's bundled CA roots and hostname/certificate validation on
  every redirect. Only GitHub and its two release asset hosts are permitted, with
  at most five redirects. The TLS clock uses `time.cloudflare.com` and
  `pool.ntp.org`; clock failure fails closed. HTTPS trusts the release maintainer;
  the checksum detects corruption, and is not an independent code signature.
- Network work uses a separate FreeRTOS task (12 KiB stack), 10-second socket
  timeouts, a 2 KiB manifest cap and 4 KiB download buffer. Firmware transfer has
  a three-minute deadline. The updater performs no VOLTRA commands.
- Automatic connection gives startup Wi-Fi a 15-second opportunity. A person can
  select a VOLTRA immediately. A deferred boot check runs once Wi-Fi is ready
  while disconnected. Failed checks never trap the app or retry continuously;
  the manual button can retry. No Internet is needed for ordinary operation.
- A found release opens the Updates prompt once from Devices, without interrupting
  a connected training session. Checking can be interrupted by a connection.
  Installing reserves the disconnected remote: selection, automatic connection,
  setup and mode changes are blocked until failure or restart. Unload is not gated.

## Flash layout and recovery

Layout 2 preserves NVS at 0x9000 (20 KiB) and OTA metadata at 0xe000 (8 KiB).
Normal A is 0x10000 / 3 MiB, Normal B is 0x310000 / 3 MiB, Simulator is
0x610000 / 2 MiB, and coredump is 0x810000 / 128 KiB. These are board constants,
not shared application assumptions. Both Normal slots must match before OTA.

The old layout needs one [USB installation](USB_INSTALL.md), with a verified full
16 MiB recovery backup. OTA never rewrites the partition table, bootloader, NVS or
Simulator. Failed/interrupted downloads leave the running firmware selected.
The pinned SDK enables boot rollback; startup marks a new image valid only after
hardware, BLE (Normal) and UI initialization. This is startup validation, not a
physical motor test or a claim that every runtime defect can roll back.

Normal records its current slot in local NVS for Simulator return. Simulator is
an explicit one-session mode and selects the saved Normal slot for the next boot.
No load intent, resistance setting or pending request is restored after updates.

## Existing owner bootstrap

The working local build has ignored diagnostic bootstrap fields. Public builds
explicitly exclude `LocalConfig.h`. A local USB migration build can retain those
fields in the remote's NVS; public updates read that device-local record without
embedding it in any release. Failure to save stops migration startup. Never upload
local firmware, flash backups or that header. Fresh owners still use the generated
identity path, whose physical pairing is **UNVERIFIED / unsuccessful in earlier
trials**. Publication does not resolve that protocol/provisioning issue.

For this existing owner's first USB migration, build `-Target Normal` as well as
`-Target Release`, then use `tools/Flash-Guition.ps1 -ConfirmUnloaded` after actually
unloading all VOLTRAs. The wrapper prepares an ignored private USB bundle containing
the local Normal image. `-PublicFirmware` explicitly selects the public image.
The public release packager never reads `.pio/build/guition_k5_normal`.

## Build and publish

Install Python and PlatformIO 6.1.19. On Windows use the project setup/build scripts
to avoid the Xtensa command-length limit:

```powershell
./tools/Build-Guition.ps1 -Target Release
./.venv/Scripts/python.exe tools/package_release.py
```

On Linux/macOS or GitHub Actions:

```sh
python -m pip install platformio==6.1.19
export PLATFORMIO_CORE_DIR="$PWD/.tools/platformio"
python -m platformio run -e guition_k5_release -e guition_k5_offline
python tools/package_release.py
```

`package_release.py` rejects wrong descriptors, lengths and partition maps. It
creates the OTA manifest/image, USB installation bundle and object/ELF archive for
relinking with modified dependencies. Sources and pinned dependency versions are
public; LGPL/Apache/MIT dependencies retain their terms. Local config is never an
input to the `guition_k5_release` environment.

Push reviewed source, bump VERSION, and push matching tag `vX.Y.Z`. The release
workflow compiles and packages from that exact tag, then creates a draft release.
Review its assets/release notes and publish it to offer the update. Do not replace
assets under a published tag; use a new version. Pull requests get compile/package
checks, with no publication permissions. Hardware cannot be validated by CI.

## Evidence and deferred checks

CONFIRMED source design: compile-time simulator isolation, explicit install intent,
TLS checks, separate A/B slots, current image retained until verification.
Build/package/CI results are recorded in BASELINE.md after execution.
UNVERIFIED until physical trials: startup HTTPS/time synchronization on the board,
first USB migration, actual OTA transfer/reboot/rollback, power-cut recovery,
mode switching across an OTA and retained local bootstrap connectivity.
No autonomous resistance tests or new automated test code were added.

References: [Espressif IDF 4.4 OTA](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/api-reference/system/ota.html),
[GitHub release links](https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases).
