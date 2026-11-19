#!/usr/bin/env python3
"""Fetch only the pinned public shader translator, never Black title inputs."""
import hashlib
import json
import pathlib
import subprocess
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]


def prepare(renderer=False):
    lock = json.loads((ROOT / 'dependencies.lock.json').read_text())
    source = ROOT / 'vendor/black-shader'
    meta = lock['shader_compiler']
    files = dict(meta['files'])
    if renderer:
        files.update(lock['vulkan_renderer']['files'])
    for name, expected in files.items():
        target = source / name
        if not target.exists():
            local = ROOT / 'vendor/black' / name
            if local.is_file():
                data = local.read_bytes()
            else:
                url = f"https://raw.githubusercontent.com/RanggaCasper/Black-Android/{meta['black_commit']}/{name}"
                with urllib.request.urlopen(url, timeout=60) as response:
                    data = response.read(1024 * 1024 + 1)
            if hashlib.sha256(data).hexdigest() != expected:
                raise RuntimeError('Shader source checksum mismatch: ' + name)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        if hashlib.sha256(target.read_bytes()).hexdigest() != expected:
            raise RuntimeError('Preserve modified shader source before restoring: ' + name)
    meta = lock['repositories']['glslang']
    dest = ROOT / 'vendor/glslang'
    if not dest.exists():
        subprocess.run(['git', 'init', '-q', str(dest)], check=True)
        subprocess.run(['git', '-C', str(dest), 'remote', 'add', 'origin', meta['url']], check=True)
        subprocess.run(['git', '-C', str(dest), 'fetch', '--depth', '1', 'origin', meta['commit']], check=True)
        subprocess.run(['git', '-C', str(dest), 'checkout', '--detach', 'FETCH_HEAD'], check=True)
    for revision, expected in [('HEAD', meta['commit']), ('HEAD^{tree}', meta['tree'])]:
        actual = subprocess.check_output(['git', '-C', str(dest), 'rev-parse', revision], text=True).strip()
        if actual != expected:
            raise RuntimeError('Wrong glslang revision: ' + revision)
    if subprocess.check_output(['git', '-C', str(dest), 'status', '--porcelain'], text=True).strip():
        raise RuntimeError('Preserve modified glslang checkout before restoring')
    return source / 'native'


if __name__ == '__main__':
    print(prepare())
