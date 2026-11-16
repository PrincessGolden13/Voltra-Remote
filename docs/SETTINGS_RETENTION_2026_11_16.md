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
Connected-and-saved or completed setup shows the saved network and exactly one
button, OK, which stops the temporary hotspot and returns to Settings. There is
no extra back button after success. Before success, a back arrow remains and CANCEL exits
setup without overwriting the stored network. A disconnected saved network can
be changed explicitly; a failed candidate does not overwrite the previous one.

## Limits and validation

These changes preserve existing data rather than automatically formatting a
faulted store. They cannot reconstruct already-erased settings or repair arbitrary
flash corruption. A faulted, unconfirmed OTA may roll back on restart into an
older image that lacks this protection; the new application cannot retrofit the
old slot. Do not claim guaranteed recovery under that scenario. Ordinary valid-NVS
OTA retains the same storage layout and readable legacy settings.

New test source remains deferred. Builds and existing validators are recorded
in TESTING.md after execution. Physical checks still required: saved Wi-Fi,
theme, rest alert, dim/sleep, presets and remembered devices across reboot and
OTA; OK immediately after successful save and after the hotspot closes; reopening
Wi-Fi without starting setup; cancellation/failed replacement preserving the old
network; save failure blocking OTA; NVS fault/rollback behavior. No physical motor
commands, USB flashing or raw private storage downloads are part of this change.
