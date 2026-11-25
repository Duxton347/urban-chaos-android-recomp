import unittest,pathlib,sys,subprocess,tempfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from fuse_tail_fragments import fuse
class TailFusion(unittest.TestCase):
 def test_real_frame_and_nonlocal_return(self):
  def body(addr,instructions):
   return '''X86_FUNC(__imp__sub_ADDRESS) {
 X86_LOAD_FLAGS();
 uint32_t ca=0,cb=0;
\tconst uint32_t x86_expected_slot = ctx.esp;
 const bool x86_resuming=ctx.resume_pc!=0;
 const uint32_t x86_expected_return=ctx.mem[ctx.esp];
 const uint32_t x86_resume_pc=ctx.resume_pc;ctx.resume_pc=0;
 switch (x86_resume_pc) {case 0:break;default:throw 1;}
\t// authored guest operation
INSTRUCTIONS
}
X86_WEAK_FUNC(sub_ADDRESS,__imp__sub_ADDRESS)
'''.replace('ADDRESS',addr).replace('INSTRUCTIONS',instructions)
  ret='''ctx.return_slot=ctx.esp;ctx.return_target=ctx.mem[ctx.esp++];
 if(x86_resuming||ctx.return_target!=x86_expected_return||ctx.return_slot!=x86_expected_slot){X86_SAVE_FLAGS();if(!x86_resuming)throw X86Transfer{ctx.return_target,ctx.esp};}return;'''
  src=body('00011000','ctx.mem[--ctx.esp]=77;\ngoto loc_00012100;\nloc_00012100:\n{ X86_SAVE_FLAGS(); sub_00012000(ctx, base); return; }')
  src+=body('00012000','goto loc_00012100;\nloc_00012100:\nif(ctx.alter)ctx.mem[ctx.esp+1]=0x9999;\n++ctx.esp;\n'+ret)
  changed,proof=fuse(src);self.assertEqual(proof['fused_edges'],1)
  prefix='''#include <cstdint>
#include <cassert>
struct X86Transfer{uint32_t target,esp;};
struct C{uint32_t esp=32,mem[64]{},resume_pc=0,return_slot=0,return_target=0,flags=0;bool alter=false;};
#define X86_FUNC(n) void n(C& ctx,uint8_t* base)
#define X86_LOAD_FLAGS() unsigned flag=ctx.flags
#define X86_SAVE_FLAGS() ctx.flags=flag
#define X86_WEAK_FUNC(a,b) void a(C& c,uint8_t* p){b(c,p);}
void sub_00012000(C&,uint8_t*);
'''
  for optimized,code in ((False,src),(True,changed)):
   main='''int main(){C c;c.mem[32]=0x1234;bool threw=false;
 try{__imp__sub_00011000(c,nullptr);}catch(const X86Transfer&){threw=true;}
 assert(threw==EXPECTED);assert(c.esp==33&&c.return_target==0x1234);
 // Independent dynamic entry is preserved and still detects the wrong native frame.
 c={};c.mem[32]=77;c.mem[33]=0x5678;threw=false;
 try{__imp__sub_00012000(c,nullptr);}catch(const X86Transfer&){threw=true;}
 assert(threw&&c.return_target==0x5678);
 c={};c.mem[32]=0x1234;c.alter=true;threw=false;
 try{__imp__sub_00011000(c,nullptr);}catch(const X86Transfer&){threw=true;}
 assert(threw&&c.return_target==0x9999);}

'''.replace('EXPECTED','false' if optimized else 'true')
   with tempfile.TemporaryDirectory() as folder:
    p=pathlib.Path(folder);(p/'case.cpp').write_text(prefix+code+main)
    subprocess.run(['c++','-std=c++20',str(p/'case.cpp'),'-o',str(p/'run')],check=True,capture_output=True)
    subprocess.run([str(p/'run')],check=True)
 def test_real_calls_are_excluded(self):
  # A normal call is not eligible for the tail-edge transformation.
  source='X86_PUSH32(0x100); sub_00012000(ctx, base);'
  self.assertEqual(fuse(source)[0],source)
