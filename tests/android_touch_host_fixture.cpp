#include "core/input.h"
#include "app/app.h"
#include "android_host.h"
#include <jni.h>
#include <stdexcept>
#include <iostream>
namespace app {const GameInfo kGame={};}
namespace input {bool Connected(int port);}
extern "C" void Java_org_urbanchaos_launcher_NativeGame_padButton(JNIEnv*,jclass,jint,jboolean);
extern "C" void Java_org_urbanchaos_launcher_NativeGame_padStick(JNIEnv*,jclass,jint,jfloat,jfloat,jboolean);
extern "C" void Java_org_urbanchaos_launcher_NativeGame_clearTouch(JNIEnv*,jclass);
static void Check(bool ok){if(!ok)throw std::runtime_error("Android touch host contract failed");}
int main(){
 Java_org_urbanchaos_launcher_NativeGame_padButton(nullptr,nullptr,0,true);
 Java_org_urbanchaos_launcher_NativeGame_padButton(nullptr,nullptr,0,false);
 // Discovery of a connected port must never drain the tap before GetState polls.
 for(int n=0;n<20;n++)Check(input::Connected(0));
 Check(!input::Connected(1)&&!input::Connected(9));
 Check(input::Poll(0).analog[0]==255);Check(input::Poll(0).analog[0]==0);
 Java_org_urbanchaos_launcher_NativeGame_padStick(nullptr,nullptr,1,1,-1,true);
 Check(input::Poll(0).rx==32767&&input::Poll(0).ry==-32767);
 Java_org_urbanchaos_launcher_NativeGame_clearTouch(nullptr,nullptr);
 Check(input::Poll(0).rx==0);
 host::ClearInput();
 std::cout<<"Android touch host discovery/short-tap/axis contract passed; authored inputs only\n";
}
