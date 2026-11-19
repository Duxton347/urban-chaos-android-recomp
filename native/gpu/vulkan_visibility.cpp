#include "gpu/vulkan_visibility.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace urban::vulkan {
QueryError::QueryError(QueryFailure why,const char* message,VkResult vk):std::runtime_error(message),reason(why),result(vk){}
namespace {
[[noreturn]] void Invalid(const char* message){throw QueryError(QueryFailure::Lifecycle,message);}
void Check(VkResult r,const char* message){if(r!=VK_SUCCESS)throw QueryError(QueryFailure::Driver,message,r);}
}
struct VisibilityQueries::Sample {
 std::shared_ptr<uint32_t> live;double scale;uint64_t samples=0;bool ready=false;
 Sample(std::shared_ptr<uint32_t> counter,double factor):live(std::move(counter)),scale(factor){++*live;}
 ~Sample(){--*live;}
};
VkQueryPool VisibilityQueries::MakePool() {
 VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
 info.queryType=VK_QUERY_TYPE_OCCLUSION;info.queryCount=limits_.per_submission;
 VkQueryPool pool=VK_NULL_HANDLE;Check(vkCreateQueryPool(device_,&info,nullptr,&pool),"UC-VK-004: query pool creation failed");
 return pool;
}
VisibilityQueries::VisibilityQueries(VkDevice device,bool precise,QueryLimits limits):device_(device),limits_(limits) {
 if(!precise)throw QueryError(QueryFailure::PreciseUnavailable,"UC-VK-001: precise occlusion was not enabled on the device");
 if(!device_||!limits.per_submission||limits.per_submission>65536||!limits.inflight||limits.inflight>16||
    !limits.live_samples||limits.live_samples>1048576||!limits.pending_reports||limits.pending_reports>65536||
    !limits.pending_sample_refs||limits.pending_sample_refs>1048576)
  Invalid("UC-VK-003: invalid device or query limits");
 auto pool=MakePool();
 try{free_.push_back(pool);}catch(...){vkDestroyQueryPool(device_,pool,nullptr);throw;}
}
VisibilityQueries::~VisibilityQueries() {
 // Queue ownership and device destruction remain with the renderer.
 vkDeviceWaitIdle(device_);
 for(auto& batch:batches_)vkDestroyQueryPool(device_,batch.pool,nullptr);
 for(auto pool:free_)vkDestroyQueryPool(device_,pool,nullptr);
}
void VisibilityQueries::BeginCommands(VkCommandBuffer commands,uint64_t serial) {
 if(recording_||!commands||serial<=last_serial_)Invalid("UC-VK-003: invalid recording order");
 if(batches_.size()>=limits_.inflight)throw QueryError(QueryFailure::Capacity,"UC-VK-002: completed submissions must be polled before pool reuse");
 VkQueryPool pool;
 if(free_.empty())pool=MakePool();else{pool=free_.back();free_.pop_back();}
 try{batches_.push_back({pool,commands,serial,{}});}catch(...){vkDestroyQueryPool(device_,pool,nullptr);throw;}
 recording_=&batches_.back();last_serial_=serial;
 vkCmdResetQueryPool(commands,pool,0,limits_.per_submission);
}
void VisibilityQueries::BeginDraw(bool count,double scale) {
 if(!recording_||draw_open_)Invalid("UC-VK-003: counted draw needs an open command buffer");
 if(!std::isfinite(scale)||scale<=0)throw QueryError(QueryFailure::Conversion,"UC-VK-005: invalid samples per guest pixel");
 if(!count){draw_open_=true;active_=UINT32_MAX;return;}
 if(recording_->samples.size()>=limits_.per_submission||*live_>=limits_.live_samples)
  throw QueryError(QueryFailure::Capacity,"UC-VK-002: visibility capacity exhausted; samples were not reused");
 auto sample=std::make_shared<Sample>(live_,scale);
 recording_->samples.push_back(sample);
 try{counted_.push_back(sample);}catch(...){recording_->samples.pop_back();throw;}
 active_=uint32_t(recording_->samples.size()-1);draw_open_=true;
 vkCmdBeginQuery(recording_->commands,recording_->pool,active_,VK_QUERY_CONTROL_PRECISE_BIT);
}
void VisibilityQueries::EndDraw() {
 if(!recording_||!draw_open_)Invalid("UC-VK-003: no draw to end");
 if(active_!=UINT32_MAX)vkCmdEndQuery(recording_->commands,recording_->pool,active_);
 draw_open_=false;active_=UINT32_MAX;
}
void VisibilityQueries::Clear() {
 if(draw_open_)Invalid("UC-VK-003: counter reset inside a draw");
 counted_.clear();
}
void VisibilityQueries::Report(uint32_t address,uint64_t timestamp) {
 if(!recording_||draw_open_||(address&3)||address>UINT32_MAX-15)
  Invalid("UC-VK-003: invalid report address or recording scope");
 if(pending_.size()>=limits_.pending_reports)throw QueryError(QueryFailure::Capacity,"UC-VK-002: too many pending reports");
 if(counted_.size()>limits_.pending_sample_refs-pending_refs_)
  throw QueryError(QueryFailure::Capacity,"UC-VK-002: pending snapshot storage exhausted");
 pending_.push_back({address,timestamp,recording_->serial,counted_});
 pending_refs_+=uint32_t(counted_.size());
}
void VisibilityQueries::SealCommands() {
 if(!recording_||draw_open_)Invalid("UC-VK-003: unbalanced draw/command scope");
 recording_=nullptr;
}
void VisibilityQueries::Reap(uint64_t completed) {
 while(!batches_.empty()&&batches_.front().serial<=completed) {
  auto& batch=batches_.front();
  if(!batch.samples.empty()) {
   struct Value {uint64_t samples,available;};
   std::vector<Value> values(batch.samples.size(),{0,0});
   auto r=vkGetQueryPoolResults(device_,batch.pool,0,uint32_t(values.size()),values.size()*sizeof(Value),
       values.data(),sizeof(Value),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
   if(r!=VK_SUCCESS&&r!=VK_NOT_READY)Check(r,"UC-VK-004: query result retrieval failed");
   bool all=true;
   for(size_t n=0;n<values.size();n++) {
    if(!values[n].available){all=false;continue;}
    batch.samples[n]->samples=values[n].samples;batch.samples[n]->ready=true;
   }
   if(!all)return;
  }
  free_.push_back(batch.pool);batches_.pop_front();
 }
}
size_t VisibilityQueries::Poll(uint64_t completed,const std::function<void(const VisibilityReport&)>& writer) {
 if(!writer||completed<last_completed_||(recording_&&recording_->serial<=completed))
  Invalid("UC-VK-003: invalid completion watermark or report writer");
 last_completed_=completed;Reap(completed);size_t delivered=0;
 while(!pending_.empty()&&pending_.front().serial<=completed) {
  auto& report=pending_.front();long double pixels=0;bool ready=true;
  for(auto& sample:report.samples) {
   if(!sample->ready){ready=false;break;}
   pixels+=static_cast<long double>(sample->samples)/sample->scale;
  }
  if(!ready)break;
  if(!std::isfinite(pixels))throw QueryError(QueryFailure::Conversion,"UC-VK-005: logical pixel count overflow");
  // The Xbox counter is 32 bits. Normalize per draw, round the sum once, then wrap the counter.
  uint32_t count=uint32_t(std::fmod(std::floor(pixels+0.5L),4294967296.0L));
  writer({report.address,report.timestamp,count});
  pending_refs_-=uint32_t(report.samples.size());
  pending_.pop_front();++delivered;
 }
 return delivered;
}
}
