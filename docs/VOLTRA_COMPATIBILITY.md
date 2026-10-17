# VOLTRA compatibility

No physical firmware version is supported/validated yet. Unknown capabilities
are unavailable. Normal firmware implements connection/read-only diagnostics and
explicit unload only; see [current status](NORMAL_MODE_2026_10_17.md).

CONFIRMED (source): the reference implementations share the 0x55 framing/GATT
family. Extended packet lengths and additional parameters are variations within
that family. INFERRED: one profile-aware implementation is appropriate. There is
no evidence here requiring protocol_v1/protocol_v2 trees.

## Discoverable evidence

Android's pinned `VoltraNotificationParser.kt`, `VoltraControlFrames.kt`,
`VoltraNotificationParserTest.kt`, and `paramInfo.csv` contain:

| Decimal ID | Report | What it does NOT establish |
| --- | --- | --- |
| 21537 | OVERDRIVE_AVAILABLE, uint8 | Permission to activate Overdrive |
| 21533 | OVERDRIVE_ACTIVE_STATUS, uint8 | A universally understood state enum |
| 21534 | OVERDRIVE_USER_CFG_FORCE_MAX, uint16 lb, catalog up to 250 | A verified Twin combined maximum |
| 21268 | EP_MAX_ALLOWED_FORCE, uint16 lb | Complete effective-load envelope in every mode |

CONFIRMED (source): Android tests exercise reported 250 lb capability. Ryan's
5–230 lb range is a separate beta assumption; HA uses 200 lb constants.
INFERRED: these reported fields are better discovery candidates than a fixed
firmware threshold. UNVERIFIED: precedence, freshness, entitlements, range and
units on the user's units. Do not transplant Android's fallback defaults or
automatically expand a limit based on one plausible number.

CONFIRMED (Android reports): 0x77 responses contain multiple module/firmware
strings. Record the complete module set, not just one app-visible version.
The October 17 [limit resolver](WEIGHT_LIMITS.md) now supplies a scoped nominal
firmware fallback for VOLTRA I 1.x/2.x, after applicable reports. It is used by
the offline adapter. Normal diagnostics decode limited physical reports, but
unknown Twin/readiness leaves the physical effective limit unavailable.

## Policy

- Capabilities are separate from current state, requested state and remote
  hardware capabilities. A currently reported mode is not proof of all features.
- Missing/unknown capability or maximum disables related controls.
- Weight Training is the product focus. Additional modes, device settings,
  cable adjustment, DTR, Overdrive activation and testing workflows are deferred.
- Chains/eccentric/inverse-chains require a verified effective peak limit, not
  just a base-weight clamp. Modifier reduction is implemented in the shared
  controller and simulator; physical command support remains pending.
- Unknown commands may have side effects. Do not probe by writing adjacent IDs,
  guessed selectors or beta flags. Catalog writable flags do not authorize use.
- Unknown safety behavior across firmware remains a release blocker for loading.

See [source pins](UPSTREAM_AUDIT.md) and [Twin limits](TWIN_MODE.md).
