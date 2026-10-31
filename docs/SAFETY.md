# Safety contract

Current status (October 24): Normal firmware implements real single-device
settings, shared modifier units and explicit guarded loading/unloading. The user
confirmed remote-tap loading on Black after the mounting-state check. Auto Load's
one-shot trigger is implemented but not physically verified; local dropset
execution still requires telemetry and user verification. See
[current evidence and limits](DROPSETS_UNITS_2026_10_24.md).
The separately linked simulator has no physical BLE implementation. Its loaded
state comes only from synthetic reports, including responses to mock commands.

Mandatory invariants from PROJECT_REQUIREMENTS.md:

- No automatic load on any startup, restore, reconnect, UI, mode or Twin event.
- Each load needs an explicit current user action, validated connection/
  handshake, fresh confirmed mode, known capability/limit, known safe cable and
  readiness/interlock state, valid required Twin state and no conflicting intent.
- Unknown or stale required evidence fails closed. Observations have per-field
  timestamps. An unrelated notification cannot freshen a safety field.
- Auto Load/Auto Unload configuration is always read-only, including bootstrap.
  The user clarified on October 18 that enabled Auto Load is a preference; only
  the explicit long press arms its delayed load. The enabled preference itself
  must not block unloaded weight adjustment or be silently changed.
  The revised brief permits a one-shot Auto Load trigger from a long weight
  press. The physical path requires fresh enabled/safety/readiness reports and
  sends one trigger, without retries or setting changes.
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

## Historical foundation limitations

The following foundation description predates the October 24 guarded profile
above. Its unavailable-load and percent-only statements are superseded by the
linked current evidence; all no-bypass/no-retry invariants remain applicable.

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

Mountain selection requires fresh valid Mountain and inverse switch reports;
the previous special curve is cleared and confirmed before the other is enabled.
The separate accessory-display register must confirm percentage interpretation
for this physical profile. The old adaptation register alone is insufficient.
Unavailable dropset recipes may be edited locally but are stored disabled; their
presence on screen never authorizes live writes or automatic resistance changes.

Later October 24 dropset follow-up: the profile minimum supersedes the historical
one-tick base floor above (confirmed Black minimum 5 LB; native 2 KG evidence is
documented separately). After the final reduction is confirmed, floor/RP-quota
exhaustion leaves automation dormant until unload/rearming. It does not resend
the minimum, retry a load, or unload automatically. The user-authorized cumulative
dropset counters are local presentation only; automatic changes still use raw,
fresh completed-rep/motion evidence and readback-confirmed settings.

A confirmed unloaded exit from Weight Training now clears pending intent and
ends the BLE session so the UI can return to device selection with a reason.
The unloaded motor observation must be at least as recent as the mode observation.
Loaded/unknown state retains the unload target, and a later connection cannot
restore or replay any load, drop, or settings intent.

RP/rest follow-up: confirmed workout status 3 ends the logical set and resets
the recipe quota even without unload. A weight-induced statistics close during
active status 2 does not. Rest cancels any pending automatic base target; it does
not restore the starting weight or load. A brief RP motion gap clears the hold
and requires fresh movement plus a complete new hold; beyond 2000 ms RP stops.
Each drop still requires motion no older than 500 ms, fresh control context and
confirmed preceding settings. Stairs keeps its strict completed-rep sequence
guard. The expedited base-only path retains those guards and never changes
Magic/EO units or values, retries loading, or forces immediate cable engagement.
The visual target cue follows readback, not GATT success, and does not establish
that measured force has changed during an unfinished rep.

October 31: the motor register is retained as the full reported uint16, but
the supported 0x00xx / 0x01xx variants share a low-byte mode decoder, matching
the pinned Android and ESP32 references. Black's 0x0100 report previously
invalidated an otherwise accepted Weight Training connection. Other upper bits
and unknown modes remain blocked; no generic truncation is used. Presentation,
readiness and command confirmation use the same interpretation. Raw flags are
never copied into outgoing commands, and decoding an unloaded report does not
itself authorize loading. All existing fresh-state, lock, cable-zero, activation,
standalone and explicit-action requirements still apply.
