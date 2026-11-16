// Research output sink: pull the real SDK mixer and measure it, without a sound device or audio artifact.
#include "audio/audio.h"
#include "core/log.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
namespace audio {
void StartOutput(std::function<void(float*, uint32_t)> render) {
    std::thread([render=std::move(render)] {
        std::vector<float> buffer(kRate/100*2);
        uint64_t frames=0, active=0, nonfinite=0, ticks=0;
        float peak=0;
        for (;;) {
            render(buffer.data(), kRate/100);
            frames+=kRate/100;
            for(float sample:buffer) {
                if(!std::isfinite(sample)) { ++nonfinite; continue; }
                float magnitude=std::abs(sample);
                if(magnitude>1e-7f)++active;
                peak=std::max(peak,magnitude);
            }
            if(++ticks%100==0)
                LOGI("audio-metrics: frames %llu active-samples %llu peak %.6g nonfinite %llu",
                     static_cast<unsigned long long>(frames),static_cast<unsigned long long>(active),peak,
                     static_cast<unsigned long long>(nonfinite));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }).detach();
}
}
