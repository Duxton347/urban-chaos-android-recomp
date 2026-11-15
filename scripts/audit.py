#!/usr/bin/env python3
"""Regenerate public source provenance and path consumers without commercial inputs."""
import ast,hashlib,json,pathlib,re,subprocess,xml.etree.ElementTree as ET
R=pathlib.Path(__file__).resolve().parents[1];E=R/'docs/evidence'
def git(repo,*args):return subprocess.check_output(['git','-C',str(R/'vendor'/repo),*args],text=True).strip()
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
lock={'schema':1,'runtime_base':'c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7','repositories':{},'toolchain':{'java':'17.0.17+10','gradle':'8.7','agp':'8.5.2','sdk':34,'ndk':'28.2.13676358','cmake':'3.31.6','abi':['arm64-v8a']},'downloads':json.loads((E/'bootstrap.json').read_text()),'candidate':{'filename':'Default.xbe','size':5283840,'sha256':'b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac','android_compatible':False}}
for n,license,role in [('burner','UNRESOLVED for project adaptations','read-only audit'),('black','MIT; bundled assets require separate review','read-only audit'),('runtime','MIT','host tools; Android game integration gated'),('SDL','zlib + preserved bundled notices','diagnostic'),('zydis','MIT','host tools'),('tomlplusplus','MIT','host tools')]:
 expected={'burner':'bdaef722ef1d589af30e5a58dd8543f64564c4fc','black':'8e9c6fc7d570503ea68d039cff16c09339ee6d30','runtime':'c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7','SDL':'fa2c02bb6e21974a89ea9824bc53c9932abe5f9c','zydis':'a2278f1d254e492f6a6b39f6cb5d1f5d515659dc','tomlplusplus':'30172438cee64926dc41fdd9c11fb3ba5b2ba9de'}
 if git(n,'rev-parse','HEAD')!=expected[n]:raise SystemExit(n+': revision drift')
 lock['repositories'][n]=dict(url=git(n,'remote','get-url','origin'),commit=git(n,'rev-parse','HEAD'),tree=git(n,'rev-parse','HEAD^{tree}'),license=license,role=role)
lock['repositories']['zycore']={'url':'https://github.com/zyantific/zycore-c','commit':git('zydis/dependencies/zycore','rev-parse','HEAD'),'license':'MIT'}
lock['runtime_black_pin']='99078b119b5d5e0d1d6afb1390e29437c0c984c0'
lock['licenses']={str(p.relative_to(R)):sha(p) for p in sorted((R/'licenses').glob('*'))}
lock['toolchain']['packages']={}
for p in (R/'.toolchain/android-sdk').rglob('source.properties'):
 lock['toolchain']['packages'][str(p.relative_to(R/'.toolchain/android-sdk'))]=dict(sha256=sha(p),properties=p.read_text())
jdk=R/'.toolchain/downloads/jdk.tar.gz'
if jdk.exists():lock['downloads']['jdk.tar.gz']=dict(url='https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.17%2B10/OpenJDK17U-jdk_x64_linux_hotspot_17.0.17_10.tar.gz',sha256=sha(jdk))
lock['toolchain']['min_sdk']=29
if (E/'sdk-archives.json').exists():lock['toolchain']['sdk_archives']=json.loads((E/'sdk-archives.json').read_text())
if (R/'android/gradle/verification-metadata.xml').exists():lock['gradle_verification']={'path':'android/gradle/verification-metadata.xml','sha256':sha(R/'android/gradle/verification-metadata.xml')}
(R/'dependencies.lock.json').write_text(json.dumps(lock,indent=2)+'\n')
(E/'runtime-diff.txt').write_text(git('runtime','diff','--stat',lock['runtime_base'],lock['runtime_black_pin'])+'\n'+git('runtime','diff','--name-only',lock['runtime_base'],lock['runtime_black_pin'])+'\n')
refs=[]
for p in sorted((R/'vendor/burner/scripts').rglob('*.py')):
 try:t=ast.parse(p.read_text())
 except (SyntaxError,UnicodeError):continue
 for n in ast.walk(t):
  if isinstance(n,ast.Constant) and isinstance(n.value,str) and len(n.value)<240 and '\n' not in n.value and re.search(r'generated/|generated-.*original|integration-status|scanned|fixture.*\.bin|config/|assets/Default.xbe',n.value):refs.append(dict(consumer=str(p.relative_to(R)),line=n.lineno,value=n.value))
(E/'input-consumers.json').write_text(json.dumps(refs,indent=2)+'\n')
# Per-file provenance: project-level license plus local exceptions/notices must be retained.
files=[]
for repo,folders,license in [('runtime',['tools/recomp','tools/symbols','runtime/xbe'],'MIT'),('zydis',['src','include','dependencies/zycore'],'MIT'),('tomlplusplus',['include'],'MIT'),('SDL',['src','include','android-project/app/src/main/java'],'zlib; local notices preserved')]:
 for folder in folders:
  for p in sorted((R/'vendor'/repo/folder).rglob('*')):
   if p.is_file() and '.git' not in p.parts:
    text=p.read_text(errors='replace') if p.suffix in ('.h','.c','.cpp','.java') else ''
    lines=[l.strip() for l in text.splitlines() if re.search(r'copyright|SPDX|license|permission',l,re.I)]
    files.append(dict(path=str(p.relative_to(R)),sha256=sha(p),license_scope=license,notice_lines=lines[:15]))
(E/'source-license-inventory.json').write_text(json.dumps(files,indent=2)+'\n')
print('Lockfile, runtime diff,',len(refs),'consumer references,',len(files),'source license records')
