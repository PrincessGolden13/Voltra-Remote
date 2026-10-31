# Dropset restore, loaded amounts, rest alert and Wi-Fi

CONFIRMED (user): the RP countdown and the 2 s Auto Unload / 1 s pause limit
worked on Black. The next requested changes are below. Source implementation
does not by itself prove physical compatibility; installation/trials are
recorded separately in BASELINE.md.

## End-of-set restore

User clarified the contradictory example: start 50, finish 20, unload and
restore **50**, never reload. Both Stairs and RP use the existing grouped set
boundary (actual resting/ending/unloaded report), not AA85 caused by each base change.
Black's observed workout phase 4 marks ending before its motor returns unloaded;
phase 3 marks rest. Neither is inferred from the cable pause timer.
The original base/unit are captured when the observed dropset starts. On a
real end, cancel unsent drops, request unload once if needed, await confirmed
unloaded control state, then request the original base with current accessories
unchanged. The restore deadline is 8 s. A failed/timed-out unload or restore,
disconnect, device/unit change or incompatible envelope stops it without retry.
Manual base adjustment/recipe disable supersedes restoration. No restore intent
is stored in NVS or created at boot/reconnect. Explicit unload remains available.

## Loaded modifier amounts

CONFIRMED (source): kirby6365/voltra_remote commit
8e1ea1cb752acd3108c61f1a502b63978a172832, src/voltra/voltra_client.cpp,
setChains/setEccentric and request dispatch, uses the ordinary accessory amount
registers while loaded. Its mode-5 reassertion/retry is intentionally excluded.
Our existing loaded control context accepts motor 1 or completed Auto Load 35/14,
with fresh locks, standalone state, units, parameters and limits. Amount writes
may now run in this context, with decreases first, settled per-register readback,
transaction deadlines and no motor command. The current base is preserved for
an accessory-only edit. Unsupported active modes remain refused.

CONFIRMED (official documentation): switching chain curve requires unloading.
Changing %/absolute interpretation also remains unloaded-only in this profile.
The UI/controller distinguish amount edits from curve/unit changes and give a
specific message for the latter; there is no implicit unload/reload workaround.

- [Pinned client source](https://github.com/kirby6365/voltra_remote/blob/8e1ea1cb752acd3108c61f1a502b63978a172832/src/voltra/voltra_client.cpp)
- [Beyond Power training controls](https://cortex.beyond-power.com/docs/cortex/training-control)

## Rest alert and Apply

The user clarified persistence means the configured alert duration only. That
already resides in the remote-wide voltra-ui/config NVS blob, shared by devices
and both firmware partitions; it does not depend on TrainingApplication's
per-connection elapsed-rest clock. Each detent now moves to the next 5 s mark,
0 (off) through 59:55. Existing nonmultiple settings migrate only when edited.
Save still occurs on edits; editing these values no longer rebuilds the screen.

The Apply target is wider (48% of display width plus a 2% extended hit area).
Application refusal messages now remain visible while loaded, with the same
unload touch binding. Input diagnostics identify gesture movement cancellation
and distinguish normal screen changes, stale delivery, invalid reads and queue
overflow. The independent sampler's conservative cancellation rules remain.
CONFIRMED (physical capture, apply-repeat.txt): a tap at 142,96 missed the old
Stairs button's upper boundary; 129,125 reached its overlapping status/unload
target and sent unload while Black was already unloaded. A subsequent 125,117
selected Stairs. All four captured Apply actions succeeded, with no refusal.
Mode buttons are taller, have separated extended hit areas, and the status line
is below the controls. A fresh idle/unloaded status is no longer clickable.
This fixes a demonstrated targeting fault; it does not prove every occasional
Apply miss has been reproduced or resolved. Diagnostic logging remains enabled.

## Phone Wi-Fi setup

Settings -> Wi-Fi -> Set up Wi-Fi starts a temporary password-protected
VOLTRA-Setup-XXXX access point. The remote displays its random session password
and 192.168.4.1. The phone page offers scanned 2.4 GHz SSIDs or manual entry,
and a password field. Captive DNS/HTTP redirect supports phones' setup prompts;
the explicit address remains the fallback. No phone app or cloud service is used.

The shared UI talks to WifiSetup; Esp32WifiSetup owns radio, HTTP/DNS and NVS.
It is linked into Normal only. Simulator preview uses a synthetic status and
does not initialize radio. Wi-Fi pauses during a VOLTRA connection/connection
attempt, so provisioning/network scans never run during controlled training.
Previously saved credentials are attempted when the remote is disconnected.

HTTP runs in its own task, bounded to three sockets, 1 s I/O timeouts and 512-byte
POSTs. Setup lasts at most 5 minutes, or closes 15 s after successful save. The
portal accepts only clients on its AP subnet and a random session token; it
validates lengths/encoding, escapes displayed data and uses no external assets.
Credentials are saved to a separate local NVS blob only after successful Wi-Fi
connection. A failed candidate does not overwrite the previous saved network.
Passwords are never printed, returned to the browser or put in Git. NVS uses
the board's existing storage configuration; flash encryption is not claimed.
There is no VOLTRA command endpoint or firmware upload/update implementation.

Build/render/phone checks are recorded in BASELINE.md. New automated test code
remains deferred per the project brief; future cases are listed in TESTING.md.
