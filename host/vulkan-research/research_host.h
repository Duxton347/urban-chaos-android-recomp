#pragma once
// Cloud-only host adapter. No Android lifecycle or controller behavior is simulated.
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include "app/app.h"
#include "core/log.h"
#include "harness.h"
namespace host {
inline bool Foreground(){return true;}
inline uint32_t SurfaceEpoch(){return 1;}
inline void Presented(uint64_t frame){harness::g_frames.store(frame);}
inline void WindowPixels(int& w,int& h){w=640;h=480;}
inline uint64_t Parameter(const char* name) { const char* text=std::getenv(name);if(!text)return 0;char* end=nullptr;auto n=std::strtoull(text,&end,10);return end&&!*end?n:0; }
inline bool CaptureDue(uint64_t frame){auto interval=Parameter("URBAN_VK_CAPTURE_EVERY");return interval&&frame>=Parameter("URBAN_VK_CAPTURE_FROM")&&frame%interval==0;}
inline void Capture(std::span<const uint8_t> pixels,uint32_t w,uint32_t h,uint64_t frame){
 if(!w||!h||w>16384||h>16384||pixels.size()!=uint64_t(w)*h*4)throw std::runtime_error("UC-VK-007: invalid capture extent");
 auto folder=app::ExeDir()/"cap";std::filesystem::create_directories(folder);
 auto path=folder/("vulkan_"+std::to_string(frame)+".tga");
 uint8_t header[18]{};header[2]=2;header[12]=uint8_t(w);header[13]=uint8_t(w>>8);header[14]=uint8_t(h);header[15]=uint8_t(h>>8);header[16]=32;header[17]=0x28;
 std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(header),18);file.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
 if(!file)throw std::runtime_error("UC-VK-007: capture write failed");
 LOGI("vulkan: original target captured at frame %llu, %ux%u",(unsigned long long)frame,w,h);
}
}
namespace dynres {inline void Frame(){} }
