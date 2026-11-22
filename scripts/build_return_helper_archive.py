#!/usr/bin/env python3
"""Incrementally rebuild reviewed return helpers, preserving the baseline archive.
Title inputs, object, source and output remain ignored; no game assets are published.
"""
import argparse,pathlib,json,hashlib,subprocess,shutil,sys,re
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from relocate_return_helpers import transform

def sha(p):return hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--baseline',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True)
 p.add_argument('--expected-base',required=True);p.add_argument('--target',choices=['linux','arm64'],required=True)
 p.add_argument('--review',type=pathlib.Path,default=ROOT/'config/research/relocated-return-helpers.json');a=p.parse_args()
 baseline,out=a.baseline.resolve(),a.out.resolve()
 for path in (baseline,out):
  if not any((ROOT/x) in path.parents for x in ('build','private')):raise ValueError('Private paths required')
 if out.exists():raise ValueError('New output required')
 cache=(baseline/'CMakeCache.txt').read_text()
 def entry(name):
  m=re.search(r'^'+name+r':[^=]*=(.+)$',cache,re.M)
  if not m:raise ValueError('Missing build field: '+name)
  return m[1]
 if entry('URBAN_GUEST_OPTIMIZATION')!='2':raise ValueError('Reviewed optimization 2 required')
 generated=pathlib.Path(entry('URBAN_GENERATED'));headers=pathlib.Path(entry('URBAN_RUNTIME_HEADERS'))
 base=baseline/'liburban_generated_compile_probe.a'
 if sha(base)!=a.expected_base:raise ValueError('Baseline digest mismatch')
 review=json.loads(a.review.read_text());source=generated/review['source_file'];text=source.read_text()
 if sha(source)!=review['source_sha256']:raise ValueError('Generated source changed')
 out.mkdir(parents=True);changed=out/source.name;changed.write_text(transform(text,review['helpers']))
 tool=ROOT/'.toolchain/android-sdk/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64'
 flags=[]
 if a.target=='arm64':
  if entry('ANDROID_ABI')!='arm64-v8a':raise ValueError('Wrong ABI')
  flags=['--target=aarch64-none-linux-android29','--sysroot='+str(tool/'sysroot'),'-fPIC']
 obj=out/(source.name+'.o');compiler=tool/'bin/clang++';ar=tool/'bin/llvm-ar'
 command=[str(compiler),*flags,'-std=gnu++20','-O2','-g0','-DNDEBUG','-fsigned-char','-fno-strict-aliasing','-I'+str(generated),'-I'+str(headers),'-I'+str(ROOT/'vendor/runtime/runtime'),'-c',str(changed),'-o',str(obj)]
 with (out/'compile.log').open('w') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT)
 members=subprocess.check_output([str(ar),'t',str(base)],text=True).splitlines()
 if members.count(obj.name)!=1:raise ValueError('Archive member mismatch')
 archive=out/base.name;shutil.copyfile(base,archive);subprocess.run([str(ar),'r',str(archive),str(obj)],check=True)
 if subprocess.check_output([str(ar),'t',str(archive)],text=True).splitlines()!=members:raise ValueError('Unexpected archive members')
 shutil.copyfile(baseline/'CMakeCache.txt',out/'CMakeCache.txt')
 (out/'helper-receipt.json').write_text(json.dumps({'target':a.target,'base_sha256':sha(base),'archive_sha256':sha(archive),'review_sha256':sha(a.review),'modified_source_sha256':sha(changed),'command':command,'modified_functions':len(review['helpers']),'gameplay_validated':False},indent=2)+'\n')
if __name__=='__main__':main()
