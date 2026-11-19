#!/usr/bin/env python3
"""Build explicit continuation-aware SDK tool copy; keep original tools and receipt unchanged."""
import argparse
import hashlib
import json
import pathlib
import subprocess
from prepare_runtime_overlay import prepare, ROOT, PIN

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',type=pathlib.Path,default=ROOT/'build/context-tools-patched')
args=parser.parse_args()
output=args.out.resolve()
if (ROOT/'build') not in output.parents or output.exists():
    raise SystemExit('Context tool output must be new and inside ignored build/')
lock=json.loads((ROOT/'dependencies.lock.json').read_text())
for name,relative in (('zydis','vendor/zydis'),('tomlplusplus','vendor/tomlplusplus'),('zycore','vendor/zydis/dependencies/zycore')):
    checkout=ROOT/relative
    actual=subprocess.check_output(['git','-C',str(checkout),'rev-parse','HEAD'],text=True).strip()
    if actual!=lock['repositories'][name]['commit']:
        raise SystemExit('Dependency pin mismatch: '+name)
output.mkdir(parents=True)
overlay = output/'sdk'
patches = prepare(overlay, context=True, tools_only=True)
cmake = ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
subprocess.run([str(cmake),'-S',str(ROOT/'host/context-tools'),'-B',str(output/'cmake'),
                '-DURBAN_CONTEXT_SDK='+str(overlay),'-DCMAKE_BUILD_TYPE=Release'],check=True)
subprocess.run([str(cmake),'--build',str(output/'cmake'),'--parallel','2'],check=True)
binary=output/'cmake/xbe_recomp_resumable'
with binary.open('rb') as source: digest=hashlib.file_digest(source,'sha256').hexdigest()
(output/'receipt.json').write_text(json.dumps({'sdk':PIN,'patches':patches,'binary_sha256':digest,'g1_qualified':False},indent=2)+'\n')
print('Continuation-aware public SDK tool compiled; production qualification remains false')
