"""Validate and package compiled artifacts; never contacts or flashes hardware."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_PARTITIONS = [
    (1, 2, 0x9000, 0x5000, b"nvs"),
    (1, 0, 0xE000, 0x2000, b"otadata"),
    (0, 0x10, 0x10000, 0x300000, b"normal"),
    (0, 0x11, 0x310000, 0x300000, b"normal_b"),
    (0, 0x12, 0x610000, 0x200000, b"simulator"),
    (1, 3, 0x810000, 0x20000, b"coredump"),
]

def validate_image(path, version, channel, maximum):
    data = path.read_bytes()
    if len(data) < 360 or len(data) > maximum or data[0] != 0xE9:
        raise ValueError(f"Invalid image/size: {path.name}")
    magic, layout, ver, board, mode = struct.unpack_from("<II32s16s16s", data, 288)
    fields = [v.split(b"\0", 1)[0].decode("ascii") for v in (ver, board, mode)]
    if (magic, layout, *fields) != (0x56524D54, 2, version, "guition-k5", channel):
        raise ValueError(f"Image identity/version does not match release: {path.name}")
    return data

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/release")
    args = parser.parse_args()
    version = (ROOT / "VERSION").read_text().strip()
    config = json.loads((ROOT / "config/release.json").read_text())
    if not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise ValueError("Invalid VERSION")
    if config["board"] != "guition-k5" or config["layout"] != 2:
        raise ValueError("Packager needs a matching board/layout implementation")
    normal = ROOT / ".pio/build/guition_k5_release"
    simulator = ROOT / ".pio/build/guition_k5_offline"
    firmware = validate_image(normal / "firmware.bin", version, "normal", 0x300000)
    validate_image(simulator / "firmware.bin", version, "simulator", 0x200000)
    table = (normal / "partitions.bin").read_bytes()
    if table != (simulator / "partitions.bin").read_bytes():
        raise ValueError("Partition tables differ")
    entries = [struct.unpack_from("<HBBII16sI", table, i) for i in range(0, len(table)-31, 32) if table[i:i+2] == b"\xaa\x50"]
    actual = [(e[1], e[2], e[3], e[4], e[5].rstrip(b"\0")) for e in entries]
    if actual != EXPECTED_PARTITIONS:
        raise ValueError("Unexpected partition layout")
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", ROOT / ".tools/platformio"))
    boot_app = core / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / "voltra-remote-guition-k5.bin").write_bytes(firmware)
    manifest = dict(schema=1, board=config["board"], channel="normal", layout=config["layout"],
                    version=version, size=len(firmware), sha256=hashlib.sha256(firmware).hexdigest())
    (out / "guition-k5.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")
    usb = out / "usb"
    usb.mkdir(exist_ok=True)
    images = [("bootloader.bin", normal / "bootloader.bin", 0, 0x8000),
              ("partitions.bin", normal / "partitions.bin", 0x8000, 0x1000),
              ("boot_app0.bin", boot_app, 0xE000, 0x2000),
              ("normal.bin", normal / "firmware.bin", 0x10000, 0x300000),
              ("simulator.bin", simulator / "firmware.bin", 0x610000, 0x200000)]
    files = []
    for name, source, offset, maximum in images:
        data = source.read_bytes()
        if not 0 < len(data) <= maximum:
            raise ValueError(f"Image exceeds its partition: {name}")
        (usb / name).write_bytes(data)
        files.append(dict(name=name, offset=offset, size=len(data), sha256=hashlib.sha256(data).hexdigest()))
    (usb / "bundle.json").write_text(json.dumps(dict(board="guition-k5", layout=2, version=version, files=files), indent=2)+"\n", encoding="utf-8")
    shutil.copy2(ROOT / "tools/install_firmware.py", usb / "install_firmware.py")
    shutil.copy2(ROOT / "docs/USB_INSTALL.md", usb / "README.md")
    for name in ("LICENSE", "THIRD_PARTY_NOTICES.md"):
        shutil.copy2(ROOT / name, usb / name)
    shutil.copytree(ROOT / "third_party/licenses", usb / "licenses", dirs_exist_ok=True)
    with zipfile.ZipFile(out / "voltra-remote-guition-k5-usb.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(usb.rglob("*")):
            if path.is_file(): archive.write(path, path.relative_to(usb))
    # Object files permit relinking the application with modified libraries.
    with zipfile.ZipFile(out / "voltra-remote-guition-k5-relink.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for env in (normal, simulator):
            for path in sorted(env.rglob("*")):
                if path.is_file() and path.suffix in (".o", ".a", ".elf", ".map"):
                    archive.write(path, path.relative_to(ROOT / ".pio/build"))
        archive.write(ROOT / "docs/FIRMWARE_UPDATES.md", "BUILD.md")
    print(f"Validated {version}: {len(firmware)} bytes; packaged in {out}")

if __name__ == "__main__":
    main()
