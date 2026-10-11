# Validation status and pending tests

The explicit instruction under REQUIRED TEST CASES defers new test code for now.
Existing upstream validators are run; no artificial coverage or passing CTest
placeholder is supplied. This is not a tested production control implementation.

## Existing upstream results

- PASS: Ryan `tools/check_packets.py`: 203 SDK packets, generated weight range
  5–230, all 593 pinned modifier packets. This checks bytes, not safe resistance.
- PASS: Ryan `tools/check_upstream.py <SDK checkout>`: exact pinned commit,
  identifiers, customized identity, init/STOP/SETUP/GO, weight table and guided
  capture consistency. Starter pins the current audited SDK commit.
- PASS: Node SDK `vitest run src/voltra/protocol/__tests__ --maxWorkers=2`:
  **229 tests in 10 files**, Vitest 4.1.11, Node 24.19.0. No radio was used.
  The bundled runtime lacked npm, so pnpm 11.19.0 resolved upstream package
  ranges with scripts disabled in the external checkout; this was not a frozen
  npm-lock install. The resulting pnpm lock remains with that checkout.

Android tests were inspected but not executed; no Android/JDK toolchain was
installed. HA was inspected rather than installed as a runtime service. SDK
hardware scripts and automated motor tests were not run.

Build/render results are recorded in BASELINE.md after execution.

## Five-screen update

Native build succeeds with the expanded controller, mock and shared UI.
Offscreen LVGL renders exercise all five screens and both dropset editors.
These are layout checks, not passing interaction or safety tests.

Windows Computer Use was attempted for native interaction checks. Window
capture failed twice with `SetIsBorderRequired failed: No such interface
supported (0x80004002)`. The custom LVGL surface exposes only a pane to UI
Automation, so accessibility does not provide an alternative interaction oracle.
New test code remains deferred under the original instruction.

Manual checks still required: four-entry MRU across relaunch, five-second
availability changes/offline no-op, short/long-press exclusivity, all screen
navigation, mode value transfer, unit/envelope edge cases, zero selection,
Auto Load cancellation and ignored responses, duplicate/gapped reps, RP hold
rearming, confirmed drop counts, and stale/disconnect/unload cancellation.
F7–F11 provide offline inputs for these checks; no physical motor is involved.

## Future regression matrix (pending, not implemented)

| Area | Required cases |
| --- | --- |
| Framing | Known captured vectors; both CRC failures; ordinary/extended length; every fragment split; coalesced frames; noise; truncation; buffer bound; partial expiry |
| Parameters | Signed eccentric; unknown widths/IDs; bad count; invalid battery; no partial merge; actual read/ACK envelopes |
| State | Write success never confirms; wrong/late readback; stale individual fields; disconnect/reconnect generation; failed/time-out intent |
| Safety | Every missing/unknown/false load guard; boot/reconnect/wake/mode/Twin never loads; no force path; Auto Load/Unload writes absent |
| Unload | Available while loaded/pending/stale/error; priority queue; confirm physical report; disconnected feedback |
| Limits | Verified effective maximum; reduce overload modifiers first while increasing base; stop at valid envelope; unknown max disables control |
| Capabilities | Old/stable/beta/unknown reports; unsupported options hidden; no guessed MTN version cutoff; read-only feature discovery |
| Twin | Current role vs historical role; peer stale/lost/incompatible; combined max/routing; no half/double/duplicate assumption |
| UI/hardware | 360/466/480 layout; circle margins; touch targets; positive clockwise events; press debounce; board capability rejection |
| Sessions | Slow/rejected/missing ACK; multiple independent devices; cross-device notification rejection; reconnect cancels intent |
| Telemetry | Reps/sets/force/velocity/position units; ordering; boundaries; separate peer freshness; malformed packets |

Reuse simulator mocks; introduce a deterministic MockClock for lifecycle tests.
CI and simulator targets must not link a physical BLE backend. Protocol/safety/
state/Twin/reconnect/hardware bugs need regression cases as implementation proceeds.

## October 11 review

Windows build and responsive offscreen renders pass (see BASELINE.md). These
are not interaction/safety test results. No new test code was added. Add future
coverage for mixed-unit envelope limits, preset unit migration/readback mismatch,
queued explicit edits versus unsolicited reports, and wake-input consumption.
