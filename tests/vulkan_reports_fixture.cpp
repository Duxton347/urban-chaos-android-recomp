// Real Vulkan rasterization/queries of authored geometry; no title assets or game execution.
#include "gpu/vulkan_visibility.h"
#include "gpu/visibility_report.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <source_location>
#include <string>
#include <vector>
using namespace urban::vulkan;
static void Require(bool value,const std::source_location& at=std::source_location::current()) {
 if(!value)throw std::runtime_error("Vulkan fixture assertion at line "+std::to_string(at.line()));
}
static void Ok(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan fixture VkResult "+std::to_string(r));}
static std::vector<uint32_t> Read(const char* path) {
 std::ifstream f(path,std::ios::binary|std::ios::ate);Require(bool(f));auto bytes=f.tellg();Require(bytes>0&&bytes%4==0);
 std::vector<uint32_t> data(size_t(bytes)/4);f.seekg(0);Require(bool(f.read(reinterpret_cast<char*>(data.data()),bytes)));return data;
}
struct Image {VkImage image=VK_NULL_HANDLE;VkDeviceMemory memory=VK_NULL_HANDLE;VkImageView view=VK_NULL_HANDLE;};
struct Fixture {
 VkInstance instance=VK_NULL_HANDLE;VkPhysicalDevice physical=VK_NULL_HANDLE;VkDevice device=VK_NULL_HANDLE;
 VkQueue queue=VK_NULL_HANDLE;uint32_t family=0;VkCommandPool commands=VK_NULL_HANDLE;
 VkRenderPass pass=VK_NULL_HANDLE;VkFramebuffer framebuffer=VK_NULL_HANDLE;Image color,depth;
 VkPipelineLayout layout=VK_NULL_HANDLE;VkPipeline masked=VK_NULL_HANDLE,normal=VK_NULL_HANDLE;
 VkPhysicalDeviceProperties properties{};VkPhysicalDeviceFeatures features{};
 uint64_t completed=0;
 Image MakeImage(VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect) {
  Image result;VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;
  info.format=format;info.extent={8,8,1};info.mipLevels=1;info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;
  info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=usage;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
  Ok(vkCreateImage(device,&info,nullptr,&result.image));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,result.image,&req);
  VkPhysicalDeviceMemoryProperties memory;vkGetPhysicalDeviceMemoryProperties(physical,&memory);uint32_t type=UINT32_MAX;
  for(uint32_t n=0;n<memory.memoryTypeCount;n++)if(req.memoryTypeBits&(1u<<n)){type=n;break;}Require(type!=UINT32_MAX);
  VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=req.size;allocation.memoryTypeIndex=type;
  Ok(vkAllocateMemory(device,&allocation,nullptr,&result.memory));Ok(vkBindImageMemory(device,result.image,result.memory,0));
  VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=result.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
  view.format=format;view.subresourceRange={aspect,0,1,0,1};Ok(vkCreateImageView(device,&view,nullptr,&result.view));return result;
 }
 VkPipeline MakePipeline(VkShaderModule vertex,VkShaderModule fragment,bool write) {
  VkPipelineShaderStageCreateInfo stages[2]={{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};
  stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vertex;stages[0].pName="main";
  stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fragment;stages[1].pName="main";
  VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkViewport viewport{0,0,8,8,0,1};VkRect2D scissor{{0,0},{8,8}};
  VkPipelineViewportStateCreateInfo view{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};view.viewportCount=1;view.pViewports=&viewport;view.scissorCount=1;view.pScissors=&scissor;
  VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
  VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
  VkPipelineDepthStencilStateCreateInfo z{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};z.depthTestEnable=VK_TRUE;z.depthWriteEnable=VK_TRUE;z.depthCompareOp=VK_COMPARE_OP_LESS;z.maxDepthBounds=1;
  VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=write?15:0;
  VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
  VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pipeline.stageCount=2;pipeline.pStages=stages;
  pipeline.pVertexInputState=&input;pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&view;
  pipeline.pRasterizationState=&raster;pipeline.pMultisampleState=&samples;pipeline.pDepthStencilState=&z;
  pipeline.pColorBlendState=&blend;pipeline.layout=layout;pipeline.renderPass=pass;
  VkPipeline result;Ok(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipeline,nullptr,&result));return result;
 }
 Fixture(const char* vertex_path,const char* fragment_path) {
  VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
  VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;Ok(vkCreateInstance(&create,nullptr,&instance));
  uint32_t count=0;Ok(vkEnumeratePhysicalDevices(instance,&count,nullptr));Require(count>0);std::vector<VkPhysicalDevice> devices(count);Ok(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
  physical=devices[0];vkGetPhysicalDeviceProperties(physical,&properties);vkGetPhysicalDeviceFeatures(physical,&features);Require(features.occlusionQueryPrecise);
  vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,nullptr);std::vector<VkQueueFamilyProperties> queues(count);vkGetPhysicalDeviceQueueFamilyProperties(physical,&count,queues.data());
  bool found=false;for(uint32_t n=0;n<count;n++)if(queues[n].queueFlags&VK_QUEUE_GRAPHICS_BIT){family=n;found=true;break;}Require(found);
  float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
  VkPhysicalDeviceFeatures enabled{};enabled.occlusionQueryPrecise=VK_TRUE;
  VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;dc.pEnabledFeatures=&enabled;
  Ok(vkCreateDevice(physical,&dc,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
  VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=family;pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;Ok(vkCreateCommandPool(device,&pool,nullptr,&commands));
  VkFormatProperties support;vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_D32_SFLOAT,&support);Require(support.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  color=MakeImage(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
  depth=MakeImage(VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
  VkAttachmentDescription attachments[2]{};
  for(auto& a:attachments){a.samples=VK_SAMPLE_COUNT_1_BIT;a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;}
  attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;attachments[0].finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachments[1].format=VK_FORMAT_D32_SFLOAT;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  VkAttachmentReference colors{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},zs{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&colors;subpass.pDepthStencilAttachment=&zs;
  VkSubpassDependency dependency{};dependency.srcSubpass=VK_SUBPASS_EXTERNAL;dependency.dstSubpass=0;
  dependency.srcStageMask=dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstAccessMask=dependency.srcAccessMask|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&subpass;rp.dependencyCount=1;rp.pDependencies=&dependency;Ok(vkCreateRenderPass(device,&rp,nullptr,&pass));
  VkImageView views[2]={color.view,depth.view};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=pass;fb.attachmentCount=2;fb.pAttachments=views;fb.width=8;fb.height=8;fb.layers=1;Ok(vkCreateFramebuffer(device,&fb,nullptr,&framebuffer));
  VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,8};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&range;Ok(vkCreatePipelineLayout(device,&pl,nullptr,&layout));
  auto vert=Read(vertex_path),frag=Read(fragment_path);VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};VkShaderModule v,f;
  sm.codeSize=vert.size()*4;sm.pCode=vert.data();Ok(vkCreateShaderModule(device,&sm,nullptr,&v));sm.codeSize=frag.size()*4;sm.pCode=frag.data();Ok(vkCreateShaderModule(device,&sm,nullptr,&f));
  normal=MakePipeline(v,f,true);masked=MakePipeline(v,f,false);vkDestroyShaderModule(device,v,nullptr);vkDestroyShaderModule(device,f,nullptr);
 }
 VkCommandBuffer Begin(){VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=commands;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;VkCommandBuffer cb;Ok(vkAllocateCommandBuffers(device,&ai,&cb));VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;Ok(vkBeginCommandBuffer(cb,&bi));return cb;}
 void Pass(VkCommandBuffer cb){VkClearValue clear[2]{};clear[1].depthStencil.depth=1;VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=framebuffer;begin.renderArea={{0,0},{8,8}};begin.clearValueCount=2;begin.pClearValues=clear;vkCmdBeginRenderPass(cb,&begin,VK_SUBPASS_CONTENTS_INLINE);}
 void Draw(VkCommandBuffer cb,VisibilityQueries& queries,float z,bool discard=false,bool write=true,bool count=true,double scale=1){
  vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,write?normal:masked);struct Push{float z;uint32_t discard;}push{z,discard?1u:0u};
  vkCmdPushConstants(cb,layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,8,&push);
  queries.BeginDraw(count,scale);vkCmdDraw(cb,3,1,0,0);queries.EndDraw();
 }
 void Submit(VkCommandBuffer cb,uint64_t serial){Ok(vkEndCommandBuffer(cb));VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkFence fence;Ok(vkCreateFence(device,&fi,nullptr,&fence));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cb;Ok(vkQueueSubmit(queue,1,&submit,fence));Ok(vkWaitForFences(device,1,&fence,VK_TRUE,10000000000ull));vkDestroyFence(device,fence,nullptr);completed=serial;}
 ~Fixture(){if(device){vkDeviceWaitIdle(device);vkDestroyPipeline(device,normal,nullptr);vkDestroyPipeline(device,masked,nullptr);vkDestroyPipelineLayout(device,layout,nullptr);vkDestroyFramebuffer(device,framebuffer,nullptr);vkDestroyRenderPass(device,pass,nullptr);for(auto p:{color,depth}){vkDestroyImageView(device,p.view,nullptr);vkDestroyImage(device,p.image,nullptr);vkFreeMemory(device,p.memory,nullptr);}vkDestroyCommandPool(device,commands,nullptr);vkDestroyDevice(device,nullptr);}if(instance)vkDestroyInstance(instance,nullptr);}
};
template<class F> static void Reject(QueryFailure reason,F operation) {
 try{operation();throw std::runtime_error("Expected QueryError");}catch(const QueryError& error){Require(error.reason==reason);}
}
int main(int argc,char** argv) {
 try {
  Require(argc==3);Fixture f(argv[1],argv[2]);
  Reject(QueryFailure::PreciseUnavailable,[&]{VisibilityQueries unsupported(f.device,false);});
  VisibilityQueries q(f.device,true,{8,2,64,16});
  std::map<uint32_t,std::array<uint8_t,20>> mapped;std::vector<uint32_t> actual;uint32_t next=0x100;
  auto target=[&]{uint32_t address=next;next+=32;mapped[address].fill(0xCD);return address;};
  auto writer=[&](const VisibilityReport& report){
   auto& bytes=mapped.at(report.address);PublishReport(bytes,report.timestamp,report.pixels);
   uint32_t low,high,count,status;std::memcpy(&low,bytes.data(),4);std::memcpy(&high,bytes.data()+4,4);std::memcpy(&count,bytes.data()+8,4);std::memcpy(&status,bytes.data()+12,4);
   Require(low==uint32_t(report.timestamp)&&high==uint32_t(report.timestamp>>32));Require(count==report.pixels&&status==0);
   for(size_t n=16;n<20;n++)Require(bytes[n]==0xCD);actual.push_back(count);
  };
  auto untouched=[&]{for(auto& [address,bytes]:mapped)if(bytes[12]==0xCD)for(auto byte:bytes)Require(byte==0xCD);};
  constexpr uint64_t timestamp=0x1234567800000001ull;
  auto cb=f.Begin();q.BeginCommands(cb,1);f.Pass(cb);
  q.Clear();f.Draw(cb,q,0.5f,false,false);q.Report(target(),timestamp); // color writes off, still 64 passing samples
  q.Clear();f.Draw(cb,q,0.5f);q.Report(target(),timestamp); // LESS rejects equal depth
  q.Clear();f.Draw(cb,q,0.75f);q.Report(target(),timestamp); // fully hidden
  q.Clear();f.Draw(cb,q,0.25f,true,false);q.Report(target(),timestamp); // discard even with color writes off
  q.Clear();f.Draw(cb,q,0.4f);q.Report(target(),timestamp); // discarded fragments did not update depth
  vkCmdEndRenderPass(cb);q.SealCommands();Require(q.Poll(0,writer)==0);untouched();
  f.Submit(cb,1);Require(q.Poll(f.completed,writer)==5);Require((actual==std::vector<uint32_t>{64,0,0,0,64}));
  // Recycled GPU pool must not overwrite the cached value still referenced since ClearReport.
  cb=f.Begin();q.BeginCommands(cb,2);f.Pass(cb);f.Draw(cb,q,0.5f,false,true,true,2);
  q.Report(target(),timestamp+1);q.Report(target(),timestamp+1); // 64 cached + 64/2 = 96, repeated snapshot
  q.Clear();f.Draw(cb,q,0.25f);q.Report(target(),timestamp+1);q.Clear();q.Report(target(),timestamp+1);
  vkCmdEndRenderPass(cb);q.SealCommands();Require(q.Poll(1,writer)==0);untouched();
  f.Submit(cb,2);Require(q.Poll(f.completed,writer)==4);
  Require((actual==std::vector<uint32_t>{64,0,0,0,64,96,96,64,0}));
  cb=f.Begin();q.BeginCommands(cb,3);f.Pass(cb);
  q.Clear();f.Draw(cb,q,0.6f,false,true,false);q.Report(target(),timestamp+2);
  q.Clear();f.Draw(cb,q,0.3f);q.Report(target(),timestamp+2);vkCmdEndRenderPass(cb);
  q.Clear();for(int n=0;n<3;n++){f.Pass(cb);f.Draw(cb,q,0.5f,false,true,true,128);vkCmdEndRenderPass(cb);}
  q.Report(target(),timestamp+2); // normalization sums 0.5 three times and rounds once to 2
  q.Clear();f.Pass(cb);f.Draw(cb,q,0.5f,false,true,true,64.0/(4294967296.0+7));vkCmdEndRenderPass(cb);q.Report(target(),timestamp+2);
  q.SealCommands();Require(q.Poll(2,writer)==0);untouched();f.Submit(cb,3);
  size_t pending=q.PendingReports();
  try{q.Poll(f.completed,[](const VisibilityReport&){throw std::runtime_error("authored writer failure");});Require(false);}
  catch(const std::runtime_error& error){Require(std::string(error.what())=="authored writer failure");}
  Require(q.PendingReports()==pending);Require(q.Poll(f.completed,writer)==4);Require(q.Poll(f.completed,writer)==0);
  Require((actual==std::vector<uint32_t>{64,0,0,0,64,96,96,64,0,0,64,2,7}));q.Clear();Require(q.LiveSamples()==0);
  Reject(QueryFailure::Lifecycle,[&]{q.Poll(2,writer);});
  Reject(QueryFailure::Lifecycle,[&]{q.BeginCommands(f.Begin(),3);});
  {
   VisibilityQueries small(f.device,true,{1,1,1,1});cb=f.Begin();small.BeginCommands(cb,4);f.Pass(cb);
   small.BeginDraw(false);Reject(QueryFailure::Lifecycle,[&]{small.Clear();});Reject(QueryFailure::Lifecycle,[&]{small.Report(target(),0);});Reject(QueryFailure::Lifecycle,[&]{small.SealCommands();});small.EndDraw();
   Reject(QueryFailure::Conversion,[&]{small.BeginDraw(true,std::numeric_limits<double>::quiet_NaN());});
   f.Draw(cb,small,0.5f);Reject(QueryFailure::Capacity,[&]{small.BeginDraw(true);});small.Report(target(),0);
   Reject(QueryFailure::Capacity,[&]{small.Report(target(),0);});vkCmdEndRenderPass(cb);small.SealCommands();
   auto next_cb=f.Begin();Reject(QueryFailure::Capacity,[&]{small.BeginCommands(next_cb,5);});
   f.Submit(cb,4);Require(small.Poll(f.completed,writer)==1);small.Clear();
   small.BeginCommands(next_cb,5);small.Report(target(),0);small.SealCommands();Require(small.Poll(4,writer)==0);
   f.Submit(next_cb,5);Require(small.Poll(f.completed,writer)==1);Require(small.LiveSamples()==0);
  }
  Require(actual[13]==64&&actual[14]==0);
  {
   VisibilityQueries dual(f.device,true,{2,2,8,4});
   auto first=f.Begin();dual.BeginCommands(first,6);f.Pass(first);dual.Clear();
   f.Draw(first,dual,0.5f);dual.Report(target(),42);vkCmdEndRenderPass(first);dual.SealCommands();
   auto second=f.Begin();dual.BeginCommands(second,7);f.Pass(second);dual.Clear();
   f.Draw(second,dual,0.25f,false,true,false);f.Draw(second,dual,0.75f);
   dual.Report(target(),43);vkCmdEndRenderPass(second);dual.SealCommands();
   Require(dual.Poll(5,writer)==0);untouched();f.Submit(first,6);
   Require(dual.Poll(5,writer)==0);Require(dual.Poll(f.completed,writer)==1);
   Require(dual.PendingReports()==1);f.Submit(second,7);Require(dual.Poll(f.completed,writer)==1);
   Require(actual[15]==64&&actual[16]==0);dual.Clear();Require(dual.LiveSamples()==0);
  }
  {
   VisibilityQueries retained(f.device,true,{2,1,1,2});
   cb=f.Begin();retained.BeginCommands(cb,8);f.Pass(cb);f.Draw(cb,retained,0.5f);
   retained.Report(target(),44);vkCmdEndRenderPass(cb);retained.SealCommands();
   f.Submit(cb,8);Require(retained.Poll(f.completed,writer)==1);Require(retained.LiveSamples()==1);
   cb=f.Begin();retained.BeginCommands(cb,9);f.Pass(cb);
   Reject(QueryFailure::Capacity,[&]{retained.BeginDraw(true);});
   retained.Report(target(),45);vkCmdEndRenderPass(cb);retained.SealCommands();
   f.Submit(cb,9);Require(retained.Poll(f.completed,writer)==1);retained.Clear();Require(retained.LiveSamples()==0);
   Require(actual[17]==64&&actual[18]==64);
  }
  {
   VisibilityQueries snapshots(f.device,true,{2,1,2,4,1});
   cb=f.Begin();snapshots.BeginCommands(cb,10);f.Pass(cb);f.Draw(cb,snapshots,0.5f);
   snapshots.Report(target(),46);Reject(QueryFailure::Capacity,[&]{snapshots.Report(target(),47);});
   snapshots.Clear();snapshots.Report(target(),48);vkCmdEndRenderPass(cb);snapshots.SealCommands();
   f.Submit(cb,10);Require(snapshots.Poll(f.completed,writer)==2);Require(snapshots.LiveSamples()==0);
   Require(actual[19]==64&&actual[20]==0);
  }
  std::array<uint8_t,15> short_record;short_record.fill(0xA5);
  try{PublishReport(short_record,0,64);Require(false);}catch(const std::invalid_argument&){}
  for(auto byte:short_record)Require(byte==0xA5);
  std::string name;for(char c:std::string(f.properties.deviceName)){if(c=='"'||c=='\\')name+='\\';name+=(c>=32?c:'?');}
  std::printf("{\"device_name\":\"%s\",\"status\":\"PASS\",\"device_type\":%u,\"api_version\":%u,\"driver_version\":%u,\"precise_enabled\":true,\"reports\":%zu,\"visible\":64,\"hidden\":0,\"discarded\":0,\"gpu_pool_reuse\":true,\"snapshot_clear\":true,\"pending_before_submission\":true,\"counter_normalization\":true,\"counter_wrap\":true,\"capacity_rejected\":true,\"writer_retry\":true,\"record_bytes\":16,\"game_data_used\":false,\"android_device_tested\":false}\n",name.c_str(),f.properties.deviceType,f.properties.apiVersion,f.properties.driverVersion,actual.size());
  return 0;
 }catch(const std::exception& error){std::fprintf(stderr,"Vulkan report fixture failed: %s\n",error.what());return 2;}
}
