#include "gpu/vulkan_visibility.h"
#include "gpu/visibility_report.h"
// Compile the production report layout for AArch64 as well as the Vulkan query implementation.
void UrbanVisibilityReportContract(std::span<uint8_t> guest_mapping,uint64_t timestamp,uint32_t count) {
 urban::vulkan::PublishReport(guest_mapping,timestamp,count);
}
