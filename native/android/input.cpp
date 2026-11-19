// SDL event-thread ownership; guest reads never pump or consume Android window events.
#include "android_host.h"
#include "app/app.h"
#include "core/input.h"
#include "input/touch_state.h"
#include "diagnostics/journal.h"
#include <algorithm>
#include <array>
#include <mutex>
#include <jni.h>
namespace {
std::mutex mutex;
std::array<SDL_Gamepad*,4> pads{};
std::array<input::Pad,4> state{};
urban::touch::State touch;
std::atomic<int> display_mode{0};
uint64_t NowUs(){return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
void Snapshot() {
 for(size_t n=0;n<pads.size();++n){
  auto& p=state[n];p={};auto* pad=pads[n];if(!pad)continue;p.connected=true;
  constexpr SDL_GamepadButton digital[]={SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_BACK,SDL_GAMEPAD_BUTTON_LEFT_STICK,SDL_GAMEPAD_BUTTON_RIGHT_STICK};
  for(unsigned k=0;k<8;k++)if(SDL_GetGamepadButton(pad,digital[k]))p.buttons|=uint16_t(1u<<k);
  constexpr SDL_GamepadButton analog[]={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_NORTH,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER};
  for(unsigned k=0;k<6;k++)p.analog[k]=SDL_GetGamepadButton(pad,analog[k])?255:0;
  p.analog[6]=uint8_t(std::max(int(SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)),0)>>7);
  p.analog[7]=uint8_t(std::max(int(SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)),0)>>7);
  p.lx=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFTX);p.ly=int16_t(-1-int(SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFTY)));
  p.rx=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_RIGHTX);p.ry=int16_t(-1-int(SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_RIGHTY)));
 }
}
}
namespace host {
void ClearInput(){std::lock_guard lock(mutex);for(auto& p:state)p={};touch.Clear();}
urban::display::Mode DisplayMode(){return urban::display::Mode(display_mode.load());}
void PollInputDiagnostics(){
 urban::touch::Stats stats;{std::lock_guard lock(mutex);stats=touch.Snapshot();}
 urban::diagnostics::ProcessJournal().Emit(urban::diagnostics::Code::InputLatency,
  {{"presses",stats.presses},{"delivered",stats.delivered},{"total_us",stats.total_us},{"max_us",stats.max_us}});
}
void PadEvent(const SDL_Event& event){
 std::lock_guard lock(mutex);
 if(event.type==SDL_EVENT_GAMEPAD_ADDED){
  bool exists=false;for(auto* pad:pads)if(pad&&SDL_GetGamepadID(pad)==event.gdevice.which)exists=true;
  if(!exists)for(auto& pad:pads)if(!pad){pad=SDL_OpenGamepad(event.gdevice.which);break;}
 }
 if(event.type==SDL_EVENT_GAMEPAD_REMOVED)for(auto& pad:pads)if(pad&&SDL_GetGamepadID(pad)==event.gdevice.which){SDL_CloseGamepad(pad);pad=nullptr;}
 if(Foreground())Snapshot();
}
}
namespace input {
bool Connected(int port){std::lock_guard lock(mutex);return port==0||(port>0&&port<4&&state[port].connected);}
void Init(){int count=0;auto* ids=SDL_GetGamepads(&count);for(int n=0;n<count;n++){SDL_Event e{};e.type=SDL_EVENT_GAMEPAD_ADDED;e.gdevice.which=ids[n];host::PadEvent(e);}SDL_free(ids);}
Pad Poll(int port){Pad result;{std::lock_guard lock(mutex);if(port>=0&&port<4)result=state[port];if(port==0)result=touch.Merge(result,NowUs());}if(app::kGame.pad)app::kGame.pad(port,result);return result;}
void Rumble(int,uint16_t,uint16_t){} // Not advertised until Android haptics/controller routing is verified.
}

extern "C" JNIEXPORT void JNICALL Java_org_urbanchaos_launcher_NativeGame_menuButton(JNIEnv*,jclass,jint key,jboolean down){
 constexpr int original[]={urban::touch::A,urban::touch::B,urban::touch::Start,urban::touch::Up,urban::touch::Down,urban::touch::Left,urban::touch::Right};
 std::lock_guard lock(mutex);if(key>=0&&key<7)touch.Set(original[key],down,NowUs());
}
extern "C" JNIEXPORT void JNICALL Java_org_urbanchaos_launcher_NativeGame_padButton(JNIEnv*,jclass,jint key,jboolean down){std::lock_guard lock(mutex);touch.Set(key,down,NowUs());}
extern "C" JNIEXPORT void JNICALL Java_org_urbanchaos_launcher_NativeGame_padStick(JNIEnv*,jclass,jint stick,jfloat x,jfloat y,jboolean active){std::lock_guard lock(mutex);touch.Axis(stick,x,y,active);}
extern "C" JNIEXPORT void JNICALL Java_org_urbanchaos_launcher_NativeGame_clearTouch(JNIEnv*,jclass){std::lock_guard lock(mutex);touch.Clear();}
extern "C" JNIEXPORT void JNICALL Java_org_urbanchaos_launcher_NativeGame_displayMode(JNIEnv*,jclass,jint mode){if(mode>=0&&mode<=2)display_mode=mode;}
