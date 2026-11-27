#!/usr/bin/env python3
"""Create a reviewed renderer build overlay, preserving the pinned MIT Black source."""
import hashlib
import json
import pathlib
import shutil
import subprocess
from bootstrap_vulkan_shaders import prepare, ROOT


def software_headless_icd():
    for candidate in ('/usr/lib/chromium/vk_swiftshader_icd.json',
                      '/opt/google/chrome/vk_swiftshader_icd.json'):
        path = pathlib.Path(candidate)
        if path.is_file():
            return path
    raise RuntimeError('No explicitly supported software headless Vulkan ICD found')


def prepare_renderer(destination, android=False):
    source = prepare(renderer=True)
    destination = pathlib.Path(destination).resolve()
    if destination.exists() or not any((ROOT / part) in destination.parents for part in ('build', 'private')):
        raise ValueError('Renderer overlay must be new and in ignored build/ or private/')
    destination.mkdir(parents=True)
    shutil.copyfile(source / 'vulkan.cpp', destination / 'vulkan.cpp')
    patch = ROOT / 'patches/black/vulkan-headless-research.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    if android:
        patch = ROOT / 'patches/black/vulkan-android-host.patch'
        subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
        patch = ROOT / 'patches/black/vulkan-android-display.patch'
        subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-progress-metadata.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    if android:
        patch = ROOT / 'patches/black/vulkan-android-assembly-metadata.patch'
        subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-async-visibility.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-completion-fences.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-texture-write-epoch.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-program-fog.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-independent-facing.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-facing-usage.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-native-vertices.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-depth-colour-alias.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-signed-textures.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-render-target-texel-centres.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    patch = ROOT / 'patches/black/vulkan-shader-cache.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    identity=hashlib.sha256((lock['repositories']['glslang']['commit']+''.join(lock['shader_compiler']['files'][name] for name in ('native/vk_shader.cpp','native/vk_shader.h'))+'urban-spv-cache-v1').encode()).hexdigest()
    result=destination/'vulkan.cpp'
    text=result.read_text()
    if text.count('@URBAN_SHADER_CACHE_COMPILER_ID@')!=1:raise ValueError('Shader cache namespace marker mismatch')
    result.write_text(text.replace('@URBAN_SHADER_CACHE_COMPILER_ID@',identity))
    return result
