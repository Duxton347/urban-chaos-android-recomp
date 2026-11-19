#!/usr/bin/env python3
"""Link the actual private Android game library. Does not build, publish or qualify an APK."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys
from prepare_runtime_overlay import prepare, ROOT, PIN
from prepare_vulkan_renderer import prepare_renderer
from build_research_runtime import private_path
sys.path.insert(0,str(ROOT/'tools'))
from inspect_xbe import inspect


def sha(path):
    with path.open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('xbe','generated','archive','out'):
        parser.add_argument('--'+name,required=True)
    parser.add_argument('--profile',type=pathlib.Path,default=ROOT/'config/research/user-xbe-resumable.json')
    parser.add_argument('--resume',action='store_true',help='Preserve failed attempt logs and rebuild the same verified inputs')
    parser.add_argument('--rebuild',action='store_true',help='Replace a superseded linked library while preserving receipts and source overlays')
    args=parser.parse_args()
    xbe,generated,archive,output=map(private_path,(args.xbe,args.generated,args.archive,args.out))
    cache=(archive.parent/'CMakeCache.txt').read_text()
    optimized=re.search(r'^URBAN_GUEST_OPTIMIZATION:STRING=([12])$',cache,re.M)
    if not optimized:
        raise ValueError('Actual Android runtime requires a guest built explicitly with optimization 1 or 2; compile probes use 0')
    if args.resume and args.rebuild:raise ValueError('Choose resume or rebuild')
    if (output.exists() and not (args.resume or args.rebuild)) or any(output in p.parents or p in output.parents for p in (xbe,generated,archive)):
        raise ValueError('Output must be new, private and separate from all inputs')
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    for name in ('runtime','SDL'):
        dest=ROOT/'vendor'/name
        if subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()!=lock['repositories'][name]['commit']:
            raise ValueError('Wrong source pin: '+name)
        if subprocess.check_output(['git','-C',str(dest),'status','--porcelain'],text=True).strip():
            raise ValueError('Preserve modified source checkout: '+name)
    profile=json.loads(args.profile.read_text())
    if profile['sdk_commit']!=PIN or not profile.get('resumable_guest') or '#define IMAGE_RESUMABLE_GUEST 1' not in (generated/'image_symbols.h').read_text():
        raise ValueError('Matching continuation-aware runtime/profile/generated guest required')
    if any(location!='stack' for binding in profile['bindings'].values() for location in binding.get('abi',[])):
        raise ValueError('Only the reviewed stack ABI profile is supported')
    info=inspect(xbe,profile['xbe_sha256'])
    sdk=ROOT/'.toolchain/android-sdk';ndk=sdk/'ndk/28.2.13676358';cmake=sdk/'cmake/3.31.6/bin/cmake'
    if 'Pkg.Revision = 28.2.13676358' not in (ndk/'source.properties').read_text():raise ValueError('Wrong NDK')
    reader=ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf'
    headers=subprocess.check_output([str(reader),'-h',str(archive)],text=True)
    machines=[line.split(':',1)[1].strip() for line in headers.splitlines() if 'Machine:' in line]
    if not machines or any(machine!='AArch64' for machine in machines):raise ValueError('AArch64 guest archive required')
    if args.resume or args.rebuild:
        old=json.loads((output/'receipt.json').read_text())
        if old['status']!=('PRIVATE_ANDROID_LIBRARY_LINKED' if args.rebuild else 'BUILD_FAILED') or old['xbe_sha256']!=info['sha256'] or old['profile_sha256']!=sha(args.profile) or old['guest_archive_sha256']!=sha(archive):
            raise ValueError('Only the same verified private inputs can be resumed or rebuilt')
        if args.rebuild and sha(output/'cmake/libmain.so')!=old['library_sha256']:raise ValueError('Superseded library checksum mismatch')
        attempt=1
        while (output/f'receipt-attempt-{attempt}.json').exists():attempt+=1
        for name in ('receipt.json','build.log','sdk','vulkan'):
            previous=output/name
            if previous.exists():previous.rename(output/(name+f'-attempt-{attempt}' if name not in ('receipt.json','build.log') else name.replace('.',f'-attempt-{attempt}.',1)))
        if args.rebuild:(output/'cmake/libmain.so').unlink()
    output.mkdir(parents=True,exist_ok=True)
    receipt={'status':'BUILDING','sdk':PIN,'xbe_sha256':info['sha256'],'profile_sha256':sha(args.profile),
             'guest_optimization':int(optimized.group(1)),'guest_build_cache_sha256':sha(archive.parent/'CMakeCache.txt'),
             'guest_archive_sha256':sha(archive),'guest_objects':len(machines),'apk_created':False,
             'android_device_tested':False,'game_boot_tested':False,'g1_qualified':False}
    try:
        receipt['runtime_patches']=prepare(output/'sdk',context=True,android=True)
        renderer=prepare_renderer(output/'vulkan',android=True)
        receipt['renderer_source_sha256']=sha(renderer)
        with (output/'build.log').open('w') as log:
            subprocess.run([str(cmake),'-S',str(ROOT/'native/android'),'-B',str(output/'cmake'),'-G','Ninja',
                '-DCMAKE_MAKE_PROGRAM='+str(cmake.parent/'ninja'),'-DCMAKE_TOOLCHAIN_FILE='+str(ndk/'build/cmake/android.toolchain.cmake'),
                '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-29','-DANDROID_STL=c++_shared','-DCMAKE_BUILD_TYPE=Release',
                '-DURBAN_GENERATED='+str(generated),'-DURBAN_GUEST_ARCHIVE='+str(archive),'-DURBAN_OVERLAY='+str(output/'sdk'),
                '-DURBAN_VULKAN_SOURCE='+str(renderer),'-DURBAN_PROFILE='+str(args.profile.resolve()),'-DURBAN_TITLE_ID='+info['certificate']['title_id'],
                '-DURBAN_SHADER_BUILD='+str(ROOT/'build/vulkan-shaders-arm64')],check=True,stdout=log,stderr=subprocess.STDOUT)
            subprocess.run([str(cmake),'--build',str(output/'cmake'),'--target','main','--parallel','2'],check=True,stdout=log,stderr=subprocess.STDOUT)
        library=output/'cmake/libmain.so';elf=subprocess.check_output([str(reader),'-h','-l','--dyn-syms',str(library)],text=True)
        (output/'elf.txt').write_text(elf)
        if 'AArch64' not in elf or 'SDL_main' not in elf:raise ValueError('Android runtime ELF/entry missing')
        receipt.update(status='PRIVATE_ANDROID_LIBRARY_LINKED',library_sha256=sha(library),library_bytes=library.stat().st_size)
    except BaseException:
        receipt['status']='BUILD_FAILED'
        raise
    finally:
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({key:receipt[key] for key in ('status','library_bytes','apk_created','game_boot_tested','android_device_tested')}))

if __name__=='__main__':main()
