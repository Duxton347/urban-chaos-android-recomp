// Authored geometry exercises the selected real SDK rasterizer and report delivery.
#include "gpu/backend.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <thread>
static gpu::Surface Surface(uint32_t format) {
    uint32_t header=mem::AllocSystem(20),data=mem::AllocContiguous(64*8,0,0xFFFFFFFF,4096);
    kernel::W32(header,0x50001);kernel::W32(header+4,mem::PhysicalFromVirtual(data));
    kernel::W32(header+12,(format<<8)|0x20);kernel::W32(header+16,7|(7<<12));
    return {.header=header,.data=data,.format=kernel::R32(header+12),.size=kernel::R32(header+16)};
}
int ReferenceVisibilitySelfTest(bool reject_points,bool asynchronous) {
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
            backend->GetReport(report);
            if(!asynchronous)backend->Flush();else {
                auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
                do{backend->FlushVisibilityReports();if(kernel::R32(report+12)==0)break;std::this_thread::yield();}while(std::chrono::steady_clock::now()<end);
            }
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
        if(asynchronous){
            auto saved=state;
            uint32_t tex=mem::AllocSystem(20),pixels=mem::AllocContiguous(64*8,0,0xFFFFFFFF,4096);
            kernel::W32(tex,0x40001);kernel::W32(tex+4,mem::PhysicalFromVirtual(pixels));
            kernel::W32(tex+12,(0x12<<8)|0x20);kernel::W32(tex+16,7|(7<<12));
            state.textures[0]=tex;state.vs_handle=0x144;state.streams[0].stride=28;state.rs[gpu::kRsZEnable]=0;
            state.tss[0][gpu::kTssColorOp]=2;state.tss[0][gpu::kTssColorArg1]=2;
            state.tss[0][gpu::kTssAlphaOp]=2;state.tss[0][gpu::kTssAlphaArg1]=2;
            state.simple[gpu::kMAlphaTestEnable>>2]=1;state.simple[gpu::kMAlphaFunc>>2]=0x204;state.simple[gpu::kMAlphaRef>>2]=128;
            auto textured=[&]{
                const struct Vertex {float x,y,z,w;uint32_t color;float u,v;} data[]={
                    {1,1,.25f,1,0xFFFFFFFF,.5f,.5f},{6,1,.25f,1,0xFFFFFFFF,.5f,.5f},{1,6,.25f,1,0xFFFFFFFF,.5f,.5f}};
                std::memcpy(kernel::HostPtr(vertices),data,sizeof(data));backend->Draw(state,5,0,3,nullptr);
            };
            std::memset(kernel::HostPtr(pixels),0xFF,64*8);gpu::MarkDirty(tex);backend->ClearReport();textured();
            if(result()!=visible)throw std::runtime_error("Opaque authored texture did not pass alpha test");
            std::memset(kernel::HostPtr(pixels),0,64*8);gpu::MarkDirty(tex);backend->ClearReport();textured();
            if(result()!=0)throw std::runtime_error("Texture lock update within one presentation reused stale alpha");
            state=saved;
        }
        if(asynchronous){
            uint32_t batch=mem::AllocSystem(8*20);
            for(unsigned i=0;i<8;i++){
                backend->Clear(state,0,nullptr,3,0,1,0);backend->ClearReport();triangle(.25f);
                kernel::W32(batch+i*20+12,0xFFFFFFFF);kernel::W32(batch+i*20+16,0xC0DEC0DE);
                backend->GetReport(batch+i*20);backend->FlushVisibilityReports();
            }
            auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            for(;;){
                backend->FlushVisibilityReports();bool ready=true;
                for(unsigned i=0;i<8;i++)ready&=kernel::R32(batch+i*20+12)==0;
                if(ready)break;
                if(std::chrono::steady_clock::now()>=end)throw std::runtime_error("Queued visibility reports did not complete");
                std::this_thread::yield();
            }
            for(unsigned i=0;i<8;i++){assert(kernel::R32(batch+i*20+8)==visible);assert(kernel::R32(batch+i*20+16)==0xC0DEC0DE);}
        }
        if(!threaded && reject_points) {
            bool rejected=false;
            try { backend->Draw(state,1,0,3,nullptr); } catch(const std::runtime_error&) { rejected=true; }
            assert(rejected); // unsupported counted topology cannot silently report zero
        }
        std::printf("BACKEND_VISIBILITY: %u passing samples; depth occlusion, nearer geometry, count enable/reset, color-write disabled and 16-byte completion report passed (threaded=%d); authored geometry only\n",visible,threaded);
    }
    return 0;
}

int ReferenceVisibilitySelfTest(bool reject_points){return ReferenceVisibilitySelfTest(reject_points,false);}
