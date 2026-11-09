# Phone to cloud to remote

The owner's workflow is to report a bug from the gym, have Codex edit this
repository in a cloud environment, build and publish a GitHub firmware release,
then install it over the remote's saved Wi-Fi. The home PC need not be running.
This is Codex Cloud, separate from pairing a phone to a desktop host.

## One-time cloud setup

In ChatGPT on the web or desktop, choose Work in -> Cloud -> Select environment
-> Create environment (also under Settings -> Codex Cloud -> Environments).
Connect GitHub, select `PrincessGolden13/Voltra-Remote`, and select Get started.
Have the setup agent follow this document and the repository's AGENTS.md. Review
its setup/build results and select Publish; wait for Environment published.
On the phone, open Codex -> Cloud and select that published environment.

The account needs access to Codex Cloud and this repository. Saving code and
publishing releases require the appropriate GitHub write access. Complete account
authorization in the app; never paste account tokens into this public repository
or chat. This document does not itself grant a cloud agent any extra permission.

Official references (checked November 9, 2026):
[Codex Cloud](https://learn.chatgpt.com/docs/cloud),
[environment setup](https://learn.chatgpt.com/docs/environments/cloud-environments),
[mobile](https://learn.chatgpt.com/docs/mobile).
Labels and availability can depend on the app/workspace.

## Prepare the Linux environment

Use Python 3.11, CMake 3.24 or newer, a C++17 compiler, Git and PlatformIO 6.1.19.
Allow dependency downloads from PlatformIO and GitHub. The existing GitHub Actions
workflow is the verified Linux build recipe:

```sh
python -m pip install platformio==6.1.19
export PLATFORMIO_CORE_DIR="$PWD/.tools/platformio"
python -m platformio run -e guition_k5_release -e guition_k5_offline -j 2
cmake -S . -B build/core -DVOLTRA_BUILD_SIMULATOR=OFF
cmake --build build/core --parallel 2
python tools/package_release.py
```

Keep the toolchain/cache in the reusable environment where supported. The Linux
build compiles the UI in both embedded targets. The graphical desktop simulator
currently has Windows/macOS display adapters only; do not claim a Linux visual
check from an offline-core compile. New test code remains deferred; run existing
validators where available and record gaps in docs/TESTING.md.

No USB, COM3, personal local config, saved Wi-Fi or flash backups are needed to
build public firmware. Never upload those private files. The existing owner's
USB migration stores the bootstrap profile on the remote; public updates read
that local record. Fresh-owner pairing is still an unresolved separate issue.

## A bug report from the gym

Open a cloud task for this environment and describe the observed behavior,
expected behavior, reproduction steps and installed remote version. Photos or
short video can help with UI bugs. Include the VOLTRA version and Twin state when
relevant, without private identifiers or passwords.

A reusable prompt:

> Fix this bug in PrincessGolden13/Voltra-Remote: [steps, actual result, expected
> result, installed version]. Read AGENTS.md and the current baseline. Preserve
> the resistance safety rules. Build both public firmware targets and the offline
> core. Explain the result and any checks that require my physical observation.
> Prepare the next firmware release for installation over Wi-Fi.

Cloud builds cannot physically inspect a remote or prove motor/protocol behavior.
Do not promise an immediate fix for bugs that require more physical evidence.

## Publish and install a fix

1. Review the fix and required checks; commit or merge it to `main`. Bump `VERSION`
   and add `docs/releases/vX.Y.Z.md` as part of the release change.
2. Push the matching `vX.Y.Z` Git tag. GitHub Actions builds both images, validates
   the package and creates a draft release. Follow the run through completion.
3. Review and publish that draft as the latest ordinary release. A cloud agent
   with suitable GitHub tools/access can do this when instructed; otherwise use
   GitHub in the phone's browser. A merged commit or unpublished draft alone is
   not offered to remotes. Never replace assets under a published tag.
4. Unload the VOLTRAs and leave the connection on the remote. Choose Settings ->
   Updates -> Check for updates, then Install & Restart. Keep power connected.
   Each power-on also checks saved Wi-Fi and offers a newer release; installation
   always needs the explicit tap. The remote downloads compiled firmware.

The one-time USB migration to the A/B layout is required on older remotes.
Normal updates thereafter preserve preferences and Simulator without USB. Changes
to the partition table/bootloader or severe recovery failures can still require
physical USB access; ordinary application fixes use the wireless path.

The repository carries engineering instructions and evidence into new cloud
tasks. This desktop chat and ignored local files are not automatically transferred.
