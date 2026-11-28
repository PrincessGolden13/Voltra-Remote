# Standalone Live weight cadence

At v0.4.28 the remote can keep the last valid target visible through a short
Bluetooth gap. That does not recover changes occurring between samples. In the
owner's recordings, the native display moves from 69 to 55 lb in 1–2 lb
changes over ten consecutive samples at 100 ms intervals. In the remote clip,
58 lb at about 34.8 seconds is followed by 83 lb at 35.4 seconds, with dashes
between them. The clips show separate sets and cannot establish an exact
sample-by-sample comparison between the two displays.

The remote renders a new target as soon as it is available. The fallback
requested register 20859 at least 300 ms apart. The physical link has 90 ms
outgoing pacing, and the required state queries and control readbacks get
priority. On a 90 ms grant grid the 300 ms interval needs at least four grants,
with a longer delay when a required query uses the next grant.

The standalone fallback can now request a new device target after 180 ms once
the register has returned a valid value in this Bluetooth connection. An unproven
device uses the original interval and bounded probe budget. The global pacing,
priority order, one outstanding fallback query, existing timeouts and streamed
B4 preference stay in effect. The readout continues to display device values in
whole pounds. It does not generate intermediate numbers.

The latest available prior trace was Grey on v0.4.26 and had no B4. The clips
do not identify which stream or register supplied White's target numbers. A
synchronized physical comparison with the native display remains open.
See [previous video evidence](LIVE_GUIDANCE_2026_11_28.md) and
[the register source](SINGLE_LIVE_2026_11_27.md).
