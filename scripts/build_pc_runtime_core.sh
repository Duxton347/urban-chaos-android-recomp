#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_CMAKE="$PWD/.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake"
"$TASK_CMAKE" -S host/runtime-core -B build/pc-runtime-core -DCMAKE_BUILD_TYPE=Release
"$TASK_CMAKE" --build build/pc-runtime-core --parallel 4
"$(dirname "$TASK_CMAKE")/ctest" --test-dir build/pc-runtime-core --output-on-failure -V
