# Upstream audit — 2026-09-06 (Mountain time)

Follow-up: [SDK 0.15.0 applicability review, September 20](SDK_REVIEW_2026_09_20.md)
records the user-supplied 1adf43b commit, fixes already covered by our C++ subset,
and remaining physical-protocol uncertainties. The original pins below remain
the historical baseline, not a claim that 0.12.3 is the latest SDK.

The complete user brief is preserved in PROJECT_REQUIREMENTS.md. Repository was
empty (unborn master, no remote, no configured Git author). Research checkouts
are outside the application, under `%TEMP%/voltra-reference-audit/`.

## Pinned sources

| Source | Reviewed commit | Key areas inspected |
| --- | --- | --- |
| [Ryan starter](https://github.com/RyanMarkoff-eaton/voltra-diy-remote/tree/f0dffbb5ba15eeaa64c8c713a1d7291dfeb75a1a) | f0dffbb5ba15eeaa64c8c713a1d7291dfeb75a1a | PlatformIO targets, README, VALIDATION, packet data, validators, four-commit history |
| [Node SDK](https://github.com/HJewkes/voltra-node-sdk/tree/a53804e46c87a9064bc776d0e9ecedc2e66867fe) | a53804e46c87a9064bc776d0e9ecedc2e66867fe | Changelog through 0.12.3, protocol history, CRC/builder, BLE constants, tests, adapter isolation |
| [Home Assistant](https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant/tree/93b0819745e665ab4d35f4cc501d48c6296c81f7) | 93b0819745e665ab4d35f4cc501d48c6296c81f7 | protocol.py framing, parameter table, compute_safety, models/client, recent history |
| [Android](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/tree/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb) | da514d2db1a16ebb4ded3156bc8fd200bed1e6eb | Protocol notes, static-analysis report, parameter CSV, SDK research notes, Overdrive parser/tests, relevant history |
| [Cortex](https://cortex.beyond-power.com/docs/cortex/training) | Live documentation accessed 2026-09-06 | Getting started, training/device control, common flows |

CONFIRMED here means a statement checked in the pinned source, a locally executed
build/validator, or official documentation as explicitly indicated. It does not
mean this project's physical remote or VOLTRA has been validated.

## Important differences

- CONFIRMED (source): Ryan already pins the current SDK commit above. The initial
  suspicion of an older SDK snapshot does not apply to this checkout.
- CONFIRMED (source): SDK history corrects a false battery interpretation,
  reclassifies SETUP, removes PREPARE's workout-state overwrite (a5e5829), and
  renames direct-load status fields to safety-check fields (0cbf83c). These are
  reasons to interpret state rather than reuse command names literally.
- CONFIRMED (source): SDK 0.12.3 fixes mock entry points that could instantiate a
  real Web Bluetooth adapter. Our simulator links only its mock adapter.
- CONFIRMED (source): SDK 0.12.0 addresses multi-device scanning/routing and
  user-renamed devices. Do not rely exclusively on the VTR- name prefix.
- CONFIRMED (source): Android has Overdrive report parsing and tests in addition
  to coworker metadata absent from the starter's application model.
- CONFIRMED (source): Home Assistant's load gate only rejects some lock/battery/
  OTA flags when explicitly true. Our required unknown-fails-closed policy is
  stricter; that gate must not be copied unchanged.
- UNVERIFIED: completeness of Android's referenced private captures and binary
  exports. The public notes describe artifacts that are not all in the public
  repository. No private/official-app binary was downloaded or redistributed.

See PROTOCOL_NOTES.md, VOLTRA_COMPATIBILITY.md and TWIN_MODE.md for decisions.
This is a targeted foundation audit, not an exhaustive validation of all code or
all upstream history. No upstream application is a runtime dependency.
