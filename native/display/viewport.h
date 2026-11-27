#pragma once
#include <algorithm>
#include <cmath>
namespace urban::display {
enum class Mode { Fit, Fill, Stretch };
struct Rect {float x,y,w,h;};
inline Rect Viewport(float w,float h,float aspect,Mode mode) {
 if(!std::isfinite(w)||!std::isfinite(h)||!std::isfinite(aspect)||w<=0||h<=0||aspect<=0)return {};
 if(mode==Mode::Stretch)return {0,0,w,h};
 float vw=mode==Mode::Fill?std::max(w,h*aspect):std::min(w,h*aspect),vh=vw/aspect;
 return {(w-vw)*.5f,(h-vh)*.5f,vw,vh};
}
}
