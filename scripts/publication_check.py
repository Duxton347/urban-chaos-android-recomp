#!/usr/bin/env python3
"""Reject prohibited tracked files; check artifacts against an exact filename allowlist."""
import argparse,pathlib,subprocess,tarfile
R=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--artifacts',type=pathlib.Path);a=p.parse_args()
files=subprocess.check_output(['git','-C',str(R),'ls-files','-z']).decode().split('\0');bad=[]
def prohibited(name):
 parts=pathlib.PurePosixPath(name).parts
 return any(x in parts for x in ('private','generated','iso','saves','.toolchain','vendor')) or pathlib.PurePosixPath(name).suffix.lower() in ('.xbe','.iso','.xiso','.xex','.enc','.keystore','.jks','.pem','.apk')
for name in files:
 if not name:continue
 path=R/name
 if prohibited(name):bad.append(name);continue
 if path.is_file():
  with path.open('rb') as f:magic=f.read(4)
  if magic in (b'XBEH',b'XEX2',b'\x7fELF'):bad.append(name)
if a.artifacts:
 allowed={'xbe_recomp','xbe_symbols','host-tools.receipt.json','UrbanChaos-tooling-source.tar.gz','SHA256SUMS'}
 actual={f.name for f in a.artifacts.iterdir()}
 if actual-allowed:bad.extend(sorted(actual-allowed))
 for item in a.artifacts.iterdir():
  if not item.is_file() or item.is_symlink():bad.append(str(item))
  if item.suffixes[-2:]==['.tar','.gz']:
   with tarfile.open(item) as t:
    bad.extend(m.name for m in t if prohibited(m.name) or m.issym() or m.islnk())
if bad:raise SystemExit('Publication blocked: '+', '.join(bad))
print('Publication allowlist passed; no proprietary inputs or APKs staged')
