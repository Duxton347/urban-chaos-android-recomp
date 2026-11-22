import hashlib,sys,pathlib,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from relocate_return_helpers import transform,GUARD,FAST
class ReturnHelpers(unittest.TestCase):
 def test_only_reviewed_helper_changes(self):
  body='X86_FUNC(__imp__sub_00011000) {\n '+GUARD+'\n}'
  untouched='X86_FUNC(__imp__sub_00012000) {\n '+GUARD+'\n}'
  text=body+'\nX86_WEAK_FUNC(a,b)\n'+untouched+'\nX86_WEAK_FUNC(c,d)\n'
  result=transform(text,{'00011000':hashlib.sha256(body.encode()).hexdigest()})
  self.assertIn(untouched,result);self.assertEqual(result.count(FAST),1);self.assertEqual(result.count(GUARD),1)
  with self.assertRaises(ValueError):transform(text,{'00011000':'0'*64})
  with self.assertRaises(ValueError):transform(text,{'00013000':'0'*64})
 def test_relocated_stack_execution(self):
  import tempfile,subprocess
  body="""X86_FUNC(__imp__sub_00011000) {
 const bool x86_resuming=ctx.resuming;
 const unsigned x86_expected_return=ctx.stack[ctx.sp];
 unsigned saved=ctx.stack[ctx.sp++];
 ctx.sp-=8;ctx.stack[ctx.sp]=saved;
 ctx.return_target=ctx.stack[ctx.sp++];
 if(ctx.alter)ctx.return_target=0x12345678;
 FLAGS();GUARD
}
""".replace('GUARD',GUARD)
  source=body+'X86_WEAK_FUNC(a,b)\n'
  # Include a newline before the weak function as required by generated boundaries.
  source=source.replace('}\nX86_WEAK_FUNC','}\nX86_WEAK_FUNC')
  changed=transform(source,{'00011000':hashlib.sha256(body.rstrip().encode()).hexdigest()})
  prefix="""#include <cassert>
#define X86_FUNC(n) void n(Context& ctx)
#define X86_WEAK_FUNC(a,b)
#define FLAGS() ++ctx.flags
struct X86Transfer {unsigned pc,sp;};
struct Context {unsigned stack[64]{},sp=32,return_target=0,esp=0,flags=0;bool resuming=false,alter=false;};
"""
  main="""int main(){Context c;c.stack[32]=0x11000;__imp__sub_00011000(c);
 assert(c.sp==26&&c.return_target==0x11000&&c.flags==1);
 c={};c.stack[32]=0x11000;c.alter=true;bool threw=false;
 try{__imp__sub_00011000(c);}catch(const X86Transfer&){threw=true;}assert(threw);
 c={};c.stack[32]=0x11000;c.resuming=true;c.alter=true;__imp__sub_00011000(c);
 assert(c.return_target==0x12345678);}
"""
  with tempfile.TemporaryDirectory() as folder:
   p=pathlib.Path(folder);(p/'case.cpp').write_text(prefix+changed+main)
   subprocess.run(['c++','-std=c++20',str(p/'case.cpp'),'-o',str(p/'run')],check=True,capture_output=True)
   subprocess.run([str(p/'run')],check=True)
