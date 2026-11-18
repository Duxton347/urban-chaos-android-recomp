#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_SDK="${ANDROID_HOME:-$PWD/.toolchain/android-sdk}"
TASK_CMAKE="$TASK_SDK/cmake/3.31.6/bin/cmake"
TASK_NDK="$TASK_SDK/ndk/28.2.13676358"
mkdir -p build
TASK_OVERLAY_ROOT="$(mktemp -d "$PWD/build/source-arm64-XXXXXX")"
trap 'rm -rf "$TASK_OVERLAY_ROOT"' EXIT
python3 - "$TASK_OVERLAY_ROOT/overlay" <<'PYTHON'
import sys
sys.path.insert(0, 'scripts')
from prepare_runtime_overlay import prepare
prepare(sys.argv[1])
PYTHON
"$TASK_CMAKE" -S native/portability -B build/android-portability -G Ninja \
 -DCMAKE_MAKE_PROGRAM="$TASK_SDK/cmake/3.31.6/bin/ninja" \
 -DCMAKE_TOOLCHAIN_FILE="$TASK_NDK/build/cmake/android.toolchain.cmake" \
 -DURBAN_SOURCE_OVERLAY="$TASK_OVERLAY_ROOT/overlay" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DCMAKE_BUILD_TYPE=Release
"$TASK_CMAKE" --build build/android-portability --parallel 4
"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/CMakeFiles/urban_cpu_contract.dir/cpu_contract.cpp.o

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_recorded_packet_contract.a

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_audio_cone_contract.a

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_audio_spatial_contract.a

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_game_source_contract.a

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_diagnostics_contract.a

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_vulkan_visibility_contract.a
