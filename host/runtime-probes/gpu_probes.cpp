// Synthetic tests of actual SDK queue/vertex code; no image symbols, guest code or game data.
#include "gpu/backend.h"
#include "gpu/vertex.h"
#include "kernel/kernel.h"
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

namespace {
struct Gate {
    std::mutex mutex;
    std::condition_variable cv;
    bool started=false, released=false;
};
class BlockingBackend final : public gpu::Backend {
public:
    explicit BlockingBackend(Gate& gate):gate_(gate) {}
    bool Init(uint32_t,uint32_t) override { return true; }
    void Clear(const gpu::State&,uint32_t,const uint8_t*,uint32_t,uint32_t,float,uint32_t) override {}
    void Draw(const gpu::State&,uint32_t,uint32_t,uint32_t,const uint8_t*) override {
        std::unique_lock lock(gate_.mutex);
        gate_.started=true; gate_.cv.notify_all();
        gate_.cv.wait(lock,[&]{return gate_.released;});
    }
    void Copy(const gpu::Surface&,const gpu::Surface&,uint32_t,uint32_t,uint32_t) override {}
    void Present(const gpu::Surface&) override {}
    void Sync(uint32_t,uint32_t) override {}
    void SyncLate(uint32_t) override {}
    void Flush() override {}
    void SetGamma(const uint8_t*) override {}
private:
    Gate& gate_;
};
}
int BackendBusySelfTest() {
    if (!mem::Init()) return 2;
    kernel::g_base=mem::Base();
    Gate gate;
    auto backend=gpu::CreateThreadedBackend(std::make_unique<BlockingBackend>(gate));
    if (!backend->Init(1,1) || backend->DeviceBusy()) return 3;
    backend->Draw(gpu::State{},4,0,3,nullptr);
    bool pending=backend->DeviceBusy(); // must submit, not wait or report completion
    bool started;
    {
        std::unique_lock lock(gate.mutex);
        started=gate.cv.wait_for(lock,std::chrono::seconds(2),[&]{return gate.started;});
        gate.released=true;
    }
    gate.cv.notify_all();
    if (!pending || !started) return 4;
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while (backend->DeviceBusy() && std::chrono::steady_clock::now()<deadline) std::this_thread::yield();
    if (backend->DeviceBusy()) return 5;
    std::puts("BACKEND_BUSY_SELFTEST: empty queue idle, queued draw pending without blocking, completed draw idle; actual SDK worker, no title instructions");
    std::fflush(stdout);
    return 0;
}

int VertexModeSelfTest() {
    if (!mem::Init()) return 2;
    kernel::g_base=mem::Base();
    uint32_t data=mem::AllocContiguous(4096,0,mem::kPhysicalSize-1,4096), vb=mem::AllocSystem(20);
    float positions[]={17,23,0.5f,0.25f, 18,23,0.5f,0.5f, 17,24,0.5f,1};
    std::memcpy(kernel::HostPtr(data),positions,sizeof(positions));
    kernel::W32(vb,1);kernel::W32(vb+4,mem::PhysicalFromVirtual(data));
    gpu::State state{};
    state.vs_handle=1;
    state.vs=std::make_shared<gpu::VertexShader>();
    state.vs->inputs[0]={0,0,0x42};state.vs->stream_stride[0]=16;
    state.vs->pretransformed=true;
    state.streams[0]={vb,16};
    gpu::Assembled result;
    if(!gpu::Assemble(state,4,0,3,nullptr,result) || !result.pretransformed) return 3;
    if(result.vertices.size()!=12 || std::memcmp(result.vertices.data(),positions,sizeof(positions))) return 4;
    state.vs->pretransformed=false;
    if(!gpu::Assemble(state,4,0,3,nullptr,result) || result.pretransformed) return 5;
    state.vs_handle=4;state.vs.reset();
    if(!gpu::Assemble(state,4,0,3,nullptr,result) || !result.pretransformed) return 6;
    std::puts("VERTEX_MODE_SELFTEST: expanded immediate XYZRHW preserves position/RHW and screen-space mode; programmable and FVF modes remain distinct; no title instructions");
    std::fflush(stdout);return 0;
}
