#!/usr/bin/env python3
"""Restore exact public sources and tool archives from dependencies.lock.json."""
import concurrent.futures,hashlib,json,pathlib,subprocess,urllib.request,zipfile,tarfile
R=pathlib.Path(__file__).resolve().parents[1];T=R/'.toolchain';D=T/'downloads';D.mkdir(parents=True,exist_ok=True)
lock=json.loads((R/'dependencies.lock.json').read_text())
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def repo(item):
 n,v=item;dest=R/'vendor'/n
 if not dest.exists():
  subprocess.run(['git','clone','--no-checkout',v['url'],str(dest)],check=True)
  subprocess.run(['git','-C',str(dest),'checkout','--detach',v['commit']],check=True)
 head=subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()
 if head!=v['commit']:raise RuntimeError(n+': wrong commit; preserve local changes and restore pin explicitly')
 if n=='zydis':subprocess.run(['git','-C',str(dest),'submodule','update','--init','--recursive'],check=True)
 if subprocess.check_output(['git','-C',str(dest),'status','--porcelain'],text=True).strip():raise RuntimeError(n+': modified checkout')
 return n,head

def download(item):
 n,v=item;p=D/n
 if not p.exists():
  with urllib.request.urlopen(v['url'],timeout=180) as src,(D/(n+'.part')).open('wb') as dst:
   while b:=src.read(1024*1024):dst.write(b)
  (D/(n+'.part')).rename(p)
 if digest(p)!=v['sha256']:raise RuntimeError(n+': checksum mismatch; archive not extracted')
 target=T/('android-sdk/cmdline-tools' if n=='cmdline.zip' else n.split('.')[0])
 if not target.exists():
  target.mkdir(parents=True)
  if n.endswith('.zip'):
   with zipfile.ZipFile(p) as z:z.extractall(target)
  else:
   with tarfile.open(p) as a:a.extractall(target,filter='data')
 if n=='cmdline.zip':
  q=target/'cmdline-tools'
  if q.exists():q.rename(target/'latest')
  for q in (target/'latest/bin').glob('*'):q.chmod(0o755)
 if n=='gradle.zip':(target/'gradle-8.7/bin/gradle').chmod(0o755)
 return n,v['sha256']
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 futures=[pool.submit(repo,x) for x in lock['repositories'].items() if x[0]!='zycore']+[pool.submit(download,x) for x in lock['downloads'].items() if x[0].endswith(('.zip','.tar.gz'))]
 for f in concurrent.futures.as_completed(futures):print(*f.result(),flush=True)
