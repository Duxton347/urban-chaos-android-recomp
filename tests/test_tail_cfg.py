import pathlib,sys,subprocess,tempfile,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from fuse_tail_cfg import fuse_cfg
class TailCfg(unittest.TestCase):
 def test_shared_join_loop_flags_and_nonlocal_returns(self):
  def func(addr,ops):return '''X86_FUNC(__imp__sub_ADDRESS) {
 X86_LOAD_FLAGS();uint32_t ca=0,cb=0;
\tconst uint32_t x86_expected_slot = ctx.esp;
 const bool x86_resuming=ctx.resume_pc!=0;
 const uint32_t x86_expected_return=ctx.mem[ctx.esp];
 const uint32_t x86_resume_pc=ctx.resume_pc;ctx.resume_pc=0;
 switch (x86_resume_pc) {case 0:break;default:throw 1;}
\t// authored instruction
OPS
}
X86_WEAK_FUNC(sub_ADDRESS,__imp__sub_ADDRESS)
'''.replace('ADDRESS',addr).replace('OPS',ops)
  def tail(addr):return '{ X86_SAVE_FLAGS(); sub_'+addr+'(ctx, base); return; }'
  ret='''if(ctx.alter)ctx.mem[ctx.esp+1]=0x9999;
 ++ctx.esp;ctx.return_slot=ctx.esp;ctx.return_target=ctx.mem[ctx.esp++];
 if(x86_resuming||ctx.return_target!=x86_expected_return||ctx.return_slot!=x86_expected_slot){X86_SAVE_FLAGS();if(!x86_resuming)throw X86Transfer{ctx.return_target,ctx.esp};}return;'''
  source=func('00011000','ctx.mem[--ctx.esp]=77;\nif(ctx.path)'+tail('00012000')+'\n'+tail('00013000'))
  source+=func('00012000','loc_00012100:\n++ctx.iter;flag=ctx.iter;\n'+tail('00013000'))
  source+=func('00013000','loc_00012100:\nif(ctx.iter<3)'+tail('00012000')+'\n'+ret)
  changed,proof=fuse_cfg(source,['00011000']);self.assertEqual(proof['graphs'][0]['blocks'],3)
  # Only the original entry changes; dynamic fragments remain byte-identical.
  self.assertTrue(changed.endswith(source[source.index('X86_FUNC(__imp__sub_00012000)'):]))
  prefix='''#include <cassert>
#include <cstdint>
struct X86Transfer{uint32_t target,esp;};
struct C{uint32_t mem[64]{},esp=32,resume_pc=0,return_slot=0,return_target=0,flags=0,iter=0;bool alter=false,path=false;};
#define X86_FUNC(n) void n(C& ctx,uint8_t* base)
#define X86_LOAD_FLAGS() unsigned flag=ctx.flags
#define X86_SAVE_FLAGS() ctx.flags=flag
#define X86_WEAK_FUNC(a,b) void a(C& c,uint8_t* p){b(c,p);}
void sub_00011000(C&,uint8_t*);void sub_00012000(C&,uint8_t*);void sub_00013000(C&,uint8_t*);
'''
  for optimized,code in ((False,source),(True,changed)):
   main='''int main(){for(bool path:{false,true})for(bool alter:{false,true}){
 C c;c.mem[32]=0x1234;c.path=path;c.alter=alter;bool threw=false;
 try{sub_00011000(c,nullptr);}catch(const X86Transfer&){threw=true;}
 assert(threw==(alter||!OPT));assert(c.iter==3&&c.esp==33&&c.flags==3);
 assert(c.return_target==(alter?0x9999:0x1234)&&c.return_slot==32);
 }C c;c.mem[32]=77;c.mem[33]=0x5678;bool threw=false;
 try{sub_00013000(c,nullptr);}catch(const X86Transfer&){threw=true;}
 assert(threw&&c.return_target==0x5678&&c.flags==3);}
'''.replace('OPT','true' if optimized else 'false')
   with tempfile.TemporaryDirectory() as directory:
    p=pathlib.Path(directory);(p/'test.cpp').write_text('#include <initializer_list>\n'+prefix+code+main)
    subprocess.run(['c++','-std=c++20',str(p/'test.cpp'),'-o',str(p/'test')],check=True,capture_output=True)
    subprocess.run([str(p/'test')],check=True,capture_output=True)
 def test_unknown_calls_and_missing_blocks_are_rejected(self):
  with self.assertRaises(ValueError):fuse_cfg('',['00011000'])
  template="""X86_FUNC(__imp__sub_00011000) {
 X86_LOAD_FLAGS();
\tconst uint32_t x86_expected_slot = ctx.esp;
 switch (x86_resume_pc) {case 0: break; default: break;}
\t// authored operation
 OPS
}
X86_WEAK_FUNC(sub_00011000,__imp__sub_00011000)
"""
  for ops in ('x86_kernel(ctx, base, 1);','X86_PUSH32(0x123); sub_00012000(ctx, base);','{ X86_SAVE_FLAGS(); sub_00012000(ctx, base); return; }'):
   with self.subTest(ops=ops),self.assertRaises(ValueError):fuse_cfg(template.replace('OPS',ops),['00011000'])
