# November 18: unload confirmation and Auto Load reset

Baseline: v0.4.7, main 424568d. Diagnostic inbox report 5a65d590 (firmware
0.4.7) establishes the following sequence, using device uptime milliseconds:

| Time | Observed event |
| --- | --- |
| 22946 | Auto Load status 21447 = 14; UI says Unload to adjust. |
| 25571 | Explicit weight tap sends unload command 16009 = 4. |
| 25926 | Direct motor readback 16009 = 0; old code completes Unload. |
| 26764 | Second tap fails: Finish or cancel Auto Load on VOLTRA. |
| 27145 | Auto Load status 21447 = 0. |

CONFIRMED (report): at refusal, the direct motor is 0 aged 90 ms, while
Auto Load status 14 is 1193 ms old and predates Unload. Mount 2, safety status 0,
mode 1, child lock 0, OTA 0, cable position/offset 13, and standalone coworker
zero/zero are present. The earlier v0.4.7 safety-status fix is no longer the
reported blocker. There is no failed load write: the new tap is refused locally.

CONFIRMED (source): core polling read motor in group 0 and Auto Load status in
group 4. Pending motor commands read only the motor. A motor 0 response cleared
the unload transaction while the cached Auto Load status still blocked readiness.
The application treated a tap during that interval as a load refusal and opened
a persistent notice, even though the status subsequently reset on its own.

CONFIRMED (pinned upstream): kirby6365/voltra_remote at
8e1ea1cb752acd3108c61f1a502b63978a172832, docs/PROTOCOL.md and existing
test_auto_load_phases, interprets status 14 as completed Auto Load with motor 35.
Motor 0/status 14 is not loaded, but this does not establish permission for a
new load in that combination. The readiness allowlist is not broadened.

## Change

Core reads and pending motor reads include Auto Load status in the same request.
Motor confirmation runs after the complete validated response has been applied,
so parameter ordering cannot consult the preceding status. Unload remains busy
until a direct 0x0f response contains normalized motor 0 AND status 0 together.
A motor-only response, asynchronous push, command echo 4, cached status 0 or BLE
write success cannot finish it. The existing three-second confirmation deadline
still stops an unconfirmed request; no resistance write is retried.

During this transaction readiness stays false. When fresh unloaded UI receives
a tap or Auto Load hold, it keeps the pending message without opening the failure
dialog or storing a future load action. Stale/loaded/armed states still route a
weight tap to unload; explicit Unload remains unconditional. After reset, another
explicit tap/hold and every ordinary readiness check are required. Completed
status 14 with motor 0 gets reset guidance instead of an instruction to cancel
an active Auto Load. Other unknown/nonzero statuses remain blocked.

## Verification limits

No new test code, physical commands or USB flashing. Existing fixture/upstream
validators, offline build and public firmware/package results are recorded in
TESTING.md. These checks do not execute this state transition or prove physical
resistance behavior. Owner checks: unload then tap during reset; wait then tap
Load/hold Auto Load once; unload remains accessible; timeout or lost connection
never replays an action. Future regression cases include direct coherent 0/0,
motor-only/push 0, cached 0 followed by motor 0, status 14/unknown/missing,
reverse response ordering, timeout, disconnect and immediate second taps.
