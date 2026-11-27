#!/usr/bin/env python3
"""Qualify Black's pinned translator against SDK HLSL and actual Vulkan queries."""
import argparse
import json
import os
import pathlib
import subprocess

from bootstrap_vulkan_shaders import prepare
from build_vulkan_reports import digest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--android', action='store_true', help='Also compile the real translator and glslang for AArch64')
    args = parser.parse_args()
    source = prepare()
    sdk = pathlib.Path(os.environ.get('ANDROID_HOME', ROOT / '.toolchain/android-sdk'))
    ndk = sdk / 'ndk/28.2.13676358'
    cmake = sdk / 'cmake/3.31.6/bin/cmake'
    if not cmake.is_file():
        cmake = ROOT / '.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
    if subprocess.check_output([str(cmake), '--version'], text=True).splitlines()[0] != 'cmake version 3.31.6':
        raise RuntimeError('Pinned CMake required')
    output = ROOT / 'build/vulkan-shaders'
    output.mkdir(parents=True, exist_ok=True)
    with (output / 'build.log').open('w') as log:
        def run(command):
            subprocess.run(list(map(str, command)), check=True, stdout=log, stderr=subprocess.STDOUT)
        run([cmake, '-S', ROOT / 'host/vulkan-shaders', '-B', output,
             '-DCMAKE_BUILD_TYPE=Release', '-DURBAN_BLACK_SHADER_SOURCE=' + str(source)])
        run([cmake, '--build', output, '--parallel', '2'])
        fixture = subprocess.run([str(output / 'vulkan_shader_fixture'), str(output / 'outputs'),
                                  str(ROOT / 'tests/fixtures/hlsl/fullscreen.hlsl')], capture_output=True, text=True, timeout=60)
        (output / 'test.log').write_text(fixture.stdout + fixture.stderr)
        fixture.check_returncode()
        receipt = json.loads(fixture.stdout.strip().splitlines()[-1])
        # This executable is built by build_vulkan_reports.py in CI before this test.
        icd = next((pathlib.Path(p) for p in (
            '/usr/lib/chromium/vk_swiftshader_icd.json', '/opt/google/chrome/vk_swiftshader_icd.json',
            '/usr/share/vulkan/icd.d/lvp_icd.x86_64.json', '/usr/share/vulkan/icd.d/lvp_icd.json') if pathlib.Path(p).is_file()), None)
        if icd is None:
            raise RuntimeError('No software Vulkan ICD; HLSL execution not tested')
        env = dict(os.environ, VK_ICD_FILENAMES=str(icd), VK_DRIVER_FILES=str(icd))
        env.pop('VK_INSTANCE_LAYERS', None)
        replay = subprocess.run([str(ROOT / 'build/vulkan-reports/vulkan_reports_fixture'),
            str(output / 'outputs/authored-vertex.spv'), str(output / 'outputs/authored-fragment.spv')],
            env=env, capture_output=True, text=True, timeout=30)
        (output / 'vulkan-replay.log').write_text(replay.stdout + replay.stderr)
        replay.check_returncode()
        receipt['vulkan_execution'] = json.loads(replay.stdout.strip().splitlines()[-1])
        receipt['android_compiled'] = False
        if args.android:
            if 'Pkg.Revision = 28.2.13676358' not in (ndk / 'source.properties').read_text():
                raise RuntimeError('Pinned NDK required')
            arm = ROOT / 'build/vulkan-shaders-arm64'
            run([cmake, '-S', ROOT / 'host/vulkan-shaders', '-B', arm,
                 '-DCMAKE_TOOLCHAIN_FILE=' + str(ndk / 'build/cmake/android.toolchain.cmake'),
                 '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-29',
                 '-DCMAKE_BUILD_TYPE=Release', '-DURBAN_BLACK_SHADER_SOURCE=' + str(source)])
            run([cmake, '--build', arm, '--target', 'urban_black_shader', '--parallel', '2'])
            reader = ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf'
            archives = [arm / 'liburban_black_shader.a', arm / 'glslang/glslang/libglslang.a',
                        arm / 'glslang/glslang/libglslang-default-resource-limits.a']
            receipt['android_archives'] = {}
            for archive in archives:
                headers = subprocess.check_output([str(reader), '-h', str(archive)], text=True)
                machines = [line.split(':', 1)[1].strip() for line in headers.splitlines() if 'Machine:' in line]
                if not machines or any(machine != 'AArch64' for machine in machines):
                    raise RuntimeError('Non-AArch64 or empty archive: ' + str(archive))
                receipt['android_archives'][archive.name] = {'sha256': digest(archive), 'aarch64_objects': len(machines)}
            receipt['android_compiled'] = True
    receipt.update(cache_enabled=False, game_vulkan_backend_integrated=False, game_frames_tested=False,
                   android_device_tested=False, validation_layers_enabled=False, validation_errors=None,
                   ndk='28.2.13676358', cmake='3.31.6')
    receipt['spirv_sha256'] = {p.name: digest(p) for p in sorted((output / 'outputs').glob('*.spv'))}
    receipt['dependencies'] = json.loads((ROOT / 'dependencies.lock.json').read_text())['shader_compiler']
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
