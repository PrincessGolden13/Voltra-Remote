#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'This build is for macOS. On Windows use tools/Build-Desktop.ps1.' >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
    echo 'CMake is missing from PATH. See docs/MAC_SIMULATOR.md.' >&2
    exit 1
fi
cmake -S "$project_root" -B "$project_root/build/mac" -G 'Unix Makefiles' \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_C_COMPILER="$(xcrun --find clang)" \
    -DCMAKE_CXX_COMPILER="$(xcrun --find clang++)" \
    -DCMAKE_OBJCXX_COMPILER="$(xcrun --find clang++)" "$@"
cmake --build "$project_root/build/mac" --parallel "${JOBS:-2}"
echo 'Built build/mac/voltra_simulator.app. Run: bash tools/Run-Simulator.sh'
