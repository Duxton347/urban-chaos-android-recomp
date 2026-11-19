#!/usr/bin/env python3
"""Run pinned public tools. Generated output is NOT G1 qualification."""
import argparse, hashlib, json, pathlib, subprocess, sys, tomllib, re
from inspect_xbe import inspect,CANDIDATE_SHA256,Invalid
R=pathlib.Path(__file__).resolve().parents[1]
PIN='c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7'
def run(cmd,cwd,log):
    with log.open('w') as f:p=subprocess.run(list(map(str,cmd)),cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
    if p.returncode:raise Invalid(f'Ferramenta falhou ({p.returncode}); veja {log}')
def generate(xbe,config,output,expected_sha256=CANDIDATE_SHA256):
    xbe=pathlib.Path(xbe).resolve();config=pathlib.Path(config).resolve();output=pathlib.Path(output).resolve()
    build=(R/'build').resolve()
    if build not in output.parents or output.exists():raise Invalid('Saída deve ser nova e dentro de build/ ignorado pelo Git')
    if subprocess.check_output(['git','-C',str(R/'vendor/runtime'),'rev-parse','HEAD'],text=True).strip()!=PIN:raise Invalid('Runtime fora do pin')
    if subprocess.check_output(['git','-C',str(R/'vendor/runtime'),'status','--porcelain'],text=True).strip():raise Invalid('Runtime modificado; checkout limpo exigido')
    if not re.fullmatch(r'[0-9a-fA-F]{64}',expected_sha256):raise Invalid('SHA-256 explícito inválido')
    report=inspect(xbe,expected_sha256.lower())
    # Validate includes recursively, retain hashes. Explicit config required; scanner alone is not qualification.
    configs={};visiting=set()
    def load(p):
        if p in visiting:raise Invalid('Ciclo em includes TOML')
        if str(p) in configs:return
        visiting.add(p);data=p.read_bytes();cfg=tomllib.loads(data.decode());configs[str(p)]=hashlib.sha256(data).hexdigest()
        for inc in cfg.get('includes',[]):load((p.parent/inc).resolve())
        visiting.remove(p)
    load(config)
    output.mkdir(parents=True);sdk=R/'vendor/runtime';tools=R/'build/host'
    receipt=json.loads((tools/'host-tools.receipt.json').read_text())
    if receipt['repositories']['runtime']!=PIN:raise Invalid('Receita host fora do pin')
    meta=dict(status='RUNNING',sdk=PIN,xbe=report['sha256'],reference_candidate_match=report['candidate_match'],source_profile=('REFERENCE' if report['candidate_match'] else 'EXPLICIT_RESEARCH_VARIANT'),configs=configs,tools={})
    try:
        for name in ('xbe_symbols','xbe_recomp'):
            with (tools/name).open('rb') as f:meta['tools'][name]=hashlib.file_digest(f,'sha256').hexdigest()
            if meta['tools'][name]!=receipt['tools'][name]:raise Invalid('Binário host difere da receita de build')
        run([tools/'xbe_symbols',xbe,sdk/'xdk','--out',output/'scanned.toml','--name','urban_chaos'],output,output/'scanner.log')
        for n in (1,2):run([tools/'xbe_recomp',config,xbe,output/f'pass{n}','--xdk',sdk/'xdk'],output,output/f'recomp{n}.log')
        def inventory(d):return {str(p.relative_to(d)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(d.rglob('*')) if p.is_file()}
        a,b=inventory(output/'pass1'),inventory(output/'pass2');meta['deterministic']=a==b;meta['generated']=a
        if a!=b or not a:raise Invalid('Geração vazia ou não determinística')
        if not any(re.search(r'^X86_FUNC\(', p.read_text(),re.M) for p in (output/'pass1').glob('x86_recomp.*.cpp')):raise Invalid('Geração sem funções; revisar code_sections/configuração')
        # Unmapped indirect calls, ABI and hook correctness still require title evidence.
        warnings=[]
        for log in (output/'scanner.log',output/'recomp1.log'):
            warnings.extend(line for line in log.read_text(errors='replace').splitlines() if re.search(r'unresolved|unknown|unsupported|missing|warning',line,re.I))
        meta.update(status='REQUIRES_REVERSE_ENGINEERING',diagnostics=warnings,g1_qualified=False)
    except BaseException as e:
        meta.update(status='BLOCKED',error=str(e),g1_qualified=False);raise
    finally:(output/'generation-report.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('xbe');p.add_argument('--config',required=True);p.add_argument('--out',required=True);p.add_argument('--expected-sha256',default=CANDIDATE_SHA256,help='Identidade explícita da cópia de pesquisa; não qualifica compatibilidade');a=p.parse_args()
    try:print(json.dumps(generate(a.xbe,a.config,a.out,a.expected_sha256),indent=2));return 0
    except (OSError,ValueError,subprocess.SubprocessError) as e:print(str(e),file=sys.stderr);return 2
if __name__=='__main__':sys.exit(main())
