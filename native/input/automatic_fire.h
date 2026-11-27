#pragma once
#include <cstdint>
namespace urban::autofire {
// Input requests only. Weapon cadence/ammunition/projectiles remain in the original game.
struct Pulse {
 uint32_t tick=0,world=0,actor=0,weapon=0;bool valid=false,current=false,previous=false;
 void Clear(){*this={};}
 void Update(uint32_t w,uint32_t a,uint32_t item,uint32_t frame,bool eligible){
  if(!valid||w!=world||a!=actor||item!=weapon||uint32_t(frame-tick)>1){world=w;actor=a;weapon=item;tick=frame;valid=true;previous=false;current=eligible;return;}
  if(frame==tick){if(!eligible)current=false;return;}
  previous=current;current=eligible&&!previous;tick=frame;
 }
 double Value(unsigned mode)const{switch(mode){case 0:return current;case 1:return previous;case 2:return current&&!previous;case 3:return current?1.0:0.0;case 4:return previous?1.0:0.0;default:return 0;}}
};
void Configure(const char* xbe_sha256);
void Enable(bool enabled);
void Foreground(bool active);
void UiAllowed(bool allowed);
void Cancel();
struct Stats {uint64_t traces,requests,manual,blocked,queries,fresh,hostile,friendly,coneBlocked,stateBlocked,lifeBlocked,shieldBlocked,identityBlocked,controlsBlocked;bool enabled;};
Stats Snapshot();
}
namespace input {struct Pad;}
namespace urban::autofire {void Pad(int port,input::Pad&);}
