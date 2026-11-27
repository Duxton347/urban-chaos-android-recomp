// SDK-generated and authored shaders only; no original Xbox shader programs.
#include "vk_shader.h"
#include "gpu/shaders.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <source_location>
#include <cstdio>
#include <stdexcept>
using Stage=vkshader::Stage;
static void Require(bool v,const std::source_location& at=std::source_location::current()) {
 if(!v)throw std::runtime_error("shader fixture assertion at line "+std::to_string(at.line()));
}
static uint64_t Key(const std::string& source,const char* entry,Stage stage) {
 auto hash=gpu::Hash(source.data(),source.size());hash=gpu::Hash(entry,std::char_traits<char>::length(entry),hash);
 const char* label=stage==Stage::Vertex?"vertex":"fragment";return gpu::Hash(label,std::char_traits<char>::length(label),hash);
}
struct Reflected {std::set<uint32_t> bindings,locations;uint32_t stage=UINT32_MAX;};
static Reflected Reflect(const std::vector<uint32_t>& words) {
 Require(words.size()>=5&&words[0]==0x07230203);Reflected out;std::map<uint32_t,uint32_t> locations;std::set<uint32_t> inputs,opaque;
 for(size_t at=5;at<words.size();) {
  uint32_t count=words[at]>>16,op=words[at]&65535;Require(count>0&&at+count<=words.size());
  if(op==15){Require(count>=5);out.stage=words[at+1];}
  if((op==25||op==26||op==27)&&count>=2)opaque.insert(words[at+1]);
  // Vulkan disallows Function pointers to opaque image/sampler values (VUID 06924).
  if(op==32&&count>=4&&words[at+2]==7)Require(!opaque.contains(words[at+3]));
  if(op==59&&count>=4&&words[at+3]==1)inputs.insert(words[at+2]);
  if(op==71&&count>=4) {
   if(words[at+2]==33)out.bindings.insert(words[at+3]);
   if(words[at+2]==30)locations[words[at+1]]=words[at+3];
   if(words[at+2]==34)Require(words[at+3]==0);
  }
  at+=count;
 }
 for(auto id:inputs)if(locations.contains(id))out.locations.insert(locations[id]);
 return out;
}
int main(int argc,char** argv) {
 try {
  Require(argc==3);std::filesystem::path output=argv[1];std::filesystem::create_directories(output);
  // Cache disabled for the first compiler/ABI qualification; no game cache is read.
  vkshader::Init({});unsigned compiled=0;
  auto compile=[&](const std::string& source,const char* entry,Stage stage,const char* name){
   std::vector<uint32_t> words;Require(vkshader::Compile(source,entry,stage,Key(source,entry,stage),words));
   auto info=Reflect(words);Require(info.stage==(stage==Stage::Vertex?0u:4u));
   for(auto binding:info.bindings)Require(binding<vkshader::kBindings);
   std::ofstream file(output/(std::string(name)+".spv"),std::ios::binary);file.write(reinterpret_cast<const char*>(words.data()),words.size()*4);Require(bool(file));++compiled;return info;
  };
  auto fixed=compile(gpu::FixedVertexHlsl((1<<0)|(1<<3),false),"main",Stage::Vertex,"sdk-fixed");
  Require(fixed.locations.contains(0)&&fixed.locations.contains(3)&&fixed.bindings.contains(1));
  compile(gpu::FixedVertexHlsl((1<<0)|(1<<3),true),"main",Stage::Vertex,"sdk-rhw");
  gpu::FixedKey lights{};lights.light=1|8;lights.types=3;compile(gpu::FixedVertexHlsl(15,false,0,lights),"main",Stage::Vertex,"sdk-lit");
  gpu::PixelKey pixel{};compile(gpu::PixelHlsl(pixel),"main",Stage::Pixel,"sdk-pixel");
  pixel.tex[0].kind=gpu::kTex2D;uint32_t stage[9]={4,2,0,0,2,2,0,0,1};std::copy(stage,stage+9,pixel.ff[0]);
  auto texture=compile(gpu::PixelHlsl(pixel),"main",Stage::Pixel,"sdk-texture");
  Require(texture.bindings.contains(2)&&texture.bindings.contains(6));
  pixel.tex[0].shadow=1;compile(gpu::PixelHlsl(pixel),"main",Stage::Pixel,"sdk-shadow");
  pixel.tex[0].shadow=0;pixel.alphakill=1;compile(gpu::PixelHlsl(pixel),"main",Stage::Pixel,"sdk-alpha");
  const std::string helper=R"(
float4 tap(Texture2D image, SamplerState sampler, float2 uv) { return image.SampleLevel(sampler,uv,0); }
Texture2D image0 : register(t0);
SamplerState sampler0 : register(s0);
float4 main(float2 uv : TEXCOORD0) : SV_Target { return tap(image0,sampler0,uv); }
)";
  auto specialized=compile(helper,"main",Stage::Pixel,"opaque-helper");
  Require(specialized.bindings.contains(2)&&specialized.bindings.contains(6));
  std::ifstream input(argv[2]);Require(bool(input));std::string authored((std::istreambuf_iterator<char>(input)),{});
  compile(authored,"vs",Stage::Vertex,"authored-vertex");compile(authored,"ps",Stage::Pixel,"authored-fragment");
  std::printf("{\"status\":\"PASS\",\"compiled_shaders\":%u,\"sdk_hlsl\":true,\"black_compiler_reused\":true,\"descriptor_bindings_checked\":true,\"game_data_used\":false}\n",compiled);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"Shader reuse fixture failed: %s\n",e.what());return 2;}
}
