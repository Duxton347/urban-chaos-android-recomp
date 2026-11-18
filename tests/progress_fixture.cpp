#include "diagnostics/progress.h"
#include <stdexcept>
#include <vector>
using namespace urban::progress;
static void Check(bool ok){if(!ok)throw std::runtime_error("progress contract failed");}
int main(int argc,char** argv) {
 Check(argc==2);
 auto* slot=Local();
 {
  Scope guest(Kind::Guest,0x11000);
  try {Scope kernel(Kind::Kernel,219);Check(slot->current==Packed(Kind::Kernel,219));throw 1;}
  catch(int){}
  Check(slot->current==Packed(Kind::Guest,0x11000));
 }
 Check(slot->current==0&&slot->calls==2);
 {GpuOperation shader(2);Check(slot->current==Packed(Kind::Vulkan,2));}
 Check(slot->current==0&&gpu[2].calls==1&&gpu[2].max_us<=gpu[2].us);
 // A live read is observable without examining the worker's non-atomic state.
 {Read read(16);Check(inflight==1);read.Complete(16);}
 {Read read(8);read.Complete(3);}
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
 urban::diagnostics::Journal journal;
 Check(journal.Start(argv[1],"authored-progress",std::string(64,'a')));
 Poll(journal);journal.Finish();
}
