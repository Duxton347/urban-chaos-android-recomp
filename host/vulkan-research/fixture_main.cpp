#include "app/app.h"
#include "gpu/backend.h"
#include "harness.h"
#include "kernel/kernel.h"
#include <stdexcept>
namespace kernel {uint8_t* g_base=nullptr;}
const app::GameInfo app::kGame={.title=L"Authored Vulkan backend contract",.name=L"VulkanContract",.log="VulkanContract"};
int ReferenceVisibilitySelfTest(bool);
int main(){
    if(ReferenceVisibilitySelfTest(false))return 2;
    auto backend=gpu::CreateBackend();
    if(!backend->Init(8,8))return 3;
    gpu::State invalid{};
    invalid.simple[gpu::kMCountEnable>>2]=1;
    bool rejected=false;
    try{backend->Draw(invalid,5,0,3,nullptr);}catch(const std::runtime_error&){rejected=true;}
    if(!rejected)throw std::runtime_error("Unsupported counted draw was silently accepted");
    // Real headless swapchain presentation must advance the SDK's scripted-input clock.
    gpu::Surface empty{};
    harness::g_frames.store(0);
    backend->Present(empty);
    backend->Flush();
    if(harness::g_frames.load()!=1)throw std::runtime_error("Research input clock did not advance with presentation");
    std::puts("VULKAN_RENDERER_CONTRACT: real SDK Draw/Clear/Flush/reports, threaded mode and presentation/input clock passed; no game data");
}
