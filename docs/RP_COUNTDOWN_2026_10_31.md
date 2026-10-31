# RP hold countdown and Auto Unload limit

CONFIRMED (user, October 31): repeated Drops Apply now works on the first tap
with the independent touch sampler in 540ef06. The new request is a vertical
Live-only countdown, visible after a qualifying pause has lasted 250 ms, plus
an RP pause ceiling one second below the machine's Auto Unload holding delay.

The controller exposes presentation-only remaining permille from the same
hold start used by RP execution. At 250 ms into a 1 s hold, 75% remains; the
bar does not start a second timer. Display interpolation uses the local clock
only while fresh held motion remains valid (500 ms). Only new qualifying
device motion reports can trigger the existing base-only drop operation.
Movement, cumulative drift, stale motion, pending drop, rest/unload, recipe
disable, floor and quota stop/hide the relevant countdown. The vertical bar
drains downward at the right edge and does not cover the weight unload target.
All dimensions derive from display metrics.

CONFIRMED (source): Android reference commit
`da514d2db1a16ebb4ded3156bc8fd200bed1e6eb`,
`app/src/main/assets/voltra/paramInfo.csv`, catalogs parameter 21525 / 0x5415
as AUTO_UNLOAD_HOLDING_TIME, uint8, range 0..10, default 0. Its CSV unit is
unspecified. Official Cortex documentation identifies Auto Unload's hold-still
argument as seconds and zero as disabled. Together these support the read-only
decoder. CONFIRMED (Black, this installed build): the user reported a 2 s
holding time and the fresh read returned 21525=2 while unloaded. Other values
and firmware still require physical correlation; no timer setting was changed.

- [Parameter catalog at pinned source](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv)
- [Official training controls](https://cortex.beyond-power.com/docs/cortex/training-control)
- [Official Weight Training Auto Unload behavior](https://help.beyond-power.com/en/articles/14167829-advanced-mode-quick-reference)

The profile polls 21525 alongside existing read-only control state, accepts
only fresh 0..10 values, and exposes optional seconds in TrainingReport. No
Auto Unload configuration write exists. Absent/stale/out-of-range values and
disabled hold time retain the existing 1..9 s editor range. A positive delay
caps it at min(9, delay-1). At 1 s there is no valid RP hold: the editor shows
unavailable and Apply refuses. Zero holds are never armed.

The controller checks Apply and presets independently of the UI. A newly
reported shorter delay stops an incompatible armed recipe and requires another
explicit Apply, rather than silently shortening its trigger. Stairs is unaffected.
The setting controls a maximum, not a promise of precise device timing: our RP
motion qualification and VOLTRA's own Auto Unload detector are independent.

Offline render options `--hold-ms`, `--hold-seconds` and
`--auto-unload-seconds` use the existing mock motion/controller path with a
fixed preview clock. They require `--render`; hold progression additionally
requires loaded profile 1, RP and enabled drops. They are synthetic UI previews,
not physical packet fixtures or a new automated test suite. The mock does not
implement the physical Auto Unload motor behavior.

Build, render and physical verification results are recorded in BASELINE.md.
