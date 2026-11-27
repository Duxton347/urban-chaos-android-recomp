#!/usr/bin/env python3
"""Read pinned public sources and emit bounded provenance/findings, never game bytes."""
import hashlib,json,pathlib,subprocess
R=pathlib.Path(__file__).resolve().parents[1];lock=json.loads((R/'dependencies.lock.json').read_text());out={'status':'BLOCKED_GAME_RECOMPILATION','references':{},'findings':[]}
for name in ('burner','runtime','black','xboxkit'):
 p=R/'vendor'/name;sha=subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip()
 if sha!=lock['repositories'][name]['commit']:raise SystemExit(name+': pin mismatch')
 out['references'][name]={'commit':sha,'url':lock['repositories'][name]['url'],'license':lock['repositories'][name]['license']}
 checks={'burner':['README.md','COPYRIGHT','docs/BUILDING.md','dependencies.lock.json','scripts/build_xbe_game_trial.py'],'runtime':['README.md','LICENSE','cmake/XbeRecomp.cmake','tools/recomp/main.cpp'],'black':['README.md','LICENSE','CMakeLists.txt','native/touch.cpp'],'xboxkit':['README.md','LibXGD/LibXGD.csproj','LibXGD/XGD.cs','XboxKit/XboxKit.csproj']}[name]
 out['references'][name]['files']={f:hashlib.sha256((p/f).read_bytes()).hexdigest() for f in checks}
area=R/'vendor/burner/build/xbe-full-integration-20261014'
out['findings']=[{'id':'private-generation-inputs','present':area.exists(),'evidence':'burner/docs/BUILDING.md:3-7','result':'not reproducible from the published checkout alone'}, {'id':'xboxkit-license','evidence':'both csproj PackageLicenseExpression=MIT; no LICENSE text in tracked tree','result':'audit-only; no XboxKit C# embedded'}, {'id':'runtime-pins','burner':lock['runtime_base'],'black':lock['runtime_black_pin'],'result':'different pins; compare contracts before reuse'}, {'id':'diagnostic','result':'historical module is not a game port and is excluded from current APK delivery'}]
(R/'build').mkdir(exist_ok=True);(R/'build/reference-audit.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
