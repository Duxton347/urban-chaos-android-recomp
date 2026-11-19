#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 -m unittest discover -s tests -v
mkdir -p build/tests
python3 - <<'PY'
import sys,pathlib
sys.path.insert(0,'tests')
from test_tools import xiso
p=pathlib.Path('build/tests')
for n,b in [('good.iso',xiso()),('ps2.iso',b'PLAYSTATION SYSTEM.CNF'),('escape.iso',xiso(b'../escape')),('truncated.iso',xiso()[:34*2048+4])]: (p/n).write_bytes(b)
PY
TASK_JDK="${JAVA_HOME:-$PWD/.toolchain/jdk/jdk-17.0.17+10}/bin"
"$TASK_JDK/javac" -d build/tests android/app/src/main/java/org/urbanchaos/diagnostic/StoragePaths.java android/app/src/main/java/org/urbanchaos/diagnostic/Xiso.java tests/XisoTest.java
"$TASK_JDK/java" -cp build/tests org.urbanchaos.diagnostic.XisoTest "$PWD/build/tests"

python3 scripts/test_diagnostics.py

python3 scripts/test_subtitles.py
