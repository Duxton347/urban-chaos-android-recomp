#include <jni.h>
#include "game/source.h"
#include "image_symbols.h"
#include "cpu/x86_context.h"
#include "build_source_profile.h"
extern "C" JNIEXPORT jstring JNICALL Java_org_urbanchaos_launcher_NativeGame_validate(JNIEnv* env,jclass,jstring path){
 if(!path)return env->NewStringUTF("UC-IMPORT-SOURCE");const char* value=env->GetStringUTFChars(path,nullptr);if(!value)return nullptr;
 std::string code;
 try{auto source=urban::game::Source::Open(value,{URBAN_BUILD_XBE_SHA256,URBAN_BUILD_TITLE_ID,X86EntryPoint});code=source.metadata.exact_revision?"UC-IMPORT-000":"UC-IMPORT-REVISION";}
 catch(const urban::game::SourceError& error){code=error.code;}
 catch(...){code="UC-IMPORT-NATIVE";}
 env->ReleaseStringUTFChars(path,value);return env->NewStringUTF(code.c_str());
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_org_urbanchaos_launcher_NativeGame_readTextArchive(JNIEnv* env,jclass,jstring path,jstring name){
 if(!path||!name)return nullptr;
 const char* file=env->GetStringUTFChars(path,nullptr);if(!file)return nullptr;
 const char* entry=env->GetStringUTFChars(name,nullptr);if(!entry){env->ReleaseStringUTFChars(path,file);return nullptr;}
 jbyteArray result=nullptr;
 try{auto source=urban::game::Source::Open(file,{URBAN_BUILD_XBE_SHA256,URBAN_BUILD_TITLE_ID,X86EntryPoint});auto bytes=source.ReadTextArchive(entry);
  result=env->NewByteArray(jsize(bytes.size()));if(result)env->SetByteArrayRegion(result,0,jsize(bytes.size()),reinterpret_cast<const jbyte*>(bytes.data()));
 }catch(...){/* Revision mismatch and invalid archive remain an explicit preparation error in Java. */}
 env->ReleaseStringUTFChars(name,entry);env->ReleaseStringUTFChars(path,file);return result;
}
