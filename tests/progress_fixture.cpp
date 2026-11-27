#include "diagnostics/progress.h"
#include <stdexcept>
#include <vector>
using namespace urban::progress;
static void Check(bool ok){if(!ok)throw std::runtime_error("progress contract failed");}
int main(int argc,char** argv) {
 Check(argc==2);
 auto* slot=Local();
#if defined(__linux__)
 bool cpu_valid=false;auto cpu_before=CpuNanoseconds(*slot,cpu_valid);Check(cpu_valid);
 volatile uint64_t authored_sum=0;for(unsigned n=0;n<1000000;++n)authored_sum=authored_sum+n;
 auto cpu_after=CpuNanoseconds(*slot,cpu_valid);Check(cpu_valid&&cpu_after>cpu_before);
#endif
 {
  Scope guest(Kind::Guest,0x11000);
  try {Scope kernel(Kind::Kernel,219,CodeCaller(0x11024,0x11000,0x12000));Check(slot->current==Packed(Kind::Kernel,219));throw 1;}
  catch(int){}
  Check(slot->current==Packed(Kind::Guest,0x11000));
 }
 Check(slot->current==0&&slot->calls==2);
 Check(slot->last_kernel==((uint64_t(219)<<32)|0x11024));
 Check(CodeCaller(42,0x11000,0x12000)==0&&CodeCaller(0x12000,0x11000,0x12000)==0);
 Check(ResourceId("D:\\SENTINEL_PATH")==ResourceId("d:\\sentinel_path"));
 {GpuOperation shader(2);Check(slot->current==Packed(Kind::Vulkan,2));}
 Check(slot->current==0&&gpu[2].calls==1&&gpu[2].max_us<=gpu[2].us);
 // A live read is observable without examining the worker's non-atomic state.
 {Read read(16);Check(inflight==1);read.Complete(16);}
 {Read read(8);read.Identify(ResourceId("D:\\SENTINEL_PATH"),1024);read.Complete(3);}
 Check(reads==2&&bytes==19&&short_reads==1&&inflight==0);
 std::vector<std::thread> workers;
 for(unsigned n=0;n<70;n++)workers.emplace_back([]{
  for(unsigned i=0;i<1000;++i){Scope call(Kind::Hle,0x12000);StreamStarved();}
 });
 for(auto& worker:workers)worker.join();
 Check(allocated==71&&starved==70000);
 uint64_t calls=0;
 for(auto& s:slots){Check(s.current==0);calls+=s.calls;}
 Check(calls==63003);
 slot->sample_attempts=0;
 uint32_t words[]={0x11004,0x11004,0x11008,0xffffffff};unsigned sampled=0;
 SampleCodeCandidates(0x1000,0x1000,0x1010,0x11000,0x12000,[&](uint32_t at){Check(at>=0x1000&&at<0x1010);++sampled;return words[(at-0x1000)/4];});
 Check(sampled==4&&slot->code_candidates[0]==0x11004&&slot->code_candidates[1]==0x11008&&slot->code_candidates[2]==0);
 SampleCodeCandidates(0x1000,0x1000,0x1010,0x11000,0x12000,[&](uint32_t){throw std::runtime_error("sampling cadence failed");return 0;});
 slot->sample_attempts=0;
 SampleCodeCandidates(0xffc,0x1000,0x1010,0x11000,0x12000,[&](uint32_t){throw std::runtime_error("outside stack read");return 0;});
 Check(slot->code_candidates[0]==0);
 visibility_polls=7;visibility_submits=3;idle_flushes=5;same_present_texture_updates=2;two_sided_draws=11;
 native_vertex_draws=17;converted_vertex_draws=19;vertex_upload_bytes=123456;depth_colour_conversions=5;depth_colour_binds=23;shader_cache_hits=31;shader_cache_rejected=2;shader_compilations=37;signed_texture_binds=29;signed_texture_upload_bytes=16384;
 urban::diagnostics::Journal journal;
 Check(journal.Start(argv[1],"authored-progress",std::string(64,'a')));
 Poll(journal);journal.Finish();
}
