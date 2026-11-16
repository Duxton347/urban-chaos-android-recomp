#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ $# -lt 2 || $# -gt 4 ]]; then
 echo 'Usage: compile_private_generated.sh <ignored-generated-dir> <ignored-build-dir> [linux|android-arm64] [patched-runtime-headers]' >&2
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
TASK_MODE="${3:-linux}"
TASK_ARGS=()
case "$TASK_MODE" in
 linux) ;;
 android-arm64)
  TASK_SDK="${ANDROID_HOME:-$PWD/.toolchain/android-sdk}"
  TASK_CMAKE="$TASK_SDK/cmake/3.31.6/bin/cmake"
  TASK_NDK="$TASK_SDK/ndk/28.2.13676358"
  TASK_ARGS=(-G Ninja "-DCMAKE_MAKE_PROGRAM=$TASK_SDK/cmake/3.31.6/bin/ninja"
   "-DCMAKE_TOOLCHAIN_FILE=$TASK_NDK/build/cmake/android.toolchain.cmake"
   -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29)
  ;;
 *) echo 'Unsupported target; use linux or android-arm64' >&2; exit 2 ;;
esac
if [[ $# -eq 4 ]]; then
 TASK_HEADERS=$(realpath "$4")
 python3 - "$PWD" "$TASK_HEADERS" <<'PYHEADERS'
import pathlib,sys
root,path=map(pathlib.Path,sys.argv[1:])
if not any((root/d) in path.parents for d in ('build','private')) or not (path/'cpu/x86_context.h').is_file():
 raise SystemExit('Patched SDK headers must exist inside ignored private/ or build/')
PYHEADERS
 TASK_ARGS+=("-DURBAN_RUNTIME_HEADERS=$TASK_HEADERS")
fi
"$TASK_CMAKE" -S host/guest-compile -B "$2" -DURBAN_GENERATED="$TASK_GENERATED" -DCMAKE_BUILD_TYPE=Release "${TASK_ARGS[@]}"
"$TASK_CMAKE" --build "$2" --parallel 2
