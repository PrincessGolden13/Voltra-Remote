# November 16: settings retention and Wi-Fi completion

Baseline: published v0.4.5, source 2e71053471b92d6e314e55388ec58e7437362672;
documentation main 5c676a6. The owner reports having to re-enter settings after an
update and a misleading Wi-Fi restart-setup button after successful setup.

## Evidence and bounded corrections

CONFIRMED (source): OTA writes only the inactive Normal application slot. NVS
remains at 0x9000/0x5000; partition layout, Wi-Fi credential version 1 and remote
preference schemas 1/2 are unchanged. There is no application-level NVS clear in
the update path. Neither that design nor successful builds prove retention on
the owner's latest update.

CONFIRMED (pinned dependencies): Arduino 2.0.17 initArduino() erases the whole NVS
partition on NO_FREE_PAGES or NEW_VERSION_FOUND initialization errors.
[Source](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-misc.c).
NimBLE-Arduino 2.3.6 has equivalent recovery through nvs_flash_erase().
[Source](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEDevice.cpp).
These paths can erase all settings before application setup; they are a concrete
retention risk, not a confirmed diagnosis of the owner's event.

Both board images now link a wrapper around esp_partition_erase_range that
refuses only complete NVS-partition erases. Sector garbage collection, normal
key writes and OTA application erases forward to the SDK unchanged. No SDK cache
is modified. On failed NVS initialization the physical image leaves BLE inactive
and opens the Wi-Fi storage-error screen instead of invoking NimBLE's formatting
path. Arduino's supported verifyRollbackLater hook defers automatic OTA acceptance
until the existing application startup confirmation, now including storage health.

CONFIRMED (source): failed preference namespace opens and malformed preset records
could return defaults/partial settings while leaving writes enabled. Preferences
and recent-device stores now distinguish absent records from failed/malformed or
future-schema records and refuse to overwrite the latter. Preference writes and
new Wi-Fi credential saves are read back before reporting success. INSTALL first
verifies preferences and saved connected Wi-Fi; failure blocks installation.
Credentials are never logged or included in diagnostic reports.

CONFIRMED (source): Open Wi-Fi always started a new portal, and its button always
became STOP SETUP/RESTART SETUP even after saved connection success. Success was
also obscured by the still-active hotspot's instructions for fifteen seconds.
That is a misleading/disruptive UI path; no credential-delete operation was found
behind Restart setup. Saved credentials now prevent automatic portal reopening.
Newly completed setup shows the saved network and exactly one button, OK, which
stops the temporary hotspot and returns to Settings. There is
no extra back button after success. Before success, a back arrow remains and CANCEL exits
setup without overwriting the stored network. A disconnected saved network can
be changed explicitly; a failed candidate does not overwrite the previous one.

## Configured network overview and ping

The owner's later follow-up adds a separate overview when reopening Wi-Fi with
an existing connection. It shows the current SSID when connected, or explicitly
labels a saved network as disconnected. Opening this screen requests an ICMP
test to Google Public DNS 8.8.8.8; it waits for the saved station connection
instead of starting a portal. Pending, checking, reachable, no-reply and failed/
unavailable states stay separate from Wi-Fi association. A reply establishes
reachability to this public IP, not DNS resolution or reachability to every
service. No reply leaves Internet connectivity unconfirmed, since ICMP can be
filtered. This test sends no credentials, SSID or diagnostic report.

The overview offers RESET WI-FI, opening the phone portal for a new network.
It keeps the last saved credentials until a replacement connects and is saved;
cancellation or failure must not erase them. After this setup succeeds, only OK
is shown again. Returning later opens the overview and repeats the connection
test. Ping runs asynchronously, is bounded and is cancelled on leaving the
screen, changing networks, setup or BLE radio use. Simulator networking stays
disabled. These changes are source behavior until physically verified.

Policy constants: three ICMP requests, one-second interval/timeout per request,
a six-second check deadline and at most 25 seconds waiting for saved Wi-Fi.
IDF 4.4.7 provides ping/ping_sock.h and manages the worker task. Cancelled checks
retain their callback context through on_ping_end, generation-check late results,
and check session deletion before releasing the context. These are bounded
resource/lifetime policies, not packet delivery guarantees.

## Limits and validation

These changes preserve existing data rather than automatically formatting a
faulted store. They cannot reconstruct already-erased settings or repair arbitrary
flash corruption. A faulted, unconfirmed OTA may roll back on restart into an
older image that lacks this protection; the new application cannot retrofit the
old slot. Do not claim guaranteed recovery under that scenario. Ordinary valid-NVS
OTA retains the same storage layout and readable legacy settings.

New test source remains deferred. Builds and existing validators are recorded
in TESTING.md. Physical checks still required: saved Wi-Fi,
theme, rest alert, dim/sleep, presets and remembered devices across reboot and
OTA; OK immediately after successful save and after the hotspot closes; reopening
Wi-Fi without starting setup; cancellation/failed replacement preserving the old
network; save failure blocking OTA; NVS fault/rollback behavior. No physical motor
commands, USB flashing or raw private storage downloads are part of this change.

CONFIRMED (build/artifacts): CI 43885326367 at source 3b702fa builds both public
board images, offline core and release package. The downloaded ZIP matches
GitHub's digest. Both final ELFs contain strong GLOBAL erase-wrapper and deferred
rollback-hook definitions. Image descriptors, manifest size/hash, USB/OTA Normal
consistency and simulator isolation pass. No visual or physical interaction pass
is claimed; the repository's desktop adapters require Windows/macOS.
This build predates the subsequent configured-network overview/ping follow-up.
CONFIRMED final source: CI 43890790858 at e1c1e63 passes both public images, core
and package checks. Final ELF inspection retains the strong NVS hooks and puts
ping session APIs exclusively in Normal. Image identities/manifest/USB consistency
pass; hashes/sizes are recorded in TESTING.md. Physical network replies and
reset/setup interactions remain unverified.
