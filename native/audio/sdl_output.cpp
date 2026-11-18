#include "audio/sdl_output.h"
#include "audio/audio.h"
#include "core/log.h"
#include <SDL3/SDL_init.h>
#include <algorithm>
#include <cmath>

namespace urban::audio_output {
static_assert(audio::kRate==kRate);
uint32_t FramesForDemand(int bytes) {
 if(bytes<=0)return 0;
 return uint32_t(std::min((uint64_t(bytes)+7)/8,uint64_t(kBlock)*8));
}
void Output::Fail(Fault fault) noexcept {
 fault_=fault;state_=State::Failed;
}
void SDLCALL Output::Feed(void* data,SDL_AudioStream* stream,int additional,int) {
 auto& out=*static_cast<Output*>(data);
 ++out.callbacks_;
 if(out.state_!=State::Running)return;
 uint32_t frames=FramesForDemand(additional);
 if(additional>int(kBlock*8*8))++out.limited_;
 while(frames&&out.state_==State::Running) {
  uint32_t count=std::min(frames,kBlock);
  std::fill(out.block_.begin(),out.block_.end(),0.0f);
  try {out.render_(out.block_.data(),count);}
  catch(...) {out.Fail(Fault::Render);return;}
  out.rendered_+=count;
  uint64_t invalid=0,active=0;float peak=0;
  for(uint32_t n=0;n<count*kChannels;n++) {
   float& sample=out.block_[n];
   if(!std::isfinite(sample)){sample=0;++invalid;}
   if(std::abs(sample)>1e-7f)++active;
   peak=std::max(peak,std::abs(sample));
  }
  out.nonfinite_+=invalid;out.active_+=active;
  uint32_t ppm=uint32_t(std::min(peak,1000.0f)*1000000.0f);
  uint32_t prior=out.peak_ppm_;
  while(ppm>prior&&!out.peak_ppm_.compare_exchange_weak(prior,ppm)){}
  // Pause/stop may have been requested while the mixer was running.
  if(out.state_!=State::Running)return;
  if(!SDL_PutAudioStreamData(stream,out.block_.data(),int(count*kChannels*sizeof(float)))) {
   out.Fail(Fault::Submit);return;
  }
  out.submitted_+=count;frames-=count;
 }
}
bool Output::Start(Renderer renderer) {
 std::lock_guard lock(control_);
 if(stream_||!renderer)return false;
 fault_=Fault::None;callbacks_=0;rendered_=0;submitted_=0;active_=0;nonfinite_=0;limited_=0;peak_ppm_=0;
 if(!SDL_InitSubSystem(SDL_INIT_AUDIO)){Fail(Fault::Init);return false;}
 owns_audio_=true;render_=std::move(renderer);
 SDL_AudioSpec spec{SDL_AUDIO_F32,int(kChannels),int(kRate)};
 stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,Feed,this);
 if(!stream_) {
  render_={};SDL_QuitSubSystem(SDL_INIT_AUDIO);owns_audio_=false;
  Fail(Fault::Open);return false;
 }
 state_=State::Paused;
 if(!want_paused_) {
  state_=State::Running;
  if(!SDL_ResumeAudioStreamDevice(stream_)){Fail(Fault::Resume);return false;}
 }
 return true;
}
bool Output::SetPaused(bool paused) {
 std::lock_guard lock(control_);want_paused_=paused;
 if(!stream_)return state_!=State::Failed;
 if(fault_!=Fault::None||state_==State::Failed)return false;
 if((paused&&state_==State::Paused)||(!paused&&state_==State::Running))return true;
 if(paused) {
  state_=State::Paused;
  if(!SDL_PauseAudioStreamDevice(stream_)){Fail(Fault::Pause);return false;}
 } else {
  state_=State::Running;
  if(!SDL_ResumeAudioStreamDevice(stream_)){Fail(Fault::Resume);return false;}
 }
 if(fault_!=Fault::None){state_=State::Failed;return false;}
 return true;
}
void Output::StopLocked() {
 state_=State::Stopped;
 if(stream_){SDL_DestroyAudioStream(stream_);stream_=nullptr;}
 render_={};
 if(owns_audio_){SDL_QuitSubSystem(SDL_INIT_AUDIO);owns_audio_=false;}
}
void Output::Stop(){std::lock_guard lock(control_);StopLocked();}
SDL_AudioDeviceID Output::Device() {
 std::lock_guard lock(control_);
 return stream_?SDL_GetAudioStreamDevice(stream_):0;
}
Stats Output::Snapshot() const {
 return {state_.load(),fault_.load(),callbacks_.load(),rendered_.load(),submitted_.load(),active_.load(),
         nonfinite_.load(),limited_.load(),peak_ppm_.load()};
}
Output& ProcessOutput(){static auto* output=new Output;return *output;}
}
namespace audio {
void StartOutput(std::function<void(float*,uint32_t)> render) {
 bool ready=urban::audio_output::ProcessOutput().Start(std::move(render));
 LOGI("audio-output: state %u",unsigned(urban::audio_output::ProcessOutput().Snapshot().state));
 if(!ready)PollOutputDiagnostics();
}
bool SetOutputPaused(bool paused) {
 bool ready=urban::audio_output::ProcessOutput().SetPaused(paused);
 LOGI("audio-output: state %u",unsigned(urban::audio_output::ProcessOutput().Snapshot().state));
 return ready;
}
void StopOutput(){urban::audio_output::ProcessOutput().Stop();LOGI("audio-output: state 0");}
void PollOutputDiagnostics() {
 auto stats=urban::audio_output::ProcessOutput().Snapshot();
 // Only the host/event loop calls this function. No logging or file IO occurs in Feed.
 static auto previous=urban::audio_output::Fault::None;
 if(stats.fault!=previous&&stats.fault!=urban::audio_output::Fault::None)
  LOGE("audio-output: failure operation %u",unsigned(stats.fault));
 previous=stats.fault;
 LOGI("audio-metrics: frames %llu active-samples %llu peak %.6g nonfinite %llu",
      (unsigned long long)stats.submitted,(unsigned long long)stats.active,
      double(stats.peak_ppm)/1000000,(unsigned long long)stats.nonfinite);
}
}
