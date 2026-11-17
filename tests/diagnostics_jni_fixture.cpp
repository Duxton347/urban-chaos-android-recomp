#include <jni.h>
#include "core/log.h"
// Author-created event from the same SDK logger instance that owns the observer.
extern "C" JNIEXPORT void JNICALL Java_DiagnosticsJniTest_emitFixture(JNIEnv*,jclass) {
 LOGI("renderer: frame %u, %u draws, %u empty frames",120u,321u,0u);
 LOGE("kernel export %u (%s) is not implemented, return address %08X",233u,"SENTINEL_PRIVATE_ARGUMENT",0x12345);
}
