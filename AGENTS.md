# Standing engineering rules

Read `docs/PROJECT_REQUIREMENTS.md` for the complete user brief. Own development,
debugging, documentation, builds and Git; request human help only for inaccessible
information, permissions or physical observations/actions.

- Core scope: Weight Training controls. Device settings, cable length, DTR,
  isokinetic testing and additional modes are deferred scope.
- Keep UI -> controller -> requested/confirmed state -> protocol/GATT -> generic
  BLE transport boundaries. UUIDs, handshake, framing and parameters belong to
  VOLTRA protocol/profile; generic scan/connect/discover/subscribe/write do not.
- Isolate board GPIO, display/touch drivers, encoder direction and memory setup
  behind HardwarePlatform and logical display/touch/rotary interfaces. Future
  hardware must share the application. Derive layout from display metrics; no
  fixed 480x480 assumption. Validate required hardware capabilities.
- UiState is local presentation; RequestedState is pending intent; VoltraState
  and TwinState are machine-confirmed only. BLE write success is not confirmation.
- Separate VoltraCapabilities from HardwareCapabilities. Prefer device-reported
  capabilities over version inference. Unknown capabilities remain disabled.
- Twin-aware operation is required; pairing/unpairing and role changes are outside
  scope. Do not assume doubling limits, halving weights, duplicate commands or
  two BLE connections. Require evidence for combined limits and command routing.
- Never load on boot, reconnect, restore, mode change, wake or Twin transition.
  Every load requires a current explicit user action and fresh, known-safe machine
  state, readiness, mode, capability and required Twin peer state. No force/bypass.
- Auto Load and Auto Unload configuration is strictly read-only in every path.
- Keep unload immediately accessible, including while requests are pending or
  state is stale. Never claim a disconnected remote can physically stop a device.
- Initial setup must not enable or physically test resistance commands. Automated
  tests use mocks/fixtures only and never intentionally engage a physical motor.
- Isolate simulator builds from real BLE at compile/link time.
- Model independent device sessions without confusing multiple devices with Twin.
- Keep reference repositories outside this source tree; record upstream commits
  and evidence. Preserve a verified build baseline before major refactoring.
- Label findings CONFIRMED (with scope), INFERRED or UNVERIFIED. Source evidence
  does not prove physical compatibility. Do not invent packet fixtures or passing
  tests for unimplemented features.
- Add meaningful regression coverage for safety/protocol/state/Twin/reconnect/
  hardware bugs when tests are authorized. The brief explicitly defers creating
  new test code right now; run existing upstream validators and track future cases.
- Keep secrets and unnecessary device identifiers out of Git. Use ignored local
  configuration. Group commits logically; no destructive Git operations.
- Name and document necessary constants, units, provenance and tunable policies.
