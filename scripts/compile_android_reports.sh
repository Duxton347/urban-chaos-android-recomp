#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_SDK="${ANDROID_HOME:-$PWD/.toolchain/android-sdk}"
TASK_JAVA="${JAVA_HOME:-$PWD/.toolchain/jdk/jdk-17.0.17+10}/bin/javac"
mkdir -p build/android-reports-contract
"$TASK_JAVA" -cp "$TASK_SDK/platforms/android-34/android.jar" -d build/android-reports-contract \
 launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/ReportSession.java \
 launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/NativeReports.java \
 launcher/android/src/main/java/org/urbanchaos/launcher/diagnostics/AndroidReports.java
