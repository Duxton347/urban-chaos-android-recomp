#!/usr/bin/env python3
"""Prepare checksum-verified public source and attempt its UNMODIFIED Windows build.

No title inputs, qualification reports or replacement functions are fabricated.
The original nonzero build result is propagated to CI.
"""
import argparse,hashlib,json,os,pathlib,stat,subprocess,sys,urllib.request,zipfile
R=pathlib.Path(__file__).resolve().parents[1]
FORBIDDEN={'.xbe','.iso','.xiso','.xex','.enc','.jks','.keystore'}
def sha(path):
 with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def download(meta,path):
 path.parent.mkdir(parents=True,exist_ok=True)
 if not path.exists():
  part=path.with_suffix(path.suffix+'.part')
  with urllib.request.urlopen(meta['url'],timeout=180) as src,part.open('wb') as out:
   while b:=src.read(1048576):out.write(b)
  part.replace(path)
 if sha(path)!=meta['sha256']:raise ValueError('Archive checksum mismatch: '+path.name)
 return path

def extract_public_source(archive,dest,expected):
 if sha(archive)!=expected:raise ValueError('Source checksum mismatch')
 receipt=dest/'.public-source-receipt.json'
 reuse=dest.exists()
 if reuse and not (receipt.exists() and json.loads(receipt.read_text())['sha256']==expected):
  raise ValueError('Refusing to overwrite existing source directory')
 with zipfile.ZipFile(archive) as z:
  entries=[];total=0;seen=set()
  for i in z.infolist():
   p=pathlib.PurePosixPath(i.filename)
   if p.is_absolute() or '..' in p.parts or '\\' in i.filename or ':' in i.filename:raise ValueError('Unsafe archive path')
   if stat.S_ISLNK(i.external_attr>>16):raise ValueError('Symlink in public source archive')
   if p.suffix.lower() in FORBIDDEN:raise ValueError('Title input or secret in public source archive')
   key=str(p).casefold()
   if key in seen:raise ValueError('Case-colliding archive paths')
   seen.add(key);total+=i.file_size;entries.append(i)
  if total>512*1024*1024:raise ValueError('Public source archive exceeds size bound')
  if 'cmakelists.txt' not in seen:raise ValueError('Source root CMakeLists.txt missing')
  if reuse:
   for i in entries:
    if i.is_dir():continue
    target=dest/pathlib.PurePosixPath(i.filename)
    if not target.resolve().is_relative_to(dest.resolve()) or target.is_symlink() or not target.is_file() or target.read_bytes()!=z.read(i):raise ValueError('Prepared source modified: '+i.filename)
   return dest
  dest.mkdir(parents=True)
  for i in entries:
   target=dest/pathlib.PurePosixPath(i.filename)
   if i.is_dir():target.mkdir(parents=True,exist_ok=True);continue
   target.parent.mkdir(parents=True,exist_ok=True)
   with z.open(i) as src,target.open('xb') as out:
    while b:=src.read(1048576):out.write(b)
 receipt.write_text(json.dumps({'sha256':expected,'unmodified_source':True},indent=2)+'\n')
 return dest

def classify(text):
 if 'FileNotFoundError' in text and 'integration-status.json' in text:return 'MISSING_QUALIFIED_TITLE_INPUTS'
 if 'UCRR_SERVICES_HEAVY_MUTEX' in text:return 'MUTEX_WRAPPER_NOT_SATISFIED'
 return 'UPSTREAM_BUILD_FAILED'

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--prepare-only',action='store_true');p.add_argument('--work',type=pathlib.Path,default=R/'build/pc-recovery/original-build');a=p.parse_args()
 work=a.work.resolve();allowed=(R/'build').resolve()
 if allowed not in work.parents:raise ValueError('Work directory must stay in ignored build/')
 lock=json.loads((R/'config/pc-recovery.lock.json').read_text());meta=lock['archives']['source-v0.18.zip'];downloads=R/'private/pc-recovery/downloads'
 archive=download(meta,downloads/'source-v0.18.zip');source=extract_public_source(archive,work/'source',meta['sha256'])
 state={'source_sha256':meta['sha256'],'g1_qualified':False,'game_compiled':False,'game_executed':False,'status':'PUBLIC_SOURCE_PREPARED_NOT_BUILD'}
 report=work/'attempt-status.json'
 if a.prepare_only:report.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state,indent=2));return 0
 if os.name!='nt':state['status']='WINDOWS_RUNNER_REQUIRED';report.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state,indent=2));return 78
 cmake_meta=lock['archives']['cmake-windows.zip'];tools=R/'.toolchain/pc-recovery';tools.mkdir(parents=True,exist_ok=True)
 cmake_archive=download(cmake_meta,tools/'cmake-windows.zip');cmake=tools/'cmake-3.31.6-windows-x86_64/bin/cmake.exe'
 if not cmake.exists():
  with zipfile.ZipFile(cmake_archive) as z:z.extractall(tools)
 # Install an actual interpreter at the path expected by the original wrapper.
 # A junction preserves the full Python distribution, not a stub python.exe.
 python_dir=source/'.tools/python';python_dir.parent.mkdir(parents=True,exist_ok=True)
 if not python_dir.exists():subprocess.run(['cmd','/c','mklink','/J',str(python_dir),sys.prefix],check=True)
 commands=[['configure',str(cmake),'-S',str(source),'-B',str(work/'out'),'-G','Visual Studio 17 2022'],['build',str(cmake),'--build',str(work/'out'),'--config','RelWithDebInfo','--target','UrbanChaosXbeTrial']]
 text='';exit_code=0
 for label,*cmd in commands:
  print('Executing original source:',label,flush=True)
  result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace')
  print(result.stdout,flush=True);text+=result.stdout;state[label+'_exit']=result.returncode
  if result.returncode:exit_code=result.returncode;break
 state['status']=classify(text) if exit_code else 'BUILD_EXIT_ZERO_REQUIRES_VERIFICATION'
 (work/'original-build.log').write_text(text,encoding='utf-8')
 report.write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(state,indent=2))
 return exit_code
if __name__=='__main__':sys.exit(main())
