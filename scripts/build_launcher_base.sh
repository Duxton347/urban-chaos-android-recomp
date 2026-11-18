#!/usr/bin/env bash
# Intermediate Java/manifest assembly only; private APK assembler injects and verifies the actual game libraries.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 scripts/verify_toolchain.py
export JAVA_HOME="$PWD/.toolchain/jdk/jdk-17.0.17+10"
export ANDROID_HOME="$PWD/.toolchain/android-sdk"
export GRADLE_USER_HOME="$PWD/.toolchain/gradle-home"
export PATH="$JAVA_HOME/bin:$PATH"
if [[ ! -f .toolchain/debug.keystore ]]; then
 "$JAVA_HOME/bin/keytool" -genkeypair -keystore .toolchain/debug.keystore -storepass android -keypass android -alias androiddebugkey -dname 'CN=Android Debug,O=Android,C=US' -keyalg RSA -keysize 2048 -validity 10000
fi
printf 'sdk.dir=%s\n' "$ANDROID_HOME" > launcher/local.properties
if [[ -n "${HTTPS_PROXY:-}" ]]; then
 TASK_PROXY_HOST=$(python3 -c 'import os,urllib.parse;print(urllib.parse.urlparse(os.environ["HTTPS_PROXY"]).hostname)')
 TASK_PROXY_PORT=$(python3 -c 'import os,urllib.parse;print(urllib.parse.urlparse(os.environ["HTTPS_PROXY"]).port or 80)')
 export GRADLE_OPTS="${GRADLE_OPTS:-} -Dhttps.proxyHost=$TASK_PROXY_HOST -Dhttps.proxyPort=$TASK_PROXY_PORT -Dhttp.proxyHost=$TASK_PROXY_HOST -Dhttp.proxyPort=$TASK_PROXY_PORT -Djavax.net.ssl.trustStore=/etc/ssl/certs/java/cacerts"
fi
exec .toolchain/gradle/gradle-8.7/bin/gradle -p launcher --no-daemon "$@"
