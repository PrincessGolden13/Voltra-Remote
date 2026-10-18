# Physical connection attempt — October 18, 2026

The user brought a VOLTRA for real testing and reported that it runs 2.x firmware
and is ready. Exact firmware version, current units and Bluetooth connection state
have not yet been read from the machine. The remote is available on USB COM3.

## Observations

- CONFIRMED on the remote: Normal firmware boots, initializes display/touch/encoder
  and performs repeated BLE scans. Captured windows contained 15–26 advertisers.
- CONFIRMED for the captured runs: zero advertisements matched the VOLTRA service;
  no GATT connection/acceptance or machine-state report followed. No resistance
  command was sent. This does not prove the VOLTRA is off or identify the cause.
- The user reported a device tile. Its name and the result of tapping it were
  requested and remain pending; that observation needs reconciling with the logs.
- The existing old-coredump warning and absent first-use device-list NVS namespace
  remain visible at boot. UI and BLE scanning continue afterward.

## Changes

Normal firmware now reports scan counts, connection failures, discovered GATT
properties, subscription outcome and session status over USB. Protocol logging
is limited to the first 24 frame headers per connection, bootstrap command IDs
and the existing normalized parameter values. No addresses, client identities,
activation keys or raw payloads are logged. Advertised service UUIDs are included
in discovery diagnostics. Generic BLE logging is delivered through a callback;
USB output remains in the board host. Simulator code has no physical diagnostics.

CONFIRMED in installed NimBLE-Arduino 2.3.6 source: NimBLEAdvertisedDevice::update
replaces the payload on a new advertisement and appends scan-response data.
Our former end-of-window read could therefore lose service data seen earlier.
The transport now merges names and service UUIDs from onDiscovered/onResult into
a bounded per-scan snapshot, then passes the completed scan to the application.
This preserves the protocol service filter and GATT validation. It does not
connect based on names or infer device capabilities. Whether this affected this
VOLTRA is UNVERIFIED; the correction did not produce a match in the latest run.

The external SDK's existing host-contract tests document a related real-device
issue with service information arriving in scan responses. The reviewed SDK pin
is unchanged from NORMAL_MODE_2026_10_17.md. No upstream implementation was copied.

## Validation and next step

Normal firmware builds successfully: 1,188,109 program bytes / 31,768 static RAM
bytes; firmware.bin is 1,188,480 bytes. Upload recovery/partition checks passed
and esptool verified every written image. Startup and scans were observed on
the flashed correction. Connection, approval and training are still unverified.
The isolated simulator also rebuilds successfully after the shared entry-point
change (927,001 program bytes). No simulator behavior change was introduced.
The flashed Normal image and its SHA256 manifest are retained in ignored
build/guition/2026-10-18-discovery.

Raw local logs are ignored under build/voltra-live/2026-10-18. No new automated
test code was added. Future regression cases: delayed scan response, later
advertisement without services, duplicate callbacks, per-scan reset, bounded
multiple-device results, and callbacks concurrent with result consumption.

Next physical observation: identify the displayed tile and its tap result, then
check that VOLTRA Bluetooth is enabled/available and that another controller is
not occupying its connection. Once its service is visible, retry the read-only
connection and capture firmware, unit, limit and readiness reports. Training
writes remain blocked by the previously documented unresolved profile mappings.
