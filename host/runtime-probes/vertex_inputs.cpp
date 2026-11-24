// Authored input payloads only. Compare native fetch against the portable conversion on the same GPU.
#include "gpu/backend.h"
#include "gpu/vertex.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include "diagnostics/progress.h"
#include <array>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <algorithm>

void NativeVertexInputsSelfTest() {
 auto b=gpu::CreateBackend();if(!b->Init(8,8))throw std::runtime_error("Vertex input backend init");
 auto header=mem::AllocSystem(20),pixels=mem::AllocContiguous(512,0,0xFFFFFFFF,4096);
 kernel::W32(header,0x50001);kernel::W32(header+4,mem::PhysicalFromVirtual(pixels));
 kernel::W32(header+12,(0x12<<8)|0x20);kernel::W32(header+16,7|(7<<12));
 gpu::State s{};s.rt={header,pixels,kernel::R32(header+12),kernel::R32(header+16)};s.viewport={0,0,8,8,0,1};s.zscale=16777215.f;
 s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();s.vs->program_type=0x2078;
 auto mov=[&](unsigned reg,unsigned output,bool last){s.vs->program.insert(s.vs->program.end(),{0,(1u<<21)|(reg<<9)|0x1Bu,2u<<26,(15u<<12)|(1u<<11)|(output<<3)|unsigned(last)});};
 mov(0,0,false);mov(3,3,true);
 auto data=mem::AllocContiguous(4096,0,0xFFFFFFFF,4096),vb=mem::AllocSystem(12);
 kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(data));s.simple[gpu::kMColorWriteEnable>>2]=0x01010101;
 auto native_before=urban::progress::native_vertex_draws.load();unsigned cases=0;
 for(unsigned type:{0x12,0x22,0x32,0x42,0x72,0x40,0x11,0x21,0x31,0x41,0x14,0x24,0x34,0x44,0x15,0x25,0x35,0x45,0x16}) {
  std::array<unsigned char,16> color{};
  auto kind=type&15;
  if(kind==2){const float f[]={.25f,.5f,.75f,1};std::memcpy(color.data(),f,16);if(type==0x72){float one=1;std::memcpy(color.data()+8,&one,4);}}
  else if(kind==0||kind==4){color={51,102,153,204};}
  else if(kind==1){const int16_t n[]={-8192,16384,24576,32767};std::memcpy(color.data(),n,8);}
  else if(kind==5){const int16_t n[]={-2,0,1,1};std::memcpy(color.data(),n,8);}
  else {uint32_t packed=uint32_t(1536)|(uint32_t(511)<<11)|(uint32_t(127)<<22);std::memcpy(color.data(),&packed,4);}
  std::array<unsigned char,512> native{};
  for(unsigned shifted:{0u,1u}) {
   s.vs->inputs[0]={0,shifted,0x42};s.vs->inputs[3]={0,16+shifted,type};s.vs->hash=0;s.vs->stream_stride[0]=36;s.streams[0]={vb,36};
   const float positions[][4]={{1,1,0,1},{6,1,0,1},{1,6,0,1}};
   for(unsigned v=0;v<3;v++){auto* dst=static_cast<unsigned char*>(kernel::HostPtr(data))+36*v;std::memcpy(dst+shifted,positions[v],16);std::memcpy(dst+16+shifted,color.data(),16);}
   gpu::Assembled assembled;if(!gpu::Assemble(s,5,0,3,nullptr,assembled))throw std::runtime_error("Authored vertex assembly failed");
   if(shifted&&assembled.vertices.empty())throw std::runtime_error("Unaligned layout bypassed portable conversion");
   b->Clear(s,0,nullptr,0xF0,0xFF000000,1,0);b->Draw(s,5,0,3,nullptr);b->Sync(pixels,512);
   auto* result=static_cast<unsigned char*>(kernel::HostPtr(pixels));
   if(!shifted)std::memcpy(native.data(),result,512);
   else for(unsigned y=0;y<8;y++)for(unsigned x=0;x<32;x++)
    if(std::abs(int(native[y*64+x])-int(result[y*64+x]))>1)throw std::runtime_error("Native/portable vertex pixels differ for type "+std::to_string(type));
  }
  ++cases;
 }
 // Packed Xbox streams need natural component alignment, not D3D12's dword stride/offset restriction.
 auto packed_data=mem::AllocContiguous(4096,0,0xFFFFFFFF,4096),packed_vb=mem::AllocSystem(12);
 kernel::W32(packed_vb,1);kernel::W32(packed_vb+4,mem::PhysicalFromVirtual(packed_data));
 for(unsigned type:{0x31u,0x35u}) {
  std::array<unsigned char,512> native{};
  for(unsigned shifted:{0u,1u}) {
   s.vs->inputs[0]={1,shifted,0x42};s.vs->inputs[3]={0,0,type};s.vs->inputs[4]={0,6,0x44};s.vs->hash=0;
   s.streams[0]={packed_vb,10};s.streams[1]={vb,20};
   const float positions[][4]={{1,1,0,1},{6,1,0,1},{1,6,0,1}};
   const int16_t normal[]={-8192,16384,24576};const int16_t integers[]={-2,0,1};const unsigned char extra_bytes[]={51,102,153,204};
   for(unsigned v=0;v<3;v++){
    auto* dst=static_cast<unsigned char*>(kernel::HostPtr(packed_data))+10*v;
    std::memcpy(dst,type==0x31?normal:integers,6);std::memcpy(dst+6,extra_bytes,4);
    std::memcpy(static_cast<unsigned char*>(kernel::HostPtr(data))+20*v+shifted,positions[v],16);
   }
   gpu::Assembled a;if(!gpu::Assemble(s,5,0,3,nullptr,a))throw std::runtime_error("Compact stream assembly failed");
   if(!shifted&&!a.vertices.empty())throw std::runtime_error("Compact stream still expanded because of dword alignment");
   if(shifted&&a.vertices.empty())throw std::runtime_error("Misaligned FLOAT4 bypassed portable conversion");
   b->Clear(s,0,nullptr,0xF0,0xFF000000,1,0);b->Draw(s,5,0,3,nullptr);b->Sync(pixels,512);
   auto* result=static_cast<unsigned char*>(kernel::HostPtr(pixels));
   if(!shifted)std::memcpy(native.data(),result,512);
   else for(unsigned y=0;y<8;y++)for(unsigned x=0;x<32;x++)if(std::abs(int(native[y*64+x])-int(result[y*64+x]))>1)throw std::runtime_error("Compact native stream differs from portable conversion");
  }
 }
 if(urban::progress::native_vertex_draws.load()==native_before)throw std::runtime_error("Native vertex fixture did not use native fetch");
 std::printf("NATIVE_VERTEX_INPUTS: %u formats and 2 compact stride-10 layouts agree with portable conversion (pixel tolerance 1/255), misaligned FLOAT4 layouts fall back; authored data only\n",cases);
}

// Xbox permits a linear 32-bit colour view over D24S8 storage. GPU writes must remain visible through that view.
void DepthColourAliasSelfTest() {
 auto b=gpu::CreateBackend();if(!b->Init(8,8))throw std::runtime_error("Depth alias backend init");
 auto surface=[](unsigned format){auto h=mem::AllocSystem(20),d=mem::AllocContiguous(512,0,0xFFFFFFFF,4096);kernel::W32(h,0x50001);kernel::W32(h+4,mem::PhysicalFromVirtual(d));kernel::W32(h+12,(format<<8)|0x20);kernel::W32(h+16,7|(7<<12));return gpu::Surface{h,d,kernel::R32(h+12),kernel::R32(h+16)};};
 gpu::State s{};s.rt=surface(0x12);s.zs=surface(0x2E);s.viewport={0,0,8,8,0,1};s.zscale=16777215.f;
 std::memset(kernel::HostPtr(s.zs.data),0xA5,512); // stale CPU storage must never become the GPU result
 auto texture=mem::AllocSystem(20);kernel::W32(texture,0x40001);kernel::W32(texture+4,mem::PhysicalFromVirtual(s.zs.data));kernel::W32(texture+12,(0x40<<8)|0x20);kernel::W32(texture+16,7|(7<<12));s.textures[0]=texture;
 s.tss[0][gpu::kTssColorOp]=2;s.tss[0][gpu::kTssColorArg1]=2;s.tss[0][gpu::kTssAlphaOp]=2;s.tss[0][gpu::kTssAlphaArg1]=2;s.tss[0][gpu::kTssMagFilter]=s.tss[0][gpu::kTssMinFilter]=1;
 s.vs_handle=0x104;s.simple[gpu::kMColorWriteEnable>>2]=0x01010101;
 auto vertices=mem::AllocContiguous(4096,0,0xFFFFFFFF,4096),vb=mem::AllocSystem(12);kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(vertices));s.streams[0]={vb,24};
 const float triangle[]={1,1,0,1,2.5f,2.5f,6,1,0,1,2.5f,2.5f,1,6,0,1,2.5f,2.5f};std::memcpy(kernel::HostPtr(vertices),triangle,sizeof(triangle));
 for(unsigned format:{0x12u,0x1Eu,0x3Fu,0x40u,0x41u}) {
 kernel::W32(texture+12,(format<<8)|0x20);
 for(auto [depth,stencil]:{std::pair{0x123456u,0xABu},std::pair{0x654321u,0x37u},std::pair{0xFFFFFFu,0xFFu},std::pair{0u,0u}}) {
  b->Clear(s,0,nullptr,3,0,float(depth)/16777215.f,stencil);b->Clear(s,0,nullptr,0xF0,0xFF000000,1,0);
  const auto before=urban::progress::depth_colour_conversions.load();
  b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
  b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
  if(urban::progress::depth_colour_conversions.load()!=before+1)throw std::runtime_error("Unchanged depth alias was reconverted or not converted");
  auto* px=static_cast<unsigned char*>(kernel::HostPtr(s.rt.data))+2*64+2*4;
  const unsigned lo=depth&255,mid=(depth>>8)&255,hi=depth>>16;
  std::array<unsigned,4> expected={hi,mid,lo,stencil};
  if(format==0x12||format==0x1E)expected={stencil,lo,mid,format==0x1E?255u:hi};
  if(format==0x3F)expected={mid,lo,stencil,hi};
  if(format==0x41)expected={lo,mid,hi,stencil};
  std::printf("DEPTH_ALIAS_PIXEL: format=%02X z=%06X stencil=%02X BGRA=%u,%u,%u,%u\n",format,depth,stencil,px[0],px[1],px[2],px[3]);std::fflush(stdout);
  for(unsigned c=0;c<4;c++)if(px[c]!=expected[c])throw std::runtime_error("D24S8 colour alias fetched stale storage or wrong channel packing");
  if(*static_cast<unsigned char*>(kernel::HostPtr(s.zs.data))!=0xA5)throw std::runtime_error("Depth alias unexpectedly required CPU readback");
 } }
 // CPU writes explicitly take ownership back from the cached GPU depth image.
 const unsigned char cpu_colour[]={40,50,60,70};
 for(unsigned offset=0;offset<512;offset+=4)std::memcpy(static_cast<unsigned char*>(kernel::HostPtr(s.zs.data))+offset,cpu_colour,4);
 kernel::W32(texture+12,(0x40<<8)|0x20);gpu::MarkDirty(texture);
 b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
 auto* cpu_pixel=static_cast<unsigned char*>(kernel::HostPtr(s.rt.data))+2*64+2*4;
 if(cpu_pixel[0]!=70||cpu_pixel[1]!=60||cpu_pixel[2]!=50||cpu_pixel[3]!=40)throw std::runtime_error("Stale GPU depth overrode a CPU colour write");
 // A GPU mesh write must invalidate a previously packed alias, just like Clear does.
 b->Clear(s,0,nullptr,3,0,1,0x5A);
 auto depth_write=s;depth_write.rs[gpu::kRsZEnable]=1;depth_write.simple[gpu::kMZWriteEnable>>2]=1;depth_write.simple[gpu::kMZFunc>>2]=0x207;depth_write.simple[gpu::kMColorWriteEnable>>2]=0;depth_write.textures.fill(0);
 float depth_vertices[18];std::memcpy(depth_vertices,triangle,sizeof(depth_vertices));for(unsigned v=0;v<3;v++)depth_vertices[v*6+2]=.25f;
 std::memcpy(kernel::HostPtr(vertices),depth_vertices,sizeof(depth_vertices));b->Draw(depth_write,5,0,3,nullptr);
 std::memcpy(kernel::HostPtr(vertices),triangle,sizeof(triangle));b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
 if(cpu_pixel[0]!=64||cpu_pixel[1]||cpu_pixel[2]||cpu_pixel[3]!=0x5A)throw std::runtime_error("GPU mesh depth write reused an old packed alias");
 // Merely allocating a colour view with all colour writes masked off cannot take ownership from depth.
 auto no_write=s;no_write.rt={texture,s.zs.data,kernel::R32(texture+12),kernel::R32(texture+16)};
 no_write.textures.fill(0);no_write.simple[gpu::kMColorWriteEnable>>2]=0;
 b->Draw(no_write,5,0,3,nullptr);b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
 if(cpu_pixel[0]!=64||cpu_pixel[1]||cpu_pixel[2]||cpu_pixel[3]!=0x5A)throw std::runtime_error("Empty colour view allocation stole live depth authority");
 // Encoding must happen before bilinear filtering: averaging integer depth first changes its bytes.
 kernel::W32(texture+12,(0x40<<8)|0x20);
 s.tss[0][gpu::kTssMagFilter]=s.tss[0][gpu::kTssMinFilter]=2;
 const int32_t top[]={0,0,8,4},bottom[]={0,4,8,8};
 b->Clear(s,1,reinterpret_cast<const uint8_t*>(top),3,0,float(0x010000)/16777215.f,0x20);
 b->Clear(s,1,reinterpret_cast<const uint8_t*>(bottom),3,0,float(0x020000)/16777215.f,0xE0);
 float filtered[18];std::memcpy(filtered,triangle,sizeof(filtered));for(unsigned v=0;v<3;v++)filtered[v*6+5]=4.5f;
 std::memcpy(kernel::HostPtr(vertices),filtered,sizeof(filtered));
 b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
 auto* pixel=static_cast<unsigned char*>(kernel::HostPtr(s.rt.data))+2*64+2*4;
 if(pixel[0]<1||pixel[0]>2||pixel[1]||pixel[2]||pixel[3]<127||pixel[3]>129)throw std::runtime_error("Depth alias filtered depth before packing");
 // Normalized edge coordinates do not authorize replacing the guest's WRAP sampler with CLAMP.
 auto colour_data=mem::AllocContiguous(64,0,0xFFFFFFFF,4096);
 const unsigned char colours[]={0,0,255,255,255,0,0,255};std::memcpy(kernel::HostPtr(colour_data),colours,8);
 kernel::W32(texture+4,mem::PhysicalFromVirtual(colour_data));kernel::W32(texture+12,(0x12<<8)|0x20);kernel::W32(texture+16,1);
 s.tss[0][gpu::kTssAddressU]=1;s.tss[0][gpu::kTssAddressV]=3;
 for(unsigned v=0;v<3;v++){filtered[v*6+4]=0;filtered[v*6+5]=.5f;}
 std::memcpy(kernel::HostPtr(vertices),filtered,sizeof(filtered));
 b->Draw(s,5,0,3,nullptr);b->Sync(s.rt.data,512);
 if(pixel[0]<126||pixel[0]>129||pixel[1]||pixel[2]<126||pixel[2]>129)throw std::runtime_error("Native inputs overrode declared WRAP address mode");
 std::puts("DEPTH_COLOUR_ALIAS: 5 formats, depth/stencil updates, cache reuse and post-pack filtering passed without CPU depth readback");
}
