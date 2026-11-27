#include "audio/spatial.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
int main(){
    using namespace urban::audio;
    SpatialFactors factors;
    assert(factors.Get(SpatialFactors::Distance)==1);
    assert(factors.Set(SpatialFactors::Rolloff,0,1));assert(factors.Get(SpatialFactors::Rolloff)==1);
    assert(factors.Set(SpatialFactors::Distance,.01f,0));factors.Commit();assert(factors.Get(SpatialFactors::Rolloff)==0);
    assert(!factors.Set(SpatialFactors::Distance,0,0)&&!factors.Set(SpatialFactors::Doppler,11,0));
    assert(!factors.Set(SpatialFactors::Rolloff,-1,0)&&!factors.Set(SpatialFactors::Rolloff,1,2));
    assert(!factors.Set(SpatialFactors::Distance,std::numeric_limits<float>::infinity(),0));
    assert(std::abs(InverseDistance(5,1,100,1)-.2f)<1e-6f);
    assert(InverseDistance(5,1,100,0)==1&&InverseDistance(0,1,100,1)==1);
    assert(InverseDistance(5,2,1,1)==1);
    assert(InverseDistance(200,1,100,1)==InverseDistance(100,1,100,1));
    Velocity velocity;assert(velocity.Set(-20,0,0,1));assert(velocity.Get()[0]==0);velocity.Commit();
    const std::array<float,3> zero{},position{10,0,0},away{20,0,0};
    float toward=DopplerRatio(position,velocity.Get(),zero,1,1),recede=DopplerRatio(position,away,zero,1,1);
    assert(toward>1&&recede<1&&DopplerRatio(position,away,zero,1,0)==1);
    assert(DopplerRatio(position,zero,away,1,1)>1);
    assert(std::abs(DopplerRatio(position,away,zero,.5f,2)-recede)<1e-6f);
    assert(DopplerRatio(zero,away,zero,1,1)==1);
    assert(std::isfinite(DopplerRatio(position,away,zero,1e30f,10)));
    assert(!velocity.Set(0,std::numeric_limits<float>::quiet_NaN(),0,0)&&velocity.Get()[0]==-20);
    std::puts("AUDIO_SPATIAL: deferred factors/velocity, inverse rolloff, units, approach/recession and finite singularity guards passed; authored values only");
}
