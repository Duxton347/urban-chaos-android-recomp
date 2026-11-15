#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
CMAKE="$PWD/.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake"
"$CMAKE" -S host -B build/host -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build build/host --parallel 4
python3 scripts/host_receipt.py
c++ --version | head -1
