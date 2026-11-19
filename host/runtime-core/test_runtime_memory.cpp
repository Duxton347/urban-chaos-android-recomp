// Exercise actual SDK memory services. No title code, replacement logic or fake imports.
#include "kernel/memory.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
int main() {
    if (!mem::Init()) return 1;
    uint32_t address=0, bytes=4096;
    if (mem::AllocateVirtual(address,bytes,mem::kMemReserve|mem::kMemCommit,4)!=0 || !mem::IsCommitted(address)) return 2;
    uint32_t expected=0x12345678, observed=0;
    std::memcpy(mem::Base()+address,&expected,sizeof(expected));
    std::memcpy(&observed,mem::Base()+address,sizeof(observed));
    if (observed!=expected) return 3;
    mem::RegionInfo region{};
    if (!mem::QueryVirtual(address,region) || region.state!=mem::kMemCommit) return 4;
    uint32_t release=0;
    if (mem::FreeVirtual(address,release,mem::kMemRelease)!=0) return 5;
    uint32_t physical=mem::AllocContiguous(4096,0,mem::kPhysicalSize-1,4096);
    if (!physical) return 6;
    uint32_t alias=mem::kWriteCombineBase+mem::PhysicalFromVirtual(physical);
    std::memcpy(mem::Base()+physical,&expected,sizeof(expected));
    std::memcpy(&observed,mem::Base()+alias,sizeof(observed));
    if (observed!=expected) return 7;
    mem::FreeContiguous(physical);
    std::puts("REAL_SDK_MEMORY: virtual reserve/commit/query/free and physical/write-combine alias passed; no game executed");
    return 0;
}
