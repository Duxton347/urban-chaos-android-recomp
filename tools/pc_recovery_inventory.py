#!/usr/bin/env python3
"""Inventory public PC source archives/history. Emit metadata, never title payloads."""
import argparse,hashlib,json,pathlib,re,struct,subprocess,zipfile
R=pathlib.Path(__file__).resolve().parents[1]
GENERATED=re.compile(r'(?:^|/)(?:src/recomp/gen/.*\.[ch]|generated/.*\.(?:cpp|h)|x86_func_mapping\.cpp)$',re.I)
def digest(path):
 with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def inventory(downloads,repo,lock):
 report={'kind':'PC_RECOVERY_SOURCE_INVENTORY','archives':{},'history':[],'qualified_game_build':False}
 for name,meta in lock['archives'].items():
  p=downloads/name
  if not p.exists() or name=='cmake-windows.zip':continue
  sha=digest(p)
  if sha!=meta['sha256']:raise ValueError(name+': checksum mismatch')
  with zipfile.ZipFile(p) as z:
   entries=[i for i in z.infolist() if not i.is_dir()];names=[i.filename for i in entries]
   info={'sha256':sha,'url':meta['url'],'entries':len(entries),'uncompressed_bytes':sum(i.file_size for i in entries),'generated_units':[n for n in names if GENERATED.search(n)],'original_images':[n for n in names if n.lower().endswith(('.xbe','.iso','.xiso'))],'integration_status':[n for n in names if n.endswith('integration-status.json')]}
   for i in entries:
    if i.filename.endswith('config/game.json'):
     data=z.read(i);a=json.loads(data)
     info['declared_identity']={k:v for k,v in a.items() if k in ('title_id','version','xbe_size','xbe_sha256','xdk_build','entry_point','kernel_thunk_table','kernel_import_count','section_count','xboxrecomp_revision')}
     info['identity_source']={'path':i.filename,'sha256':hashlib.sha256(data).hexdigest()}
    elif i.filename.endswith('config/seed_functions.json'):
     info['legacy_seed_count']=len(json.loads(z.read(i)))
    elif i.filename=='runtime/UrbanChaosXbeTrial.exe':
     b=z.read(i);pe=struct.unpack_from('<I',b,0x3c)[0]
     if b[:2]!=b'MZ' or b[pe:pe+4]!=b'PE\0\0':raise ValueError('Invalid published PE')
     candidate='b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac';size=5283840
     locations=[m.start() for m in re.finditer(b'XBEH',b)]
     info['pc_binary']={'path':i.filename,'size':len(b),'sha256':hashlib.sha256(b).hexdigest(),'pe_machine':hex(struct.unpack_from('<H',b,pe+4)[0]),'embedded_raw_candidate':any(hashlib.sha256(b[o:o+size]).hexdigest()==candidate for o in locations)}
   report['archives'][name]=info
 commits=subprocess.check_output(['git','-C',str(repo),'rev-list','--all'],text=True).splitlines()
 for rev in commits:
  names=subprocess.check_output(['git','-C',str(repo),'ls-tree','-r','--name-only',rev],text=True).splitlines()
  report['history'].append({'commit':rev,'files':len(names),'configs':[n for n in names if n.startswith('config/')],'generated_units':[n for n in names if GENERATED.search(n)],'original_images':[n for n in names if n.lower().endswith(('.xbe','.iso','.xiso'))],'integration_status':[n for n in names if n.endswith('integration-status.json')]})
 report['history_commits_checked']=len(commits)
 return report

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--downloads',type=pathlib.Path,default=R/'private/pc-recovery/downloads');p.add_argument('--repo',type=pathlib.Path,default=R/'vendor/burner');p.add_argument('--out',type=pathlib.Path,default=R/'build/pc-recovery/inventory.json');a=p.parse_args()
 lock=json.loads((R/'config/pc-recovery.lock.json').read_text());report=inventory(a.downloads,a.repo,lock);a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({'archives_checked':len(report['archives']),'commits_checked':report['history_commits_checked'],'generated_units_found':sum(len(i['generated_units']) for i in report['archives'].values()),'original_images_found':sum(len(i['original_images']) for i in report['archives'].values()),'out':str(a.out),'qualified_game_build':False},indent=2))
if __name__=='__main__':main()
