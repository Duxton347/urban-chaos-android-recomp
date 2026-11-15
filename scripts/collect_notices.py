import pathlib,re,hashlib,json
R=pathlib.Path(__file__).resolve().parents[1];blocks={}
for folder in ['src','include','android-project/app/src/main/java']:
 for p in sorted((R/'vendor/SDL'/folder).rglob('*')):
  if p.suffix not in ('.h','.c','.cpp','.java'):continue
  for block in re.findall(r'/\*.*?\*/',p.read_text(errors='replace'),re.S):
   if re.search(r'copyright|permission is hereby|licensed under',block,re.I):blocks.setdefault(block,[]).append(str(p.relative_to(R/'vendor/SDL')))
out=['# SDL source notices\n\nUnmodified SDL release-3.4.16. HIDAPI is used under its BSD option.\n']
for block,paths in blocks.items():out.append('\nFiles: '+', '.join(paths)+'\n\n'+block+'\n')
(R/'licenses/SDL-source-notices.txt').write_text('\n'.join(out))
ndk=R/'.toolchain/android-sdk/ndk/28.2.13676358'
for name,src in [('Android-NDK-NOTICE.txt',ndk/'NOTICE'),('LLVM-NOTICE.txt',ndk/'toolchains/llvm/prebuilt/linux-x86_64/NOTICE')]:
 (R/'licenses'/name).write_bytes(src.read_bytes())
assets=R/'android/app/src/main/assets/licenses';assets.mkdir(parents=True,exist_ok=True)
for name in ('SDL-source-notices.txt','Android-NDK-NOTICE.txt','LLVM-NOTICE.txt'):(assets/name).write_bytes((R/'licenses'/name).read_bytes())
print(len(blocks),'distinct preserved SDL copyright/license comments')
