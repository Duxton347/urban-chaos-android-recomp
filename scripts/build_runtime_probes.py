#!/usr/bin/env python3
"""Build/run SDK GPU regression tests with entirely synthetic state, without a game."""
import json
import pathlib
import subprocess
import tempfile
from prepare_runtime_overlay import prepare, ROOT, PIN

cmake = ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
(ROOT/'build').mkdir(exist_ok=True)
# New paths prevent stale overlays masking a changed patch in local reruns or CI.
with tempfile.TemporaryDirectory(prefix='runtime-probes-', dir=ROOT/'build') as temporary:
    output = pathlib.Path(temporary)
    hashes = prepare(output/'overlay')
    subprocess.run([str(cmake), '-S', str(ROOT/'host/runtime-probes'), '-B', str(output/'cmake'),
                    '-DURBAN_OVERLAY='+str(output/'overlay')], check=True)
    subprocess.run([str(cmake), '--build', str(output/'cmake'), '--parallel', '2'], check=True)
    subprocess.run([str(cmake.parent/'ctest'), '--test-dir', str(output/'cmake'), '--output-on-failure', '-V'], check=True)
    print(json.dumps({'sdk': PIN, 'patches': hashes, 'synthetic_gpu_probes_passed': 2, 'game_data_used': False}))
