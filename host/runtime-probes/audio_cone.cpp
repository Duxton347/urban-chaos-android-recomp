// Authored sound directions and settings, no original sound data or title metadata.
#include "audio/cone.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
static bool Near(float a,float b) {return std::abs(a-b)<1e-5f;}
int main() {
    urban::audio::Cone cone;
    assert(cone.Gain(0,0,-1)==1);
    assert(cone.Angles(60,120,0)&&cone.Direction(10,0,0,0)&&cone.OutsideVolume(-6000,0));
    assert(Near(cone.Gain(2,0,0),1));
    assert(Near(cone.Gain(-2,0,0),.001f));
    assert(Near(cone.Gain(2,2,0),std::sqrt(.001f))); // halfway through the angular transition
    assert(Near(cone.Gain(0,0,0),1));
    // Each property has independent deferred ownership; immediate updates cancel only that property.
    assert(cone.Direction(-1,0,0,1));assert(Near(cone.Gain(1,0,0),1));
    assert(cone.OutsideVolume(-2000,1));assert(Near(cone.Gain(-1,0,0),.001f));
    assert(cone.Angles(0,0,0));cone.Commit();
    assert(Near(cone.Gain(-1,0,0),1)&&Near(cone.Gain(1,0,0),.1f));
    assert(cone.OutsideVolume(-6000,1)&&cone.OutsideVolume(-4000,0));cone.Commit();
    assert(Near(cone.Gain(1,0,0),.01f));
    assert(!cone.Angles(121,120,0)&&!cone.Angles(0,361,0)&&!cone.Angles(0,360,2));
    assert(!cone.OutsideVolume(-10001,0)&&!cone.OutsideVolume(1,0)&&!cone.OutsideVolume(0,2));
    assert(!cone.Direction(std::numeric_limits<float>::infinity(),0,0,0));
    assert(!cone.Direction(0,std::numeric_limits<float>::quiet_NaN(),0,0));
    assert(!cone.Direction(0,0,1,2));assert(Near(cone.Gain(1,0,0),.01f));
    assert(cone.OutsideVolume(-10000,0)&&cone.Gain(1,0,0)==0);
    assert(cone.Direction(0,0,0,0)&&cone.Gain(1,0,0)==1);
    assert(cone.Angles(360,360,0)&&cone.Direction(1,0,0,0)&&cone.Gain(-1,0,0)==1);
    std::puts("AUDIO_CONE: real directional gain, full-angle convention, angular transition, defaults, deferred/immediate isolation, validation and finite edge cases passed; authored vectors only");
}
