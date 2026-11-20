# November 20: one paired control and a quieter Twin screen

The owner's physical v0.4.9 feedback supersedes its Twin presentation: a pair
must appear and select as one logical VOLTRA. Overlapping named circles already
convey the relationship. Do not add a Twin status caption, a TOTAL LB heading,
a second Twin glyph over the training dial, or implementation details about
host/peer verification to the ordinary screen.

CONFIRMED (owner): Black is the host and Red the follower in the observed pair.
The earlier independent selection targets incorrectly let a person select Red
as a separate training device. Its varying highlight came from individual
advertisement availability rather than one pair-level presentation state.
The correction groups navigation through one remembered session endpoint. The
reciprocal pair names/IDs and shared selection endpoint are saved as navigation
metadata, with availability and freshness reset on restore. That metadata is
not a decoded live host role or command authorization.
Names/colors are not protocol identities and must never be hardcoded as roles.

The owner explicitly clarified that either icon must open the actual master for
any pair. Reusing the first session endpoint does not meet that requirement.
Publication of v0.4.10 is held; PR10 is a draft. The earlier checked artifact at
47f5f91 is only a build baseline and is not approved as master autodetection.

The generic selection contract separates a saved discovery candidate from
current role evidence. Selection uses read-only sessions and waits for fresh
standalone or current-master evidence before opening training. It can inspect
the exact identified peer sequentially, without assuming that an unknown local
role is a follower. Unknown roles must not fall back to the cached candidate.
Only the simulator has explicit synthetic role fixtures at this stage. The
physical backend continues to report Unknown; no exclusive current-role decoder
has been established. This is an intentionally unfinished release, not a claim
that the user's pair will now resolve or that every Twin firmware is supported.

Read-only probe mode permits the mandatory session handshake and documented
read queries, blocks motor/settings writes and automatic workout entry, and
survives transport resets. Selection checks no more than two exact saved/scanned
endpoints, validates reciprocal identity after switching, and fails without
opening a follower or unknown role. Current role/topology loss revokes a resolved
Twin route and cancels pending intent; it cannot select a new master by fallback.
No role, readiness or resistance intent is saved. Ordinary stale standalone
readback retains the established unload path; it cannot authorize a new load.
Any future physical role decoder must bind its evidence to the current matched
pair identity/epoch and clear it on changes; merely fresh role and topology
timestamps are not sufficient. Exact positive host motor captures are not an
exclusive role decoder for all firmware and were not installed as one.

The pair's two-pound combined-LB increment is supported by the published
per-unit one-pound setting encoding. A minimum adjustment step is separate from
the minimum selectable base resistance and the effective maximum. Never double
a single-unit maximum to manufacture a combined capability; use a verified,
scoped device-reported limit when available.

Control support remains a separate open issue. The newest stored Twin report
when this follow-up started was still 4ac675b8 from v0.4.8. It does not contain
the A7 and optional diagnostic fields added in v0.4.9. The owner's role
observation establishes the desired navigation endpoint, but does not define
motor A003, live peer safety or the scope of maximum register 21268. Unknown
motor flags are not masked and no force command bypass is introduced. A fresh
v0.4.9 report was requested while independent corrections continued.

Build/render evidence and any subsequently received report findings are recorded
in TESTING.md and BASELINE.md. No physical motor command or new automated test
source is authorized by this UI follow-up.

An additional source audit found albertzhang86/voltra-remote at
c4f2884798941babd83788cf79609620e767bc85. Its exact motor helpers also do not
decode A003 or justify masking 8000. Its historical role-gate regression explains
why LAST_COWORKER_ROLE must not be required as a proxy for a live host role.
No new combined maximum or peer lock/status decoder was found. Separately, its
real runtime captures disprove 16003 as the Live target-resistance source; see
[the read-only target correction](LIVE_TARGET_SOURCE_2026_11_20.md).
