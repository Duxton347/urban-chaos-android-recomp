#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 scripts/bootstrap_sources.py
export JAVA_HOME="$PWD/.toolchain/jdk/jdk-17.0.17+10"
export PATH="$JAVA_HOME/bin:$PATH"
TASK_SDK="$PWD/.toolchain/android-sdk"
"$TASK_SDK/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$TASK_SDK" --licenses < <(yes)
"$TASK_SDK/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$TASK_SDK" 'platforms;android-34' 'build-tools;34.0.0' 'ndk;28.2.13676358' 'cmake;3.31.6'
scripts/build_host.sh
