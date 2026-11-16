#!/usr/bin/env python3
"""Regress actual SDK nonlocal returns with authored x86, never a commercial image."""
import argparse
import hashlib
import importlib
import json
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from test_tools import xbe


def command(argv, log):
    with log.open('w') as output:
        subprocess.run(list(map(str,argv)),stdout=output,stderr=subprocess.STDOUT,check=True)


def reference(area, symbols):
    try:
        result=subprocess.run([str(area/'reference-x86')],capture_output=True,timeout=3)
        assert result.returncode==42, 'Authored native x86 reference did not return 42'
        return {'engine':'native-i386','result':42}
    except OSError as error:
        if error.errno!=8: raise
    sys.path.insert(0,str(ROOT/'.toolchain/reference-isa'))
    try:
        engine=importlib.import_module('unicorn')
        registers=importlib.import_module('unicorn.x86_const')
    except ImportError:
        return {'engine':'unavailable','reason':'native i386 execution unavailable; optional reference engine not installed'}
    cpu=engine.Uc(engine.UC_ARCH_X86,engine.UC_MODE_32)
    cpu.mem_map(0x10000,0x30000)
    cpu.mem_write(0x11000,(area/'code.bin').read_bytes())
    cpu.mem_write(0x18000,(area/'data.bin').read_bytes())
    values=[]
    def stop(machine, interrupt, user):
        assert interrupt==0x80 and machine.reg_read(registers.UC_X86_REG_EAX)==1
        values.append(machine.reg_read(registers.UC_X86_REG_EBX));machine.emu_stop()
    cpu.hook_add(engine.UC_HOOK_INTR,stop)
    cpu.emu_start(symbols['_start'],0x12000,count=10000)
    assert values==[42], 'Authored reference fixture failed'
    return {'engine':'Unicorn','version':engine.__version__,'result':42,'scope':'authored synthetic fixture only'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool-area',type=pathlib.Path,default=ROOT/'build/context-tools-patched')
    args=parser.parse_args();tool_area=args.tool_area.resolve()
    tool=tool_area/'cmake/xbe_recomp_resumable';sdk=ROOT/'vendor/runtime';patched=tool_area/'sdk'
    receipt=json.loads((tool_area/'receipt.json').read_text())
    assert receipt['binary_sha256']==hashlib.sha256(tool.read_bytes()).hexdigest()
    for name,digest in receipt['patches'].items():
        assert digest==hashlib.sha256((ROOT/'patches/runtime'/name).read_bytes()).hexdigest(),'Stale tool patch receipt'
    (ROOT/'build').mkdir(exist_ok=True)
    area=pathlib.Path(tempfile.mkdtemp(prefix='context-regression-',dir=ROOT/'build'))
    subprocess.run(['as','--32',str(ROOT/'tests/fixtures/context_switch.s'),'-o',str(area/'fixture.o')],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x11000','--section-start=.data=0x18000','--section-start=.bss=0x19000','-e','_start',str(area/'fixture.o'),'-o',str(area/'reference-x86')],check=True)
    for section,name in (('.text','code.bin'),('.data','data.bin')):
        subprocess.run(['objcopy','-O','binary','-j',section,str(area/'reference-x86'),str(area/name)],check=True)
    rows=subprocess.check_output(['nm','-g','--defined-only',str(area/'reference-x86')],text=True).splitlines()
    symbols={row.split()[2]:int(row.split()[0],16) for row in rows if len(row.split())==3}
    oracle=reference(area,symbols)
    code=(area/'code.bin').read_bytes();image=xbe()[:0x400]+code
    struct.pack_into('<I',image,0x368,max(0x100,len(code)))
    struct.pack_into('<I',image,0x370,len(code))
    (area/'synthetic.xbe').write_bytes(image)
    results={}
    for mode in ('legacy','resumable','resumable-repeat','patched-default'):
        enabled=mode.startswith('resumable');cfg=area/f'{mode}.toml'
        cfg.write_text('[xbe]\ncode_sections=[".text"]\npartitions=1\ngap_fill=false\npointer_seeds=false\nresumable='+str(enabled).lower()+'\n[functions]\n'+''.join(f'"0x{symbols[name]:x}"="{name}"\n' for name in ('entry','checkpoint','restore_context','worker')))
        compiler=ROOT/'build/host/xbe_recomp' if mode=='legacy' else tool
        output=area/mode
        command([compiler,cfg,area/'synthetic.xbe',output],area/f'{mode}-generate.log')
        if mode in ('resumable-repeat','patched-default'):continue
        headers=patched/'runtime' if enabled else sdk/'runtime'
        argv=['c++','-std=c++20','-O1','-g0','-march=x86-64-v2','-fsigned-char','-fno-strict-aliasing','-I'+str(headers),'-I'+str(sdk/'runtime'),'-I'+str(output)]
        if enabled:argv+=['-DXBE_RESUMABLE_GUEST=1']
        argv += [str(ROOT/'host/context-tests/main.cpp'),str(headers/'cpu/dispatch.cpp'),str(sdk/'runtime/kernel/memory.cpp'),str(sdk/'runtime/kernel/ordinals.cpp'),str(sdk/'runtime/core/log.cpp'),*map(str,output.glob('x86_recomp.*.cpp')),str(output/'x86_func_mapping.cpp'),'-pthread','-o',str(area/f'run-{mode}')]
        command(argv,area/f'{mode}-build.log')
        run=subprocess.run([str(area/f'run-{mode}'),str(area/'code.bin'),str(area/'data.bin')],capture_output=True,text=True,timeout=8)
        (area/f'{mode}-run.log').write_text(run.stdout+run.stderr)
        results[mode]={'exit':run.returncode,'output':run.stdout.strip()}
        assert run.returncode==(0 if enabled else 1),(mode,run.stdout,run.stderr)
    nested=subprocess.run([str(area/'run-resumable'),str(area/'code.bin'),str(area/'data.bin'),'nested'],capture_output=True,text=True,timeout=8)
    assert nested.returncode==0,('nested native callback',nested.stdout,nested.stderr)
    results['nested_callback']={'exit':nested.returncode,'output':nested.stdout.strip()}
    def hashes(directory):
        return {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in directory.iterdir() if p.is_file()}
    assert hashes(area/'resumable')==hashes(area/'resumable-repeat'),'Nondeterministic continuation generation'
    for name in ('x86_recomp.0.cpp','x86_func_mapping.cpp'):
        assert (area/'legacy'/name).read_bytes()==(area/'patched-default'/name).read_bytes(),'Default code generation changed'
    report={'scope':'synthetic context switching only; no game data','reference':oracle,'results':results,'deterministic':True,'default_cpp_unchanged':True,'sdk':receipt['sdk'],'patches':receipt['patches']}
    (ROOT/'build/context-regression-result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
