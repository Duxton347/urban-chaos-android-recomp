#!/usr/bin/env python3
"""Assemble a private, actual-game APK. Never upload it or include title assets; no device success inferred."""
import argparse
import hashlib
import json
import pathlib
import os
import re
import subprocess
import zipfile
from build_research_runtime import ROOT, private_path


def sha(path):
    with path.open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build',required=True)
    parser.add_argument('--out',required=True)
    parser.add_argument('--locale-pack',help='Private translated dictionary, no source archive payloads')
    args=parser.parse_args()
    locale=private_path(args.locale_pack) if args.locale_pack else None
    if locale and (locale.stat().st_size>2*1024*1024 or not locale.read_bytes().startswith(b'UCPTBR01')):raise ValueError('Invalid private locale dictionary')
    native,output=map(private_path,(args.native_build,args.out))
    if output.exists() or output in native.parents or native in output.parents:raise ValueError('New private output required')
    receipt=json.loads((native/'receipt.json').read_text())
    if receipt['status']!='PRIVATE_ANDROID_LIBRARY_LINKED':raise ValueError('Actual linked game runtime required')
    for name in ('native/gpu/signed_texels.h',):
        if receipt.get('renderer_headers',{}).get(name)!=sha(ROOT/name):
            raise ValueError('Native renderer header changed or missing from build receipt; rebuild the runtime: '+name)
    sdk=ROOT/'.toolchain/android-sdk';ndk=sdk/'ndk/28.2.13676358';tools=sdk/'build-tools/34.0.0'
    library=native/'cmake/libmain.so'
    if sha(library)!=receipt['library_sha256']:raise ValueError('Native library receipt mismatch')
    reader=ndk/'toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf'
    elf=subprocess.check_output([str(reader),'-h','--dyn-syms',str(library)],text=True)
    symbols=['SDL_main','Java_org_urbanchaos_launcher_NativeGame_validate','Java_org_urbanchaos_launcher_NativeGame_menuButton','Java_org_urbanchaos_launcher_diagnostics_NativeReports_start']
    symbols += ['Java_org_urbanchaos_launcher_NativeGame_'+name for name in ('padButton','padStick','padLookDelta','clearTouch','displayMode','readTextArchive')]
    if 'AArch64' not in elf or any(name not in elf for name in symbols):raise ValueError('Runtime architecture or actual JNI entry missing')
    def sources():
        paths=[]
        for directory in ('launcher/app/src','launcher/core/src','launcher/android/src','vendor/SDL/android-project/app/src/main/java'):
            paths.extend(p for p in (ROOT/directory).rglob('*') if p.is_file())
        paths.extend(ROOT/name for name in ('launcher/settings.gradle','launcher/build.gradle','launcher/gradle.properties','launcher/app/build.gradle','launcher/gradle/verification-metadata.xml'))
        return {str(p.relative_to(ROOT)):sha(p) for p in sorted(paths)}
    app_sources=sources()
    if locale:app_sources['private-locale-pack']=sha(locale)
    build='experimental-'+hashlib.sha256(json.dumps({'native':receipt['library_sha256'],'sources':app_sources},sort_keys=True).encode()).hexdigest()[:16]
    output.mkdir(parents=True)
    with (output/'build.log').open('w') as log:
        subprocess.run(['bash',str(ROOT/'scripts/build_launcher_base.sh'),'-PurbanNativeBuild='+str(native),'-PurbanBuildId='+build,'assembleDebug'],cwd=ROOT,check=True,stdout=log,stderr=subprocess.STDOUT)
    after=sources()
    if locale:after['private-locale-pack']=sha(locale)
    if after!=app_sources:raise ValueError('Application sources changed during Gradle build; rebuild the private APK')
    base=ROOT/'launcher/app/build/outputs/apk/debug/app-debug.apk'
    libraries={'libmain.so':library,'libSDL3.so':native/'cmake/SDL/libSDL3.so','libc++_shared.so':ndk/'toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so'}
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    unsigned=output/'unsigned.apk';aligned=output/'aligned.apk';final=output/'UrbanChaos-experimental-arm64.apk'
    with zipfile.ZipFile(base) as source,zipfile.ZipFile(unsigned,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as target:
        for item in source.infolist():
            if item.filename.startswith(('lib/','META-INF/')):continue
            if pathlib.PurePosixPath(item.filename).suffix.lower() in ('.iso','.xbe','.xmv','.saf','.enc'):raise ValueError('Title asset in base APK')
            target.writestr(item,source.read(item.filename))
        if locale:target.write(locale,'assets/localization/hope-street-ptbr.bin')
        for name,path in libraries.items():target.write(path,'lib/arm64-v8a/'+name)
        for name,expected in lock['licenses'].items():
            path=ROOT/name
            if sha(path)!=expected:raise ValueError('License checksum mismatch: '+name)
            target.write(path,'assets/licenses/'+path.name)
    subprocess.run([str(tools/'zipalign'),'-f','4',str(unsigned),str(aligned)],check=True)
    java_env=dict(os.environ,JAVA_HOME=str(ROOT/'.toolchain/jdk/jdk-17.0.17+10'))
    subprocess.run([str(tools/'apksigner'),'sign','--ks',str(ROOT/'.toolchain/debug.keystore'),'--ks-pass','pass:android','--key-pass','pass:android','--ks-key-alias','androiddebugkey','--out',str(final),str(aligned)],check=True,env=java_env)
    subprocess.run([str(tools/'apksigner'),'verify','--verbose',str(final)],check=True,env=java_env)
    subprocess.run([str(tools/'zipalign'),'-c','4',str(final)],check=True)
    with zipfile.ZipFile(final) as packaged:
        names=packaged.namelist()
        if {name for name in names if name.startswith('lib/')}!={'lib/arm64-v8a/'+name for name in libraries}:raise ValueError('Unexpected native ABI/library packaging')
        for name,path in libraries.items():
            with packaged.open('lib/arm64-v8a/'+name) as source:
                if hashlib.file_digest(source,'sha256').hexdigest()!=sha(path):raise ValueError('Packaged library mismatch')
    result={'status':'PRIVATE_EXPERIMENTAL_APK_BUILT','build_id':build,'app_sources':app_sources,'apk_sha256':sha(final),'apk_bytes':final.stat().st_size,
            'native_library_sha256':receipt['library_sha256'],'xbe_sha256':receipt['xbe_sha256'],'libraries':{name:sha(path) for name,path in libraries.items()},
            'device_tested':False,'game_android_boot_verified':False,'mission_playable_verified':False,'assets_bundled':False,'public_upload_authorized':False,'private_locale_pack_sha256':sha(locale) if locale else None}
    (output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    (output/'SHA256SUMS').write_text(result['apk_sha256']+'  '+final.name+'\n')
    unsigned.unlink();aligned.unlink()
    print(json.dumps(result))

if __name__=='__main__':main()
