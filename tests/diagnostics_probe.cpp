#include "diagnostics/journal.h"
#include "core/log.h"
#include <cstdlib>
#include <stdexcept>
#include <vector>
using namespace urban::diagnostics;
namespace fs=std::filesystem;
static const std::string hash(64,'a');
static void Require(bool value){if(!value)throw std::runtime_error("diagnostics fixture assertion");}
int main(int argc,char**argv){
 if(argc<2)return 2;fs::path root=argv[1];
 if(argc==3&&std::string_view(argv[2])=="--abrupt"){
  Journal j;Require(j.Start(root,"fixture-build",hash));Require(j.Emit(Code::SessionBegin));
  Require(j.Emit(Code::GuestTrap,{{"address",0x12345}}));std::_Exit(17);
 }
 auto& j=ProcessJournal();Require(j.Start(root/"standard","fixture-build",hash));
 logx::Init((root/"raw").string(),"fixture");
 logx::SetObserver([](logx::Level lv,const char* format,const char* expanded){ProcessJournal().Observe(int(lv),format,expanded);});
 Require(j.Emit(Code::SessionBegin));
 LOGI("renderer: frame %u, %u draws, %u empty frames",60u,123u,4u);
 LOGI("renderer: frame %llu (Vulkan headless)",1800ull);
 LOGI("vulkan: visibility report completed, samples=%u",64u);
 LOGI("renderer: frame %llu (Vulkan Android)",2400ull);
 LOGI("vulkan-device: vendor %u device %u api %u driver %u precise %u phase %u",0x5143u,650u,4206592u,12345u,1u,1u);
 LOGE("vulkan-count-failure: operation %u",4u);
 LOGI("audio-metrics: frames %llu active-samples %llu peak %f nonfinite %llu",480000ull,1234ull,0.125,0ull);
 LOGE("audio-output: failure operation %u",5u);
 LOGI("audio-output: state %u",2u);
 LOGE("indirect call to %08X has no recompiled function (return address %08X, esp %08X)",0x11111,0x22222,0x33333);
 LOGE("kernel export %u (%s) is not implemented, return address %08X",232u,"SENTINEL_PROPRIETARY_NAME",0x44444);
 LOGI("missing %s","D:\\SENTINEL_PRIVATE_FOLDER\\SENTINEL_GAME_BYTES");
 LOGE("fixture error %s","SENTINEL_EXCEPTION_MESSAGE");
 LOGI("private debug %s","SENTINEL_SECRET_TOKEN");
 logx::SetObserver(nullptr);j.Finish();
 Require(!fs::exists(root/"standard/session.open"));
 Require(j.Start(root/"detailed","fixture-build",hash,true));
 logx::SetObserver([](logx::Level lv,const char* format,const char* expanded){ProcessJournal().Observe(int(lv),format,expanded);});
 LOGI("private debug %s","SENTINEL_SECRET_TOKEN");
 logx::SetObserver(nullptr);j.Finish();
 {
  Journal rotated;Require(rotated.Start(root/"rotated","fixture-build",hash,true,4096));
  for(unsigned n=0;n<1000;n++)Require(rotated.Emit(Code::Trace,{{"counter",n}}));
  rotated.Finish();
 }
 {
  Journal parallel;Require(parallel.Start(root/"parallel","fixture-build",hash));
  Require(parallel.Emit(Code::SessionBegin));
  std::vector<std::thread> workers;
  for(unsigned n=0;n<8;n++)workers.emplace_back([&,n]{
   for(unsigned m=0;m<20;m++)Require(parallel.Emit(Code::FrameStats,{{"counter",n*20+m}}));
  });
  for(auto& worker:workers)worker.join();parallel.Finish();
 }
 {
  Journal blocked;Require(blocked.Start(root/"blocked","fixture-build",hash,true,4096));
  fs::create_directory(root/"blocked/runtime.3.jsonl");
  FILE* sentinel=std::fopen((root/"blocked/runtime.3.jsonl/owned-fixture.txt").string().c_str(),"wb");
  Require(sentinel!=nullptr);std::fclose(sentinel);
  bool failed=false;
  for(unsigned n=0;n<100;n++)if(!blocked.Emit(Code::Trace,{{"counter",n}})){failed=true;break;}
  Require(failed&&!blocked.Healthy());
  Require(fs::exists(root/"blocked/runtime.3.jsonl/owned-fixture.txt"));
 }
 Journal invalid;Require(!invalid.Start(root/"invalid","/private/user/path",hash));
 Require(SourceCode("ps2_image")==Code::SourcePs2);
 Require(SourceCode("wrong_title")==Code::SourceTitle);
 Require(SourceCode("section_integrity")==Code::SourceIntegrity);
 Require(SourceCode("unclassified")==Code::SourceOther);
 std::puts("Native diagnostic fixture passed (author-created metadata only)");
}
