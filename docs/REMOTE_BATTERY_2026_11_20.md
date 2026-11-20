# Remote battery estimate — November 20, 2026

The owner requests a warning at 10% for the remote itself, deferred while the
Live screen is in use. VOLTRA/peer battery reports are separate and must never
supply the remote's reading. The Guition Board adapter previously returned no
battery percentage, so a UI-only warning could not work on the physical remote.

## Hardware evidence

CONFIRMED in the reviewed K518 reference source/schematic, not by a measurement
on this owner's remote:

- [KrX3D/WaveShare-Knob-Esp32S3](https://github.com/KrX3D/WaveShare-Knob-Esp32S3/tree/74511f2260e3e7ce3c84ca3136569307cfb8b26b)
  explicitly supports Guition JC3636K518C-I-YR1 and its DRV2605. Schematic page
  `2_ESP32S3-R8.png` maps BATT_ADC to GPIO1 / ADC1 channel 0. Page `4_OTHER.png`
  shows R62/R63, both 10 kOhm, dividing the shared `5V` rail by two. Its
  `example/esphome/SmartKnob/adc_battery.yaml` uses GPIO1, 12 dB attenuation
  and a multiplier of two.
- [prompt-knob-firmware at e8f3e021](https://github.com/andrew-malitchuk/prompt-knob-firmware/tree/e8f3e021e1545c3030b50c96e99c7c1520c674b6)
  identifies the K518C and independently uses GPIO1 / the same divider in
  `src/pincfg.h` and `src/battery.cpp`. The earlier b738e065 bring-up reference
  also declares that pin and divider. Its documentation warns that K718 siblings
  differ; their GPIO6 mapping is not adopted here.

The schematic's supply comes from the daughter-board connector. These reviewed
pages do not show the battery/charger path or establish a charger-status input,
PMIC interface or fuel gauge. The supplementary
[roon-knob battery notes at 4e2a6270](https://github.com/muness/roon-knob/blob/4e2a6270136a0ab2969cc1ec0de67a206dad2e45/docs/dial/hw-reference/battery.md)
report approximately 4.86 V on USB and 3.2–4.2 V on battery. This is a third-party
observation, not the owner's unit or an independent calibration. No charging
state is inferred.

## Implemented board profile

INFERRED: within a plausible single-cell voltage window, the divided supply rail
can estimate charge using the K518 reference's LiPo curve:

| Supply mV | Estimated percent |
| --- | --- |
| 3300 | 0 |
| 3600 | 10 |
| 3700 | 25 |
| 3850 | 50 |
| 4000 | 75 |
| 4200 | 100 |

Linear interpolation between these points is presentation policy, not a measured
capacity curve. Load, cell chemistry, age, temperature and ADC/divider tolerance
can change the estimate. It is not a fuel-gauge measurement of remaining capacity.

The existing Board adapter configures only the evidenced GPIO1 ADC input, using
Arduino 2.0.17's `ADC_11db` name for the attenuation. It averages 32 calibrated
millivolt samples every 10 seconds, applies the divider before a 7/8-old + 1/8-new
voltage filter, and caches the resulting optional percentage. UI reads are cheap;
no ADC work runs in LVGL, the touch task or the BLE/protocol adapter.

A zero/failed sample, implausible ADC value, supply below 3000 mV or above
4300 mV clears the estimate and its filter. In particular, the normal approximately
5 V USB rail is unavailable rather than 100%. Readings older than 22 seconds
are unavailable. A valid near-empty cell-window reading can return 0%; a missing
reading cannot. Invalid/external-power samples clear the filter so a later
battery-mode sample does not inherit an inflated USB value. This is a range
classification, not USB or charger detection; an external source inside the
battery window cannot be independently identified by this input alone.

There are no charger, regulator, power-switch, peripheral-rail or PMIC writes.
ADC1 avoids the ESP32 ADC2/Wi-Fi conflict. Privacy-safe diagnostics retain measured
rail millivolts and the estimate (`-1` means unavailable) when the result changes.

## Validation limits

No physical ADC, charging, battery discharge, GPIO output or VOLTRA operation was
performed for this change. Embedded compilation and UI checks are recorded in
TESTING.md when complete. No new automated test source is introduced.

UNVERIFIED on this owner's remote: rail values on battery and USB, percentage
calibration and actual warning timing near 10%. The next physical debug report
can establish the sampled rail without changing power or charger configuration.
