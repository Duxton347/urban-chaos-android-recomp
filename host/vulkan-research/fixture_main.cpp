#include "app/app.h"
#include "gpu/backend.h"
#include "harness.h"
#include "kernel/kernel.h"
#include <stdexcept>
#include "diagnostics/progress.h"
namespace kernel {uint8_t* g_base=nullptr;}
const app::GameInfo app::kGame={.title=L"Authored Vulkan backend contract",.name=L"VulkanContract",.log="VulkanContract"};
int ReferenceVisibilitySelfTest(bool);
int ReferenceVisibilitySelfTest(bool,bool);
int main(){
    if(ReferenceVisibilitySelfTest(false)||ReferenceVisibilitySelfTest(false,true))return 2;
    auto backend=gpu::CreateBackend();
    if(!backend->Init(8,8))return 3;
    gpu::State invalid{};
    invalid.simple[gpu::kMCountEnable>>2]=1;
    bool rejected=false;
    try{backend->Draw(invalid,5,0,3,nullptr);}catch(const std::runtime_error&){rejected=true;}
    if(!rejected)throw std::runtime_error("Unsupported counted draw was silently accepted");
    // Repeated completed-result/idle polls must not produce empty GPU submissions or presentations.
    uint64_t submissions=urban::progress::gpu[7].calls.load();
    harness::g_frames.store(0);
    for(int i=0;i<128;i++){backend->FlushVisibilityReports();backend->Flush();}
    if(urban::progress::gpu[7].calls.load()!=submissions||harness::g_frames.load()!=0)throw std::runtime_error("Idle result polls submitted GPU work or advanced presentation");
    // A fence must submit pending report work and wait for its actual publication.
    uint32_t report=mem::AllocSystem(20);kernel::W32(report+12,0xFFFFFFFF);kernel::W32(report+16,0xC0DEC0DE);
    backend->ClearReport();backend->GetReport(report);
    uint64_t before_fence=urban::progress::gpu[7].calls.load();uint32_t fence=backend->Fence();
    if(urban::progress::gpu[7].calls.load()!=before_fence+1)throw std::runtime_error("Fence did not submit its pending work");
    backend->WaitFence(fence);
    if(backend->FencePending(fence)||backend->DeviceBusy()||kernel::R32(report+12)!=0||kernel::R32(report+8)!=0||kernel::R32(report+16)!=0xC0DEC0DE)throw std::runtime_error("GPU fence completion/report publication mismatch");
    // Real headless swapchain presentation must advance the SDK's scripted-input clock.
    gpu::Surface empty{};
    harness::g_frames.store(0);
    backend->Present(empty);
    backend->Flush();
    if(harness::g_frames.load()!=1)throw std::runtime_error("Research input clock did not advance with presentation");
    std::puts("VULKAN_RENDERER_CONTRACT: real SDK Draw/Clear/Flush/reports, threaded mode and presentation/input clock passed; no game data");
}
