#!/usr/bin/env python3
"""Authored policy/ABI and disposable shader-cache contracts; never runs the game."""
import json,pathlib,subprocess,tempfile,shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='mission-contracts-',dir=ROOT/'build') as tmp:
 out=pathlib.Path(tmp)
 for name,sources in [('automatic-fire',['tests/automatic_fire_fixture.cpp','native/input/automatic_fire.cpp','vendor/runtime/runtime/kernel/memory.cpp','vendor/runtime/runtime/core/log.cpp']),('shader-cache',['tests/shader_cache_fixture.cpp'])]:
  binary=out/name
  subprocess.run(['c++','-std=c++20','-O2','-UNDEBUG','-fsigned-char','-pthread','-I'+str(ROOT/'native'),'-I'+str(ROOT/'vendor/runtime/runtime'),'-I'+str(ROOT/'vendor/runtime/harness/compat'),*[str(ROOT/s) for s in sources],'-o',str(binary)],check=True)
  subprocess.run([str(binary),*([str(out/'cache')] if name=='shader-cache' else [])],check=True)
 pad=out/'sdk/harness/pad_script.cpp';pad.parent.mkdir(parents=True)
 shutil.copyfile(ROOT/'vendor/runtime/harness/pad_script.cpp',pad)
 subprocess.run(['git','apply','--directory='+str((out/'sdk').relative_to(ROOT)),str(ROOT/'patches/runtime/research-right-stick.patch')],cwd=ROOT,check=True)
 binary=out/'scripted-stick'
 subprocess.run(['c++','-std=c++20','-O2','-UNDEBUG','-I'+str(ROOT/'vendor/runtime/runtime'),'-I'+str(ROOT/'vendor/runtime/harness'),str(ROOT/'tests/scripted_stick_fixture.cpp'),str(pad),str(ROOT/'vendor/runtime/runtime/core/log.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],cwd=out,check=True)
print(json.dumps({'status':'PASS','automatic_fire_hook_contract':True,'shader_cache_contract':True,'game_data_used':False,'gameplay_validated':False}))
