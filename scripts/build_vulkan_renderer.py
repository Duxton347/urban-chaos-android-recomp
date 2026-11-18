#!/usr/bin/env python3
"""Compile the adapted renderer against both SDK/host ABIs; no title inputs or game execution."""
import argparse
import json
import os
import pathlib
import subprocess
import tempfile
from prepare_runtime_overlay import prepare, ROOT
from prepare_vulkan_renderer import prepare_renderer, software_headless_icd
from build_vulkan_reports import digest


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--android', action='store_true')
    args = parser.parse_args()
    sdk = pathlib.Path(os.environ.get('ANDROID_HOME', ROOT/'.toolchain/android-sdk'))
    ndk = sdk/'ndk/28.2.13676358'
    if 'Pkg.Revision = 28.2.13676358' not in (ndk/'source.properties').read_text():
        raise RuntimeError('Pinned NDK required')
    cmake = sdk/'cmake/3.31.6/bin/cmake'
    if not cmake.is_file():
        cmake = ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
    if subprocess.check_output([str(cmake),'--version'],text=True).splitlines()[0] != 'cmake version 3.31.6':
        raise RuntimeError('Pinned CMake required')
    output = ROOT/'build/vulkan-renderer'
    output.mkdir(parents=True,exist_ok=True)
    receipt = {'status':'BUILDING','game_data_used':False,'game_execution_tested':False,
               'android_device_tested':False,'android_host_integrated':False,'architectures':{}}
    with tempfile.TemporaryDirectory(prefix='renderer-source-',dir=ROOT/'build') as temporary:
        source = pathlib.Path(temporary)
        receipt['runtime_patches'] = prepare(source/'sdk',context=True)
        renderer = prepare_renderer(source/'vulkan')
        receipt['renderer_source_sha256'] = digest(renderer)
        receipt['renderer_patch_sha256'] = digest(ROOT/'patches/black/vulkan-headless-research.patch')
        for android in ([False,True] if args.android else [False]):
            name = 'arm64' if android else 'host'
            build = output/name
            headers = ndk/'toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include' if android else ROOT/'build/vulkan-reports/headers'
            command = [cmake,'-S',ROOT/'host/vulkan-research','-B',build,
                       '-DURBAN_VULKAN_SOURCE='+str(renderer),'-DURBAN_OVERLAY='+str(source/'sdk'),
                       '-DURBAN_VULKAN_HEADERS='+str(headers),'-DCMAKE_BUILD_TYPE=Release']
            if android:
                command += ['-DCMAKE_TOOLCHAIN_FILE='+str(ndk/'build/cmake/android.toolchain.cmake'),
                            '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-29']
            with (output/(name+'.log')).open('w') as log:
                subprocess.run(list(map(str,command)),check=True,stdout=log,stderr=subprocess.STDOUT)
                subprocess.run([str(cmake),'--build',str(build),'--parallel','2'],check=True,stdout=log,stderr=subprocess.STDOUT)
            archive = build/'liburban_vulkan_renderer_contract.a'
            reader = ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf'
            elf = subprocess.check_output([str(reader),'-h',str(archive)],text=True)
            machines = [line.split(':',1)[1].strip() for line in elf.splitlines() if 'Machine:' in line]
            if len(machines)!=2 or any(machine != ('AArch64' if android else 'Advanced Micro Devices X86-64') for machine in machines):
                raise RuntimeError('Renderer archive architecture mismatch')
            receipt['architectures'][name] = {'sha256':digest(archive),'objects':len(machines)}
            if not android:
                icd = software_headless_icd()
                receipt['software_icd_sha256'] = digest(icd)
                env = dict(os.environ,VK_ICD_FILENAMES=str(icd),VK_DRIVER_FILES=str(icd))
                for key in ('VK_INSTANCE_LAYERS','URBAN_VK_CAPTURE_EVERY','URBAN_VK_CAPTURE_FROM'):
                    env.pop(key,None)
                fixture = subprocess.run([str(build/'vulkan_renderer_fixture')],cwd=output,env=env,
                    capture_output=True,text=True,timeout=60)
                (output/'fixture.log').write_text(fixture.stdout+fixture.stderr)
                fixture.check_returncode()
                if any(marker in fixture.stdout+fixture.stderr for marker in ('does not compile','does not link','failed (','UC-VK-')):
                    raise RuntimeError('Renderer fixture logged a shader, driver or contract failure')
                if 'VULKAN_RENDERER_CONTRACT:' not in fixture.stdout:
                    raise RuntimeError('Renderer fixture did not complete')
                receipt['authored_renderer_fixture_passed'] = True
                receipt['fixture_executable_sha256'] = digest(build/'vulkan_renderer_fixture')
    receipt['status'] = 'PASS'
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
