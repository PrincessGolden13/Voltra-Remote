"""One version/feed definition for firmware, UI and release packaging."""
import json
import re
from pathlib import Path
Import("env")
root = Path(env.subst("$PROJECT_DIR"))
version = (root / "VERSION").read_text().strip()
config = json.loads((root / "config/release.json").read_text())
if not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
    raise ValueError("VERSION must be a numeric major.minor.patch release")
if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", config["repository"]):
    raise ValueError("Invalid GitHub repository")
env.Append(CPPDEFINES=[
    ("VOLTRA_VERSION", '\\"' + version + '\\"'),
    ("VOLTRA_UPDATE_REPOSITORY", '\\"' + config["repository"] + '\\"'),
])
env.Append(LINKFLAGS=["-Wl,--undefined=voltraFirmwareIdentity"])
