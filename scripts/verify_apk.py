#!/usr/bin/env python3
import argparse,hashlib,json,pathlib,struct,subprocess,zipfile,sys
R=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('apk',nargs='?',type=pathlib.Path,default=R/'android/app/build/outputs/apk/debug/app-debug.apk');parser.add_argument('--output',type=pathlib.Path,default=R/'docs/evidence/apk-verification.json');args=parser.parse_args()
apk=args.apk;sdk=R/'.toolchain/android-sdk';bt=sdk/'build-tools/34.0.0';out={}
def run(name,args):
 p=subprocess.run(list(map(str,args)),capture_output=True,text=True);out[name]=dict(exit=p.returncode,stdout=p.stdout,stderr=p.stderr)
 if p.returncode:raise SystemExit(name+' failed: '+p.stderr)
with apk.open('rb') as f:out['sha256']=hashlib.file_digest(f,'sha256').hexdigest()
out['bytes']=apk.stat().st_size
with zipfile.ZipFile(apk) as z:
 libs=[n for n in z.namelist() if n.startswith('lib/') and n.endswith('.so')]
 assert {n.split('/')[1] for n in libs}=={'arm64-v8a'},libs
 assert {n.split('/')[-1] for n in libs}=={'libmain.so','libSDL3.so','libc++_shared.so'},libs
 out['libraries']={}
 for name in libs:
  b=z.read(name);assert b[:4]==b'\x7fELF' and b[4:6]==bytes([2,1]);machine=struct.unpack_from('<H',b,18)[0];assert machine==183
  out['libraries'][name]=dict(elf='ELF64 little-endian AArch64',machine=machine,sha256=hashlib.sha256(b).hexdigest(),size=len(b))
 assert not any(n.lower().endswith(('.xbe','.iso','.xiso','.keystore','.jks')) for n in z.namelist())
 out['license_assets']=[n for n in z.namelist() if n.startswith('assets/licenses/')]
 assert len(out['license_assets'])>=6
run('signature',[bt/'apksigner','verify','--verbose','--print-certs',apk])
run('zipalign',[bt/'zipalign','-c','-p','4',apk])
run('manifest',[bt/'aapt','dump','badging',apk])
run('permissions',[bt/'aapt','dump','permissions',apk])
assert 'MANAGE_EXTERNAL_STORAGE' not in out['permissions']['stdout']
assert "native-code: 'arm64-v8a'" in out['manifest']['stdout']
out['runtime_status']={'installed':False,'launched':False,'vulkan_presented_on_device':False,'game_executed':False}
args.output.write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
