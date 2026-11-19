#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace urban::vulkan {
enum class QueryFailure { PreciseUnavailable=1, Capacity, Lifecycle, Driver, Conversion };
struct QueryError : std::runtime_error {
 QueryFailure reason;VkResult result;
 QueryError(QueryFailure,const char*,VkResult=VK_SUCCESS);
};
struct VisibilityReport { uint32_t address;uint64_t timestamp;uint32_t pixels; };
struct QueryLimits {
 uint32_t per_submission=4096,inflight=4,live_samples=65536,pending_reports=4096;
 uint32_t pending_sample_refs=262144; // bound snapshot copies as well as live GPU samples
};
// One owner/render thread. Device and queue belong to the enclosing renderer.
// The precise flag must reflect the features ENABLED on VkDevice, not just supported by the GPU.
class VisibilityQueries {
 struct Sample;
 struct Batch {VkQueryPool pool;VkCommandBuffer commands;uint64_t serial;std::vector<std::shared_ptr<Sample>> samples;};
 struct Pending {uint32_t address;uint64_t timestamp,serial;std::vector<std::shared_ptr<Sample>> samples;};
 VkDevice device_;
 QueryLimits limits_;
 std::shared_ptr<uint32_t> live_=std::make_shared<uint32_t>(0);
 std::deque<Batch> batches_;
 std::vector<VkQueryPool> free_;
 std::vector<std::shared_ptr<Sample>> counted_;
 std::deque<Pending> pending_;
 Batch* recording_=nullptr;
 bool draw_open_=false;
 uint32_t active_=UINT32_MAX;
 uint64_t last_serial_=0,last_completed_=0;
 uint32_t pending_refs_=0;
 VkQueryPool MakePool();
 void Reap(uint64_t);
public:
 VisibilityQueries(VkDevice,bool precise_enabled,QueryLimits={});
 VisibilityQueries(const VisibilityQueries&)=delete;
 VisibilityQueries& operator=(const VisibilityQueries&)=delete;
 ~VisibilityQueries(); // waits for device idle before destroying owned pools; device must still exist
 // Record reset OUTSIDE a render pass, before the command buffer's counted draws.
 void BeginCommands(VkCommandBuffer,uint64_t submission_serial);
 // Each BeginDraw/EndDraw pair must be INSIDE the same render pass/dynamic rendering scope.
 // Normalization removes internal resolution scaling and MSAA samples per guest pixel.
 void BeginDraw(bool count,double samples_per_guest_pixel=1);
 void EndDraw();
 void Clear();
 void Report(uint32_t guest_address,uint64_t timestamp);
 void SealCommands();
 // completed_serial comes from actual fences/timeline completion. No WAIT query flag is used.
 // Writer publishes the SDK's 16-byte record only after its report is complete.
 size_t Poll(uint64_t completed_serial,const std::function<void(const VisibilityReport&)>& writer);
 size_t PendingReports() const{return pending_.size();}
 uint32_t LiveSamples() const{return *live_;}
};
}
