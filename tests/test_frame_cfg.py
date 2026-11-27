import pathlib,sys,tempfile,subprocess,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from fuse_frame_cfg import fuse_frame
class FrameCfg(unittest.TestCase):
 def test_real_calls_continuations_and_nonlocal_returns(self):
  def function(address,payload,continuation=''):
   return '''X86_FUNC(__imp__sub_ADDR) {
\tconst uint32_t x86_frame = ctx.esp;
\ttry {
 X86_LOAD_FLAGS();uint32_t ca=0,cb=0;(void)ca;(void)cb;
\tconst uint32_t x86_expected_slot = ctx.esp;
 const bool x86_resuming=ctx.resume_pc!=0;
 const uint32_t x86_expected_return=x86_resuming?0:ctx.mem[ctx.esp];
 const uint32_t x86_resume_pc=ctx.resume_pc;ctx.resume_pc=0;
 switch (x86_resume_pc){case 0:break;CONT default:throw 9;}
\t// ADDR authored instruction
PAYLOAD
\t} catch (const X86Resume& r) {
  if(r.esp>=x86_frame)throw;
  x86_resume(ctx,base,r);
 }
}
X86_WEAK_FUNC(sub_ADDR,__imp__sub_ADDR)
'''.replace('ADDR',address).replace('PAYLOAD',payload).replace('CONT',continuation)
  def tail(a):return '{ X86_SAVE_FLAGS(); sub_'+a+'(ctx, base); return; }'
  ret='''++ctx.esp;if(ctx.alter)ctx.mem[ctx.esp]=0x9999;
 ctx.return_slot=ctx.esp;ctx.return_target=ctx.mem[ctx.esp++];
 if(x86_resuming||ctx.return_target!=x86_expected_return||ctx.return_slot!=x86_expected_slot){X86_SAVE_FLAGS();if(!x86_resuming)throw X86Transfer{ctx.return_target,ctx.esp};}return;'''
  source=function('00001000','ctx.mem[--ctx.esp]=77;\n'+tail('00001100'))
  source+=function('00001100','ctx.mem[--ctx.esp]=0x1105;sub_00002000(ctx,base);\nloc_00001105:\n++ctx.iter;\nif(ctx.iter<3)'+tail('00001100')+'\n'+tail('00001200'),'case 0x1105:goto loc_00001105;')
  source+=function('00001200',ret)
  changed,proof=fuse_frame(source,'00001000',0x1300);self.assertEqual((proof['blocks'],proof['edges']),(3,3));self.assertTrue(changed.endswith(source[source.index('X86_FUNC(__imp__sub_00001100)'):]))
  prefix='''#include <cassert>
#include <cstdint>
#include <initializer_list>
struct X86Transfer{uint32_t target,esp;};struct X86Resume{uint32_t esp;};
struct C{uint32_t mem[64]{},esp=32,resume_pc=0,return_slot=0,return_target=0,iter=0,calls=0,cf=0,zf=0,sf=0,of=0,pf=0;bool alter=false,escape=false;};
#define X86_FUNC(n) void n(C& ctx,uint8_t* base)
#define X86_LOAD_FLAGS() unsigned cf=ctx.cf,zf=ctx.zf,sf=ctx.sf,of=ctx.of,pf=ctx.pf
#define X86_SAVE_FLAGS() (ctx.cf=cf,ctx.zf=zf,ctx.sf=sf,ctx.of=of,ctx.pf=pf)
#define X86_WEAK_FUNC(a,b) void a(C& c,uint8_t*p){b(c,p);}
void x86_resume(C&,uint8_t*,const X86Resume&){throw 99;}
void sub_00001000(C&,uint8_t*);void sub_00001100(C&,uint8_t*);void sub_00001200(C&,uint8_t*);
void sub_00002000(C& c,uint8_t*){++c.calls;if(c.escape)throw X86Resume{40};assert(c.mem[c.esp]==0x1105);++c.esp;}
'''
  for optimized,code in [(False,source),(True,changed)]:
   main='''int main(){for(bool alter:{false,true}){C c;c.mem[32]=0x1234;c.alter=alter;bool threw=false;try{sub_00001000(c,nullptr);}catch(const X86Transfer&){threw=true;}
assert(c.calls==3&&c.iter==3&&c.esp==33&&c.return_slot==32&&c.return_target==(alter?0x9999:0x1234));assert(threw==(alter||!OPT));}
 C c;c.mem[32]=0x1234;c.escape=true;bool escape=false;try{sub_00001000(c,nullptr);}catch(const X86Resume&){escape=true;}assert(escape&&c.calls==1);
 C external;external.mem[32]=77;external.mem[33]=0x4567;bool threw=false;try{sub_00001200(external,nullptr);}catch(const X86Transfer&){threw=true;}assert(threw&&external.return_target==0x4567);
 C resumed;resumed.mem[32]=77;resumed.mem[33]=0x7654;resumed.resume_pc=0x1105;try{sub_00001100(resumed,nullptr);}catch(const X86Transfer&){}assert(resumed.iter==3&&resumed.calls==2&&resumed.return_target==0x7654);}
'''.replace('OPT','true' if optimized else 'false')
   with tempfile.TemporaryDirectory() as t:
    p=pathlib.Path(t);(p/'a.cpp').write_text(prefix+code+main);result=subprocess.run(['c++','-std=c++20',str(p/'a.cpp'),'-o',str(p/'a')],capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stderr);subprocess.run([str(p/'a')],check=True)
 def test_unknown_extent_rejected(self):
  with self.assertRaises(ValueError):fuse_frame('','00001000',0x1000)
