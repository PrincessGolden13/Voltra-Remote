# Phone setup repair and short PIN

Later same-day feedback supersedes the PIN flow below: the user confirmed the
form now works and explicitly removed the optional PIN. Settings -> Wi-Fi now
starts setup immediately, and the phone has a native select control populated
from scanned 2.4 GHz SSIDs, a Refresh button, and an explicit hidden-network
fallback. The old datalist looked like mandatory manual entry in the phone's
captive browser. The field is replaced rather than relying on suggestions.

GET /networks now reports names plus scanning/failed state. POST /scan accepts
the current session token and queues an async scan; the HTTP task never calls
radio APIs. At most one scan runs, bounded to 15 s (the bundled Wi-Fi library
also has its own timeout). A credential submission during refresh waits for
the scan to settle before joining. Scans log counts only, never SSIDs/passwords.
The page preserves selected names, shows a scan message, and checks connection
success only after its configuration POST has been accepted. Physical scan,
refresh, join and save still require the user's trial.

No website API supplies the phone's saved Wi-Fi password. The Network
Information API reports connection characteristics, not credentials or SSID;
the remote therefore scans with its own radio and asks for the selected
network's password once. This does not add a companion phone app or cloud flow.
[Browser API reference](https://developer.mozilla.org/en-US/docs/Web/API/NetworkInformation).

The sections below document the earlier same-day implementation and evidence.

CONFIRMED (user): joining the original hotspot worked. Both the phone's
automatic captive window and direct 192.168.4.1 displayed only `{}`; the
browser offered to save document.json. Home-network provisioning did not work.
The user requested four digits instead of the long hotspot password and
explicitly accepted reduced security for this temporary setup network.

## Socket rejection

CONFIRMED (source): every explicit old `{}` reply was the HTTP 403 branch in
setupClient. That function allocated sockaddr_in and accepted only AF_INET.
This board's bundled SDK enables CONFIG_LWIP_IPV6. ESP-IDF 4.4.7's HTTP server
creates its listener with PF_INET6 when that flag is enabled. IPv4 connections
on that listener can be presented as IPv4-mapped IPv6, requiring a larger
sockaddr_in6 and recognition of the ::ffff: prefix.

INFERRED (matching reported symptom): this rejected the phone before HTML was
served. The previous build did not log socket families, so the actual rejected
phone socket was not captured. The corrected implementation uses
sockaddr_storage and decodes both native IPv4 and IPv4-mapped IPv6. It checks
the peer's setup subnet and the socket's local AP address, retaining the
restriction against access through the joined home-network interface.

The root returns text/html, including a readable wrong-network page instead
of empty JSON on refusal. API refusal bodies contain useful messages. Captive
fallback retains the canonical redirect and supplies an HTML link. Diagnostics
log HTML delivery or rejected address families, never network passwords/PINs.

- [ESP-IDF v4.4.7 HTTP listener, httpd_server_init](https://github.com/espressif/esp-idf/blob/v4.4.7/components/esp_http_server/src/httpd_main.c#L243-L269)
- Local board SDK: framework-arduinoespressif32 3.20017.241212+sha.dcc1105b,
  esp32s3/qio_qspi/include/sdkconfig.h, CONFIG_LWIP_IPV6=1.

## Four-digit entry

CONFIRMED (source): Arduino-ESP32 softAP rejects nonempty passwords shorter
than eight characters. A four-digit WPA2 password cannot be implemented.
Use a temporary open AP and a generated four-digit setup PIN, 1000..9999,
entered on the phone form along with the home SSID/password. The remote says
"(no password)" and "Page PIN". The PIN changes each setup, is not embedded in
the page or saved, and must match before any credential submission is queued.
The existing session token, field bounds, five-minute expiry and successful-save
closure remain. This PIN is a usability credential, not Wi-Fi encryption.

- [Arduino-ESP32 2.0.17 softAP passphrase validation](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/src/WiFiAP.cpp#L116-L136)

No BLE/protocol/training path changes. Simulator links only the synthetic
Wi-Fi status. Validation and physical installation are recorded in BASELINE.md;
new automated test code remains deferred and cases are tracked in TESTING.md.
