// Synthetic runtime/API tests. Never execute guest title instructions.
#include "xdk/hle.h"
#include "xdk/layout.h"
#include "xbe/xbe.h"
#include "gpu/backend.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
namespace kernel { extern const xbe::Image* g_image; }
X86_EXTERN_FUNC(GUEST(Direct3D_CreateDevice));
X86_EXTERN_FUNC(GUEST(D3DDevice_SetViewport));
X86_EXTERN_FUNC(GUEST(D3DDevice_IsBusy));

int ViewportSelfTest() {
    if (!mem::Init()) return 2;
    kernel::g_base = mem::Base();
    static xbe::Image empty_image;
    kernel::g_image = &empty_image;
    // Commit synthetic storage covering the reviewed metadata addresses, without loading a title.
    uint32_t addresses[] = {SYM(D3D_Device), SYM(D3D_pDevice), SYM(D3D_RenderState),
        SYM(D3D_IndexData), SYM(D3D_DirtyFlags), SYM(D3D_Streams), SYM(D3D_FvfShader)};
    uint32_t low = *std::min_element(std::begin(addresses), std::end(addresses)) & ~0xFFFu;
    uint32_t high = *std::max_element(std::begin(addresses), std::end(addresses)) + XDK_CDevice_Size + 0x1000;
    if (!mem::Commit(low, high-low)) return 3;
    uint32_t top = mem::AllocStack(0x10000), viewport = mem::AllocSystem(24);
    X86Context ctx{};
    auto call = [&](X86Func* fn, std::initializer_list<uint32_t> args) {
        ctx.esp=top-4*(args.size()+1); kernel::W32(ctx.esp, 0); int i=0;
        for (auto v:args) kernel::W32(ctx.esp+4+4*i++, v);
        fn(ctx, kernel::g_base);
        return ctx.esp==top && ctx.eax==0;
    };
    float min_z=0.25f, max_z=0.75f;
    kernel::W32(viewport,7); kernel::W32(viewport+4,9); kernel::W32(viewport+8,111); kernel::W32(viewport+12,77);
    std::memcpy(kernel::HostPtr(viewport+16), &min_z, 4); std::memcpy(kernel::HostPtr(viewport+20), &max_z, 4);
    if (!call(GUEST(D3DDevice_SetViewport),{viewport})) return 6;
    uint32_t mirror=SYM(D3D_Device)+XDK_CDevice_Viewport;
    if (std::memcmp(kernel::HostPtr(mirror),kernel::HostPtr(viewport),24)) return 7;
    if (!call(GUEST(D3DDevice_SetViewport),{0})) return 8;
    if (std::memcmp(kernel::HostPtr(mirror),kernel::HostPtr(viewport),24)) return 9;
    if (!call(GUEST(D3DDevice_IsBusy),{})) return 10; // synchronous research backend has no queued work
    std::puts("VIEWPORT_SELFTEST: custom viewport/depth, null reapplication, guest mirror and stdcall balance passed; no title instructions");
    std::fflush(stdout);
    return 0;
}

X86_EXTERN_FUNC(GUEST(DirectSoundCreateBuffer));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_GetVoiceProperties));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetPitch));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetVolume));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetHeadroom));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetFrequency));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetPosition));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_Release));
X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetMode));
X86_EXTERN_FUNC(GUEST(IDirectSound_CommitDeferredSettings));
X86_EXTERN_FUNC(GUEST(DirectSoundCreate));
X86_EXTERN_FUNC(GUEST(IDirectSound_Release));
X86_EXTERN_FUNC(GUEST(IDirectSound_CreateSoundStream));
X86_EXTERN_FUNC(GUEST(CDirectSoundStream_AddRef));
X86_EXTERN_FUNC(GUEST(CDirectSoundStream_Release));
X86_EXTERN_FUNC(GUEST(CDirectSoundStream_SetOutputBuffer));
X86_EXTERN_FUNC(GUEST(CDirectSoundStream_SetPitch));
int VoicePropertiesSelfTest() {
    if(!mem::Init()) return 2;
    kernel::g_base=mem::Base();static xbe::Image empty;kernel::g_image=&empty;
    uint32_t top=mem::AllocStack(0x10000), block=mem::AllocSystem(256);
    uint32_t format=block, desc=block+24, bins=block+48, pairs=block+56, result=block+80, out=block+88;
    kernel::W16(format,1);kernel::W16(format+2,1);kernel::W32(format+4,24000);
    kernel::W32(format+8,48000);kernel::W16(format+12,2);kernel::W16(format+14,16);
    kernel::W32(desc,20);kernel::W32(desc+4,0x10);kernel::W32(desc+12,format);kernel::W32(desc+16,bins);
    kernel::W32(bins,2);kernel::W32(bins+4,pairs);
    kernel::W32(pairs,0);kernel::W32(pairs+4,uint32_t(-100));
    kernel::W32(pairs+8,10);kernel::W32(pairs+12,uint32_t(-200));
    X86Context ctx{};
    auto call=[&](X86Func* fn,std::initializer_list<uint32_t> args,uint32_t expected=0){
        ctx.esp=top-4*(args.size()+1);kernel::W32(ctx.esp,0);int i=0;
        for(auto value:args)kernel::W32(ctx.esp+4+4*i++,value);
        fn(ctx,kernel::g_base);return ctx.esp==top && ctx.eax==expected;
    };
    if(!call(GUEST(DirectSoundCreateBuffer),{desc,result})) return 3;
    uint32_t voice=kernel::R32(result);
    if(!call(GUEST(IDirectSoundBuffer_SetVolume),{voice,uint32_t(-300)}) ||
       !call(GUEST(IDirectSoundBuffer_SetHeadroom),{voice,600})) return 4;
    kernel::W32(out+92,0xC0DEC0DE);
    if(!call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out})) return 5;
    if(kernel::R32(out)!=2 || kernel::R32(out+4)!=0 || int32_t(kernel::R32(out+8))!=-1000 ||
       kernel::R32(out+12)!=10 || int32_t(kernel::R32(out+16))!=-1100 ||
       int32_t(kernel::R32(out+0x44))!=-4096 || kernel::R32(out+92)!=0xC0DEC0DE) return 6;
    if(!call(GUEST(IDirectSoundBuffer_SetFrequency),{voice,48000}) ||
       !call(GUEST(IDirectSoundBuffer_SetVolume),{voice,uint32_t(-400)}) ||
       !call(GUEST(IDirectSoundBuffer_SetPosition),{voice,0x40400000,0x40800000,0,0}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out})) return 7;
    if(kernel::R32(out+0x44)!=0 || int32_t(kernel::R32(out+8))!=-1100 ||
       int32_t(kernel::R32(out+0x48))!=-1398) return 8;
    if(!call(GUEST(IDirectSoundBuffer_SetPitch),{voice,4096}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out}) ||
       int32_t(kernel::R32(out+0x44))!=4096) return 10;
    if(!call(GUEST(IDirectSoundBuffer_SetPitch),{voice,8192},0x80070057u) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out}) ||
       int32_t(kernel::R32(out+0x44))!=4096) return 11;
    if(!call(GUEST(IDirectSoundBuffer_SetMode),{voice,2,0}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out}) || kernel::R32(out+0x48)!=0 ||
       !call(GUEST(IDirectSoundBuffer_SetMode),{voice,3,0},0x80070057u) ||
       !call(GUEST(IDirectSoundBuffer_SetMode),{voice,0,2},0x80070057u) ||
       !call(GUEST(IDirectSoundBuffer_SetMode),{voice,0,1}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out}) || kernel::R32(out+0x48)!=0 ||
       !call(GUEST(IDirectSound_CommitDeferredSettings),{0}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,out}) || int32_t(kernel::R32(out+0x48))!=-1398) return 12;
    if(!call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{voice,0},0x80070057u) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{0,out},0x80004005u) ||
       !call(GUEST(IDirectSoundBuffer_Release),{voice})) return 9;
    uint32_t device_out=block+184, stream_desc=block+192, stream_out=block+220;
    if(!call(GUEST(DirectSoundCreate),{0,device_out,0})) return 13;
    uint32_t device=kernel::R32(device_out);
    kernel::W32(stream_desc,0);kernel::W32(stream_desc+4,2);kernel::W32(stream_desc+8,format);
    if(!call(GUEST(IDirectSound_CreateSoundStream),{device,0,stream_out,0},0x80070057u) ||
       !call(GUEST(IDirectSound_CreateSoundStream),{0,stream_desc,stream_out,0},0x80070057u) ||
       !call(GUEST(IDirectSound_CreateSoundStream),{device,stream_desc,stream_out,0})) return 14;
    uint32_t stream=kernel::R32(stream_out);
    if(!stream || !mem::AllocationSize(stream) || !kernel::R32(stream) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{stream,out}) ||
       int32_t(kernel::R32(out+0x44))!=-4096 || kernel::R32(out+0x48)!=0 ||
       !call(GUEST(CDirectSoundStream_SetPitch),{stream,4096}) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{stream,out}) || int32_t(kernel::R32(out+0x44))!=4096 ||
       !call(GUEST(CDirectSoundStream_SetPitch),{stream,8192},0x80070057u) ||
       !call(GUEST(IDirectSoundBuffer_GetVoiceProperties),{stream,out}) || int32_t(kernel::R32(out+0x44))!=4096 ||
       !call(GUEST(CDirectSoundStream_SetOutputBuffer),{stream,0}) ||
       !call(GUEST(CDirectSoundStream_SetOutputBuffer),{stream,voice},0x80004001u) ||
       !call(GUEST(CDirectSoundStream_AddRef),{stream},2) ||
       !call(GUEST(CDirectSoundStream_Release),{stream},1) ||
       !call(GUEST(CDirectSoundStream_Release),{stream},0) || mem::AllocationSize(stream) ||
       !call(GUEST(IDirectSound_Release),{device})) return 15;
    std::puts("VOICE_PROPERTIES_SELFTEST: live bins/gains/headroom/frequency/distance/pitch, bounded 92-byte layout, invalid arguments, lifetime and stdcall passed; no title instructions");
    std::fflush(stdout);return 0;
}

// Exercise the real APC queue and kernel export with authored callbacks, without a title.
namespace dispatch { uint32_t HostThunk(X86Func*); uint32_t KernelThunkAddress(uint32_t); }
#include <array>
#include <vector>
static std::vector<std::array<uint32_t,3>> io_completions;
static void SyntheticIoComplete(X86Context& ctx, uint8_t*) {
    kernel::Args args{ctx};
    io_completions.push_back({args[0],args[1],args[2]});
    kernel::Return(ctx,0,3);
}
int IoApcSelfTest() {
    if(!mem::Init()) return 2;
    kernel::g_base=mem::Base();
    X86Context ctx{};ctx.esp=mem::AllocStack(0x10000);ctx.esi=0x12345678;
    uint32_t stack=ctx.esp, blocks=mem::AllocSystem(24);
    kernel::Thread thread;thread.ctx=&ctx;
    uint32_t callback=dispatch::HostThunk(SyntheticIoComplete);
    uint32_t adapter=dispatch::KernelThunkAddress(232);
    const uint32_t status[]={kernel::kSuccess,kernel::kEndOfFile,kernel::kAccessDenied};
    const uint32_t expected[]={0,38,5};
    for(unsigned i=0;i<3;i++) {
        uint32_t block=blocks+8*i;
        kernel::W32(block,status[i]);kernel::W32(block+4,i?0:4096);
        kernel::QueueApc(thread,adapter,callback,block,0);
    }
    if(!io_completions.empty())return 3;
    if(!kernel::RunApcs(thread)||io_completions.size()!=3||kernel::RunApcs(thread))return 4;
    for(unsigned i=0;i<3;i++) {
        std::array<uint32_t,3> wanted={expected[i],i?0u:4096u,blocks+8*i};
        if(io_completions[i]!=wanted)return 5;
        if(kernel::R32(blocks+8*i)!=status[i])return 6;
    }
    if(ctx.esp!=stack||ctx.esi!=0x12345678)return 7;
    std::puts("IO_APC_SELFTEST: real queue, deferred exactly-once delivery, success/EOF/access errors, byte count, IOSB identity and nested callback stack passed; authored callbacks only");
    return 0;
}
