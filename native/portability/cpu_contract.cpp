// Compile-time CPU contract only. No title code or replacement gameplay.
#include "cpu/x86_ops.h"
#include <bit>
#include <cstddef>
#include <cstdint>
static_assert(sizeof(void*) == 8);
static_assert(std::endian::native == std::endian::little);
static_assert(alignof(X86Context) == 64);
static_assert(offsetof(X86Context, eax) == 0);
static_assert(offsetof(X86Context, esp) == 16);
static_assert(sizeof(X86Xmm) == 16);
extern "C" uint32_t urban_guest_unaligned_roundtrip(uint8_t* base, uint32_t address, uint32_t value) {
    X86_MEM32(address) = value;
    return X86_MEM32(address);
}
