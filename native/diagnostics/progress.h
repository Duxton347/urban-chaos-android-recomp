#pragma once
// Fixed-size, race-free progress metadata. No guest memory, resource names or paths.
#include "diagnostics/journal.h"
#include <array>
#if defined(__linux__)
#include <pthread.h>
#include <time.h>
#endif

namespace urban::progress {
enum class Kind : uint32_t { Idle, Guest, Kernel, Hle, Vulkan };
struct Slot {
 std::atomic<uint64_t> current{0}, last{0}, calls{0},last_kernel{0},last_hle{0},sample_attempts{0};
 // Transitions, not faults: resumable execution deliberately uses X86Transfer.
 std::atomic<uint64_t> transfers{0},resume_steps{0};
 std::array<std::atomic<uint32_t>,8> code_candidates{};
 std::atomic<int64_t> cpu_clock{INT64_MAX};
};
inline std::atomic<uint64_t> native_vertex_draws{0},converted_vertex_draws{0},vertex_upload_bytes{0};
inline std::atomic<uint64_t> depth_colour_conversions{0},depth_colour_binds{0};
inline std::atomic<uint64_t> signed_texture_binds{0},signed_texture_upload_bytes{0};
inline std::array<Slot,64> slots;
inline std::atomic<uint32_t> allocated{0};
inline std::atomic<uint64_t> reads{0}, bytes{0}, short_reads{0}, io_us{0}, max_io_us{0}, inflight{0}, starved{0};
inline std::atomic<uint64_t> apcs_queued{0},apcs_delivered{0},last_read_id{0},last_read_offset{0},last_read_bytes{0};
inline uint64_t Packed(Kind kind,uint32_t value){return (uint64_t(kind)<<32)|value;}
// Stable FNV-1a of the public HLE implementation name, including disabled SDK APIs.
constexpr uint32_t HleId(std::string_view name){uint32_t id=2166136261u;for(unsigned char c:name){id^=c;id*=16777619u;}return id;}
constexpr uint32_t CodeCaller(uint32_t caller,uint32_t low,uint32_t high){return caller>=low&&caller<high?caller:0;}
inline uint32_t ResourceId(std::string_view path){uint32_t id=2166136261u;for(unsigned char c:path){if(c>='A'&&c<='Z')c+=32;id^=c;id*=16777619u;}return id;}
inline Slot* Local() {
 // Slots are never recycled, so snapshots cannot follow freed thread state.
 thread_local Slot* slot=[](){
  auto n=allocated.fetch_add(1);if(n>=slots.size())return static_cast<Slot*>(nullptr);
#if defined(__linux__)
  clockid_t id;if(pthread_getcpuclockid(pthread_self(),&id)==0)slots[n].cpu_clock.store(id,std::memory_order_relaxed);
#endif
  return &slots[n];}();
 return slot;
}
// Bounded stack scan for code candidates, not a validated unwound call stack. Never exports raw words.
template<class Reader> void SampleCodeCandidates(uint32_t esp,uint32_t low,uint32_t high,uint32_t codeLow,uint32_t codeHigh,Reader read){
 auto* slot=Local();if(!slot||(slot->sample_attempts.fetch_add(1,std::memory_order_relaxed)&1023))return;
 for(auto& value:slot->code_candidates)value.store(0,std::memory_order_relaxed);
 if(low>high||esp<low||esp>high||esp%4)return;
 unsigned used=0;uint32_t last=0;uint64_t end=std::min<uint64_t>(high,uint64_t(esp)+256);
 for(uint64_t at=esp;at+4<=end&&used<slot->code_candidates.size();at+=4){
  auto value=CodeCaller(read(uint32_t(at)),codeLow,codeHigh);
  if(value&&value!=last){slot->code_candidates[used++].store(value,std::memory_order_relaxed);last=value;}
 }
}
inline uint64_t CpuNanoseconds(const Slot& slot,bool& valid){
 valid=false;
#if defined(__linux__)
 auto id=slot.cpu_clock.load(std::memory_order_relaxed);timespec t{};
 if(id!=INT64_MAX&&clock_gettime(clockid_t(id),&t)==0){valid=true;return uint64_t(t.tv_sec)*1000000000ull+uint64_t(t.tv_nsec);}
#endif
 return 0;
}
class Scope {
 Slot* slot_;uint64_t prior_=0;
public:
 Scope(Kind kind,uint32_t value,uint32_t caller=0):slot_(Local()) {
  if(slot_){auto next=Packed(kind,value);prior_=slot_->current.exchange(next,std::memory_order_relaxed);
   slot_->last.store(next,std::memory_order_relaxed);slot_->calls.fetch_add(1,std::memory_order_relaxed);
   if(kind==Kind::Kernel)slot_->last_kernel.store((uint64_t(value)<<32)|caller,std::memory_order_relaxed);
   if(kind==Kind::Hle)slot_->last_hle.store((uint64_t(value)<<32)|caller,std::memory_order_relaxed);}
 }
 ~Scope(){if(slot_)slot_->current.store(prior_,std::memory_order_relaxed);}
 Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
};
inline void Maximum(std::atomic<uint64_t>& counter,uint64_t value) {
 auto previous=counter.load(std::memory_order_relaxed);
 while(previous<value&&!counter.compare_exchange_weak(previous,value,std::memory_order_relaxed)){}
}
class Read {
 std::chrono::steady_clock::time_point start_=std::chrono::steady_clock::now();
 uint32_t requested_;uint32_t completed_=0,id_=0;uint64_t offset_=0;
public:
 explicit Read(uint32_t requested):requested_(requested){inflight.fetch_add(1,std::memory_order_relaxed);}
 void Identify(uint32_t id,uint64_t offset){id_=id;offset_=offset;}
 void Complete(uint32_t completed){completed_=completed;}
 ~Read(){
  auto us=uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_).count());
  reads.fetch_add(1,std::memory_order_relaxed);bytes.fetch_add(completed_,std::memory_order_relaxed);
  if(completed_<requested_)short_reads.fetch_add(1,std::memory_order_relaxed);
  io_us.fetch_add(us,std::memory_order_relaxed);Maximum(max_io_us,us);inflight.fetch_sub(1,std::memory_order_relaxed);
  last_read_id.store((uint64_t(id_)<<32)|requested_,std::memory_order_relaxed);
  last_read_offset.store(offset_,std::memory_order_relaxed);last_read_bytes.store(completed_,std::memory_order_relaxed);
 }
};
inline void StreamStarved(){starved.fetch_add(1,std::memory_order_relaxed);}
struct GpuMetric {std::atomic<uint64_t> calls{0},us{0},max_us{0};};
// 0 wait, 1 acquire, 2 module, 3 draw (inclusive), 4 assembly,
// 5 texture binding, 6 pipeline lookup/create, 7 submission, 8 readback. Nested totals must not be summed.
inline std::array<GpuMetric,10> gpu;
inline std::atomic<uint64_t> visibility_polls{0},visibility_submits{0},idle_flushes{0},same_present_texture_updates{0},two_sided_draws{0};
class GpuOperation {
 Scope scope_;GpuMetric& metric_;
 std::chrono::steady_clock::time_point start_=std::chrono::steady_clock::now();
public:
 explicit GpuOperation(uint32_t op):scope_(Kind::Vulkan,op),metric_(gpu.at(op)){}
 ~GpuOperation(){
  auto us=uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_).count());
  metric_.calls.fetch_add(1,std::memory_order_relaxed);metric_.us.fetch_add(us,std::memory_order_relaxed);Maximum(metric_.max_us,us);
 }
};
inline void Poll(diagnostics::Journal& journal) {
 for(uint32_t n=0;n<std::min<uint32_t>(allocated.load(),slots.size());++n){
  auto& slot=slots[n];
  if(slot.sample_attempts.load())journal.Emit(diagnostics::Code::CodeCandidates,{{"slot",n},{"attempts",slot.sample_attempts.load()},
   {"pc0",slot.code_candidates[0].load()},{"pc1",slot.code_candidates[1].load()},{"pc2",slot.code_candidates[2].load()},{"pc3",slot.code_candidates[3].load()},
   {"pc4",slot.code_candidates[4].load()},{"pc5",slot.code_candidates[5].load()},{"pc6",slot.code_candidates[6].load()},{"pc7",slot.code_candidates[7].load()}});
  auto state=slot.current.load(std::memory_order_relaxed),last=slot.last.load(std::memory_order_relaxed);
  auto kernel=slot.last_kernel.load(std::memory_order_relaxed),hle=slot.last_hle.load(std::memory_order_relaxed);
  bool cpu_valid=false;auto cpu_ns=CpuNanoseconds(slot,cpu_valid);
  journal.Emit(diagnostics::Code::ThreadProgress,{{"slot",n},{"kind",state>>32},{"target",uint32_t(state)},
   {"cpu_valid",cpu_valid},{"cpu_ns",cpu_ns},{"last_kind",last>>32},{"last_target",uint32_t(last)},{"calls",slot.calls.load(std::memory_order_relaxed)},
   {"transfers",slot.transfers.load(std::memory_order_relaxed)},{"resume_steps",slot.resume_steps.load(std::memory_order_relaxed)},
   {"kernel",kernel>>32},{"kernel_caller",uint32_t(kernel)},{"hle",hle>>32},{"hle_caller",uint32_t(hle)}});
 }
 journal.Emit(diagnostics::Code::IoProgress,{{"reads",reads.load()},{"bytes",bytes.load()},{"short_reads",short_reads.load()},
  {"inflight",inflight.load()},{"total_us",io_us.load()},{"max_us",max_io_us.load()},{"stream_empty_blocks",starved.load()},
  {"thread_slots_dropped",allocated.load()>slots.size()?allocated.load()-slots.size():0}});
 auto read=last_read_id.load(std::memory_order_relaxed);
 journal.Emit(diagnostics::Code::IoCompletion,{{"queued_apcs",apcs_queued.load()},{"delivered_apcs",apcs_delivered.load()},
  {"resource_id",read>>32},{"requested",uint32_t(read)},{"offset",last_read_offset.load()},{"completed",last_read_bytes.load()}});
 journal.Emit(diagnostics::Code::GpuSynchronization,{{"visibility_polls",visibility_polls.load()},{"visibility_submits",visibility_submits.load()},{"idle_flushes",idle_flushes.load()},{"same_present_texture_updates",same_present_texture_updates.load()},{"two_sided_draws",two_sided_draws.load()},{"native_vertex_draws",native_vertex_draws.load()},{"converted_vertex_draws",converted_vertex_draws.load()},{"vertex_upload_bytes",vertex_upload_bytes.load()},{"depth_colour_conversions",depth_colour_conversions.load()},{"depth_colour_binds",depth_colour_binds.load()},{"signed_texture_binds",signed_texture_binds.load()},{"signed_texture_upload_bytes",signed_texture_upload_bytes.load()}});
 for(uint32_t n=0;n<gpu.size();++n)journal.Emit(diagnostics::Code::GpuTiming,
  {{"operation",n},{"calls",gpu[n].calls.load()},{"total_us",gpu[n].us.load()},{"max_us",gpu[n].max_us.load()}});
}
}
