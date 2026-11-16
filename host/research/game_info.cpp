// Host metadata and diagnostics only; no replacement functions for title logic.
#include "app/app.h"
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int AudioSelfTest();
int ViewportSelfTest();
int BackendBusySelfTest();
int VertexModeSelfTest();
int VoicePropertiesSelfTest();
static void Crash(int signal, siginfo_t* info, void*) {
    char message[160];
    int n = std::snprintf(message, sizeof(message), "RESEARCH_SIGNAL=%d address=%p\n", signal, info->si_addr);
    (void)write(2, message, n);
    void* frames[48];
    int count = backtrace(frames, 48);
    backtrace_symbols_fd(frames, count, 2);
    _exit(128 + signal);
}
static void Init() {
    void* frames[1];
    backtrace(frames, 1);
    struct sigaction action{};
    action.sa_sigaction = Crash;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, nullptr);
    if (const char* probe = std::getenv("URBAN_RUNTIME_PROBE")) {
        if (!std::strcmp(probe, "audio")) std::_Exit(AudioSelfTest());
        if (!std::strcmp(probe, "viewport")) std::_Exit(ViewportSelfTest());
        if (!std::strcmp(probe, "busy")) std::_Exit(BackendBusySelfTest());
        if (!std::strcmp(probe, "vertex")) std::_Exit(VertexModeSelfTest());
        if (!std::strcmp(probe, "voice")) std::_Exit(VoicePropertiesSelfTest());
        std::_Exit(2);
    }
}
const app::GameInfo app::kGame = {
    .title=L"Urban Chaos research runtime", .short_title=L"Urban Chaos", .name=L"UrbanChaosResearch",
    .log="urban_research", .settings="research.toml", .splash=L"", .missing=L"Private Xbox game files required",
    .images={{"Default.xbe", "urban_research"}}, .options={}, .load=nullptr, .save=nullptr, .init=Init,
};
