#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include "display/viewport.h"
namespace host {
SDL_Window* Window();
bool Foreground();
uint32_t SurfaceEpoch();
void WindowPixels(int&,int&);
void WaitForeground();
void PadEvent(const SDL_Event&);
void ClearInput();
void PollInputDiagnostics();
urban::display::Mode DisplayMode();
inline urban::display::Rect FrameViewport(float w,float h,float aspect){return urban::display::Viewport(w,h,aspect,DisplayMode());}
}
