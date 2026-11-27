#include <jni.h>
#include "diagnostics/journal.h"
#include "core/log.h"

namespace {
struct Utf {
 JNIEnv* env;jstring source;const char* value;
 Utf(JNIEnv* e,jstring s):env(e),source(s),value(s?e->GetStringUTFChars(s,nullptr):nullptr){}
 ~Utf(){if(value)env->ReleaseStringUTFChars(source,value);}
};
void Observe(logx::Level level,const char* format,const char* expanded) {
 urban::diagnostics::ProcessJournal().Observe(int(level),format,expanded);
}
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_urbanchaos_launcher_diagnostics_NativeReports_start(
 JNIEnv* env,jclass,jstring directory,jstring build,jstring xbe,jboolean detailed) {
 Utf path(env,directory);if(!path.value)return JNI_FALSE;
 Utf id(env,build);if(!id.value)return JNI_FALSE;
 Utf hash(env,xbe);if(!hash.value)return JNI_FALSE;
 auto& journal=urban::diagnostics::ProcessJournal();
 if(!journal.Start(path.value,id.value,hash.value,detailed==JNI_TRUE))return JNI_FALSE;
 if(!journal.Emit(urban::diagnostics::Code::SessionBegin))return JNI_FALSE;
 logx::SetObserver(Observe);return JNI_TRUE;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_urbanchaos_launcher_diagnostics_NativeReports_isHealthy(JNIEnv*,jclass) {
 return urban::diagnostics::ProcessJournal().Healthy()?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_org_urbanchaos_launcher_diagnostics_NativeReports_stop(JNIEnv*,jclass) {
 logx::SetObserver(nullptr);
 urban::diagnostics::ProcessJournal().Finish();
}
