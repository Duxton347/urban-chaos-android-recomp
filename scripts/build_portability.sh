#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_SDK="${ANDROID_HOME:-$PWD/.toolchain/android-sdk}"
TASK_CMAKE="$TASK_SDK/cmake/3.31.6/bin/cmake"
TASK_NDK="$TASK_SDK/ndk/28.2.13676358"
"$TASK_CMAKE" -S native/portability -B build/android-portability -G Ninja \
 -DCMAKE_MAKE_PROGRAM="$TASK_SDK/cmake/3.31.6/bin/ninja" \
 -DCMAKE_TOOLCHAIN_FILE="$TASK_NDK/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DCMAKE_BUILD_TYPE=Release
"$TASK_CMAKE" --build build/android-portability --parallel 4
"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/CMakeFiles/urban_cpu_contract.dir/cpu_contract.cpp.o

"$TASK_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" -h build/android-portability/liburban_recorded_packet_contract.a
