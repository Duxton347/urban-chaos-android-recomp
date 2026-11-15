#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 scripts/verify_toolchain.py
export JAVA_HOME="$PWD/.toolchain/jdk/jdk-17.0.17+10"
export ANDROID_HOME="$PWD/.toolchain/android-sdk"
export GRADLE_USER_HOME="$PWD/.toolchain/gradle-home"
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/cmake/3.31.6/bin:$PWD/.toolchain/python/bin:$PATH"
if [[ ! -f .toolchain/debug.keystore ]]; then
 "$JAVA_HOME/bin/keytool" -genkeypair -keystore .toolchain/debug.keystore -storepass android -keypass android -alias androiddebugkey -dname 'CN=Android Debug,O=Android,C=US' -keyalg RSA -keysize 2048 -validity 10000
fi
printf 'sdk.dir=%s\n' "$ANDROID_HOME" > android/local.properties
if [[ -n "${HTTPS_PROXY:-}" ]]; then
 proxy_host=$(python3 -c 'import os,urllib.parse;print(urllib.parse.urlparse(os.environ["HTTPS_PROXY"]).hostname)')
 proxy_port=$(python3 -c 'import os,urllib.parse;print(urllib.parse.urlparse(os.environ["HTTPS_PROXY"]).port or 80)')
 export GRADLE_OPTS="${GRADLE_OPTS:-} -Dhttps.proxyHost=$proxy_host -Dhttps.proxyPort=$proxy_port -Dhttp.proxyHost=$proxy_host -Dhttp.proxyPort=$proxy_port -Djavax.net.ssl.trustStore=/etc/ssl/certs/java/cacerts"
fi
exec .toolchain/gradle/gradle-8.7/bin/gradle -p android --no-daemon "$@"
