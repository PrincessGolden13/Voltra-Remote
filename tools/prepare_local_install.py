"""Prepare a private USB bundle preserving the owner's local bootstrap build."""
import hashlib
import json
import shutil
from package_release import ROOT, validate_image

version = (ROOT / "VERSION").read_text().strip()
local = ROOT / ".pio/build/guition_k5_normal"
release = ROOT / "build/release/usb"
data = validate_image(local / "firmware.bin", version, "normal", 0x300000)
if (local / "partitions.bin").read_bytes() != (release / "partitions.bin").read_bytes():
    raise ValueError("Local and public partition maps differ")
destination = ROOT / "build/local-usb"
shutil.copytree(release, destination, dirs_exist_ok=True)
(destination / "normal.bin").write_bytes(data)
meta = json.loads((destination / "bundle.json").read_text())
for entry in meta["files"]:
    if entry["name"] == "normal.bin":
        entry.update(size=len(data), sha256=hashlib.sha256(data).hexdigest())
(destination / "bundle.json").write_text(json.dumps(meta, indent=2)+"\n")
print("Prepared private build/local-usb. Do not upload this bundle or its firmware.")
