#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace urban::audio {
class SpatialFactors {
    std::array<float,3> live_{1,1,1},pending_{};
    unsigned bits_=0;
public:
    enum Kind {Distance,Rolloff,Doppler};
    bool Set(Kind kind,float value,uint32_t deferred) {
        if(!std::isfinite(value)||deferred>1||value<0||(kind==Distance ? value==0:value>10))return false;
        if(deferred) {pending_[kind]=value;bits_|=1u<<kind;}
        else {live_[kind]=value;bits_&=~(1u<<kind);}
        return true;
    }
    void Commit() {for(unsigned i=0;i<3;i++)if(bits_&(1u<<i))live_[i]=pending_[i];bits_=0;}
    float Get(Kind kind) const {return live_[kind];}
};
class Velocity {
    std::array<float,3> live_{},pending_{};bool dirty_=false;
public:
    bool Set(float x,float y,float z,uint32_t deferred) {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||deferred>1)return false;
        if(deferred) {pending_={x,y,z};dirty_=true;}else {live_={x,y,z};dirty_=false;}
        return true;
    }
    void Commit(){if(dirty_)live_=pending_;dirty_=false;}
    const std::array<float,3>& Get() const {return live_;}
};
inline float InverseDistance(float distance,float minimum,float maximum,float rolloff) {
    if(distance<=minimum||minimum<=0||rolloff==0)return 1;
    return minimum/(minimum+rolloff*(std::max(minimum,std::min(distance,maximum))-minimum));
}
inline float DopplerRatio(const std::array<float,3>& to_source,const std::array<float,3>& source_velocity,
                         const std::array<float,3>& listener_velocity,double meters_per_unit,double factor) {
    if(factor==0)return 1;
    double distance=std::hypot(double(to_source[0]),double(to_source[1]),double(to_source[2]));
    if(distance<1e-10)return 1;
    double source=0,listener=0;
    for(unsigned i=0;i<3;i++){source+=source_velocity[i]*to_source[i]/distance;listener+=listener_velocity[i]*to_source[i]/distance;}
    constexpr double speed=343.5;
    // Keep the diagnostic mixer finite near the sonic singularity; supersonic fidelity is unqualified.
    source=std::clamp(source*meters_per_unit*factor,-speed*.99,speed*.99);
    listener=std::clamp(listener*meters_per_unit*factor,-speed*.99,speed*.99);
    return float((speed+listener)/(speed+source));
}
}
