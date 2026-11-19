#include <jni.h>
#include "core/log.h"
// Author-created event from the same SDK logger instance that owns the observer.
extern "C" JNIEXPORT void JNICALL Java_DiagnosticsJniTest_emitFixture(JNIEnv*,jclass) {
 LOGI("renderer: frame %u, %u draws, %u empty frames",120u,321u,0u);
 LOGI("renderer: frame %llu (Vulkan headless)",1800ull);
 LOGI("vulkan: visibility report completed, samples=%u",64u);
 LOGI("renderer: frame %llu (Vulkan Android)",2400ull);
 LOGI("vulkan-device: vendor %u device %u api %u driver %u precise %u phase %u",0x5143u,650u,4206592u,12345u,1u,1u);
 LOGE("vulkan-count-failure: operation %u",4u);
 LOGE("kernel export %u (%s) is not implemented, return address %08X",233u,"SENTINEL_PRIVATE_ARGUMENT",0x12345);
}
