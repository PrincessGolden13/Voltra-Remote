# November 18: Auto Load hold and machine-state feedback

CONFIRMED (owner): Auto Load now reaches the expected state on the VOLTRA, but
the remote gives no useful visible progress while holding or after activation.
The installed version was not explicitly restated in this follow-up. This is
physical confirmation of that Auto Load trial, not every readiness condition.

## Reference and interaction

Beyond Power's [Spring update](https://www.beyond-power.com/blogs/news/spring-update-voltra-i-out-now)
describes a two-second weight hold and illustrates a dotted circular progress
indicator over the weight. Its [current controls guide](https://help.beyond-power.com/en/articles/14247143-basic-controls-navigation-and-safety)
describes pulling the cable and following the on-screen stabilization countdown.
The [advanced reference](https://help.beyond-power.com/en/articles/14167829-advanced-mode-quick-reference)
describes a three-second stable cable hold. The historical article's two-second
stabilization and current guide differ; the remote must display actual device
timing rather than assume either duration for physical resistance.

The remote adapts the circular cue to its cyan visual style. A normal weight tap
under 300 ms keeps immediate Load/Unload dispatch on release. A weight hold shows
progress after 300 ms and completes at 2000 ms. Releasing between these boundaries
cancels the unloaded gesture; it cannot become an accidental normal Load.
Loaded, armed, stale or pending-load weight actions retain priority Unload/cancel
routing rather than consuming an intermediate hold. At completion,
Release for Auto Load makes the release action explicit. Other editor/preset
holds remain 650 ms. The existing six-second debug gesture consumes the touch
without an earlier Auto Load; movement, cancellation, wake and navigation still
cancel input. No resistance command is sent by the progress animation itself.

Pending Auto Load intent is shown as Starting, distinct from machine-confirmed
arming. Confirmed arming gets a prominent Auto Load presentation, pull/hold
guidance, device countdown when available, and tap-to-cancel. Loaded brightness
still comes only from the existing confirmed motor/state decoder. No eligibility
or readiness guard is broadened by this presentation change.

## Read-only protocol feedback

CONFIRMED (source): Android da514d2 paramInfo.csv defines 21448/0x53C8 as uint16
milliseconds, 0..3000. Kirby 8e1ea1c independently captures motor35 with status11
waiting,12 counting down,13 engaging,14 complete, with 250 ms countdown polling.
Android/SDK motor38/39 are the already-supported armed/active variants. There is
no new inference that motor0/status14 is armed or safe for loading.

The optional countdown register is first probed alone. Only a successful direct
read adds it to core/Auto Load reads, so an unsupported optional register cannot
break required readiness polling. While Auto Load is pending/armed, extra reads
are paced at 250 ms behind the existing critical poll deadlines. These operations
never write settings, change safety configuration, retry a trigger or engage
resistance. Countdown values must be in range, in the confirmed countdown phase
and no more than 500 ms old; the UI rechecks the observation age between reports.
No local countdown predicts engagement, and a reported zero never implies loaded.

The pending presentation flag clears on confirmation, timeout, explicit Unload,
disconnect and leaving the device. The simulator supplies explicitly synthetic
phase/countdown reports through its existing mock device behavior.

## Verification

Existing validators/builds and real LVGL preview scope are recorded in TESTING.md.
No new automated test source or physical motor operations. The owner's physical
check is still needed for legibility, touch timing, armed/engaging/loaded transitions,
countdown freshness, cancellation and debug-hold behavior on the board.
