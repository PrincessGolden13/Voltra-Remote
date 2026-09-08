#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
simulator="$project_root/build/mac/voltra_simulator.app/Contents/MacOS/voltra_simulator"
if [[ ! -x "$simulator" ]]; then
    echo 'Build the Mac simulator first: bash tools/Build-Desktop.sh' >&2
    exit 1
fi
# Keep relative render paths predictable, regardless of the caller's directory.
cd "$project_root"
exec "$simulator" "$@"
