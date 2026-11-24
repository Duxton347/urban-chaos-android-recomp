// Authored NV2A MOV instructions and colors, no original title data.
#include "gpu/backend.h"
#include "kernel/kernel.h"
#include <cstring>
#include <stdexcept>
#include <cstdio>
static gpu::Surface Target(){
 auto h=mem::AllocSystem(20),d=mem::AllocContiguous(64*8,0,0xFFFFFFFF,4096);
 kernel::W32(h,0x50001);kernel::W32(h+4,mem::PhysicalFromVirtual(d));kernel::W32(h+12,(0x12<<8)|0x20);kernel::W32(h+16,7|(7<<12));
 return {.header=h,.data=d,.format=kernel::R32(h+12),.size=kernel::R32(h+16)};
}
void ProgrammableFogSelfTest(){
 auto b=gpu::CreateBackend();if(!b->Init(8,8))throw std::runtime_error("Fog backend init");
 gpu::State s{};s.rt=Target();s.viewport={0,0,8,8,0,1};s.zscale=16777215.f;
 s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();s.vs->program_type=0x2078;
 s.vs->inputs[0]={0,0,0x42};s.vs->stream_stride[0]=16;
 auto mov=[&](unsigned mux,unsigned reg,unsigned output,unsigned mask,bool last){
  // MOV A, xyzw, output register/mask, no temporary register write.
  auto& p=s.vs->program;p.insert(p.end(),{0,(1u<<21)|(reg<<(mux==2?9:13))|0x1Bu,mux<<26,(mask<<12)|(1u<<11)|(output<<3)|unsigned(last)});
 };
 mov(2,0,0,15,false);mov(3,0,3,15,false);mov(3,1,5,8,true);
 s.vsc[0]={1,0,0,1};s.vsc[1]={.25f,0,0,0};
 auto data=mem::AllocContiguous(4096,0,0xFFFFFFFF,4096),vb=mem::AllocSystem(12);
 kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(data));s.streams[0]={vb,16};
 const float vertices[]={1,1,0,1,6,1,0,1,1,6,0,1};std::memcpy(kernel::HostPtr(data),vertices,sizeof(vertices));
 s.simple[gpu::kMColorWriteEnable>>2]=0x01010101;s.rs[gpu::kRsFogEnable]=1;s.rs[gpu::kRsFogTableMode]=0;s.rs[gpu::kRsFogColor]=0xFF0000FF;
 b->Clear(s,0,nullptr,0xF0,0xFF000000,1,0);b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,64*8);
 auto* px=static_cast<unsigned char*>(kernel::HostPtr(s.rt.data))+2*64+2*4;
 std::printf("PROGRAM_FOG_PIXEL: B=%u G=%u R=%u A=%u\n",px[0],px[1],px[2],px[3]);
 if(px[0]<185||px[0]>195||px[1]>2||px[2]<60||px[2]>68)throw std::runtime_error("Programmable oFog factor ignored in table-none mode");
 // Make back-face diffuse green, front diffuse red. Facing must obey FRONTFACE even without culling.
 s.vs->program.back()&=~1u;mov(3,2,7,15,true);s.vs->hash=0;s.vsc[2]={0,1,0,1};
 s.rs[gpu::kRsFogEnable]=0;s.rs[gpu::kRsTwoSidedLighting]=1;s.rs[gpu::kRsCullMode]=0;
 for(unsigned face:{0x900u,0x901u}){
  s.rs[gpu::kRsFrontFace]=face;
  b->Clear(s,0,nullptr,0xF0,0xFF000000,1,0);b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,64*8);
  std::printf("FACING_PIXEL: face=%X B=%u G=%u R=%u\n",face,px[0],px[1],px[2]);
  bool ok=face==0x900?(px[2]>250&&px[1]<2):(px[1]>250&&px[2]<2);
  if(!ok)throw std::runtime_error("Two-sided colors ignore independent Xbox front-face winding");
 }

}
