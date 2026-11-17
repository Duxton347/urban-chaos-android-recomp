// Authored surface/texture lifecycle model; tests actual SDK getter bodies and CallGuest.
#include "xdk/hle.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
namespace kernel { uint8_t* g_base=nullptr; }
namespace dispatch { void Init(); }
using namespace kernel;
static constexpr uint32_t kSurfaceType=0x50000, kDeviceRef=0x80000;
static uint32_t active_rt, active_depth, g_buffers[3], g_buffer_count=3;
static constexpr uint32_t kDevRenderTarget=0, kDevDepth=4;
static uint32_t Dev(uint32_t offset) { return offset==kDevRenderTarget ? active_rt:active_depth; }
static std::recursive_mutex g_lock;
static std::set<uint32_t> destroyed;
static unsigned retain_calls;
static void Retain(uint32_t resource) {
    assert(!destroyed.count(resource));
    uint32_t word=R32(resource);
    if(!(word&0xFFFF) && (word&0x70000)==kSurfaceType && R32(resource+20)) Retain(R32(resource+20));
    W32(resource,word+1);
}
static void RetailRetainContract(X86Context& ctx,uint8_t*) {
    uint32_t resource=R32(ctx.esp+4);++retain_calls;Retain(resource);
    Return(ctx,R32(resource)&0xFFFF,1);
}
const X86FuncMapping X86FuncMappings[]={{0x13000,RetailRetainContract},{0,nullptr}};
const uint32_t X86Continuations[]={0};
const uint32_t X86CodeBase=0x13000,X86CodeEnd=0x13001,X86ImageBase=0x10000,X86ImageEnd=0x20000,X86EntryPoint=0x13000,X86KernelThunk=0;
#ifdef LEGACY_GETTERS
#include "legacy_surface_getters.inc"
#else
#include "surface_getters.inc"
#endif
static void CheckDead(uint32_t resource) {
    if(!(R32(resource)&0x78FFFF)) assert(destroyed.insert(resource).second);
}
static void DropPublic(uint32_t resource) {
    assert(!destroyed.count(resource));uint32_t word=R32(resource);assert(word&0xFFFF);
    if((word&0xFFFF)==1 && (word&0x70000)==kSurfaceType && R32(resource+20)) DropPublic(R32(resource+20));
    W32(resource,word-1);CheckDead(resource);
}
static void DropDevice(uint32_t resource) {
    assert(!destroyed.count(resource));uint32_t word=R32(resource);assert(word&0x780000);
    if((word&0x780000)==kDeviceRef && (word&0x70000)==kSurfaceType && R32(resource+20)) DropDevice(R32(resource+20));
    W32(resource,word-kDeviceRef);CheckDead(resource);
}
static uint32_t Invoke(X86Func* getter,uint32_t top,bool with_index) {
    X86Context ctx{};ctx.esp=top-(with_index?8:4);W32(ctx.esp,0xF00D00FF);
    if(with_index)W32(ctx.esp+4,0xFFFFFFFF); // primary back buffer
    ctx.edi=0x12345678;ctx.esi=0x87654321;ctx.ebp=0xABCD;
    getter(ctx,g_base);
    assert(ctx.esp==top && ctx.edi==0x12345678 && ctx.esi==0x87654321 && ctx.ebp==0xABCD);
    return ctx.eax;
}
int main(int argc,char** argv) {
    const bool negative=argc==2 && !std::strcmp(argv[1],"--negative-control");
    if(!mem::Init())return 2;
    g_base=mem::Base();dispatch::Init();uint32_t top=mem::AllocStack(0x10000);
    X86Func* getters[]={GUEST(D3DDevice_GetBackBuffer2),GUEST(D3DDevice_GetRenderTarget2),GUEST(D3DDevice_GetDepthStencilSurface2)};
    for(unsigned choice=0;choice<3;choice++) {
        uint32_t texture=mem::AllocSystem(24),view=mem::AllocSystem(24);
        // The application owns the texture; the device holds the view and its parent internally.
        W32(texture,0x40000|kDeviceRef|1);W32(view,kSurfaceType|kDeviceRef);W32(view+20,texture);
        active_rt=active_depth=view;for(auto& buffer:g_buffers)buffer=view;
        retain_calls=0;
        assert(Invoke(getters[choice],top,choice==0)==view);
        if((R32(texture)&0xFFFF)!=2) {
            std::puts("SURFACE_REFS: getter failed to retain the live view's parent");
            return negative ? 0:42;
        }
        if(negative)return 43;
        assert((R32(view)&0xFFFF)==1 && retain_calls==1);
        assert(Invoke(getters[choice],top,choice==0)==view);
        assert((R32(view)&0xFFFF)==2 && (R32(texture)&0xFFFF)==2 && retain_calls==2);
        DropDevice(view);DropPublic(texture); // replace target, then release application texture owner
        assert(!destroyed.count(view) && !destroyed.count(texture));
        DropPublic(view);assert(!destroyed.count(texture)); // one returned view still alive
        DropPublic(view);assert(destroyed.count(view) && destroyed.count(texture));
    }
    active_rt=active_depth=0;retain_calls=0;
    assert(Invoke(getters[1],top,false)==0 && Invoke(getters[2],top,false)==0 && retain_calls==0);
    std::puts("SURFACE_REFS: three SDK getters, first/repeated public retains, target replacement, balanced parent/view destruction, null results and stack/register preservation passed; authored state only");
    return 0;
}
