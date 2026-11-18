#pragma once
// Fixed-size, race-free progress metadata. No guest memory, resource names or paths.
#include "diagnostics/journal.h"
#include <array>

namespace urban::progress {
enum class Kind : uint32_t { Idle, Guest, Kernel, Hle, Vulkan };
struct Slot {
 std::atomic<uint64_t> current{0}, last{0}, calls{0};
};
inline std::array<Slot,64> slots;
inline std::atomic<uint32_t> allocated{0};
inline std::atomic<uint64_t> reads{0}, bytes{0}, short_reads{0}, io_us{0}, max_io_us{0}, inflight{0}, starved{0};
inline uint64_t Packed(Kind kind,uint32_t value){return (uint64_t(kind)<<32)|value;}
// Stable FNV-1a of the public HLE implementation name, including disabled SDK APIs.
constexpr uint32_t HleId(std::string_view name){uint32_t id=2166136261u;for(unsigned char c:name){id^=c;id*=16777619u;}return id;}
inline Slot* Local() {
 // Slots are never recycled, so snapshots cannot follow freed thread state.
 thread_local Slot* slot=[](){auto n=allocated.fetch_add(1);return n<slots.size()?&slots[n]:nullptr;}();
 return slot;
}
class Scope {
 Slot* slot_;uint64_t prior_=0;
public:
 Scope(Kind kind,uint32_t value):slot_(Local()) {
  if(slot_){auto next=Packed(kind,value);prior_=slot_->current.exchange(next,std::memory_order_relaxed);
   slot_->last.store(next,std::memory_order_relaxed);slot_->calls.fetch_add(1,std::memory_order_relaxed);}
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
 uint32_t requested_;uint32_t completed_=0;
public:
 explicit Read(uint32_t requested):requested_(requested){inflight.fetch_add(1,std::memory_order_relaxed);}
 void Complete(uint32_t completed){completed_=completed;}
 ~Read(){
  auto us=uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start_).count());
  reads.fetch_add(1,std::memory_order_relaxed);bytes.fetch_add(completed_,std::memory_order_relaxed);
  if(completed_<requested_)short_reads.fetch_add(1,std::memory_order_relaxed);
  io_us.fetch_add(us,std::memory_order_relaxed);Maximum(max_io_us,us);inflight.fetch_sub(1,std::memory_order_relaxed);
 }
};
inline void StreamStarved(){starved.fetch_add(1,std::memory_order_relaxed);}
struct GpuMetric {std::atomic<uint64_t> calls{0},us{0},max_us{0};};
inline std::array<GpuMetric,3> gpu;
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
  auto& slot=slots[n];auto state=slot.current.load(std::memory_order_relaxed),last=slot.last.load(std::memory_order_relaxed);
  journal.Emit(diagnostics::Code::ThreadProgress,{{"slot",n},{"kind",state>>32},{"target",uint32_t(state)},
   {"last_kind",last>>32},{"last_target",uint32_t(last)},{"calls",slot.calls.load(std::memory_order_relaxed)}});
 }
 journal.Emit(diagnostics::Code::IoProgress,{{"reads",reads.load()},{"bytes",bytes.load()},{"short_reads",short_reads.load()},
  {"inflight",inflight.load()},{"total_us",io_us.load()},{"max_us",max_io_us.load()},{"stream_empty_blocks",starved.load()},
  {"thread_slots_dropped",allocated.load()>slots.size()?allocated.load()-slots.size():0}});
 for(uint32_t n=0;n<gpu.size();++n)journal.Emit(diagnostics::Code::GpuTiming,
  {{"operation",n},{"calls",gpu[n].calls.load()},{"total_us",gpu[n].us.load()},{"max_us",gpu[n].max_us.load()}});
}
}
