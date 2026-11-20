# Validation status and pending tests

November 20 v0.4.10: [one paired control and quieter screens](TWIN_PRESENTATION_2026_11_20.md)
and [reported target-resistance correction](LIVE_TARGET_SOURCE_2026_11_20.md).
UNRELEASED: the owner requires actual current-master resolution for every pair.
The physical decoder remains unavailable; PR10 stays draft and publication is held.
The later generic router is checked with explicit synthetic roles only.
PASS: existing offline core/packet inspector build and pinned Ryan validators
(203 SDK packets, weight5..230,593 modifier packets, SDK identity consistency).
Four published CRC-valid independent B4 captures decode a6LB reported target;
the published Twin-source frame is rejected by the independent target decoder.
No new automated test source or physical motor command is added.
PASS: actual LVGL Main/Devices/unknown-motor renders at360/466/480 show names-only
headers, no caption/banner/dial overlap and matched availability. Synthetic
pointer previews on both paired circles at every size open the same endpoint.
Live renders at all three sizes cover reported28 versusBASE25, zero, absent,
stale-- andKG. These are offline visual/source checks, not physical compatibility.
Independent static review checks pair navigation versus current authorization,
absolute two-pound grids, target context/freshness and rollback-readable storage.
PASS: actual DesktopDeviceMemory preview across separate processes restores one
pair, with connection/readback/freshness/readiness absent; either side selects the
same candidate. Subsequent role-aware LVGL previews use Orchid/Slate aliases with
no pair catalog: follower-first probes follower then master read-only, master-first
probes master, Unknown probes each endpoint once and fails, and Cancel disconnects.
At360/466/480 only fresh explicit synthetic Master reaches training. These previews
do not establish a physical role decoder or authorize any physical Twin command.
PASS: independent static review of the later role-aware framework checks bounded
reciprocal probes, dynamic readiness, cancellation, the physical query allowlist,
no automatic workout entry during discovery, and the mock's role/identity guard
before queued mutation. Physical role stays Unknown and cannot promote a Twin.
PASS baseline only: [CI44763541029](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44763541029)
at47f5f911e649db300825581e7919c5e7be0b756d builds both public images and packaging.
Artifact14643255564 digest matches
`c8e9ede972fcd9ed88b0a087ffd45a6b66b73a141b690716a54075c773105ae1`;
Normal1,977,088bytes SHA-256
`9bcaccceb3445dcaa3aa97858a1dbacb20cf92f781a62763f176f2a93962bf65`,
Simulator1,006,144bytes SHA-256
`77d65850155b7341205b3a20058f09f48954b562a739016db3b3e380e8522e80`.
Descriptors, manifest, USB hashes/offsets/partition map, USB/OTA equality, backend
isolation and strong NVS/rollback hooks pass. This artifact predates the subsequent
role-aware router/probe changes and is not approved as master autodetection.
PASS held-draft framework: [CI44766603773](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44766603773)
at ea1552bb28e0e7d0cdf36e02de36fa318e2e5379 builds public Normal, isolated
Simulator, offline core and packages. Artifact14644108166 digest matches
`b802a17b4cea580ebefe4c9fb216fcc662e389f378d87402a7ba93873b57a27c`.
Normal1,983,376bytes SHA-256
`00f34c8251dedbf6799ebf1f8c1a4651174a2a90ec98a39eaf5ab525e6a3b9fd`;
Simulator1,012,576bytes SHA-256
`266af4f2aa05aaedea809c49de4a791b94c2e8c294ad5d06c785b03bf352c1bf`.
Both descriptors, manifest identity/length/hash, all USB hashes/offsets/partition
map and USB/OTA Normal equality pass. ELF defined-symbol checks confirm physical
backend only in Normal, mock only in Simulator and both strong NVS erase-range
protection/verifyRollbackLater hooks. Receipt: ignored build/review-v0410-role.
These checks approve the draft's build/artifacts, not physical master detection.
PR10 stays draft; no merge, release tag or publication was performed.
Physical Twin adjustment/load/unload remains unresolved. The latest stored Twin
report still predatesv0.4.9, and A003, peer safety and a scoped combined maximum
remain unknown. Device-reported B4 target compatibility and exact agreement with
the owner's changing VOLTRA LCD still require a physical comparison.

November 20 v0.4.9: [Twin/readback, reconnect and Live changes](TWIN_LIVE_READBACK_2026_11_20.md).
PASS: changed offline core/packet inspector with CMake 3.31.6/GNU 14.2.0, existing
Ryan packet validator (203 SDK packets, weight 5..230, 593 modifier packets) and
pinned SDK consistency validator. The new strict A7 inspector accepts five full
published source fixtures and rejects the published short fixture without identity.
These are source/decoder checks, not physical compatibility evidence.
PASS: existing stevenpoint watch206 native protocol suite 37/37, including its A7
fixtures/Twin mode constants. This validates that upstream source, not our state
machine. Native syntax checks cover PhysicalSession and UI; embedded compilation
is validated separately by CI. No new automated test source is added.
PASS: actual LVGL headless renders at 360/466/480 inspect Twin/current/unknown/stale,
long peer names, Black/Red Venn selection, 130/150/50 combined readback with unknown
motor and unknown maximum, and Live 28 versus BASE 25, zero, missing/stale -- and KG.
Independent static review corrected stale topology recovery, contradictory reports,
own-identity mismatch quarantine and scope expiry between callbacks.
Physical checks remain: A7 compatibility/name matching, quick reselection, direct
16003 agreement with VOLTRA's changing LCD and report delivery. Physical Twin
role/peer safety/limits/A003 are unresolved; this release does not enable controls
or claim the user's Twin adjustment failure is fully repaired.
PASS: [CI44755705290](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44755705290)
at d29a5780bb82761d8f85b93a50afdae1901c1f87 builds public Normal, isolated
Simulator, offline core and packaging. Downloaded artifact digest matches GitHub:
`df0451f0fd5b4eb2e50f53ae406cff313fbc8108e08717b2c01f56ddb57a61ad`.
Normal is 1,968,944 bytes, SHA-256
`ac20394991e42991c3bfcc9c27f9e0d1e220f6cb60660b6c4b0ca670fe4d4df5`;
Simulator is 999,856 bytes, SHA-256
`016369d5c84d4237d4b97fc027dd509a3edefbecd35d1e142f1d2b9674986357`.
Both descriptors, manifest identity/size/hash, USB images/hashes/partition map and
USB/OTA Normal consistency pass. ELF defined symbols confirm backend isolation
and strong NVS protection/deferred rollback hooks. Merge325bf0a matches the checked
tree. Artifact receipts are kept in ignored build/review-v049.
PASS publication: [tag build44756585032](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44756585032)
at v0.4.9 /d84fb4b18c12520e6ec8dcac57b0164593bb783e passes public Normal,
isolated Simulator, offline core and packaging. Downloaded artifact digest matches
GitHub: `1437d4b2fa391cae72053f3bb92fa5df24819cfba5aeb1c83f00e2c319f5241d`.
Both descriptors, manifest, USB images/partition map and USB/OTA consistency pass;
Normal and Simulator images exactly match the reviewed build above. Tag receipts
are in ignored build/review-v049-tag. Release v0.4.9 was published as the latest
ordinary release on November 20 at 14:51:17 UTC under the owner's standing request.
The downloaded public latest-feed manifest and pinned v0.4.9 image pass board,
channel, layout, version, descriptor, length and SHA-256 checks; the firmware
exactly matches the reviewed tagged image. No physical installation or motor
trial was performed. The physical Twin/control and Live limits above still apply.

November 18 v0.4.8 follow-up: [Auto Load visual feedback](AUTO_LOAD_FEEDBACK_2026_11_18.md).
The owner confirms an actual Auto Load trial succeeds but remote progress/armed
feedback is missing. The new UI and read-only countdown changes were built and
checked separately; the earlier 7e13632 artifact predates them.
No new automated test source or physical motor operation is performed.
PASS changed offline core/packet inspector and existing pinned Ryan/SDK validators.
Real LVGL9.3 headless renders at360/466/480 inspect half/full touch hold, Starting,
armed guidance, received device countdown, release, cancellation and loaded return.
The temporary Linux adapter delegates to the existing shared SimulatorApp and
DesktopHardwarePlatform with synthetic pointer/time input; it does not establish
a graphical Linux desktop host or physical behavior. No clipping/overlap remains
in these scenes; cancel/loaded return restores modifiers and scale labels.
PASS: [CI44330719778](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44330719778)
at aae14dce22b6f38ff16fff3df1b23e4d06d156c7 builds public Normal, isolated
Simulator, offline core and release packaging. Reviewed archive digest matches
GitHub: `0e1282c2b67380f7a3bbe00d5db0c5abcc2ddbe17d61f17dab2ecd7bafa0e277`.
Normal is 1,951,136 bytes, SHA-256
`12d896fe75842b4e13a14f78ee2778d76c8ad905395d30f2f6e140d42c72ce61`;
Simulator is 987,488 bytes. Both descriptors, manifest length/hash, all USB bundle
hashes/partition map and USB/OTA Normal consistency pass. ELF inspection confirms
physical implementations only in Normal and mocks only in Simulator; both retain
strong NVS and deferred rollback hooks. Merge cfd23d2 has the identical tree.
These are offline source/build/artifact checks, not new physical verification.
PASS publication: [tag build44342205239](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44342205239)
at v0.4.8 /89d1dcac168cac133baa1b5e0152e73447c254da passes public Normal,
Simulator, offline core and packaging. Downloaded artifact digest matches GitHub:
`e72b01f99d31f566ea66c069c35fab76ddcab7d8467a324509cb4c0c0af76556`.
Both descriptors, manifest, USB/OTA consistency, backend isolation and strong
NVS/rollback hooks pass; Normal and Simulator hashes match the reviewed build.
The owner approved publication in chat. Release v0.4.8 is published as latest;
the public latest-feed manifest and version-pinned image return0.4.8 with matching
identity, length and SHA-256. No physical installation or motor trial was performed.

November 18 v0.4.8: [unload/reset report 5a65d590](UNLOAD_RESET_2026_11_18.md).
PASS baseline: offline core/packet inspector with CMake 3.31.6 and GNU 14.2.0;
existing inbound base-weight fixture accepts its valid CRC and rejects corruption;
pinned Ryan packet validator passes 203 SDK packets, weights 5..230 and 593
modifier packets; upstream validator against SDK a53804e passes. New test source
remains deferred. Final public firmware/package results are recorded above.
Physical unload/reset timing and subsequent Load/Auto Load remain unverified;
no physical motor command or USB operation is performed.

November 18: [hold duration, debug hint and report bb697e8f](HOLD_LOAD_2026_11_18.md).
PASS: local offline core/packet-inspector build, existing inbound fixture and
pinned Ryan validators (203 SDK packets, weights 5..230, 593 modifier packets,
SDK a53804e consistency). New test source remains deferred. Public Normal,
isolated Simulator, package and image results follow.
PASS: [CI 44321393881](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44321393881)
at 7948fdc28ad16c28af636b0a8d9a43569fb58ec6: public Normal, isolated Simulator,
offline core and release package. Downloaded artifact digest matches GitHub:
`2fab58410ea6dadb302c20b83b2b488dc530b763db79cbd1e10bc0f53d798710`.
Normal is 1,945,184 bytes, SHA-256
`389e00dc38527d4c838182778e5cfe69eb2e765f58bb5c6a7a5c8c2af5c72174`;
Simulator is 982,720 bytes. Image descriptors, manifest length/hash and USB/OTA
Normal consistency pass. ELF inspection confirms physical BLE/protocol/network
implementations only in Normal and mock transport only in Simulator; both retain
strong NVS protection and deferred rollback hooks. This is build/artifact evidence,
not a behavioral test of the new status predicate, touch logic or physical motor.
PASS publication: [v0.4.7 tag build 44322395772](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/44322395772)
at c0ba428206ec36c328fbf6d010fa5e846f5ddfd7. Tagged asset descriptors, manifest
length/hash and USB/OTA Normal consistency pass; Normal hash matches above.
The release is published as latest. Public latest-feed and version-pinned firmware
downloads were verified as v0.4.7 with correct identity, length and SHA-256.
No physical installation/load/hold trial was part of publication.
These checks do not establish physical load safety/compatibility. Required owner
observations: status 0 under otherwise-ready conditions, status 6 unchanged,
missing/stale/other status blocked, hold/release vs tap, delayed release processing,
preset override, wake/movement/stale cancellation, no hint before 3 s, hint after 3 s,
and a six-second debug hold without a preceding load. Live-write status policy
and active-mode 5 restrictions are unchanged. No physical motor or USB test ran.

November 16: [settings retention and Wi-Fi completion](SETTINGS_RETENTION_2026_11_16.md).
PASS final configured-network overview/ping/reset source:
[CI 43890790858](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43890790858)
at e1c1e637840d9b51588e36bbf2a514e9dee18b76: public Normal/Simulator, offline
core and release packaging. Downloaded artifact matches GitHub digest
`d95183fadee1a35b7bbf0a47c10bff4810ce5fefb142eef2524e2a97d53f7388`.
Normal is 1,944,336 bytes, SHA-256
`ee9ee18540014a36ede0db8661b8fc1cf8eaa738248bad19be552ef0bb622d68`;
Simulator is 981,904 bytes. Both image identities, manifest size/hash and USB/OTA
Normal consistency pass. Both ELFs retain the strong NVS/rollback hooks; only
Normal links esp_ping_new_session/start/stop/delete_session/get_profile and
physical BLE/OTA/debug networking. Simulator retains mock transport.
These are compiled artifact checks, not live ICMP or UI-interaction validation.
PASS publication: [v0.4.6 tag build 43891852006](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43891852006)
at 7cce754a82fcbe88bcddf8f959fa7d92599b0710; reviewed tagged assets retain the
same Normal hash above and pass identity/manifest/USB checks. The ordinary release
is published as latest; public manifest and pinned OTA image download were
verified with matching version, descriptor, length and SHA-256. No physical OTA
or live ping trial was performed.
Pending physical checks: current vs saved/disconnected SSID, opening before
reconnect, ICMP reply/no reply/start failure, leaving during a check, BLE pause,
reset during a check, completed setup showing only OK, and cancellation/failed
replacement keeping the prior network. The historical build below predates this
follow-up.
PASS: local offline core/packet-inspector build, existing inbound fixture, pinned
Ryan packet/upstream validators (203 SDK packets, weights 5..230, 593 modifier
packets and SDK a53804e). No new test source was added.
PASS: [CI 43885326367](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43885326367)
at 3b702fa49fb903c443d6e9ab6f614fb997b0c772: public Normal, isolated Simulator,
offline core and release packaging. Downloaded artifact matches GitHub digest
`e4afe8884262c4811839aa6fa46c6cf2a097a329879515a33f073735f6b03579`.
Normal is 1,937,744 bytes, SHA-256
`d2663a251a68cd874f490b5ddfb2306202d40744dec39db1038207d1991b4940`;
Simulator is 980,400 bytes. OTA/USB Normal images match; independent descriptors
and manifest size/hash pass. Both ELF files contain GLOBAL (not weak) functions
`__wrap_esp_partition_erase_range` and `verifyRollbackLater`; Normal/Simulator
backend isolation is retained. These checks establish compiled wiring and image
delivery artifacts, not execution of storage-fault recovery or UI interactions.
Physical settings survival, Wi-Fi OK navigation, failed saves and NVS fault/
rollback behavior remain unverified; no physical motor tests are performed.

November 15 debug flow: [changes and evidence](DEBUG_FLOW_2026_11_15.md).
PASS: baseline and changed offline core/packet inspector builds with CMake 3.31.6
and GNU 14.2.0; pinned Ryan packet validator (203 SDK vectors, generated weights
5..230 and 593 modifier packets) and upstream validator against SDK a53804e;
the existing SDK chains-report fixture passes packet inspection and a one-byte
CRC corruption is rejected. The repository's existing inbound base-weight fixture
also passes, and its corrupted CRC is rejected. `git diff --check` passes.
PASS: [GitHub Actions 43762944735](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43762944735)
at 3a71e948185eb4c5453bdc220f529cb32dfa5bb5: public Normal, isolated Simulator,
offline core and release packaging. Downloaded artifact digest matches GitHub:
`5a9d344ca4fae3c051cdfe186230f62896865c200d4b6b669b695e3cf9d52c74`.
Normal is 1,935,008 bytes, SHA-256
`21c2386b9c133ec1479dc21abe1fdfbc0c5b6348c175846e74b3f0f07dc97847`;
Simulator is 977,232 bytes. Both independent image identities match v0.4.5 and
their channels; Normal length/hash match the OTA manifest. ELF symbol inspection
confirms physical BLE/OTA/debug upload/certificate code only in Normal and
MockTransport only in Simulator. These are build/artifact checks, not execution
of the new interaction flow. No Linux visual/interactive pass is
claimed: the repository's desktop adapters require Windows/macOS. No new test
source or physical motor tests were added. Owner checks still needed: UNLOAD on
press/dial with pending or stale controls; confirmed vs failed/unconfirmed
feedback; modal input during underlying page transitions; SEND save failure,
pending-control handoff, close/reopen, missing Wi-Fi, upload failure/RETRY and
rebooted queue without a new disconnect; OTA preferences retention.

Publication follow-through: [v0.4.5 tag build 43766736376](https://github.com/PrincessGolden13/Voltra-Remote/actions/runs/43766736376)
passed at merged source 2e71053471b92d6e314e55388ec58e7437362672. Downloaded
draft assets pass Normal manifest hash/length, Normal/Simulator identities and
USB/OTA Normal consistency; the tagged Normal hash matches the reviewed image
above. The ordinary release is published as latest. The public update feed and
version-pinned firmware download were verified as v0.4.5 with matching image
identity, size and hash. This is delivery evidence, not a physical OTA/unload trial.

November 14 follow-up: [changes and evidence](BUG_FIXES_2026_11_14.md).
Future cases (new test code still deferred): Settings debug access; visible hold
countdown, 6+ second blank/control holds, drift independent of tap rejection,
controller 0xFB/0xFC ACK/readback, cancellation and wake-only input; no action
on release after a debug hold; preserved refusal snapshot after disconnect;
optional-parameter rejection must not erase required polls; direct motor 5 vs
bulk echo 1, stale direct motor and immediate single-field unload/unknown state;
no automatic reload after RP, and weight writes/readbacks distinct from force.
Keep load refusal and Unload readable on 360/466/480 displays.

November 9 bug follow-up: [source audit](PROTOCOL_AUDIT_2026_11_09.md),
[implementation/validation record](BUG_FIXES_2026_11_09.md) and
[diagnostic gesture/report cases](DEBUG_REPORTS.md). New test source remains
deferred. Physical Twin and the unrecognized void-lock state remain unresolved;
no hardware or resistance tests were performed for these changes.

November 9 rest completion follow-up: [behavior and hardware evidence](REST_ALERT_2026_11_09.md).
The factory timer's ability to vibrate is owner-confirmed. New DRV2605/2605L
hardware driving remains unverified on this remote. Check 20-second automatic
stop, early acknowledgment, persistent Rest over across navigation, no hidden
unload control, wake/dim/sleep, no boot buzz, Off defaults/reset, saved choices,
no repeat for one rest, later-set rearming and temporary stale telemetry.
CONFIRMED (cloud build): PlatformIO 6.1.19 / espressif32 6.12.0 builds of
both guition_k5_release and guition_k5_offline pass for v0.4.2. CMake 3.31.6
builds the offline core and packet inspector; the existing source-reported
base-weight fixture passes framing/CRC inspection. package_release.py validates
v0.4.2 (Normal image 1,899,568 bytes) and packages OTA, USB and relink assets.
git diff --check passes. The baseline offline core at 3bf2b14 was also built.
These checks do not execute the UI or prove motor output. Linux has no supported
graphical simulator adapter in this repository; no visual pass is claimed.
No new automated test code was added, per the master brief.

November 9 live-update follow-up cases (new test code deferred): signed GitHub
asset redirects longer than the IDF default send buffer; delayed DNS/TLS;
connection reset with manual retry; startup Wi-Fi before BLE discovery;
manual device selection while discovery is paused; stored bootstrap
profile loaded from a public update without embedding private fields.

November 9 update cases (new test code remains deferred): check once per boot;
offline boot and unavailable SNTP/TLS; interruption by manual BLE connection;
duplicate install taps; no install/load from startup, a reconnect or a check alone;
installed/same/older/newer versions; bad/truncated/oversized manifests; wrong board,
channel, layout and image descriptor; HTTP redirects to unapproved/non-HTTPS hosts;
size/hash mismatches; download cancellation/timeouts; power loss during flash;
startup rollback; Normal A/B to Simulator return; local bootstrap/preferences
survival; no radio/update symbols in Simulator; and no secrets in public artifacts.
Source checks/builds cannot establish actual OTA or physical interoperability.

October 24 idle-force regression (new test code deferred): with confirmed base
10 LB, fresh idle phase 0 and force samples 0–36 tenths LB at a stationary
148 mm cable, Live must retain 10. Active phases 1–3 use fresh reported force;
missing/stale phase or force returns to confirmed base, never a fabricated force.
Unknown core state remains unavailable. Check modifiers during active reps,
half-KG formatting, delayed set-close, unchanged drops/raw telemetry, first rep,
set summary and rest timer. The user confirmed the preceding rest-timer update.

October 24: [units, guarded loading and telemetry](DROPSETS_UNITS_2026_10_24.md)
supersedes the older unvalidated-training status below. The user confirmed
%/LB controls, remote-tap loading, return to Main, post-set counts and first-rep
tracking. Stairs/RP reductions, KG and Auto Load triggering remain unverified.
No new test code was created. Future regression cases: subscription masks must
not silently change the expected stream layout; sparse summaries cannot freshen
motion; duplicate set closes do not restart rest; no timer from load toggles,
settings, reconnect or old summaries; stale-state cancellation; new-set motion
ends rest; reported elapsed time takes precedence; alert fires once and wakes
the display; local elapsed time never becomes machine-confirmed state.

October 18: see [physical discovery observations](LIVE_CONNECTION_2026_10_18.md).
Discovery, GATT subscriptions, acceptance after user approval and read-only state
are physically established with the local diagnostic profile at ten feet.
Training is unvalidated. Queue the cases below; no new test code was added.

October 17 Normal-mode work: [implementation and blockers](NORMAL_MODE_2026_10_17.md).
New test code remains deferred. Queue future cases for zero/one/multiple completed
scans, late power-on, accepted-only persistence, failed GATT/approval, stale scan
refresh during a hold, forget/persistence failure, retained offline devices,
mode gesture versus page swipes, simulator-to-normal reboot selection, per-image
link isolation, notification overflow/generation, truncated read groups and
unknown Twin/readiness. Physical training is not validated.
Also cover immediate invalidation on unknown units/motor/workout mode, unload
from that state, and monotonic report revisions after valid state returns.

October 17: see [feedback changes and pending regression cases](UI_FIXES_2026_10_17.md).
Existing pinned Ryan packet and SDK-consistency validators were rerun successfully.
New UI/state tests remain deferred; builds and render checks are not interaction
or physical-resistance validation.

The explicit instruction under REQUIRED TEST CASES defers new test code for now.
Existing upstream validators are run; no artificial coverage or passing CTest
placeholder is supplied. This is not a tested production control implementation.

## October 18 connection checks

Normal, isolated Simulator and Windows native builds pass after the asynchronous
connection change; existing offline renders open Devices/Main/Twin successfully.
Physical captures reproduce error 574 and show a later successful radio link/MTU
negotiation. They exposed cached-only GATT enumeration; the correction now reaches
three services, three subscriptions, acceptance and live state. No autonomous
resistance command or new test code was used. Queue fresh empty GATT caches, bounded 574-only
retries, cancellation during connect/backoff/MTU, late callbacks before a different
target, no pre-subscription bootstrap, failed service/subscription feedback, and
missed-scan availability grace versus unchanged machine-state freshness. Also
cover missing CCCDs, independent three-channel fragmentation, bootstrap timing
after blocking discovery/write, approved versus refused/expired sessions, 0/1
motor readbacks versus 4/5 commands, core-read prioritization, and module tags
that must not be interpreted as installed app firmware. Local identity/profile
comparison is a physical diagnostic, not a validated provisioning algorithm.

Unloaded LB write cases queued: explicit input only; fresh zero/zero standalone
reports; nonzero/stale/unknown peer fields; optional paired field omitted versus
previously present then stale; lock/OTA/unit/motor changes between inputs; modifier
freshness; no settings or load on reconnect; accepted write without readback;
readback mismatch/clamping; rapid explicit edits; unload priority; KG and modifier
edits rejected. Auto Load enabled versus actually armed must remain distinct.

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

Manual checks still required: saved-device list across relaunch, five-second
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

Follow-up regression cases queued (new test code still deferred): continuous
rotary input beyond two seconds must not starve mock reports; every theme at
and below maximum; confirmed vs ignored maximum requests; modifier amount tap
with pending edits; no X controls; exact preset replacement at high base,
70% IC + 20% EO / max 200 -> base 105; mixed absolute/percentage recipes;
unsupported recipe rejection; knob input during preset confirmation; mismatch
must not activate dropsets or the preset indicator. No new automated pass claimed.

## October 18 single-device follow-up

No new test code was created. Existing Ryan packet and pinned SDK validators
passed again (203 SDK packets / 593 modifier packets); this is reference-data
validation, not proof of new transaction behavior or physical readiness.

Future regression cases: modern percentage values overriding misleading legacy
zeros; uint8 inverse selector and sport-mode field 21007; settings ACK mismatch;
ten-second transaction vs three-second per-write timeout; continued critical polls
during rapid explicit input; units/interpretation/maximum/Twin/motor changes during
a batch; unload cancels all queued steps; expired intent is never replayed;
unrelated reports cannot erase a failure; half-KG round trips and wire envelopes;
disabled modifiers write zero while enabled values stay positive; no redundant
style/alignment invalidation or hidden scale-label rewrite on idle frames;
sparse rep events are not motion freshness; no synthetic physical rest timer.

## October 24 feedback

Existing packet/upstream validators pass again; no new test code was created.
Future cases: one automatic idle-to-Weight-Training request per accepted connection;
mode already selected; refusal/timeout/stale/loaded/armed/other-workout context;
manual exit is not reversed; unload cancels setup; no load command during setup.
One-modifier changes must never write the other parameter; all reduction/increase
orderings must remain within the envelope. Repeated taps use latest requested
state; coalesced edits preserve both independent fields. Continuous rotary input
must retain fresh reports while waiting for the 100 ms quiet interval. Requested
weight is labelled, discarded on failure/disconnect and never authorizes loading.
Missing standalone Twin status must not create a Twin banner. Synthetic pair
limits are 500 LB / 226 KG; ordinary 2.x fallback stays 230 LB / 104 KG.
Disabled physical reports contain zero, so UI editor recall must retain the last
confirmed positive Magic/EO amounts independently per device. Future cases:
60% IC -> disable -> reopen at 60%; EO unaffected; report-driven changes update
memory; no writes on editor entry/reconnect; lower headroom clamps the draft;
absolute-unit conversion; unsupported unit interpretations are not restored;
forget/reset clears the local editor memory. This memory is session-local and
does not override machine-confirmed enabled state or preset matching.

The user confirmed IC disable/reopen retains its value. Further October 24 cases
are queued for progressive rotary input: start/reversal/300 ms pause use fine
steps; noisy batches cannot jump directly from level 0 to level 3; slowdown
brakes immediately; 73 -> 75 -> 80 clockwise and 73 -> 70 counterclockwise at
the 5-unit level; 0.5 KG precision, whole-unit coarse grids, signed stairs across
zero, pending-intent accumulation and final maximum/minimum clamps. No new
automated test pass is claimed for these cases.

Mountain cases: fresh valid 0x556F and accessory-display reports; omitted/stale/
invalid values; both inverse/Mountain switches set; clear prior special curve
before enabling the new one; readback required at every step; EO unchanged;
changing the device's display interpretation cancels percentage edits. Dropset
preview/save on an unsupported connection must not arm it, write resistance,
activate a preset or imply automatic execution is available.

Later October 24 cases (new test code still deferred): arm Stairs while loaded
at rest with completed set 10 / rep 2, observe active moving set 11 / rep 0,
then completed rep 1 and request exactly one reduction. Repeat without unloading;
reject a new set without its observed start, a gap/reset within the same set,
and a completed rep while a prior drop is unconfirmed. A fresh new-set start
may precede weight readback; observing it must neither stop drops nor confirm
the write. Duplicate completions do not drop twice. Confirm RP requires actual
movement to rearm and retains its quota across weight-induced set boundaries.
For rotary input, the later policy supersedes immediate return to fine steps:
slowing retains a 5-unit grid until pause/reversal/touch reset. Check the gentle
two-detent cadence, batched clicks, both directions, negative stairs, KG ticks,
pending previews and off-grid device limits. No delayed write on release.

Next captured ordering to cover: set 12 completed rep 1 -> request 8 LB ->
set-close while workout remains active -> base=8 -> set 13 moving rep 0 ->
settled readback -> set 13 completed rep 1 -> request 6 LB. Also cover readback
before the new start, readback with the next completed rep in one controller
tick, failed/mismatched/late confirmation, and no quota reset for RP segments.

Latest October 24 cases (new test code deferred): Live remains 10 LB while
sensor force changes 9/10/11, and while force or rep-phase reports are missing
but core state stays fresh. Stale/disconnected core state still shows unknown.
Stairs 10 -> 8 -> 6 -> 5 must confirm the final 5, then issue no more writes;
NEXT and MIN labels must agree. RP floors, signed ascending Stairs, manual
rotary limits, below-floor incoming state, invalid/unknown minimum, presets with
insufficient base headroom, and explicit mock Twin/native-KG minima are queued.
Never reinterpret an unexpected mismatch as confirmation or automatic unloading.

Dropset presentation/recovery cases queued (new test code still deferred):
15/13/11/9 LB with one reported rep per machine segment displays one logical
set / four reps; multiple reps within each RP segment accumulate into one set;
duplicate and late completions do not add twice; continuation at 5 LB keeps
counting with no further automation writes. Confirmed unload preserves the last
total and starts rest; a second loaded dropset increments the logical set once.
Weight-induced AA85 closes while active must not start rest. Disable/stop drops
mid-set without discarding existing totals. Missing/stale telemetry, revisited
set IDs, disconnect/reconnect and ordinary training outside drops must not
invent counts. Final minimum readback must settle before exhaustion; at the
floor/quota, later rep gaps/motion loss must not restart automation. Recovery
requires fresh non-training mode plus an equally recent unloaded motor; stale,
unknown or loaded state must retain unload access. Preserve the recovery reason,
clear pending intent, and never replay resistance commands after reconnect.
Manual adjustment after exhaustion disables the recipe; it must not silently
resume drops or leave an active NEXT preview for a dormant executor.

RP/rest cases queued (new test code deferred): status 3 after returning the cable
ends a grouped set without unload; next moving zero-rep start increments SET and
resets REP/RP quota while retaining the old summary during rest. Status 2 plus
AA85 from a base edit does not end it. Preserve failed-unload cancellation across
rest. A 500-2000 ms motion gap discards partial hold time and requires new motion;
a longer loss stops automation. An unchanged extended hold cannot consume two
drops, and a pending drop cannot trigger another. Two confirmed RP holds produce
15 -> 7 -> 5 LB; the floor produces no further write. Strict sequence guards
still apply to Stairs. Verify only base is sent with percent or absolute Magic/EO,
including delayed/mismatched accessory reports. Loaded drops skip rotary quiet
time but preserve readback/deadline checks. Cue occurs once per settled target,
never on GATT success, mismatched target, timeout or reconnect. It must not imply
measured force has already changed. Hardware sound/vibration remains unverified.

October 31 motor-report cases queued (new test code deferred): preserve raw
16009=0x0100 while interpreting its mode as unloaded; read fresh base/modifiers
without a false connection error. Check 0x0101 / 0x0105 loaded, 0x0104 unloaded
command echo, and prefixed Auto Load states against the same additional status
guards as their plain values. Unknown upper bits (e.g. 0x0200), unknown low-byte
modes, and stale reports must not establish readiness or acknowledge a command.
Load/unload acknowledgements use the normalized mode but all outgoing command
values remain unchanged. Reconnect, mode entry, base edits and decoder recovery
must never issue a load. A low-byte-only match cannot bypass locks, cable zero,
activation, standalone context, or current explicit user intent.

Pending-update cases queued (new test code deferred): fresh OTA_STATUS 0 and
the captured Black update-ready value 0x0105 allow ordinary control only when
all other guards pass. Missing/stale status and unrecognized nonzero values
(including nearby 0x0104/0x0106) must block new intents and cancel a pending
transaction without retry, replay, load, or rollback. Update-block feedback
must identify update status rather than require unload. An update-available
report alone must not create a load or settings command. Preserve explicit
unload access regardless of update state. Verify unloaded dial changes and
mode entry with an update waiting, without accepting/installing the update.

October 31 Auto Load/drops and touch cases queued (new test code deferred):
preserve an enabled Stairs/RP recipe through 35/11 -> 12 -> 13 -> 14, with no
drop during waiting/countdown/engagement. At 14 require fresh motion and the
normal completed-rep/qualified-hold trigger, write base only, preserve Magic/EO,
then await settled readback. Verify multiple drops, floor/quota, cancel/unload,
stale or unknown Auto Load status, unsupported motor variants, locks and Twin
loss; never reassert load or retry a drop. A new explicit Auto Load must clear
old hold/rep/quota state while retaining the recipe. Disconnect must still disarm.
During countdown the tile shows recipe values in amber, can be disabled by tap,
and must not falsely claim OFF. An unavailable new recipe still cannot be armed.
At 360 px, control taps drifting 10-18 px remain taps; >18 px cancels. Device/
preset pickers retain >9 px cancellation and >45 px horizontal paging. Verify
long press issues one action, release does not toggle, wake tap only wakes,
adjacent modifier targets stay separate, and caption/circle gap taps register.
Touch diagnostics now distinguish recognized actions from drag cancellation;
responsiveness on the physical panel remains a human observation, not a build
or packet-validator result.

Buffered-touch cases queued (new test code deferred): a complete 80 ms tap
inside a 150-250 ms transport stall delivers down then up and exactly one action;
retain intermediate swipe excursions even if the finger returns to its start.
Stationary long holds remain holds, not repeated taps; queued short taps never
become long presses. A screen change discards old input and a held release does
not act on the new screen. Stale (>350 ms) input, overflow, invalid coordinates
and an I2C error while pressed cancel without a CLICKED action; fresh input only
resumes after a real release. Check wake-only touches, both picker swipe guards,
Apply refusal feedback, unload during pending BLE work, and initialization
failure if the input task cannot start. Verify single taps on Drops Apply,
Magic/EO, device selection and presets on the physical panel. The desktop
window screenshot helper timed out during the pre-change comparison; no native
interactive pass is claimed from that attempt.

RP countdown/Auto Unload cases queued (new test code deferred): no bar before
250 ms; 1 s hold shows 75% at 250 ms, half at 500 ms; duration scales to 2..9 s.
Movement/drift resets elapsed time; stale motion, disconnect, rest/unload,
pending target, quota/floor, 0%/0 drops and Stairs hide the bar. Continuous hold
triggers once; new movement rearms. A bar reaching zero cannot send or confirm
a command. Check 360/466/480 round bounds and the primary unload touch target.
Unknown/off Auto Unload preserves max 9; 2 -> max 1, 3 -> max 2, 10 -> max 9;
1 -> no valid RP pause. Reject out-of-range/stale parameter reports. Editor,
Apply, presets, toggles and externally shortened timers share the same policy;
no automatic recipe shortening, rearm, resistance load or Auto Unload write.
Correlate raw parameter 21525 against Black's displayed holding-time setting.

Later October 31 cases queued (new test code deferred): Stairs and RP restore
the original base only at actual workout rest/unload, after settled unloaded
state; segment closes must not restore. Unload once, base once, never load.
Verify next set's base and grouped counters, unchanged accessories, floor/quota,
external unload, failed unload, stale state, different units/device, disconnect,
manual override, incompatible envelope and cancellation/timeout without replay.
Loaded amount writes must preserve base and the other accessory, accept only
current/in-flight/requested amounts, and never reassert motor mode. Reject curve
and unit switching while loaded, mode 5, unknown locks/Twin and stale controls.
Persist one global rest-alert setting across device changes, reboot and mode
switch; ±detents advance 5 s on-grid, bounded at off and 59:55.

Wi-Fi cases: protected AP, captive/manual page, hidden/duplicate/long/escaped
SSIDs, open/WPA2 networks, bad password and join timeout, candidate-save failure,
power loss before/after save, saved reconnect, AP expiry, browser partial bodies,
duplicate/oversized fields, invalid token, non-AP client refusal and no secret
logs. Wi-Fi/HTTP must stop during VOLTRA connection, without a resistance action.
Physical phone join and network persistence require human observation. Review
360/466/480 setup/Apply layouts. Diagnose occasional Apply with press/release,
cancel reason and app refusal rather than claiming every lost touch is fixed.

November 2 Wi-Fi regression cases queued (new test code remains deferred):
IPv4 and ::ffff:IPv4 peer/local sockets, truncated sockaddr, native IPv6,
getpeername/getsockname failure and non-AP interface/subnet. Root must return
HTML rather than {}, and a refused page must explain how to join setup Wi-Fi.
Captive probe redirect and direct 192.168.4.1 must both reach the form. The
open AP has no Wi-Fi password; each setup gets a four-digit PIN shown only on
the remote, not prefilled in HTML or logged. Reject absent/wrong/expired PIN,
duplicate fields and stale token without changing stored credentials. Verify
phone join, correct-PIN submission, failed home-network password, success/save,
reboot persistence and timeouts. Check 360/466/480 remote PIN instructions.

Later November 2 supersedes PIN cases: no PIN/password on setup AP/page.
One Settings -> Wi-Fi tap starts the portal. Back/Stop/expiry close it; restart
is explicit. A select control lists unique scanned SSIDs with a manual hidden
network fallback. Refresh reports scanning/failure/empty results and retains
the selection without rebuilding unchanged options. Check duplicate/escaped
names, failed initial scan, refresh while scanning, expired token, and submission
during refresh: joining must wait, with no false "Connected and saved" from a
previous saved-network report. No phone credential harvesting or mock physical
scan pass. Native/browser previews use synthetic names and never join a network.
