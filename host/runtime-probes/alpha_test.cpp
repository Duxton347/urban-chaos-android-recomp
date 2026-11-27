// Check the compiled shader, not merely a source string: disabled tests cannot inhibit early depth via OpKill.
#include "gpu/shaders.h"
#include "vk_shader.h"
#include <cstdio>
#include <stdexcept>
void ConditionalAlphaTestSelfTest(){
 gpu::State s{};gpu::TexInfo textures[4]{};
 for(bool enabled:{false,true}){
  s.simple[gpu::kMAlphaTestEnable>>2]=enabled;
  auto source=gpu::PixelHlsl(gpu::MakePixelKey(s,textures));std::vector<uint32_t> spirv;
  if(!vkshader::Compile(source,"main",vkshader::Stage::Pixel,gpu::Hash(source.data(),source.size()),spirv))throw std::runtime_error("Alpha specialization shader did not compile");
  unsigned kills=0;for(size_t i=5;i<spirv.size();){unsigned words=spirv[i]>>16,op=spirv[i]&65535;if(!words||words>spirv.size()-i)throw std::runtime_error("Invalid alpha fixture SPIR-V");if(op==252)++kills;i+=words;}
  if(bool(kills)!=enabled)throw std::runtime_error("Disabled alpha test retains fragment discard or enabled test lost it");
 }
 std::puts("CONDITIONAL_ALPHA_TEST: compiled SPIR-V discards only when alpha test is enabled; texture clip behavior remains covered by visibility tests");
}
