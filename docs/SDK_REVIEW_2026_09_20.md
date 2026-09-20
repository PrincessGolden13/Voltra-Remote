# Node SDK 0.15.0 review — September 20, 2026

The user supplied [commit 1adf43b](https://github.com/HJewkes/voltra-node-sdk/commit/1adf43b7dbe5cc96d4422dfb79d067b115fb4e9b).
CONFIRMED (source): this September 18 commit changes README acknowledgements
and the changelog, not executable code. It describes fixes shipped in 0.15.0
on September 17. Our previous SDK reference was 0.12.3 at a53804e.

Reviewed the linked commit, its pinned [CHANGELOG](https://github.com/HJewkes/voltra-node-sdk/blob/1adf43b7dbe5cc96d4422dfb79d067b115fb4e9b/CHANGELOG.md),
and the implementation/test diff in [parameter-decoder PR #119](https://github.com/HJewkes/voltra-node-sdk/pull/119).
This is a targeted applicability review, not a complete SDK re-audit.
Upstream capture claims below are upstream evidence, not physical validation here.

## Effect on this project

We do not import or execute the Node SDK. The C++ application has its own
decoder and mock-only ControlSession. There is no installed SDK dependency to
bump. No executable change was identified as necessary within the inspected
implemented subset. The new findings update the requirements for physical BLE.

| Upstream finding | Our implementation / required follow-up |
| --- | --- |
| Guided-load exit targeted the wrong setting | We have no physical guided-load builder. Do not copy the old builder when implementing physical release; verify the actual unload report. |
| Inverse chains confused a selector with a weight | Still unresolved upstream. Our IC value is a synthetic UI setting, not a validated wire representation. Verify selector meaning, width and magnitude mapping independently before physical writes. |
| Connection declared before device acceptance | Our connect() establishes transport/discovery/subscription only; readiness is separate and supplied solely by the mock. Physical acceptance/refusal handling, timeout and a fresh post-acceptance state read are still required. |
| Cached state reused after reconnect | VoltraController clears confirmed/requested state and parser data on connection changes; physical session validation remains unimplemented. |
| Write completion treated as a confirmed stop | Current controller waits for a subsequent mode report; sending unload does not set the observed mode to unloaded. Physical stop builders remain absent. |
| Notifications assumed to contain whole frames | FrameParser already buffers fragments, handles multiple frames and validates header/frame CRCs before dispatch. Future additional channels need separate buffers. |
| Wrong parameter widths / signedness | Parameters.cpp uses explicit widths, reads eccentric as signed int16, and rejects unknown/truncated/trailing entries atomically. PR #119 addresses the analogous SDK defect. |
| Auto-load phase/status interpretation | Still unresolved upstream and firmware-dependent. Our mock Auto Load behavior is not proof of a physical status mapping. |
| Heartbeat field meanings | Physical decoder absent. Upstream now describes per-rep mean forces, mean return speed in mm/s, and a relative set accumulator. These are not live force, live velocity or target weight. |
| Set summary duration / peak power | Physical decoder absent. Upstream describes total pull moving time across a set and four-byte peak power with unverified units. Do not label it per-rep duration or watts. |
| Truncated rep/set counters | Our synthetic rep counter is uint32; no physical rep decoder exists. Preserve both wire bytes and validate resets/gaps when implementing it. |
| Misnamed report families | Rowing/isometric parsing is outside our scope. Do not reuse the removed inferred layouts. |
| Battery report routing | Our decoder updates VOLTRA battery from explicit parameter IDs, not frame length. The UI footer separately represents remote battery; its simulator value is synthetic. |

## Deliberate differences and open work

- Upstream preserves entries before an unknown/truncated parameter with
  `complete: false`. Our decoder continues to reject the entire update so a
  malformed tail cannot freshen safety state. A broader catalog must retain
  this policy unless an explicit design review changes it.
- The new upstream catalog is not uniformly capture-proven. PR #119 calls the
  inverse-chains width hypothesis-grade and identifies uncatalogued parameters.
  Do not treat generated metadata alone as physical compatibility evidence.
- SDK stop handling may re-read state once. This project retains the user's
  no-automatic-correction/no-retry behavior; no such SDK behavior was imported.
- RP hold detection needs current cable motion observations. A repeated
  per-rep mean speed cannot establish that the cable is currently stationary.
  Verify a suitable live stream, timestamps and thresholds before enabling
  physical RP. Stairs likewise needs verified distinct completed-rep events.
- Device limits, compact runtime telemetry, IC and Auto Load interpretation
  remain hardware/firmware validation items. Simulator percentage settings,
  combined envelopes, MTN and Twin behavior remain explicitly synthetic.

## Validation

CONFIRMED by source inspection: the existing protections described above are
present. No code or dependencies changed, so no build or new tests were run
for this documentation-only review. Upstream test counts are not our test
results. No physical motor operation or BLE session was performed.
