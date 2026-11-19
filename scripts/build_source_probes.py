#!/usr/bin/env python3
"""Compile and test the shared ISO/XBE loader with authored files, never a game."""
import pathlib,subprocess,tempfile,sys
from prepare_runtime_overlay import prepare,ROOT
cmake=ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
(ROOT/'build').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='source-probes-',dir=ROOT/'build') as temporary:
    output=pathlib.Path(temporary);prepare(output/'overlay')
    subprocess.run([str(cmake),'-S',str(ROOT/'host/source-probes'),'-B',str(output/'cmake'),'-DURBAN_OVERLAY='+str(output/'overlay')],check=True)
    subprocess.run([str(cmake),'--build',str(output/'cmake'),'--parallel','2'],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/source_probe_cases.py'),str(output/'cmake/game_source_probe')],check=True)
