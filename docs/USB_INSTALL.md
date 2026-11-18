# First USB installation

This bundle is for the **Guition JC3636K518, ESP32-S3, 16 MB flash / 8 MB OPI
PSRAM, 360 x 360 touchscreen rotary remote**. It is not an Elecrow 480 x 480 image.
It installs Normal A/B firmware slots plus the isolated Simulator. Existing
remote preferences, saved Wi-Fi and device memory stay in NVS.

Public firmware is development software. A fresh, generated BLE bootstrap identity
has not established a working connection in our physical trials. Owners of other
remotes can build the simulator and contribute, but cannot yet assume plug-and-play
VOLTRA pairing. See the repository's connection evidence and safety documentation.

**USB-C orientation matters on this remote.** The same connector can expose
either its main ESP32-S3 or its secondary ESP32, depending on which way the plug
is inserted. If Windows shows a CH340 serial port or esptool identifies a 4 MB
ESP32, unplug the USB-C end at the remote, turn the plug 180 degrees, and reconnect.
The main processor should appear as USB Serial/JTAG (USB VID 303A, PID 1001)
and identify as ESP32-S3 with 16 MB flash. Its Windows driver is built in;
installing a CH340 driver does not select the main processor. Verify the chip,
not just the COM number. Never flash the secondary processor with this bundle.
This orientation change was physically confirmed on the owner's remote on
November 17, 2026; see HARDWARE_COMPATIBILITY.md.

1. Unload every VOLTRA and stop training before connecting the remote over USB.
2. Install Python 3 and `python -m pip install esptool==4.9.0`.
3. Extract the USB release ZIP and run from that directory:

   ```text
   python install_firmware.py --port COM3 --confirm-unloaded
   ```

   Replace COM3 with the actual serial port (on macOS, `/dev/cu.usbmodem...`).
   The installer reads all 16 MB, verifies that recovery copy against the remote,
   then installs the images. It refuses incompatible chip/flash sizes and corrupted
   bundle files. It cannot detect the display model: check the exact board label.

The `hardware-backups` directory contains private flash/NVS contents. Keep it out
of GitHub, bug reports and shared folders. Preserve it for recovery.

After reboot, configure Wi-Fi under Settings if needed. The remote checks for
published firmware at startup and prompts before installing. Settings -> Updates
also checks GitHub. Installation requires leaving the VOLTRA connection and keeping
power connected. Network loss never prevents ordinary offline BLE operation.

Do not flash the single OTA `.bin` at address zero. Use this USB bundle for the
initial partition migration; subsequent updates use only the Normal app image.
