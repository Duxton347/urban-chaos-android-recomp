#pragma once
#include <SDL3/SDL_hints.h>
// Linux research host has no SDL event pump. Its controller owns SIGINT/SIGTERM.
// The Android SDL host must use its own event/lifecycle policy instead.
inline bool KeepHeadlessSignalHandlers() {
 return SDL_SetHintWithPriority(SDL_HINT_NO_SIGNAL_HANDLERS,"1",SDL_HINT_OVERRIDE);
}
