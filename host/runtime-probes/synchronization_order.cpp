// Actual SDK HLE bodies, authored fake commands and backend ordering only; no game inputs.
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdio>
struct Context {uint32_t esp=0;};
using Args=std::array<uint32_t,6>;
#define HLE_STDCALL(name,n) uint32_t name([[maybe_unused]]Context& ctx,[[maybe_unused]]const Args& a)
#define HLE_WRAP(name,n) void name([[maybe_unused]]Context& ctx,[[maybe_unused]]const Args& a)
#define LOGI(...) ((void)0)
std::vector<std::string> order;
std::recursive_mutex g_lock;
constexpr uint32_t kDevPush=4;
constexpr uint32_t X86CodeBase=0x4000,X86CodeEnd=0x8000;
uint32_t Dev(uint32_t at){assert(at==kDevPush);return 0x1234;}
void Walk(uint32_t end){assert(end==0x1234);order.push_back("walk");}
uint32_t R32(uint32_t at){return at==0x1000?0x40001:at==0x1004?0x2000:0;}
namespace diag {bool Perf(){return false;}}
namespace mem {uint32_t VirtualFromPhysical(uint32_t at){return at;}}
namespace gpu {void MarkDirty(uint32_t res){assert(res==0x1000);order.push_back("dirty");}}
struct Backend {
 void Sync(uint32_t data,uint32_t){assert(data==0x2000);order.push_back("sync");}
 void SyncLate(uint32_t data){assert(data==0x2000);order.push_back("sync_late");}
 void WaitResource(uint32_t){order.push_back("wait_resource");}
 bool Busy(uint32_t){order.push_back("resource_busy");return false;}
 void WaitFence(uint32_t fence){assert(fence==7);order.push_back("wait_fence");}
 uint32_t Fence(){order.push_back("fence");return 7;}
 bool DeviceBusy(){order.push_back("device_busy");return false;}
 bool FencePending(uint32_t fence){assert(fence==7);order.push_back("pending");return false;}
 void Flush(){order.push_back("flush");}
};
auto g_backend=std::make_unique<Backend>();
#include "synchronization.inc"
void expected(std::initializer_list<std::string> sequence){assert(order==std::vector<std::string>(sequence));order.clear();}
int main(){
 Context ctx;Args a{};
 assert(D3DDevice_InsertFence(ctx,a)==7);expected({"walk","fence"});
 assert(D3D_SetFence(ctx,a)==7);expected({"walk","fence"});
 D3D_KickOffAndWaitForIdle(ctx,a);expected({"walk","flush"});
 D3DDevice_IsBusy(ctx,a);expected({"walk","device_busy"});
 a[0]=7;D3DDevice_BlockOnFence(ctx,a);expected({"wait_fence"});D3DDevice_IsFencePending(ctx,a);expected({"pending"});
 a[0]=0x1000;D3D_BlockOnResource(ctx,a);expected({"walk","sync","dirty"});
 D3DResource_BlockUntilNotBusy(ctx,a);expected({"walk","wait_resource","dirty"});
 D3DResource_IsBusy(ctx,a);expected({"walk","resource_busy"});
 a[5]=0x20;Lock2DSurface(ctx,a);expected({"walk","sync_late","dirty"});
 std::puts("GPU_ORDERING: pending commands consumed before resource access, fence and idle; no-overwrite lock invalidates texture cache; authored spies only");
}
