// Authored texels only: channel sign is part of the sampled image, before interpolation.
#include "gpu/backend.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <array>
void SignedTextureSelfTest(){
 auto b=gpu::CreateBackend();if(!b->Init(8,8))throw std::runtime_error("Signed texture backend init");
 auto hdr=mem::AllocSystem(20),pixels=mem::AllocContiguous(512,0,0xffffffff,4096);
 kernel::W32(hdr,0x50001);kernel::W32(hdr+4,mem::PhysicalFromVirtual(pixels));kernel::W32(hdr+12,(0x12<<8)|0x20);kernel::W32(hdr+16,7|(7<<12));
 gpu::State s{};s.rt={hdr,pixels,kernel::R32(hdr+12),kernel::R32(hdr+16)};s.viewport={0,0,8,8,0,1};s.zscale=16777215.f;s.simple[gpu::kMColorWriteEnable>>2]=0x01010101;
 auto data=mem::AllocContiguous(4096,0,0xffffffff,4096),vb=mem::AllocSystem(12);kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(data));s.streams[0]={vb,28};
 s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();s.vs->inputs[0]={0,0,0x42};s.vs->inputs[9]={0,16,0x32};s.vs->program_type=0x2078;
 auto mov=[&](unsigned reg,unsigned output,bool last){s.vs->program.insert(s.vs->program.end(),{0,(1u<<21)|(reg<<9)|0x1Bu,2u<<26,(15u<<12)|(1u<<11)|(output<<3)|unsigned(last)});};mov(0,0,false);mov(9,9,false);mov(9,10,true);
 auto texture=mem::AllocSystem(20),texels=mem::AllocContiguous(4096,0,0xffffffff,4096);kernel::W32(texture,0x40001);kernel::W32(texture+4,mem::PhysicalFromVirtual(texels));kernel::W32(texture+12,(0x12<<8)|0x20);kernel::W32(texture+16,1);s.textures[0]=texture;
 for(int i=0;i<2;i++){s.tss[i][gpu::kTssMinFilter]=s.tss[i][gpu::kTssMagFilter]=2;s.tss[i][gpu::kTssAddressU]=s.tss[i][gpu::kTssAddressV]=3;}
 s.ps=std::make_shared<gpu::PixelShader>();s.ps->def[53]=1;s.ps->def[34]=0xC8200120;s.ps->def[45]=0xC00;s.rs[gpu::kRsPsTextureModes]=1;s.simple[gpu::kMPsConstant0>>2]=0x80808080;
 auto mesh=[&](bool cube){const float xy[3][2]={{1,1},{6,1},{1,6}};for(int i=0;i<3;i++){float v[7]={xy[i][0],xy[i][1],0,1,1,cube?0.f:.5f,0};std::memcpy(static_cast<uint8_t*>(kernel::HostPtr(data))+i*28,v,sizeof(v));}};
 const unsigned char pair[]={32,64,127,64,224,192,128,192};std::memcpy(kernel::HostPtr(texels),pair,8);mesh(false);
 auto draw=[&](uint32_t sign,std::array<int,4> expected){s.tss[0][gpu::kTssColorSign]=sign;b->Clear(s,0,nullptr,0xF0,0xff000000,1,0);b->Draw(s,5,0,3,nullptr);b->Sync(pixels,512);auto* p=static_cast<uint8_t*>(kernel::HostPtr(pixels))+2*64+2*4;for(int c=0;c<4;c++)if(std::abs(int(p[c])-expected[c])>2)throw std::runtime_error("Signed texture/filter mismatch: sign="+std::to_string(sign)+" channel="+std::to_string(c)+" got="+std::to_string(p[c])+" expected="+std::to_string(expected[c]));};
 draw(0xe0000000,{128,128,128,128}); // +127/-128 interpolate to zero, not a post-filter sign conversion.
 draw(0,{255,255,255,128});draw(0xe0000000,{128,128,128,128});
 draw(0x20000000,{255,255,128,128});draw(0x10000000,{255,255,255,0});
 // Same address, different sign states on two stages in a single draw must remain independent.
 s.textures[1]=texture;s.tss[0][gpu::kTssColorSign]=0;s.tss[1][gpu::kTssColorSign]=0xe0000000;s.rs[gpu::kRsPsTextureModes]=33;s.ps->def[34]=0x0801C901;s.simple[gpu::kMPsConstant0>>2]=0x40404040;draw(0,{32,32,32,128});
 s.textures[1]=0;s.rs[gpu::kRsPsTextureModes]=1;s.ps->def[34]=0xC8200120;s.simple[gpu::kMPsConstant0>>2]=0x80808080;
 // A signed cube has six independent faces and preserves unsigned alpha; mip one is also converted.
 kernel::W32(texture+12,(6<<8)|0x00010024|(1<<20)|(1<<24));kernel::W32(texture+16,0);mesh(true);
 for(int face=0;face<6;face++)for(int y=0;y<2;y++)std::memcpy(static_cast<uint8_t*>(kernel::HostPtr(texels))+face*128+y*8,pair,8);gpu::MarkDirty(texture);draw(0xe0000000,{128,128,128,128});
 kernel::W32(texture+12,(6<<8)|0x00020024|(1<<20)|(1<<24));for(int face=0;face<6;face++){unsigned char level[]={0,0,0,160};std::memcpy(static_cast<uint8_t*>(kernel::HostPtr(texels))+face*128+16,level,4);}gpu::MarkDirty(texture);s.tss[0][gpu::kTssMipFilter]=1;s.tss[0][gpu::kTssMaxMipLevel]=1;draw(0xe0000000,{128,128,128,160});
 // Invalidation must refresh both signed and unsigned variants, including within one Present.
 s.tss[0][gpu::kTssMipFilter]=0;s.tss[0][gpu::kTssMaxMipLevel]=0;kernel::W32(texture+12,(0x12<<8)|0x20);kernel::W32(texture+16,1);mesh(false);unsigned char changed[]={64,64,64,128,64,64,64,128};std::memcpy(kernel::HostPtr(texels),changed,8);gpu::MarkDirty(texture);draw(0xe0000000,{255,255,255,128});draw(0,{192,192,192,128});
 std::puts("SIGNED_TEXTURE: filtering before sign, channel masks, unsigned alpha, per-stage cache, cube/mips and dirty variants passed");
}
