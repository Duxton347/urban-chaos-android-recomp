#!/usr/bin/env python3
"""Execute recompiled SYNTHETIC x86 on Linux. This is never a game/boot result."""
import pathlib,subprocess,sys
R=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'tests'))
from test_tools import xbe
area=R/'build/native-smoke';area.mkdir(parents=True,exist_ok=True)
image=xbe();image[0x400:0x406]=bytes.fromhex('b8 2a 00 00 00 c3') # mov eax,42; ret
(area/'synthetic.xbe').write_bytes(image)
config=area/'fixture.toml';config.write_text('[xbe]\ncode_sections=[".text"]\npartitions=1\ngap_fill=false\n[functions]\n"0x11000"="entry"\n')
subprocess.run([str(R/'build/host/xbe_recomp'),str(config),str(area/'synthetic.xbe'),str(area/'generated')],check=True)
(area/'main.cpp').write_text('''#include "cpu/x86_context.h"
#include <cstdint>
#include <cstdio>
#include <vector>
extern "C" void sub_00011000(X86Context&, uint8_t*);
int main(){std::vector<uint8_t> mem(0x20000);X86Context c{};c.esp=0x18000;sub_00011000(c,mem.data());if(c.eax!=42||c.esp!=0x18004)return 1;std::puts("SYNTHETIC_ONLY: recompiled x86 executed natively; eax=42; stack=+4");}
''')
cmd=['c++','-std=c++20','-O2','-fsigned-char','-fno-strict-aliasing','-I'+str(R/'vendor/runtime/runtime'),'-I'+str(area/'generated'),str(area/'main.cpp'),*map(str,(area/'generated').glob('x86_recomp.*.cpp')),'-o',str(area/'native-smoke')]
subprocess.run(cmd,check=True);subprocess.run([str(area/'native-smoke')],check=True)
