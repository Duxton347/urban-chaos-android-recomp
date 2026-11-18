"""Fail on source or SDK metadata drift; never silently refresh pins."""
import pathlib,subprocess,json,hashlib
R=pathlib.Path(__file__).resolve().parents[1];lock=json.loads((R/'dependencies.lock.json').read_text())
for name in ('runtime','SDL','zydis','tomlplusplus'):
 p=R/'vendor'/name;head=subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip()
 if head!=lock['repositories'][name]['commit']:raise SystemExit(name+': unexpected commit')
 if subprocess.check_output(['git','-C',str(p),'status','--porcelain'],text=True).strip():raise SystemExit(name+': modified checkout')
for name,meta in lock['toolchain']['packages'].items():
 p=R/'.toolchain/android-sdk'/name
 if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest()!=meta['sha256']:raise SystemExit('SDK metadata drift: '+name)
meta=R/lock['gradle_verification']['path']
if hashlib.sha256(meta.read_bytes()).hexdigest()!=lock['gradle_verification']['sha256']:raise SystemExit('Gradle verification metadata drift; review lock explicitly')
if 'launcher_gradle_verification' in lock:
 launcher=lock['launcher_gradle_verification'];metadata=R/launcher['path']
 if hashlib.sha256(metadata.read_bytes()).hexdigest()!=launcher['sha256']:raise SystemExit('Launcher Gradle verification metadata drift')
print('Source pins and SDK/Gradle metadata verified')
