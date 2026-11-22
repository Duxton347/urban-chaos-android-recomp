// Real Android entry point. Game code and assets are supplied by the private build/import, never simulated.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "android_host.h"
#include "app/app.h"
#include "core/input.h"
#include "audio/sdl_output.h"
#include "diagnostics/journal.h"
#include "diagnostics/progress.h"
#include "game/source.h"
#include "game/sha256.h"
#include <fstream>
#include <sstream>
#include <set>
#include "image_symbols.h"
#include "build_source_profile.h"
#include "kernel/kernel.h"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#if !IMAGE_RESUMABLE_GUEST || !defined(XBE_RESUMABLE_GUEST)
#error Android integration requires the continuation-aware guest and matching dispatcher
#endif
namespace {
SDL_Window* window=nullptr;
std::mutex focus_mutex;
std::condition_variable focus_cv;
std::atomic<bool> foreground{true};
std::atomic<uint32_t> epoch{1};
std::filesystem::path root;
void Focus(bool active){
 {std::lock_guard lock(focus_mutex);if(active&&!foreground.load())++epoch;foreground=active;}
 if(!active)host::ClearInput();
 audio::SetOutputPaused(!active);
 focus_cv.notify_all();
}
}
namespace host {
SDL_Window* Window(){return window;}
bool Foreground(){return foreground.load();}
uint32_t SurfaceEpoch(){return epoch.load();}
void WindowPixels(int& w,int& h){w=h=0;if(window)SDL_GetWindowSizeInPixels(window,&w,&h);}
void WaitForeground(){std::unique_lock lock(focus_mutex);focus_cv.wait(lock,[]{return foreground.load();});}
}
namespace app {
Settings& Get(){static Settings settings;return settings;}
void Load(){} void Save(){} void LogSettings(){} void LogSystem(){} void ApplyDisplay(){}
Mode Desktop(){int w,h;host::WindowPixels(w,h);return {uint32_t(std::max(w,1)),uint32_t(std::max(h,1))};}
Mode Output(){return Desktop();} float RenderScale(uint32_t){return 1.0f;} bool Widescreen(){return false;}
std::filesystem::path ExeDir(){return root;}
int Instance(){return 0;} void* Window(){return host::Window();}
bool OpenSplash(){return false;}void SplashFirstFrame(){}void CloseSplash(){}void OpenGameWindow(bool){}
bool StartHandover(){return false;}bool HandingOver(){return false;}void FinishHandover(){}
std::recursive_mutex& UiLock(){static std::recursive_mutex mutex;return mutex;}
void Pace(){}
const GameInfo kGame={.title=L"Urban Chaos",.short_title=L"Urban Chaos",.name=L"UrbanChaos",.log="UrbanChaos",.settings="runtime.toml"};
}
int main(int argc,char** argv){
 if(argc!=2&&argc!=3){LOGE("android: launch source missing");return 2;}
 const char* storage=SDL_GetAndroidInternalStoragePath();if(!storage)return 2;root=storage;
 std::filesystem::create_directories(root/"game");std::filesystem::create_directories(root/"saves");
 std::filesystem::create_directories(root/"config");std::filesystem::create_directories(root/"cache/shaders");
 uint32_t locale_archives=0;auto& report=urban::diagnostics::ProcessJournal();
 if(!report.Healthy())report.Start(root/"logs/native",URBAN_BUILD_ID,URBAN_BUILD_XBE_SHA256);
 logx::SetObserver([](logx::Level level,const char* format,const char* expanded){urban::diagnostics::ProcessJournal().Observe(int(level),format,expanded);});
 logx::Init((root/"logs/raw").string(),"UrbanChaos");
 try {
  long pages=sysconf(_SC_PAGESIZE);
  report.Emit(urban::diagnostics::Code::AndroidHost,{{"page_size",pages>0?uint64_t(pages):0}});
  if(pages!=4096){report.Emit(urban::diagnostics::Code::UnsupportedPageSize,{{"page_size",pages>0?uint64_t(pages):0}});report.Finish();return 1;}
  SDL_SetHint(SDL_HINT_ORIENTATIONS,"LandscapeLeft LandscapeRight");
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0");SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS,"0");
  if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD|SDL_INIT_EVENTS))throw std::runtime_error("SDL initialization failed");
  window=SDL_CreateWindow("Urban Chaos",640,480,SDL_WINDOW_FULLSCREEN|SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if(!window)throw std::runtime_error("SDL Vulkan window creation failed");
  auto source=urban::game::Source::Open(argv[1],{URBAN_BUILD_XBE_SHA256,URBAN_BUILD_TITLE_ID,X86EntryPoint});
  if(argc==3){
   // Trusted cache is within app-private config. Reject symlinks, partial manifests and unexpected files.
   auto base=std::filesystem::weakly_canonical(root/"config/localization"),locale=std::filesystem::weakly_canonical(argv[2]);
   if(locale.parent_path()!=base||locale.filename().string().size()!=64)throw std::runtime_error("UC-LOCALE-002");
   std::ifstream manifest(locale/"verified.txt");std::string name,sha;std::set<std::string> seen;
   std::string line;while(std::getline(manifest,line)){
    std::istringstream row(line);std::string extra;if(!(row>>name>>sha)||(row>>extra))throw std::runtime_error("UC-LOCALE-002");
    if(!seen.insert(name).second||seen.size()>3||(name!="globallanguage.lst"&&name!="99-01.ent"&&name!="12-01.ent"))throw std::runtime_error("UC-LOCALE-002");
    auto path=locale/name;if(std::filesystem::is_symlink(path)||!std::filesystem::is_regular_file(path))throw std::runtime_error("UC-LOCALE-002");
    auto length=std::filesystem::file_size(path);if(!length||length>(64u<<20))throw std::runtime_error("UC-LOCALE-002");
    std::vector<uint8_t> bytes(length);std::ifstream file(path,std::ios::binary);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),length)||urban::game::Sha256(bytes)!=sha)throw std::runtime_error("UC-LOCALE-002");
    source.ReplaceTextArchive(name,std::move(bytes));
   }
   if(!manifest.eof()||seen.empty())throw std::runtime_error("UC-LOCALE-002");
   locale_archives=uint32_t(seen.size());
   report.Emit(urban::diagnostics::Code::LocaleReady,{{"archives",locale_archives}});
  }
  source.Activate(false); // Only the exact compiled revision is accepted in this first Android integration.
  input::Init();
  report.Emit(urban::diagnostics::Code::BootStart);
  if(!kernel::Boot(std::move(source.xbe),IMAGE_XBE,source.game_directory.string(),(root/"saves").string()))throw std::runtime_error("Kernel boot rejected");
  report.Emit(urban::diagnostics::Code::BootAccepted);
  uint64_t last_audio=0,last_progress=0;
  for(;;){
   SDL_Event event{};
   if(SDL_WaitEventTimeout(&event,50))do {
    switch(event.type){
     case SDL_EVENT_WILL_ENTER_BACKGROUND:case SDL_EVENT_WINDOW_FOCUS_LOST:Focus(false);break;
     case SDL_EVENT_DID_ENTER_FOREGROUND:case SDL_EVENT_WINDOW_FOCUS_GAINED:Focus(true);break;
     case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:++epoch;break;
     case SDL_EVENT_TERMINATING:case SDL_EVENT_QUIT:
      audio::StopOutput();report.Finish();logx::Flush();kernel::Shutdown(0);
     default:break;
    }
    host::PadEvent(event);
   }while(SDL_PollEvent(&event));
   if(SDL_GetTicks()-last_audio>=1000){audio::PollOutputDiagnostics();last_audio=SDL_GetTicks();logx::Flush();}
   if(SDL_GetTicks()-last_progress>=10000){urban::progress::Poll(report);report.Emit(urban::diagnostics::Code::LocaleState,{{"requested",argc==3?1u:0u},{"archives",locale_archives}});host::PollInputDiagnostics();last_progress=SDL_GetTicks();}
  }
 }catch(const urban::game::SourceError& error){report.Emit(urban::diagnostics::SourceCode(error.code));}
 catch(const std::exception& error){report.Emit(std::string_view(error.what())=="UC-LOCALE-002"?urban::diagnostics::Code::LocaleRejected:urban::diagnostics::Code::BootFailed);}
 report.Finish();logx::Flush();return 1;
}
