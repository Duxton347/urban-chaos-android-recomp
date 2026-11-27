// Runs authored synthetic guest instructions through the actual SDK dispatcher and memory services.
#include "kernel/kernel.h"
#include <fstream>
#include <vector>
#include <cstdio>
namespace kernel { uint8_t* g_base=nullptr; }
namespace dispatch { void Init(); uint32_t HostThunk(X86Func*); }
static void Nested(X86Context& ctx, uint8_t*) {
    uint32_t value=kernel::CallGuest(ctx,X86EntryPoint,{});
    kernel::Return(ctx,value,0);
}
static void Load(const char* path,uint32_t address) {
    std::ifstream file(path,std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    std::memcpy(kernel::HostPtr(address),bytes.data(),bytes.size());
}
int main(int argc,char** argv) {
    if((argc!=3 && argc!=4) || !mem::Init())return 2;
    kernel::g_base=mem::Base();
    if(!mem::Commit(0x10000,0x30000))return 3;
    Load(argv[1],0x11000);Load(argv[2],0x18000);
    kernel::W32(0x30000,0xFFFFFFF0);
    X86Context ctx{};ctx.esp=0x1FFF0;ctx.esi=0x1357;
    dispatch::Init();
    uint32_t result=kernel::CallGuest(ctx,argc==4 ? dispatch::HostThunk(Nested):X86EntryPoint,{});
    std::printf("SYNTHETIC_CONTEXT_RESULT=%u ESP=%08X ESI=%08X\n",result,ctx.esp,ctx.esi);
    return result==42 && ctx.esp==0x1FFF0 && ctx.esi==0x1357 ? 0:1;
}
