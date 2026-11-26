# Twin Load restart and diagnostic report

CONFIRMED (owner and report `4a057f17`, v0.4.18): with Red and Black paired,
Red was the visible choice. The remote connected and could change weight.
The Load tap was followed by a black screen and Searching. The report logged
three new radio establishment failures (stage 3, code 574 / HCI 0x3e). The
report's current-boot ring did not include the preceding Load action.

CONFIRMED (owner's board): a read-only check found a new CRC32-valid ESP-IDF
core dump that was absent from the verified pre-v0.4.18 backup. The crash
fingerprint matches the published Normal firmware's ELF digest prefix
`bbfc1be18b2e60ee`. The relink archive also matched the release asset SHA-256;
the full Normal ELF digest matched the official app descriptor. The crashing
FreeRTOS task was `loopTask`, with a debug exception near the bottom of its
8 KiB stack. Retained return addresses include the actual Weight tap path
through `UiController::act`, `TrainingApplication::toggleWeight`, and Twin
identity resolution. The faulting store lands inside the SDK's configured
32-byte end-of-stack watchpoint. This identifies stack exhaustion on the tap
path. The dump does not establish that a motor write was received.
The raw dump and stack stay in ignored local storage.

The Guition board now overrides the pinned Arduino loop-task default with
16 KiB for Normal and Simulator, keeping the SDK stack checks enabled. No
changes to Load/Unload encoding, readiness, retry or pairing are warranted
by this crash. Physical Twin loading after the fix is UNVERIFIED.

## Independent Twin cross-check

- [Steven, `1c14860`](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L222)
  captures an already-paired Twin using a single controller session. The
  controller applies per-unit weight and the native `0x0101`/`0x0100` motor
  writes. The visible advertisement alone does not identify the active role.
- [Albert, `c4f2884`](https://github.com/albertzhang86/voltra-remote/blob/c4f2884798941babd83788cf79609620e767bc85/firmware/VoltraKnob/src/link/voltra_link.cpp#L639)
  independently uses one controller and validates the current peer via A7.
  Its load worker uses new cable and settings observations and a distinct
  captured motor profile. That profile is not evidence to replace the existing
  native Twin command selection after a failed attempt.
- The remote resolves Twin via matched A7 topology and remembered/scanned
  names. Steven's `A9 01` note names a peer query but does not give its reply
  format or alias rules. Do not guess the role or label from a lone advertiser.

The report now retains up to 40 current-boot events and a bounded crash status.
Increasing a RAM ring alone cannot recover the previous boot. Approved reports
still use the private inbox. At boot, one recognized crash summary is saved and
read back before the SDK clears the raw crash partition. This leaves at most one
replaceable summary for later Wi-Fi reporting without needing a USB connection.

CONFIRMED (inbox): the deployed validator accepted a synthetic 40-event report
and denied public GET access. The existing schema, payload size limit, and
owner-only report reads are retained.
