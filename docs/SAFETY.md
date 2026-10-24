# Safety contract

Current status: Normal firmware implements BLE connection, protocol bootstrap and
read-only observations. Explicit unload and guarded unloaded base/percentage edits
are implemented; load and Auto Load remain unsupported pending
a validated profile. The user-driven unloaded LB dial test returned matching
weights on both screens and retained motor state 0; no load test was performed.
See [current status](NORMAL_MODE_2026_10_17.md).
The separately linked simulator has no physical BLE implementation. Its loaded
state comes only from synthetic reports, including responses to mock commands.

Mandatory invariants from PROJECT_REQUIREMENTS.md:

- No automatic load on any startup, restore, reconnect, UI, mode or Twin event.
- Each future load needs an explicit current user action, validated connection/
  handshake, fresh confirmed mode, known capability/limit, known safe cable and
  readiness/interlock state, valid required Twin state and no conflicting intent.
- Unknown or stale required evidence fails closed. Observations have per-field
  timestamps. An unrelated notification cannot freshen a safety field.
- Auto Load/Auto Unload configuration is always read-only, including bootstrap.
  The user clarified on October 18 that enabled Auto Load is a preference; only
  the explicit long press arms its delayed load. The enabled preference itself
  must not block unloaded weight adjustment or be silently changed.
  The revised brief permits a one-shot Auto Load trigger from a long weight
  press. The new operation is semantic and currently implemented only by mocks.
  Existing device-side automatic behavior is not disabled by the remote; it
  requires understanding before allowing changes that could trigger resistance.
- No force/bypass loading, safety-skip, pairing or arbitrary parameter writes.
- Machine reports alone update confirmed state. Local draft and successful BLE
  transport completion never become machine confirmation.
- Preserve the weight touch target's unload priority under pending/error/stale conditions.
  Never block an unload solely because a load-readiness predicate fails.
  A disconnected or unimplemented remote cannot promise physical unloading.
- Automated physical motor engagement is forbidden. Tests/CI use offline mocks.

## Official behavior reference

[Cortex training documentation](https://cortex.beyond-power.com/docs/cortex/training)
requires unloading before mode changes and physical-risk acknowledgement before
loading; its force option bypasses a cable-zero check. This project excludes
that bypass entirely. The acknowledgement is a Cortex client-side concept and
is not proof of device-side readiness or a substitute for BLE state checks.

## Foundation limitations

Physical load/Auto Load requests remain unavailable. Explicit unload
uses the source-documented motor=4 write and requires a matching report; delivery
and physical behavior remain unverified. Offline mock commands support
delayed confirmation, timeout and priority unload cancellation of pending weight
or load intent. Tapping the bright weight requests unload; only confirmation
darkens it. Unknown state is explicitly amber. No separate STOP button is used.
Unloaded settings edits require fresh standalone/current mode/motor/unit/limit/
modifier/lock/OTA evidence and an explicit input. They send no motor command and
must be confirmed by a subsequent base-weight report. Full physical safety
predicates, command ordering and reconnect race handling
still require implementation and validation.
No physical training use is supported by this foundation.

The five-screen controller additionally implements local Stairs/RP reductions.
These require fresh normalized telemetry and an explicit live-weight capability,
never load a device, never go below one, and stop rather than retry unconfirmed
reductions. Cancellation prevents a failed unload from restarting reductions.
Selectable base weight now starts at one display tick (1 LB / 0.5 KG); explicit
loading also requires the device's separately verified loading minimum.

October 24: unloaded percentage modifier changes use sequential readback-confirmed
steps, writing only changed parameters. Reduce base/modifiers first, then make
increases. A one-modifier edit does not zero or restore the other modifier.
A timeout stops the remainder and
leaves the actual device reports visible; no automatic rollback or retry.
Unit/interpretation/maximum changes invalidate queued intent. Unload cancels the
whole queue immediately. A pending transaction cannot expire only in the UI while
its protocol writes continue. Failure messages survive unrelated reports.
Modern percentage fields are required on the current physical profile: legacy
fields returning zero must not be treated as evidence that modifiers are off.
Sparse rep/summary events never authorize motion-dependent drops or live writes.

The user now authorizes automatic Weight Training entry on connection. This is
one mode=1 request from fresh idle/unloaded state, not a load command, and its
confirmation is required. Unknown/stale, loaded/armed, child-lock, OTA and unknown
standalone context prevent entry. No retry after rejection/timeout, no automatic
unload, and no repeated mode entry when a user later exits the screen.
The local requested-weight preview never changes confirmed state or enables load;
it stays visually unloaded and carries a pending label until device confirmation.
Suppressing a spurious Twin banner does not weaken required Twin/state guards.
