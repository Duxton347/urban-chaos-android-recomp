#!/usr/bin/env python3
"""Apply explicit MIT SDK patches to a build copy, preserving the pinned checkout."""
import hashlib
import pathlib
import shutil
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
PIN = 'c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7'
PATHS = ('runtime/xdk/dsound.cpp', 'runtime/xdk/d3d8.cpp', 'runtime/gpu/backend.h',
         'runtime/gpu/render_thread.cpp', 'runtime/gpu/state.h', 'runtime/gpu/vertex.cpp', 'runtime/kernel/xboxkrnl.cpp', 'runtime/gpu/d3d12.cpp',
         'runtime/kernel/disc.cpp', 'runtime/kernel/disc.h', 'harness/reference_renderer.cpp', 'harness/reference_renderer.h', 'harness/null_backend.cpp')
PATCHES = ('directsound-device-lifetime.patch', 'd3d-device-initialization.patch', 'io-apc-dispatch.patch', 'reference-visibility.patch', 'surface-ownership.patch', 'directional-audio.patch', 'bounded-disc-reader.patch')


def prepare(destination, context=False, tools_only=False):
    if tools_only and not context:
        raise ValueError("Tool-only overlays require continuation support")
    destination = pathlib.Path(destination).resolve()
    if not any((ROOT/folder) in destination.parents for folder in ('build','private')) or destination.exists():
        raise ValueError('Overlay must be new and inside ignored build/ or private/')
    sdk = ROOT/'vendor/runtime'
    if subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD'], text=True).strip() != PIN:
        raise ValueError('Runtime pin mismatch')
    if subprocess.check_output(['git', '-C', str(sdk), 'status', '--porcelain'], text=True).strip():
        raise ValueError('Pinned runtime checkout must remain clean')
    paths = [] if tools_only else list(PATHS)
    if context:
        paths += ['runtime/cpu/x86_context.h', 'runtime/cpu/x86_ops.h', 'runtime/cpu/dispatch.cpp', 'runtime/kernel/kernel.h']
        paths += [str(path.relative_to(sdk)) for path in (sdk/'tools/recomp').glob('*') if path.suffix in ('.cpp', '.h')]
    for relative in paths:
        path = destination/relative
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(sdk/relative, path)
    hashes = {}
    selected = (() if tools_only else PATCHES) + (("resumable-context.patch",) if context else ())
    for name in selected:
        patch = ROOT/'patches/runtime'/name
        hashes[name] = hashlib.sha256(patch.read_bytes()).hexdigest()
        subprocess.run(['git', 'apply', '--directory='+str(destination.relative_to(ROOT)), str(patch)], cwd=ROOT, check=True, capture_output=True)
    return hashes
