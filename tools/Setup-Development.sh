#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'This setup is for macOS. On Windows use tools/Setup-Development.ps1.' >&2
    exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo 'Install Apple Command Line Tools: xcode-select --install, then rerun this script.' >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
    echo 'Install CMake 3.24 or newer and add it to PATH, then rerun this script.' >&2
    echo 'If using CMake.app: export PATH="/Applications/CMake.app/Contents/bin:$PATH"' >&2
    exit 1
fi
mkdir -p "$project_root/.tools"
archive="$project_root/.tools/lvgl-9.3.0.zip"
expected='7225e7d2b83f89f3d089d3c3492083d447abb990fded24ad5bbaaa541e8746af'
if [[ ! -f "$archive" ]]; then
    curl --fail --location --retry 3 'https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip' -o "$archive.download"
    mv "$archive.download" "$archive"
fi
actual="$(shasum -a 256 "$archive")"
if [[ "${actual%% *}" != "$expected" ]]; then
    echo 'LVGL archive hash differs from the audited archive; investigate before using it.' >&2
    exit 1
fi
if [[ ! -f "$project_root/.tools/lvgl-9.3.0/CMakeLists.txt" ]]; then
    unzip -q -o "$archive" -d "$project_root/.tools"
fi
echo 'Mac simulator dependencies ready. Next: bash tools/Build-Desktop.sh'
