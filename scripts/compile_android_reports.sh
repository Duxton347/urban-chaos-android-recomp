#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
TASK_SDK="${ANDROID_HOME:-$PWD/.toolchain/android-sdk}"
TASK_JAVA="${JAVA_HOME:-$PWD/.toolchain/jdk/jdk-17.0.17+10}/bin/javac"
mkdir -p build/android-reports-contract
"$TASK_JAVA" -cp "$TASK_SDK/platforms/android-34/android.jar" -d build/android-reports-contract \
 launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/ReportSession.java \
 launcher/core/src/main/java/org/urbanchaos/launcher/diagnostics/NativeReports.java \
 launcher/android/src/main/java/org/urbanchaos/launcher/diagnostics/AndroidReports.java \
 launcher/android/src/main/java/org/urbanchaos/launcher/diagnostics/ReportExportController.java
# Compile the real launcher/SDL Java sources with authored build constants; no APK or game data.
mkdir -p build/android-launcher-contract/src/org/urbanchaos/launcher build/android-launcher-contract/classes
python3 - <<'PYTHON'
from pathlib import Path
root=Path('build/android-launcher-contract')
(root/'src/org/urbanchaos/launcher/BuildConfig.java').write_text('package org.urbanchaos.launcher; final class BuildConfig {static final String BUILD_ID="authored-contract";static final String XBE_SHA256="'+'a'*64+'";}\n')
paths=[]
for folder in ('launcher/core/src/main/java','launcher/android/src/main/java','launcher/app/src/main/java','vendor/SDL/android-project/app/src/main/java','build/android-launcher-contract/src'):
    paths.extend(str(path) for path in Path(folder).rglob('*.java'))
(root/'sources.txt').write_text('\n'.join(paths)+'\n')
PYTHON
"$TASK_JAVA" -source 17 -target 17 -cp "$TASK_SDK/platforms/android-34/android.jar" -d build/android-launcher-contract/classes @build/android-launcher-contract/sources.txt
