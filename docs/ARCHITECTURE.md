# Application architecture

The product is standalone. Phone, PC, cloud, Wi-Fi, Home Assistant and Cortex
are not runtime requirements. Windows and macOS simulators are development hosts.

```text
UiController (Devices / Main / Magic / EO / Drops)
  -> TrainingApplication (selection, intent, limits, dropset engine)
     -> DeviceMemory (up to 64 saved IDs/names; desktop file / board NVS)
     -> VoltraController (connection, GATT discovery, packet observations)
        -> BleTransport (generic scan/connect/subscribe/write)
     -> ControlSession (VOLTRA semantic operations and TrainingReport)
        -> MockTransport / per-device synthetic state
        -> PhysicalSession (bootstrap/readback; training profile incomplete)
           -> NimBleTransport (Normal firmware only)

UiController -> HardwarePlatform -> Display / Touch / Rotary
                                  -> Windows / macOS adapters
                                  -> Guition K5 board / BootMode adapter
                                  -> older unidentified prototype: BLOCKED
```

LVGL 9.3.0 is shared application code. Relative geometry supports 360, 466 and
480 px round displays. Hosts apply a circular Windows region or transparent
AppKit window with circular clipping. Each reserves
only the top 10% rim for dragging, so the connected-name touch target works.

TrainingApplication scans every five seconds while disconnected, merges live
discoveries with saved accepted connections, and supports arrows/swipe/rotary paging.
Normal firmware waits for a complete scan before one startup auto-selection of a
sole online VOLTRA. Async scans do not rebuild a tile underneath a held finger.
Persistence is injected through DeviceMemory; shared UI/controller code does
not know host paths or file formats. Windows stores beside its executable;
Mac stores in the user's Application Support/VOLTRA Remote directory. File
replacement uses MoveFileExW on Windows and filesystem rename on Mac.
Board storage uses separate versioned NVS blobs for real and synthetic identities.

SimulatorApp owns the shared application lifetime, scenario keys, argument
validation and platform-independent BMP encoder. main.cpp supplies Win32
painting/events; main_mac.mm supplies an AppKit window, main-run-loop timer,
Retina-scaled framebuffer drawing, mouse/trackpad and keyboard events. Both
compile/link only the mock BLE implementation. The Mac build is an app bundle;
the Windows build remains an executable. Build directories are separate.

TrainingReport is a full semantic observation: displayed weight/units, modifier
representation and values, explicit capabilities/combined limit, Twin state,
load/Auto Load state, and normalized rep/movement/hold telemetry. The offline
mock implements the complete report. PhysicalSession supplies limited diagnostic
readback; unknown Twin/readiness prevents training control. The existing strict protocol
decoder still validates fragmented base-state packets independently. Mock
semantic reports are not claimed to be captured packets or a physical decoder.

Editor drafts live in UiController. Main-screen weight, modifier values and
activation come from reports, never transport acceptance. Weight intent is
retained only to accumulate explicit rotary input while a response is pending.
A later report wins even if it differs; three seconds without a report clears
pending UI state. Neither case triggers a retry or corrective write.

The semantic settings operation is atomic in the mock. A future physical
adapter must implement proven ordering/transactions and response matching;
multiple individual BLE writes must not pretend to be atomic.

Dropsets are remote-local settings and controller logic. Rep/hold reports drive
strictly reducing weight requests. RP requires movement between qualifying
holds, counts confirmed reductions, and compounds from returned weight.
Stairs deduplicates counters and stops on gaps/resets. Missing or mismatched
reports never cause a replay. Unknown/stale telemetry disarms reductions.
An unload request cancels reductions even if unloading is not confirmed.

Auto Load is an explicit one-shot semantic trigger. Its device-side progress
is displayed; the remote never runs a timer that sends a load. The mock models
a three-second stable hold after arming. No Auto Load/Unload configuration setter
is exposed.

No real-device-safe release exists. Remaining physical work includes validated
bootstrap/read decoding, capability and modifier representation mapping, MTN,
Twin routing, rep/hold telemetry normalization, live-setting safety, command
generations/ordering, and board drivers. Defaults in ControlSession return
unsupported/unknown; physical features cannot become enabled by UI defaults.
