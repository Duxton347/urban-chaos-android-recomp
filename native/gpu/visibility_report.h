#pragma once
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
namespace urban::vulkan {
static_assert(std::endian::native==std::endian::little,"Xbox reports require little-endian host layout");
// Caller owns/validates the guest mapping and synchronizes the original XDK getter with Flush.
inline void PublishReport(std::span<uint8_t> mapped,uint64_t timestamp,uint32_t count) {
 if(mapped.size()<16)throw std::invalid_argument("UC-VK-003: report mapping is shorter than 16 bytes");
 uint32_t low=uint32_t(timestamp),high=uint32_t(timestamp>>32),done=0;
 std::memcpy(mapped.data(),&low,4);std::memcpy(mapped.data()+4,&high,4);std::memcpy(mapped.data()+8,&count,4);
 std::atomic_thread_fence(std::memory_order_release);
 std::memcpy(mapped.data()+12,&done,4);
}
}
