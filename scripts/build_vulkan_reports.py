#!/usr/bin/env python3
"""Exercise actual Vulkan occlusion queries with authored shaders and geometry; never game data."""
import hashlib
import json
import os
import pathlib
import subprocess

ROOT=pathlib.Path(__file__).resolve().parents[1]
NDK_VERSION='28.2.13676358'

def digest(path):
    with pathlib.Path(path).open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()

def main():
    sdk=pathlib.Path(os.environ.get('ANDROID_HOME',ROOT/'.toolchain/android-sdk'))
    ndk=sdk/'ndk'/NDK_VERSION
    if not ('Pkg.Revision = '+NDK_VERSION) in (ndk/'source.properties').read_text():
        raise ValueError('Wrong NDK; fixed Vulkan headers and shader compiler required')
    compiler=ndk/'shader-tools/linux-x86_64/glslc'
    cmake=sdk/'cmake/3.31.6/bin/cmake'
    if not cmake.is_file():cmake=ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
    output=ROOT/'build/vulkan-reports';output.mkdir(parents=True,exist_ok=True)
    # Only Vulkan headers enter the HOST search path; do not shadow glibc with Android libc headers.
    headers=output/'headers';headers.mkdir(exist_ok=True)
    include=ndk/'toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include'
    for name in ('vulkan','vk_video'):
        link=headers/name
        if link.is_symlink() and link.resolve()!=(include/name).resolve():link.unlink()
        if not link.exists():link.symlink_to((include/name).resolve(),target_is_directory=True)
    candidates=[pathlib.Path(p) for p in (
        '/usr/lib/chromium/vk_swiftshader_icd.json','/opt/google/chrome/vk_swiftshader_icd.json',
        '/usr/share/vulkan/icd.d/lvp_icd.x86_64.json','/usr/share/vulkan/icd.d/lvp_icd.json')]
    icd=next((p for p in candidates if p.is_file()),None)
    if icd is None:raise RuntimeError('No software Vulkan ICD found; the Vulkan test was not executed')
    metadata=json.loads(icd.read_text())
    driver=pathlib.Path(metadata['ICD']['library_path'])
    if not driver.is_absolute():driver=icd.parent/driver
    if not driver.is_file():
        driver=next((p/driver.name for p in map(pathlib.Path,('/lib/x86_64-linux-gnu','/usr/lib/x86_64-linux-gnu')) if (p/driver.name).is_file()),driver)
    if not driver.is_file():raise RuntimeError('Selected software ICD library is unavailable')
    loaders=[pathlib.Path(p) for p in ('/lib/x86_64-linux-gnu/libvulkan.so.1','/usr/lib/x86_64-linux-gnu/libvulkan.so.1')]
    loaders.append(icd.parent/'libvulkan.so.1')
    loader=next((p for p in loaders if p.is_file()),None)
    if loader is None:raise RuntimeError('No host Vulkan loader found')
    shaders=[]
    with (output/'build.log').open('w') as log:
        for name in ('fullscreen.vert','visibility.frag'):
            source=ROOT/'tests/fixtures/vulkan'/name;target=output/(name+'.spv');shaders.append(target)
            subprocess.run([str(compiler),'--target-env=vulkan1.1',str(source),'-o',str(target)],check=True,stdout=log,stderr=subprocess.STDOUT)
        subprocess.run([str(cmake),'-S',str(ROOT/'host/vulkan-reports'),'-B',str(output),
            '-DURBAN_VULKAN_HEADERS='+str(headers),'-DURBAN_VULKAN_LIBRARY='+str(loader),'-DCMAKE_BUILD_TYPE=Release'],check=True,stdout=log,stderr=subprocess.STDOUT)
        subprocess.run([str(cmake),'--build',str(output),'--parallel','2'],check=True,stdout=log,stderr=subprocess.STDOUT)
    env=dict(os.environ,VK_ICD_FILENAMES=str(icd),VK_DRIVER_FILES=str(icd))
    # Explicit inherited layer requests are not part of this fixture.
    env.pop('VK_INSTANCE_LAYERS',None)
    process=subprocess.run([str(output/'vulkan_reports_fixture'),*map(str,shaders)],env=env,capture_output=True,text=True,timeout=30)
    (output/'test.log').write_text(process.stdout+process.stderr)
    process.check_returncode();receipt=json.loads(process.stdout.strip().splitlines()[-1])
    if receipt['device_type']!=4:raise RuntimeError('The selected test ICD did not expose a CPU Vulkan device')
    cmake_version=subprocess.check_output([str(cmake),'--version'],text=True).splitlines()[0]
    if cmake_version!='cmake version 3.31.6':raise RuntimeError('Wrong CMake version')
    receipt['cmake_version']=cmake_version
    receipt['host_compiler']=subprocess.check_output(['c++','--version'],text=True).splitlines()[0]
    receipt.update(ndk=NDK_VERSION,glslc_sha256=digest(compiler),icd_sha256=digest(icd),
        driver_library_sha256=digest(driver),loader_sha256=digest(loader),
        validation_layers_enabled=False,validation_errors=None,game_vulkan_backend_integrated=False,
        headers_source='Pinned Android NDK Vulkan/vk_video headers; host uses its own libc',
        shader_spirv_sha256={p.name:digest(p) for p in shaders})
    sources=['native/gpu/vulkan_visibility.h','native/gpu/vulkan_visibility.cpp','native/gpu/visibility_report.h',
        'native/gpu/vulkan_visibility_contract.cpp','scripts/build_vulkan_reports.py','tests/vulkan_reports_fixture.cpp','tests/fixtures/vulkan/fullscreen.vert','tests/fixtures/vulkan/visibility.frag','host/vulkan-reports/CMakeLists.txt']
    receipt['sources']={name:digest(ROOT/name) for name in sources}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt));return 0

if __name__=='__main__':
    raise SystemExit(main())
