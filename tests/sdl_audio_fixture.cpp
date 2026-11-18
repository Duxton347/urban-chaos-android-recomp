// Authored signals only. No game audio is captured or written to disk.
#include "audio/sdl_output.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <limits>
#include <source_location>
#include <stdexcept>
#include <thread>
using namespace urban::audio_output;
using namespace std::chrono_literals;
static void Require(bool value,const std::source_location& at=std::source_location::current()){
 if(!value)throw std::runtime_error("SDL output fixture assertion at line "+std::to_string(at.line()));
}
template<class F> static void Until(F ready) {
 auto end=std::chrono::steady_clock::now()+3s;
 while(!ready()){Require(std::chrono::steady_clock::now()<end);std::this_thread::sleep_for(5ms);}
}
struct Capture {std::atomic<uint64_t> pairs{0},errors{0};};
static void SDLCALL Postmix(void* userdata,const SDL_AudioSpec* spec,float* buffer,int bytes) {
 auto& data=*static_cast<Capture*>(userdata);
 if(spec->channels!=2||spec->freq!=48000){++data.errors;return;}
 for(int n=0;n+1<bytes/int(sizeof(float));n+=2) {
  float left=buffer[n],right=buffer[n+1];
  if(!std::isfinite(left)||!std::isfinite(right)){++data.errors;continue;}
  if(std::abs(left)<0.0001f&&std::abs(right)<0.0001f)continue;
  if(left<=0||right>=0||std::abs(right+2*left)>0.001f)++data.errors;
  ++data.pairs;
 }
}
int main(int argc,char** argv) {
 if(argc==2&&std::string_view(argv[1])=="--invalid-driver") {
  Output output;Require(!output.Start([](float*,uint32_t){}));
  Require(output.Snapshot().state==State::Failed&&output.Snapshot().fault==Fault::Init);
  std::puts("Unavailable audio driver fails explicitly");return 0;
 }
 Require(FramesForDemand(-1)==0&&FramesForDemand(0)==0);
 Require(FramesForDemand(1)==1&&FramesForDemand(7)==1&&FramesForDemand(8)==1&&FramesForDemand(9)==2);
 Require(FramesForDemand(std::numeric_limits<int>::max())==kBlock*8);
 Output output;Require(output.SetPaused(true));
 std::atomic<uint64_t> renders{0};Capture capture;
 auto signal=[&](float* out,uint32_t frames){
  ++renders;for(uint32_t n=0;n<frames;n++){out[2*n]=0.25f;out[2*n+1]=-0.5f;}
 };
 Require(output.Start(signal));Require(!output.Start(signal));
 Require(SDL_SetAudioPostmixCallback(output.Device(),Postmix,&capture));
 std::this_thread::sleep_for(100ms);Require(renders==0&&output.Snapshot().state==State::Paused);
 Require(output.SetPaused(false));Until([&]{return capture.pairs>=4096;});
 Require(capture.errors==0);Require(output.Snapshot().submitted>=4096);
 Require(output.SetPaused(true));uint64_t paused=renders;
 std::this_thread::sleep_for(100ms);Require(renders==paused);
 Require(output.SetPaused(false));Until([&]{return renders>paused;});output.Stop();
 uint64_t stopped=renders;std::this_thread::sleep_for(100ms);
 Require(renders==stopped&&output.Device()==0&&output.Snapshot().state==State::Stopped);
 Require(output.SetPaused(false));
 Require(output.Start([](float* out,uint32_t frames){
  for(uint32_t n=0;n<frames*2;n++)out[n]=std::numeric_limits<float>::quiet_NaN();
 }));
 Until([&]{return output.Snapshot().submitted>0;});
 Require(output.Snapshot().nonfinite>0&&output.Snapshot().active==0);output.Stop();
 Require(output.Start([](float*,uint32_t){throw std::runtime_error("authored render failure");}));
 Until([&]{return output.Snapshot().fault==Fault::Render;});
 Require(output.Snapshot().state==State::Failed);Require(!output.SetPaused(false));output.Stop();
 // Stop must wait for a callback already rendering before releasing its callable.
 std::mutex mutex;std::condition_variable changed;bool entered=false,release=false;
 Require(output.Start([&](float* out,uint32_t frames){
  std::unique_lock lock(mutex);entered=true;changed.notify_all();
  changed.wait(lock,[&]{return release;});std::fill(out,out+frames*2,0.0f);
 }));
 {std::unique_lock lock(mutex);Require(changed.wait_for(lock,3s,[&]{return entered;}));}
 auto stop=std::async(std::launch::async,[&]{output.Stop();});
 Require(stop.wait_for(40ms)==std::future_status::timeout);
 {std::lock_guard lock(mutex);release=true;}changed.notify_all();
 Require(stop.wait_for(3s)==std::future_status::ready);stop.get();
 std::printf("{\"status\":\"PASS\",\"stereo_pairs_observed\":%llu,\"channel_errors\":%llu,\"pause_resume_stop\":true,\"render_failure\":true,\"nonfinite_silenced\":true,\"game_data_used\":false}\n",
  (unsigned long long)capture.pairs.load(),(unsigned long long)capture.errors.load());
}
