#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ $# != 2 ]]; then
 echo 'Usage: compile_private_generated.sh <ignored-generated-dir> <ignored-build-dir>' >&2
 exit 2
fi
python3 - "$PWD" "$1" "$2" <<'PY'
import pathlib,sys
root=pathlib.Path(sys.argv[1]).resolve()
for raw in sys.argv[2:]:
 p=pathlib.Path(raw).resolve()
 if not any((root/d) in p.parents for d in ('build','private')):raise SystemExit('Generated sources and output must stay in ignored private/ or build/')
PY
TASK_CMAKE="$PWD/.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake"
TASK_GENERATED=$(realpath "$1")
"$TASK_CMAKE" -S host/guest-compile -B "$2" -DURBAN_GENERATED="$TASK_GENERATED" -DCMAKE_BUILD_TYPE=Release
"$TASK_CMAKE" --build "$2" --parallel 2
