import pathlib,subprocess,hashlib,json
R=pathlib.Path(__file__).resolve().parents[1];lock=json.loads((R/'dependencies.lock.json').read_text());receipt={'repositories':{},'tools':{}}
for name in ('runtime','zydis','tomlplusplus'):
 sha=subprocess.check_output(['git','-C',str(R/'vendor'/name),'rev-parse','HEAD'],text=True).strip()
 if sha!=lock['repositories'][name]['commit']:raise SystemExit(name+': pin mismatch')
 if subprocess.check_output(['git','-C',str(R/'vendor'/name),'status','--porcelain'],text=True).strip():raise SystemExit(name+': modified sources')
 receipt['repositories'][name]=sha
for name in ('xbe_recomp','xbe_symbols'):
 with (R/'build/host'/name).open('rb') as f:receipt['tools'][name]=hashlib.file_digest(f,'sha256').hexdigest()
(R/'build/host/host-tools.receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
(R/'docs/evidence/host-tools.receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
