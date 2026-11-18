#!/usr/bin/env python3
"""Build the fixed SDL audio backend and exercise authored stereo/lifecycle fixtures."""
import hashlib
import json
import os
import pathlib
import subprocess
from prepare_runtime_overlay import ROOT, PIN

def main():
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    for name,pin in [('runtime',PIN),('SDL',lock['repositories']['SDL']['commit'])]:
        repository=ROOT/'vendor'/name
        if subprocess.check_output(['git','-C',str(repository),'rev-parse','HEAD'],text=True).strip()!=pin:
            raise ValueError(name+': wrong pin')
        if subprocess.check_output(['git','-C',str(repository),'status','--porcelain'],text=True).strip():
            raise ValueError(name+': preserve the pristine checkout')
    output=ROOT/'build/sdl-audio';output.mkdir(parents=True,exist_ok=True)
    cmake=ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
    with (output/'build.log').open('w') as log:
        subprocess.run([str(cmake),'-S',str(ROOT/'host/sdl-audio'),'-B',str(output),'-DCMAKE_BUILD_TYPE=Release'],check=True,stdout=log,stderr=subprocess.STDOUT)
        subprocess.run([str(cmake),'--build',str(output),'--parallel','4'],check=True,stdout=log,stderr=subprocess.STDOUT)
    env=dict(os.environ,SDL_AUDIODRIVER='dummy')
    fixture=subprocess.run([str(output/'sdl_audio_fixture')],env=env,check=True,capture_output=True,text=True,timeout=20)
    (output/'fixture.log').write_text(fixture.stdout+fixture.stderr)
    result=json.loads(fixture.stdout.strip().splitlines()[-1])
    env['SDL_AUDIODRIVER']='urban_nonexistent_audio_driver'
    failure=subprocess.run([str(output/'sdl_audio_fixture'),'--invalid-driver'],env=env,check=True,capture_output=True,text=True,timeout=10)
    (output/'invalid-driver.log').write_text(failure.stdout+failure.stderr)
    result.update(sdl_commit=lock['repositories']['SDL']['commit'],sdk_commit=PIN,driver='dummy',
        invalid_driver_rejected=True,audible_output_verified=False,android_device_tested=False)
    sources=['native/audio/sdl_output.h','native/audio/sdl_output.cpp','tests/sdl_audio_fixture.cpp','host/sdl-audio/CMakeLists.txt']
    result['sources']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}
    (output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));return 0

if __name__=='__main__':
    raise SystemExit(main())
