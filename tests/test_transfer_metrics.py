"""Execute the patched SDK callback loop with authored transitions, not game code."""
import pathlib,subprocess,sys,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from prepare_runtime_overlay import prepare
class TransferMetrics(unittest.TestCase):
 def test_nonresumable_overlay_does_not_receive_transfer_instrumentation(self):
  with tempfile.TemporaryDirectory(prefix='transfer-stock-',dir=ROOT/'build') as directory:
   sdk=pathlib.Path(directory)/'sdk';patches=prepare(sdk,context=False)
   self.assertNotIn('guest-transfer-metrics.patch',patches)
   self.assertNotIn('slot->transfers.fetch_add',(sdk/'runtime/cpu/dispatch.cpp').read_text())
 def test_real_callback_loop_counts_resumes_and_preserves_unwinding(self):
  with tempfile.TemporaryDirectory(prefix='transfer-contract-',dir=ROOT/'build') as directory:
   area=pathlib.Path(directory);sdk=area/'sdk';prepare(sdk,context=True)
   source=(sdk/'runtime/cpu/dispatch.cpp').read_text();start=source.index('uint32_t CallGuest(');end=source.index('\n}  // namespace kernel',start)
   body=source[start:end]
   prefix=r'''
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <stdexcept>
#include "diagnostics/progress.h"
#define XBE_RESUMABLE_GUEST 1
struct X86Context {uint32_t eax=0,ecx=0,edx=0,ebx=0,esi=0,edi=0,ebp=0,esp=4096,return_slot=0,return_target=0,resume_pc=0,transfer_depth=0;};
struct X86Transfer {uint32_t target,esp;};
using X86Func=void(X86Context&,uint8_t*);
std::unordered_map<uint32_t,X86Func*> entries,g_resumes;
namespace dispatch {X86Func* Find(uint32_t t){auto i=entries.find(t);return i==entries.end()?nullptr:i->second;}}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){entries.at(target)(c,b);}
void x86_trap(X86Context&,uint32_t,const char*){throw std::runtime_error("authored missing continuation");}
namespace kernel {uint8_t* g_base=nullptr;void W32(uint32_t,uint32_t){}
'''
   suffix=r'''
}
void Done(X86Context& c,uint8_t*){c.eax=42;c.return_slot=c.esp;c.return_target=0xfffffff0;}
void Resume(X86Context& c,uint8_t*){assert(c.resume_pc==0x220);Done(c,nullptr);}
void Transfer(X86Context& c,uint8_t*){throw X86Transfer{0x220,c.esp};}
void FunctionTransfer(X86Context& c,uint8_t*){throw X86Transfer{0x330,c.esp};}
void Missing(X86Context& c,uint8_t*){throw X86Transfer{0xbad,c.esp};}
void ForeignRoot(X86Context& c,uint8_t*){c.return_slot=c.esp-8;throw X86Transfer{0xfffffff0,c.esp};}
int main(){
 entries={{1,Done},{2,Transfer},{3,FunctionTransfer},{4,Missing},{5,ForeignRoot},{0x330,Done}};g_resumes={{0x220,Resume}};
 auto* slot=urban::progress::Local();
 for(unsigned entry=1;entry<=5;entry++){
  slot->transfers=0;slot->resume_steps=0;X86Context c;c.ecx=77;c.transfer_depth=entry==5?1:0;
  bool missing=false,foreign=false;
  try {auto r=kernel::CallGuest(c,entry,nullptr,0);assert(r==42&&c.ecx==77&&c.esp==4096);}
  catch(const std::runtime_error&){missing=true;}
  catch(const X86Transfer& t){assert(t.target==0xfffffff0);foreign=true;}
  assert(missing==(entry==4)&&foreign==(entry==5));
  assert(c.transfer_depth==(entry==5?1:0));
  assert(slot->transfers==(entry==1?0:1));
  assert(slot->resume_steps==((entry==2||entry==3||entry==4)?1:0));
  assert(slot->current==0);
 }
}
'''
   (area/'contract.cpp').write_text(prefix+body+suffix)
   subprocess.run(['c++','-std=c++20','-O2','-pthread','-I'+str(ROOT/'native'),str(area/'contract.cpp'),'-o',str(area/'contract')],check=True,capture_output=True)
   subprocess.run([str(area/'contract')],check=True,capture_output=True)
