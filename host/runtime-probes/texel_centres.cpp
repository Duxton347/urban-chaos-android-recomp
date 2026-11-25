// Authored high-frequency render targets: raw screen-space copies must preserve every texel.
#include "gpu/backend.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include <array>
#include <cstring>
#include <cstdio>
#include <stdexcept>
void RenderTargetTexelCentresSelfTest(){
 auto b=gpu::CreateBackend();if(!b->Init(8,8))throw std::runtime_error("Texel centre backend init");
 auto surface=[](){auto header=mem::AllocSystem(20),pixels=mem::AllocContiguous(512,0,0xffffffff,4096);kernel::W32(header,0x50001);kernel::W32(header+4,mem::PhysicalFromVirtual(pixels));kernel::W32(header+12,(0x12<<8)|0x20);kernel::W32(header+16,7|(7<<12));return gpu::Surface{header,pixels,kernel::R32(header+12),kernel::R32(header+16)};};
 auto first=surface(),second=surface(),third=surface();gpu::State s{};s.rt=first;s.viewport={0,0,8,8,0,1};s.zscale=16777215.f;s.simple[gpu::kMColorWriteEnable>>2]=0x01010101;
 std::array<uint32_t,64> colours;for(int y=0;y<8;y++)for(int x=0;x<8;x++){uint32_t colour=0xff000000u|uint32_t(x*31+17)<<16|uint32_t(y*29+23)<<8|uint32_t(((x+y)&1)?237:13);colours[y*8+x]=colour;int32_t rect[]={x,y,x+1,y+1};b->Clear(s,1,reinterpret_cast<const uint8_t*>(rect),0xf0,colour,1,0);}
 auto data=mem::AllocContiguous(4096,0,0xffffffff,4096),vb=mem::AllocSystem(12);kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(data));s.streams[0]={vb,32};
 s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();s.vs->inputs[0]={0,0,0x42};s.vs->inputs[9]={0,16,0x42};s.vs->program_type=0x2078;
 auto mov=[&](unsigned reg,unsigned output,bool last){s.vs->program.insert(s.vs->program.end(),{0,(1u<<21)|(reg<<9)|0x1Bu,2u<<26,(15u<<12)|(1u<<11)|(output<<3)|unsigned(last)});};mov(0,0,false);mov(9,9,true);
 const float xy[][2]={{0,0},{8,0},{8,8},{0,0},{8,8},{0,8}};for(int i=0;i<6;i++){float v[]={xy[i][0],xy[i][1],0,1,xy[i][0],xy[i][1],0,1};std::memcpy(static_cast<uint8_t*>(kernel::HostPtr(data))+i*32,v,sizeof(v));}
 s.tss[0][gpu::kTssColorOp]=s.tss[0][gpu::kTssAlphaOp]=2;s.tss[0][gpu::kTssColorArg1]=s.tss[0][gpu::kTssAlphaArg1]=2;s.tss[0][gpu::kTssAddressU]=s.tss[0][gpu::kTssAddressV]=3;
 for(unsigned filter:{1u,2u}){s.tss[0][gpu::kTssMinFilter]=s.tss[0][gpu::kTssMagFilter]=filter;s.rt=second;s.textures[0]=first.header;b->Clear(s,0,nullptr,0xf0,0,1,0);b->Draw(s,5,0,6,nullptr);s.rt=third;s.textures[0]=second.header;b->Clear(s,0,nullptr,0xf0,0,1,0);b->Draw(s,5,0,6,nullptr);b->Sync(third.data,512);for(int y=0;y<8;y++)for(int x=0;x<8;x++){auto got=kernel::R32(third.data+y*64+x*4);if(got!=colours[y*8+x])throw std::runtime_error("Render-target texel drift after two raw screen passes: filter="+std::to_string(filter)+" x="+std::to_string(x)+" y="+std::to_string(y));}}
 std::puts("RENDER_TARGET_TEXEL_CENTRES: two raw screen-space passes preserve all authored texels with nearest and linear filtering");
}
