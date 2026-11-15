#!/usr/bin/env python3
"""Minimal pinned public dependencies for CI. No game inputs or credentials."""
import argparse,hashlib,json,pathlib,subprocess,tarfile,urllib.request,zipfile
R=pathlib.Path(__file__).resolve().parents[1];lock=json.loads((R/'dependencies.lock.json').read_text())
p=argparse.ArgumentParser();p.add_argument('--component',choices=['host','audit','android'],required=True);a=p.parse_args()
names=['runtime','zydis','tomlplusplus'] if a.component=='host' else (['runtime'] if a.component=='android' else ['burner','black','runtime','xboxkit'])
for name in names:
 meta=lock['repositories'][name];dest=R/'vendor'/name
 if not dest.exists():
  dest.mkdir(parents=True);subprocess.run(['git','init','-q',str(dest)],check=True)
  subprocess.run(['git','-C',str(dest),'remote','add','origin',meta['url']],check=True)
  subprocess.run(['git','-C',str(dest),'fetch','--depth','1','--filter=blob:none','origin',meta['commit']],check=True)
  if name=='black':
   subprocess.run(['git','-C',str(dest),'sparse-checkout','init','--no-cone'],check=True)
   subprocess.run(['git','-C',str(dest),'sparse-checkout','set','--no-cone','/*','!/ci/','!*.enc'],check=True)
  subprocess.run(['git','-C',str(dest),'checkout','--detach','FETCH_HEAD'],check=True)
 if subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()!=meta['commit']:raise SystemExit(name+': wrong pin')
 if name=='black':
  if subprocess.check_output(['git','-C',str(dest),'status','--porcelain'],text=True).strip():raise SystemExit('Preserve modified Black checkout before sparse audit')
  subprocess.run(['git','-C',str(dest),'sparse-checkout','init','--no-cone'],check=True)
  subprocess.run(['git','-C',str(dest),'sparse-checkout','set','--no-cone','/*','!/ci/','!*.enc'],check=True)
  if any(dest.rglob('*.enc')):raise SystemExit('Black encrypted inputs must not be checked out')
 if name=='zydis':subprocess.run(['git','-C',str(dest),'submodule','update','--init','--recursive','--depth','1'],check=True)
 if subprocess.check_output(['git','-C',str(dest),'status','--porcelain'],text=True).strip():raise SystemExit(name+': modified source')
if a.component=='host':
 meta=lock['downloads']['cmake.tar.gz'];archive=R/'.toolchain/downloads/cmake.tar.gz';archive.parent.mkdir(parents=True,exist_ok=True)
 if not archive.exists():
  with urllib.request.urlopen(meta['url'],timeout=120) as src,archive.open('wb') as dst:
   while b:=src.read(1048576):dst.write(b)
 with archive.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
 if actual!=meta['sha256']:raise SystemExit('CMake checksum mismatch')
 dest=R/'.toolchain/cmake';dest.mkdir(parents=True,exist_ok=True)
 if not (dest/'cmake-3.31.6-linux-x86_64/bin/cmake').exists():
  with tarfile.open(archive) as t:t.extractall(dest,filter='data')
print('Pinned public dependencies ready:',a.component)

if a.component=='android':
 meta=lock['downloads']['cmdline.zip'];archive=R/'.toolchain/downloads/cmdline.zip';archive.parent.mkdir(parents=True,exist_ok=True)
 if not archive.exists():
  with urllib.request.urlopen(meta['url'],timeout=120) as src,archive.open('wb') as dst:
   while b:=src.read(1048576):dst.write(b)
 with archive.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
 if actual!=meta['sha256']:raise SystemExit('Android command-line tools checksum mismatch')
 target=R/'.toolchain/android-sdk/cmdline-tools';target.mkdir(parents=True,exist_ok=True)
 if not (target/'latest').exists():
  with zipfile.ZipFile(archive) as z:z.extractall(target)
  (target/'cmdline-tools').rename(target/'latest')
 for q in (target/'latest/bin').glob('*'):q.chmod(0o755)
 print('Pinned Android command-line tools ready')
