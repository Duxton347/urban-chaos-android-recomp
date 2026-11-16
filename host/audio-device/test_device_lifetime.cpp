// Run in a fresh Linux process with the experimental SDK audio patch.
// Requires reviewed image_symbols.h metadata; never executes guest title instructions.
// Call AudioSelfTest() before boot and terminate the process with its result.
#include "xdk/hle.h"
#include "xbe/xbe.h"
#include <cstdio>
namespace kernel {extern const xbe::Image* g_image;}
X86_EXTERN_FUNC(GUEST(DirectSoundCreate));
X86_EXTERN_FUNC(GUEST(IDirectSound_AddRef));
X86_EXTERN_FUNC(GUEST(IDirectSound_Release));
int AudioSelfTest() {
 if(!mem::Init())return 2;
 kernel::g_base=mem::Base();
 static xbe::Image synthetic_image;kernel::g_image=&synthetic_image;
 uint32_t top=mem::AllocStack(0x10000),out=mem::AllocSystem(4);
 X86Context ctx{};
 auto call=[&](X86Func* fn,std::initializer_list<uint32_t> args,uint32_t expected){
  ctx.esp=top-4*(args.size()+1);kernel::W32(ctx.esp,0);int i=0;
  for(auto v:args)kernel::W32(ctx.esp+4+4*i++,v);
  fn(ctx,kernel::g_base);
  if(ctx.esp!=top || ctx.eax!=expected){std::fprintf(stderr,"SELFTEST_FAILURE result=%u expected=%u stack_balanced=%d\n",ctx.eax,expected,ctx.esp==top);return false;}
  return true;
 };
 if(!call(GUEST(DirectSoundCreate),{0,out,0},0))return 3;
 uint32_t obj=kernel::R32(out);
 if(!obj||!mem::AllocationSize(obj))return 4;
 if(!call(GUEST(IDirectSound_AddRef),{obj},2))return 5;
 if(!call(GUEST(IDirectSound_Release),{obj},1))return 6;
 if(!call(GUEST(DirectSoundCreate),{0,out,0},0)||kernel::R32(out)!=obj)return 7;
 if(!call(GUEST(IDirectSound_Release),{obj},1))return 8;
 if(!call(GUEST(IDirectSound_Release),{obj},0)||mem::AllocationSize(obj))return 9;
 if(!call(GUEST(DirectSoundCreate),{0,out,0},0))return 10;
 if(!call(GUEST(IDirectSound_Release),{kernel::R32(out)},0))return 11;
 std::puts("AUDIO_SELFTEST: 8 real HLE calls; reference ownership, repeated creation, disposal, recreation and stdcall stack balance passed; no title instructions executed");
 std::fflush(stdout);return 0;
}
