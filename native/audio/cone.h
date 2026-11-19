#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace urban::audio {
// DirectSound cone angles are full angles in degrees; outside volume is in hundredths of a dB.
class Cone {
    struct Settings { uint32_t inside=360,outside=360;int32_t volume=0;std::array<float,3> direction{0,0,1}; } live_,pending_;
    unsigned pending_bits_=0;
public:
    bool Angles(uint32_t inside,uint32_t outside,uint32_t deferred) {
        if(inside>outside || outside>360 || deferred>1)return false;
        if(deferred) {pending_.inside=inside;pending_.outside=outside;pending_bits_|=1;}
        else {live_.inside=inside;live_.outside=outside;pending_bits_&=~1u;}
        return true;
    }
    bool OutsideVolume(int32_t volume,uint32_t deferred) {
        if(volume < -10000 || volume>0 || deferred>1)return false;
        if(deferred) {pending_.volume=volume;pending_bits_|=2;}
        else {live_.volume=volume;pending_bits_&=~2u;}
        return true;
    }
    bool Direction(float x,float y,float z,uint32_t deferred) {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||deferred>1)return false;
        if(deferred) {pending_.direction={x,y,z};pending_bits_|=4;}
        else {live_.direction={x,y,z};pending_bits_&=~4u;}
        return true;
    }
    void Commit() {
        if(pending_bits_&1) {live_.inside=pending_.inside;live_.outside=pending_.outside;}
        if(pending_bits_&2)live_.volume=pending_.volume;
        if(pending_bits_&4)live_.direction=pending_.direction;
        pending_bits_=0;
    }
    float Gain(float x,float y,float z) const {
        if(live_.inside==360 || live_.volume==0)return 1;
        double dx=live_.direction[0],dy=live_.direction[1],dz=live_.direction[2];
        double length=std::sqrt((dx*dx+dy*dy+dz*dz)*(double(x)*x+double(y)*y+double(z)*z));
        // Coincident listener/source or zero orientation has no directional distinction.
        if(length<1e-20)return 1;
        double cosine=std::clamp((dx*x+dy*y+dz*z)/length,-1.0,1.0);
        double angle=std::acos(cosine)*180.0/3.14159265358979323846;
        double inside=live_.inside*0.5,outside=live_.outside*0.5;
        if(angle<=inside)return 1;
        double blend=angle>=outside ? 1:(angle-inside)/(outside-inside);
        if(live_.volume==-10000 && blend==1)return 0;
        return float(std::pow(10.0,double(live_.volume)*blend/2000.0));
    }
};
} // namespace urban::audio
