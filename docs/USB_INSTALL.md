# First USB installation

This bundle is for the **Guition JC3636K518, ESP32-S3, 16 MB flash / 8 MB OPI
PSRAM, 360 x 360 touchscreen rotary remote**. It is not an Elecrow 480 x 480 image.
It installs Normal A/B firmware slots plus the isolated Simulator. Existing
remote preferences, saved Wi-Fi and device memory stay in NVS.

Public firmware is development software. A fresh, generated BLE bootstrap identity
has not established a working connection in our physical trials. Owners of other
remotes can build the simulator and contribute, but cannot yet assume plug-and-play
VOLTRA pairing. See the repository's connection evidence and safety documentation.

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
