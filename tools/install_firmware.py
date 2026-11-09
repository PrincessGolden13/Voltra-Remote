"""USB installation with a verified full recovery backup. Requires esptool 4.9.0."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys

FLASH_SIZE = 16 * 1024 * 1024
EXPECTED = {"bootloader.bin": (0, 0x8000), "partitions.bin": (0x8000, 0x1000),
            "boot_app0.bin": (0xE000, 0x2000), "normal.bin": (0x10000, 0x300000),
            "simulator.bin": (0x610000, 0x200000)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--bundle", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--backup-dir", type=Path, default=Path("hardware-backups"))
    parser.add_argument("--confirm-unloaded", action="store_true", help="Owner has confirmed every VOLTRA is unloaded and nobody is training")
    parser.add_argument("--esptool", type=Path, help="Optional pinned esptool.py path")
    args = parser.parse_args()
    bundle = args.bundle.resolve()
    meta = json.loads((bundle / "bundle.json").read_text())
    if meta.get("board") != "guition-k5" or meta.get("layout") != 2 or {f["name"] for f in meta["files"]} != set(EXPECTED) or len(meta["files"]) != len(EXPECTED):
        raise ValueError("Unsupported or incomplete USB bundle")
    pairs = []
    for entry in meta["files"]:
        name = entry["name"]
        offset, limit = EXPECTED[name]
        data = (bundle / name).read_bytes()
        if entry["offset"] != offset or not 0 < len(data) <= limit or len(data) != entry["size"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
            raise ValueError(f"Corrupt or misplaced image: {name}")
        pairs.extend([hex(offset), str(bundle / name)])
    if not args.confirm_unloaded:
        raise SystemExit("Before USB installation: unload all VOLTRAs and stop training, then pass --confirm-unloaded. This installer cannot stop a VOLTRA.")
    tool = [sys.executable, str(args.esptool)] if args.esptool else [sys.executable, "-m", "esptool"]
    base = tool + ["--chip", "esp32s3", "--port", args.port, "--baud", "460800"]
    def run(*command):
        subprocess.run(base + list(command), check=True)
    # Read-only until the entire recovery image has been read and verified against
    # the device. NVS (0x9000..0xdfff) is absent from every write operation.
    info = subprocess.run(base + ["flash_id"], check=True, capture_output=True, text=True)
    if "Detected flash size: 16MB" not in info.stdout:
        raise SystemExit("This bundle requires the verified 16 MB ESP32-S3 board")
    args.backup_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    backup = (args.backup_dir / ("guition-before-update-" + stamp + ".bin")).resolve()
    run("read_flash", "0", hex(FLASH_SIZE), str(backup))
    if backup.stat().st_size != FLASH_SIZE:
        raise SystemExit("Incomplete backup; nothing written")
    digest = hashlib.sha256(backup.read_bytes()).hexdigest()
    run("verify_flash", "0", str(backup))
    backup.with_suffix(".json").write_text(json.dumps(dict(size=FLASH_SIZE, sha256=digest), indent=2)+"\n")
    print("Verified private recovery backup:", backup)
    run("write_flash", "--flash_mode", "keep", "--flash_freq", "80m", "--flash_size", "16MB", *pairs)
    print("USB installation complete. Keep the backup private: it contains device credentials.")

if __name__ == "__main__":
    main()
