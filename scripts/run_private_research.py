#!/usr/bin/env python3
"""Bounded private title run with metadata receipts; never qualifies gameplay."""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import signal
import subprocess
import sys
import time
import resource
from prepare_vulkan_renderer import software_headless_icd

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from inspect_xbe import inspect, Invalid


def private_path(value):
    path = pathlib.Path(value).resolve()
    if not any((ROOT/folder) in path.parents for folder in ('private', 'build')):
        raise Invalid('Title inputs and run outputs must stay in ignored private/ or build/')
    return path


def digest(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def limits():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--game', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--profile', required=True, type=pathlib.Path)
    parser.add_argument('--pad-script')
    parser.add_argument('--seconds', type=int, default=120)
    parser.add_argument('--reference-from', type=int, default=0)
    parser.add_argument('--capture-every', type=int, default=0)
    parser.add_argument('--audio-driver', choices=('dummy',), help='Explicit headless SDL transport test; never audible evidence')
    args = parser.parse_args()
    try:
        host, game, out = map(private_path, (args.host, args.game, args.out))
        script = private_path(args.pad_script) if args.pad_script else None
        if not 1 <= args.seconds <= 1800 or min(args.reference_from, args.capture_every) < 0:
            raise Invalid('Invalid bounded run duration or frame parameters')
        if out.exists() or any(out in p.parents or p in out.parents for p in (host, game)):
            raise Invalid('Run output must be new and separate from title inputs')
        if not host.is_file() or not os.access(host, os.X_OK) or not game.is_dir():
            raise Invalid('Compiled executable and game directory required')
        if script and not script.is_file():
            raise Invalid('Input script missing')
        profile = json.loads(args.profile.read_text())
        image = game/'Default.xbe'
        info = inspect(image, profile['xbe_sha256'])
        build_receipt = host.parent.parent/'receipt.json'
        build = json.loads(build_receipt.read_text())
        if build['status'] != 'RESEARCH_HOST_COMPILED' or build['profile_sha256'] != digest(args.profile):
            raise Invalid('Host build receipt/profile mismatch')
        cache = (host.parent/'CMakeCache.txt').read_text()
        if re.search(r'^URBAN_OBSERVER:FILEPATH=.+$', cache, re.M):
            raise Invalid('Use a clean host without private instrumentation for recorded acceptance runs')
        out.mkdir(parents=True)
        env = {'PATH': '/usr/bin:/bin', 'LANG': 'C.UTF-8', 'REF_ALL': '1',
               'REF_FROM_FRAME': str(args.reference_from), 'REF': str(args.capture_every)}
        if build.get('render_backend') == 'vulkan-headless':
            icd = software_headless_icd()
            env.update(VK_ICD_FILENAMES=str(icd), VK_DRIVER_FILES=str(icd),
                       URBAN_VK_CAPTURE_FROM=str(args.reference_from), URBAN_VK_CAPTURE_EVERY=str(args.capture_every))
        if script:
            env['SCRIPT'] = str(script)
        if args.audio_driver:
            if build.get('audio_output') != 'sdl':
                raise Invalid('An SDL driver requires an actual SDL-output host')
            env['SDL_AUDIODRIVER'] = args.audio_driver
        report = {'schema': 1, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  'host_sha256': digest(host), 'build_receipt_sha256': digest(build_receipt),
                  'xbe_sha256': info['sha256'], 'profile_sha256': digest(args.profile),
                  'pad_script_sha256': digest(script) if script else None,
                  'arguments': [str(host), str(game)], 'environment': env,
                  'timeout_seconds': args.seconds, 'status': 'STARTING', 'gameplay_validated': False,
                  'android_validated': False}
        report['audio_output'] = build.get('audio_output','measure')
        report['render_backend'] = build.get('render_backend','reference')
        report['sdl_driver'] = args.audio_driver
        report['audible_output_verified'] = False
        (out/'run.json').write_text(json.dumps(report, indent=2)+'\n')
        started = time.monotonic()
        with (out/'runtime.log').open('w') as log:
            process = subprocess.Popen([str(host), str(game)], cwd=out, env=env,
                                       stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True, preexec_fn=limits)
            timed_out = interrupted = False
            try:
                process.wait(timeout=args.seconds)
            except subprocess.TimeoutExpired:
                timed_out = True
            except KeyboardInterrupt:
                interrupted = True
            finally:
                if process.poll() is None:
                    try:
                        os.killpg(process.pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
        text = (out/'runtime.log').read_text(errors='replace')
        frames = re.findall(r'renderer: frame (\d+)', text)
        report.update(status='TIMED_OUT' if timed_out else 'INTERRUPTED' if interrupted else 'EXITED', exit_code=process.returncode, timed_out=timed_out, interrupted=interrupted,
                      elapsed_seconds=round(time.monotonic()-started, 3),
                      last_present_counter=int(frames[-1]) if frames else None,
                      missing_disc_requests=len(re.findall(r'missing D:\\', text, re.I)),
                      completed_visibility_reports=len(re.findall(r'visibility report completed', text)),
                      exception_recorded=bool(re.search(r'RESEARCH_SIGNAL|\[fatal\]|unimplemented kernel export|indirect call to unknown', text)) or (process.returncode<0 and not timed_out and not interrupted))
        (out/'run.json').write_text(json.dumps(report, indent=2)+'\n')
        print(json.dumps({key: report[key] for key in ('exit_code', 'timed_out', 'interrupted', 'elapsed_seconds',
              'last_present_counter', 'missing_disc_requests', 'completed_visibility_reports', 'exception_recorded', 'gameplay_validated')}))
        return 130 if interrupted else 124 if timed_out else 0 if process.returncode == 0 else 1
    except (OSError, ValueError, KeyError) as error:
        if 'report' in locals() and out.exists():
            report.update(status='FAILED_TO_RUN', error_type=type(error).__name__)
            (out/'run.json').write_text(json.dumps(report, indent=2)+'\n')
        print(f'Private research run refused: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
