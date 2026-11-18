#!/usr/bin/env python3
"""Create a reviewed renderer build overlay, preserving the pinned MIT Black source."""
import pathlib
import shutil
import subprocess
from bootstrap_vulkan_shaders import prepare, ROOT


def prepare_renderer(destination):
    source = prepare(renderer=True)
    destination = pathlib.Path(destination).resolve()
    if destination.exists() or not any((ROOT / part) in destination.parents for part in ('build', 'private')):
        raise ValueError('Renderer overlay must be new and in ignored build/ or private/')
    destination.mkdir(parents=True)
    shutil.copyfile(source / 'vulkan.cpp', destination / 'vulkan.cpp')
    patch = ROOT / 'patches/black/vulkan-headless-research.patch'
    subprocess.run(['git', 'apply', '--directory=' + str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True)
    return destination / 'vulkan.cpp'
