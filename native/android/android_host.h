#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
namespace host {
SDL_Window* Window();
bool Foreground();
uint32_t SurfaceEpoch();
void WindowPixels(int&,int&);
void WaitForeground();
void PadEvent(const SDL_Event&);
void ClearInput();
}
