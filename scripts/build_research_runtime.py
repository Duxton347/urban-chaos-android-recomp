#!/usr/bin/env python3
"""Build the private Linux research host; never qualifies a production game gate."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys

from prepare_runtime_overlay import prepare, ROOT, PIN
sys.path.insert(0, str(ROOT/'tools'))
from inspect_xbe import inspect, Invalid


def private_path(value):
    path = pathlib.Path(value).resolve()
    if not any((ROOT/folder) in path.parents for folder in ('build', 'private')):
        raise Invalid('Title inputs and research outputs must stay in ignored build/ or private/')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', required=True)
    parser.add_argument('--generated', required=True)
    parser.add_argument('--archive', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--profile', type=pathlib.Path, default=ROOT/'config/research/user-xbe.json')
    args = parser.parse_args()
    try:
        xbe, generated, archive, out = map(private_path, (args.xbe, args.generated, args.archive, args.out))
        if out.exists() or out in generated.parents or generated in out.parents or out in archive.parents or archive in out.parents:
            raise Invalid('Research output must be new and separate from title inputs')
        sdk = ROOT/'vendor/runtime'
        if subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD'], text=True).strip() != PIN:
            raise Invalid('Runtime pin mismatch')
        if subprocess.check_output(['git', '-C', str(sdk), 'status', '--porcelain'], text=True).strip():
            raise Invalid('Pinned SDK must remain clean')
        profile = json.loads(args.profile.read_text())
        if profile['sdk_commit'] != PIN:
            raise Invalid('Profile runtime mismatch')
        report = inspect(xbe, profile['xbe_sha256'])
        if profile.get('resumable_guest') and '#define IMAGE_RESUMABLE_GUEST 1' not in (generated/'image_symbols.h').read_text():
            raise Invalid('Resumable profile requires freshly regenerated continuation-aware guest code')
        if not (generated/'image_symbols.h').is_file() or not archive.is_file():
            raise Invalid('Actual generated headers and compiled guest archive required')
        cmake = ROOT/'.toolchain/cmake/cmake-3.31.6-linux-x86_64/bin/cmake'
        out.mkdir(parents=True)
        overlay = out/'source-overlay'
        sources = list((ROOT/'host/research').glob('*')) + [ROOT/'host/runtime-probes/gpu_probes.cpp', ROOT/'host/audio-device/test_device_lifetime.cpp']
        host_sources = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sources if path.is_file()}
        receipt = {'host_sources': host_sources, 'sdk': PIN, 'xbe_sha256': report['sha256'], 'g1_qualified': False, 'patches': {}, 'status': 'BUILDING'}
        try:
            receipt['patches'] = prepare(overlay, context=profile.get('resumable_guest', False))
            with (out/'build.log').open('w') as log:
                subprocess.run([str(cmake), '-S', str(ROOT/'host/research'), '-B', str(out/'cmake'),
                                '-DURBAN_GENERATED='+str(generated), '-DURBAN_GUEST_ARCHIVE='+str(archive),
                                '-DURBAN_OVERLAY='+str(overlay), '-DURBAN_PROFILE='+str(args.profile.resolve())],
                               stdout=log, stderr=subprocess.STDOUT, check=True)
                subprocess.run([str(cmake), '--build', str(out/'cmake'), '--parallel', '2'], stdout=log, stderr=subprocess.STDOUT, check=True)
            receipt['status'] = 'RESEARCH_HOST_COMPILED'
        except BaseException:
            receipt['status'] = 'BUILD_FAILED'
            raise
        finally:
            receipt['profile_sha256'] = hashlib.sha256(args.profile.read_bytes()).hexdigest()
            receipt['image_symbols_sha256'] = hashlib.sha256((generated/'image_symbols.h').read_bytes()).hexdigest()
            with archive.open('rb') as source:
                receipt['guest_archive_sha256'] = hashlib.file_digest(source, 'sha256').hexdigest()
            (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
        print(json.dumps({'status': receipt['status'], 'g1_qualified': False, 'log': str(out/'build.log')}))
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(f'Research build failed: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
