#!/usr/bin/env python3
"""Create a reviewed renderer build overlay, preserving the pinned MIT Black source."""
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
    patch = ROOT / 'patches/black/vulkan-progress-metadata.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    return destination / 'vulkan.cpp'
