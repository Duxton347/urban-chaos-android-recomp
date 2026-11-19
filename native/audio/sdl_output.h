#pragma once
#include <SDL3/SDL_audio.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>

namespace urban::audio_output {
constexpr uint32_t kRate=48000, kChannels=2, kBlock=480;
enum class State : uint32_t { Stopped, Paused, Running, Failed };
enum class Fault : uint32_t { None, Init, Open, Resume, Pause, Submit, Render };
struct Stats {
 State state; Fault fault;
 uint64_t callbacks, rendered, submitted, active, nonfinite, limited;
 uint32_t peak_ppm;
 uint64_t render_us, max_render_us, slow_blocks;
};
// Requests are bytes in the stream's stereo/F32/48-kHz input format.
uint32_t FramesForDemand(int bytes);
class Output {
 using Renderer=std::function<void(float*,uint32_t)>;
 std::mutex control_;
 SDL_AudioStream* stream_=nullptr;
 Renderer render_;
 std::array<float,kBlock*kChannels> block_{};
 bool owns_audio_=false, want_paused_=false;
 std::atomic<State> state_{State::Stopped};
 std::atomic<Fault> fault_{Fault::None};
 std::atomic<uint64_t> callbacks_{0},rendered_{0},submitted_{0},active_{0},nonfinite_{0},limited_{0};
 std::atomic<uint32_t> peak_ppm_{0};
 std::atomic<uint64_t> render_us_{0},max_render_us_{0},slow_blocks_{0};
 static void SDLCALL Feed(void*,SDL_AudioStream*,int,int);
 void Fail(Fault) noexcept;
 void StopLocked();
public:
 bool Start(Renderer);
 bool SetPaused(bool);
 // No guest thread may start a new output while a session is being torn down.
 // Destruction waits for any active SDL callback before releasing its renderer.
 void Stop();
 Stats Snapshot() const;
 SDL_AudioDeviceID Device();
 ~Output(){Stop();}
};
Output& ProcessOutput();
}
namespace audio {
bool SetOutputPaused(bool);
void StopOutput();
// Call from the host/event loop, not the real-time audio callback.
void PollOutputDiagnostics();
}
