# Twin connection and Live follow-up

## The recorded connection

The phone recording shows Black as available and Red as “Not around.” The
owner taps Black once. The app groups the two named devices, displays both
batteries, and opens the dashboard. No change to the pairing is shown. The
recording shows the completed dashboard, so its underlying Bluetooth
connection sequence is unknown.

In remote report `4f0156dc`, the first link connected in 186 ms and accepted
the VOLTRA session. Its own-matched A7 reply identified it as the follower.
The old remote disconnected it and began up to seven attempts to the reported
controller. Its last observations of that target were around 120 seconds old,
and there was no fresh advertising signal. Report `cdaff084` also reached the
follower, then spent two radio attempts and around 9.5 seconds trying the
controller. Code 13 was a GAP timeout before service discovery. These reports
do not establish the cause of the controller's radio unavailability.

The app can discover a Twin from a single accepted member. Remote training
requires a fresh own-matched controller. The published follower capture
showed that a command to the follower acted locally. It supplies no verified
already-paired wake, takeover, or control relay.
[Physical protocol capture](https://github.com/stevenpoint/voltra_remote/blob/1c14860b53b805a789b8bed2095e63443e355d7d/docs/PROTOCOL.md#L222-L266)

The remote now tries at most two radio connections per identity for a
selection. A verified controller can open the pair on its existing link.
A fresh follower can identify the exact other device for verification.
If that route is unavailable, the pair stays grouped on Devices.
The limit also covers a device whose Twin role is unknown before connecting.
An ordinary Bluetooth timeout already allowed only two attempts. A repeated
establishment error previously allowed a third.

An established idle disconnect returns directly to Devices. A live
disconnect can show a notice, and a confirmed mounting-lock release
retains its specific warning. Explicit connection failures remain visible.

## The Twin workout

Report `6fb2c796` recorded 219 AA81 notifications and zero B4 frames. The
exact AA82 return sample contained 50 tenths at the known single-unit
target offset. The AA81 return tail gives a distinct raw value of 70.
The main VOLTRA's whole-pound display during the same rep was not captured.
Neither value is established as the combined display target.

The remote recorded AA80 rest at 285768 ms and in-progress at 292694 ms.
The report later showed distinct pull and return rep samples. There is no
later explicit rest in the retained trace. AA85 count was zero. Do not
substitute measured force, selected base weight or a scaling factor for the
actual live value.

A reported Twin rest or unload returns to Adjust. The display also returns
to Adjust after 15 seconds of fresh, consecutive idle motion telemetry from
the verified controller. This is a screen transition and does not complete a
set, disarm the loaded weight, or start any automation. New rep activity can
reopen Live. The fallback needs continuous idle packets. A stopped stream
provides no idle duration. The interval is the upper bound of the catalogued
idle-to-rest setting.
[VOLTRA parameter catalog, 21444](https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android/blob/da514d2db1a16ebb4ded3156bc8fd200bed1e6eb/app/src/main/assets/voltra/paramInfo.csv#L288)

The optional BLE trace now preserves the most recent Twin workout's numeric
context across a later read-only discovery and keeps separate phase samples
of the AA82 candidate. It stays off by default and attaches only when the
owner sends a report.
