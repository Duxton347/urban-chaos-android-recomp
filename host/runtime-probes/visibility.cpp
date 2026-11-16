// Authored geometry exercises the real SDK software rasterizer and report delivery.
#include "gpu/backend.h"
#include "kernel/kernel.h"
#include "reference_renderer.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
static gpu::Surface Surface(uint32_t format) {
    uint32_t header=mem::AllocSystem(20),data=mem::AllocContiguous(64*8,0,0xFFFFFFFF,4096);
    kernel::W32(header,0x50001);kernel::W32(header+4,mem::PhysicalFromVirtual(data));
    kernel::W32(header+12,(format<<8)|0x20);kernel::W32(header+16,7|(7<<12));
    return {.header=header,.data=data,.format=kernel::R32(header+12),.size=kernel::R32(header+16)};
}
int ReferenceVisibilitySelfTest() {
    if(!mem::Init())return 2;
    kernel::g_base=mem::Base();
    for(bool threaded:{false,true}) {
        auto backend=gpu::CreateBackend();
        if(threaded)backend=gpu::CreateThreadedBackend(std::move(backend));
        assert(backend->Init(8,8));assert(backend->SupportsVisibilityReports());
        gpu::State state{};state.rt=Surface(0x12);state.zs=Surface(0x2E);
        state.viewport={0,0,8,8,0,1};state.zscale=16777215.f;state.vs_handle=0x44;
        state.rs[gpu::kRsZEnable]=1;state.simple[gpu::kMZFunc>>2]=0x201;
        state.simple[gpu::kMZWriteEnable>>2]=1;state.simple[gpu::kMCountEnable>>2]=1;
        uint32_t vertices=mem::AllocContiguous(3*20,0,0xFFFFFFFF,4096),vb=mem::AllocSystem(12);
        kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(vertices));state.streams[0]={vb,20};
        auto triangle=[&](float depth) {
            const struct Vertex {float x,y,z,w;uint32_t color;} data[]={
                {1,1,depth,1,0xFFFFFFFF},{6,1,depth,1,0xFFFFFFFF},{1,6,depth,1,0xFFFFFFFF}};
            std::memcpy(kernel::HostPtr(vertices),data,sizeof(data));
            backend->Draw(state,5,0,3,nullptr);
        };
        uint32_t report=mem::AllocSystem(20);
        auto result=[&] {
            kernel::W32(report+12,0xFFFFFFFF);kernel::W32(report+16,0xC0DEC0DE);
            backend->GetReport(report);backend->Flush();
            assert(kernel::R32(report+12)==0&&kernel::R32(report+16)==0xC0DEC0DE);
            return kernel::R32(report+8);
        };
        backend->Clear(state,0,nullptr,1,0,1,0);backend->ClearReport();triangle(.25f);
        uint32_t visible=result();assert(visible>0&&visible<64);
        backend->Clear(state,0,nullptr,1,0,1,0);backend->ClearReport();
        state.simple[gpu::kMColorWriteEnable>>2]=0x01010101;
        triangle(.25f);assert(result()==visible); // full color shading agrees with count-only path
        state.simple[gpu::kMColorWriteEnable>>2]=0;
        triangle(.25f);assert(result()==visible); // equal depth fails LESS; no synthetic positive count
        backend->ClearReport();triangle(.5f);assert(result()==0); // wholly occluded
        triangle(.125f);assert(result()==visible); // nearer geometry contributes real passing samples
        state.simple[gpu::kMCountEnable>>2]=0;triangle(.05f);assert(result()==visible);
        backend->ClearReport();state.simple[gpu::kMCountEnable>>2]=1;
        state.simple[gpu::kMAlphaTestEnable>>2]=1;state.simple[gpu::kMAlphaFunc>>2]=0x200;
        triangle(.025f);assert(result()==0); // alpha rejection is not a visible sample
        state.simple[gpu::kMAlphaTestEnable>>2]=0;
        triangle(.025f);assert(result()==visible);
        backend->Clear(state,0,nullptr,1,0,1,0);backend->ClearReport();
        state.ps=std::make_shared<gpu::PixelShader>();state.rs[gpu::kRsPsTextureModes]=5;
        state.ps->def[42]=15;
        triangle(.25f);assert(result()==0); // programmable texture clip must not be skipped
        state.ps.reset();state.rs[gpu::kRsPsTextureModes]=0;state.simple[gpu::kMPsCompareMode>>2]=0;
        triangle(.25f);assert(result()==visible);
        backend->ClearReport();assert(result()==0);
        if(!threaded) {
            bool rejected=false;
            try { backend->Draw(state,1,0,3,nullptr); } catch(const std::runtime_error&) { rejected=true; }
            assert(rejected); // unsupported counted topology cannot silently report zero
        }
        std::printf("REFERENCE_VISIBILITY: %u passing samples; depth occlusion, nearer geometry, count enable/reset, color-write disabled and 16-byte completion report passed (threaded=%d); authored geometry only\n",visible,threaded);
    }
    return 0;
}
