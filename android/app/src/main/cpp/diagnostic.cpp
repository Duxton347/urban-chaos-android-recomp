// Original diagnostic only: no guest code, addresses, game hooks or content.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <string>
#include <cmath>
static void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan result "+std::to_string(r));}
struct Diagnostic {
 SDL_Window* window=nullptr; VkInstance instance{};VkSurfaceKHR surface{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};uint32_t family{};
 VkSwapchainKHR swap{};VkCommandPool pool{};VkCommandBuffer cmd{};VkSemaphore acquired{},drawn{};VkFence fence{};
 std::vector<VkImage> images;std::vector<bool> initialized;VkExtent2D extent{};std::ofstream log;
 void createSwap(){
  check(vkDeviceWaitIdle(device));
  VkSurfaceCapabilitiesKHR cap;check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&cap));
  uint32_t n=0;check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&n,nullptr));std::vector<VkSurfaceFormatKHR> formats(n);check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&n,formats.data()));
  if(!n || !(cap.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))throw std::runtime_error("Surface cannot display diagnostic transfer frames");
  extent=cap.currentExtent;
  if(extent.width==UINT32_MAX){int w,h;SDL_GetWindowSizeInPixels(window,&w,&h);extent={uint32_t(w),uint32_t(h)};extent.width=SDL_clamp(extent.width,cap.minImageExtent.width,cap.maxImageExtent.width);extent.height=SDL_clamp(extent.height,cap.minImageExtent.height,cap.maxImageExtent.height);}
  VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};ci.surface=surface;ci.minImageCount=cap.minImageCount+1;if(cap.maxImageCount&&ci.minImageCount>cap.maxImageCount)ci.minImageCount=cap.maxImageCount;
  auto fmt=formats[0];if(fmt.format==VK_FORMAT_UNDEFINED)fmt.format=VK_FORMAT_B8G8R8A8_UNORM;
  ci.imageFormat=fmt.format;ci.imageColorSpace=fmt.colorSpace;ci.imageExtent=extent;ci.imageArrayLayers=1;ci.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;ci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;ci.preTransform=cap.currentTransform;
  for(auto alpha:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR})if(cap.supportedCompositeAlpha&alpha){ci.compositeAlpha=alpha;break;}
  ci.presentMode=VK_PRESENT_MODE_FIFO_KHR;ci.clipped=VK_TRUE;ci.oldSwapchain=swap;VkSwapchainKHR next;check(vkCreateSwapchainKHR(device,&ci,nullptr,&next));if(swap)vkDestroySwapchainKHR(device,swap,nullptr);swap=next;
  check(vkGetSwapchainImagesKHR(device,swap,&n,nullptr));images.resize(n);check(vkGetSwapchainImagesKHR(device,swap,&n,images.data()));initialized.assign(n,false);log<<"swapchain "<<extent.width<<'x'<<extent.height<<std::endl;
 }
 void init(){
  char* dir=SDL_GetPrefPath("UrbanChaos","Diagnostic");if(dir){log.open(std::string(dir)+"logs/vulkan.log",std::ios::app);SDL_free(dir);}
  if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD))throw std::runtime_error(SDL_GetError());
  window=SDL_CreateWindow("DIAGNÓSTICO — sem execução do jogo",1280,800,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);if(!window)throw std::runtime_error(SDL_GetError());
  Uint32 n=0;auto extensions=SDL_Vulkan_GetInstanceExtensions(&n);VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Urban Chaos Diagnostic";app.apiVersion=VK_API_VERSION_1_0;
  VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledExtensionCount=n;ci.ppEnabledExtensionNames=extensions;check(vkCreateInstance(&ci,nullptr,&instance));
  if(!SDL_Vulkan_CreateSurface(window,instance,nullptr,&surface))throw std::runtime_error(SDL_GetError());
  uint32_t count=0;check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devs(count);check(vkEnumeratePhysicalDevices(instance,&count,devs.data()));
  for(auto p:devs){uint32_t nf=0;vkGetPhysicalDeviceQueueFamilyProperties(p,&nf,nullptr);std::vector<VkQueueFamilyProperties> props(nf);vkGetPhysicalDeviceQueueFamilyProperties(p,&nf,props.data());for(uint32_t f=0;f<nf;f++){VkBool32 support=0;check(vkGetPhysicalDeviceSurfaceSupportKHR(p,f,surface,&support));if(support&&(props[f].queueFlags&VK_QUEUE_GRAPHICS_BIT)){physical=p;family=f;break;}}if(physical)break;}
  if(!physical)throw std::runtime_error("No Vulkan presentation queue");
  VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(physical,&props);log<<"device "<<props.deviceName<<" driver "<<props.driverVersion<<" api "<<props.apiVersion<<std::endl;
  float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
  const char* ext=VK_KHR_SWAPCHAIN_EXTENSION_NAME;VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.enabledExtensionCount=1;di.ppEnabledExtensionNames=&ext;check(vkCreateDevice(physical,&di,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
  VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pi.queueFamilyIndex=family;pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pi,nullptr,&pool));
  VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ai.commandPool=pool;ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ai.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ai,&cmd));
  VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};check(vkCreateSemaphore(device,&si,nullptr,&acquired));check(vkCreateSemaphore(device,&si,nullptr,&drawn));VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fi.flags=VK_FENCE_CREATE_SIGNALED_BIT;check(vkCreateFence(device,&fi,nullptr,&fence));createSwap();
 }
 void frame(){
  check(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX));uint32_t i;VkResult r=vkAcquireNextImageKHR(device,swap,UINT64_MAX,acquired,VK_NULL_HANDLE,&i);if(r==VK_ERROR_OUT_OF_DATE_KHR){createSwap();return;}if(r!=VK_SUBOPTIMAL_KHR)check(r);
  check(vkResetFences(device,1,&fence));check(vkResetCommandBuffer(cmd,0));VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(cmd,&bi));
  VkImageMemoryBarrier bar{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};bar.oldLayout=initialized[i]?VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:VK_IMAGE_LAYOUT_UNDEFINED;bar.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;bar.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;bar.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;bar.image=images[i];bar.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};bar.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
  vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&bar);
  float t=float(SDL_GetTicks()%5000)/5000;VkClearColorValue color{{t,0.15f,1-t,1}};vkCmdClearColorImage(cmd,images[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&bar.subresourceRange);
  bar.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;bar.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;bar.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;bar.dstAccessMask=0;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&bar);check(vkEndCommandBuffer(cmd));
  VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TRANSFER_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;submit.pWaitDstStageMask=&stage;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&drawn;check(vkQueueSubmit(queue,1,&submit,fence));
  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&drawn;present.swapchainCount=1;present.pSwapchains=&swap;present.pImageIndices=&i;r=vkQueuePresentKHR(queue,&present);check(vkQueueWaitIdle(queue));initialized[i]=true;
  if(r==VK_SUBOPTIMAL_KHR||r==VK_ERROR_OUT_OF_DATE_KHR)createSwap();else check(r);
 }
 ~Diagnostic(){if(device){vkDeviceWaitIdle(device);if(fence)vkDestroyFence(device,fence,nullptr);if(acquired)vkDestroySemaphore(device,acquired,nullptr);if(drawn)vkDestroySemaphore(device,drawn,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);if(swap)vkDestroySwapchainKHR(device,swap,nullptr);vkDestroyDevice(device,nullptr);}if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);if(instance)vkDestroyInstance(instance,nullptr);if(window)SDL_DestroyWindow(window);SDL_Quit();}
};
int main(int,char**){Diagnostic d;try{d.init();bool run=true,paused=false;while(run){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_EVENT_QUIT)run=false;if(e.type==SDL_EVENT_WILL_ENTER_BACKGROUND){paused=true;check(vkDeviceWaitIdle(d.device));d.log<<"suspend"<<std::endl;}if(e.type==SDL_EVENT_DID_ENTER_FOREGROUND){paused=false;d.createSwap();d.log<<"resume"<<std::endl;}if(e.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED&&!paused)d.createSwap();}if(paused)SDL_Delay(30);else if(run)d.frame();}return 0;}catch(const std::exception& e){d.log<<"ERROR "<<e.what()<<std::endl;SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"%s",e.what());SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Diagnóstico Vulkan",e.what(),d.window);return 1;}}
