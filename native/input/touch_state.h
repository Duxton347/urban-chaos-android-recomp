#pragma once
#include "core/input.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace urban::touch {
// Console sources, not unverified title actions. Matches the SDK Pad contract.
enum Button { A,B,X,Y,Black,White,LT,RT,Up,Down,Left,Right,Start,Back,LS,RS,Count };
struct Stats {uint64_t presses=0,delivered=0,total_us=0,max_us=0;};
class State {
 struct Key {bool held=false,pending=false,reported=false;uint64_t since=0;};
 struct Stick {int16_t x=0,y=0;bool active=false;};
 std::array<Key,Count> keys_{};
 std::array<Stick,2> sticks_{};
 Stats stats_{};
public:
 void Set(int button,bool down,uint64_t now) {
  if(button<0||button>=Count)return;
  auto& key=keys_[button];if(key.held==down)return;
  key.held=down;
  if(down){if(!key.pending)key.since=now;key.pending=true;++stats_.presses;}
 }
 void Axis(int index,float x,float y,bool active) {
  if(index<0||index>=2)return;
  if(!std::isfinite(x)||!std::isfinite(y)){x=y=0;active=false;}
  auto axis=[](float value){return int16_t(std::lround(std::clamp(value,-1.0f,1.0f)*32767.0f));};
  sticks_[index]={axis(x),axis(y),active};
 }
 void Clear(){keys_={};sticks_={};}
 Stats Snapshot() const{return stats_;}
 input::Pad Merge(input::Pad pad,uint64_t now) {
  pad.connected=true;
  for(int n=0;n<Count;++n){
   auto& key=keys_[n];bool down;
   // Preserve a short tap until a guest poll sees its down edge. Coalesce excess
   // taps instead of building a seconds-long backlog on a slow game thread.
   if(key.pending&&key.reported){down=false;key.reported=false;}
   else if(key.pending){
    down=true;key.pending=false;key.reported=true;++stats_.delivered;
    uint64_t us=now>=key.since?now-key.since:0;stats_.total_us+=us;stats_.max_us=std::max(stats_.max_us,us);
   }else{down=key.held;key.reported=down;}
   if(!down)continue;
   if(n<8)pad.analog[n]=255;else pad.buttons|=uint16_t(1u<<(n-8));
  }
  if(sticks_[0].active){pad.lx=sticks_[0].x;pad.ly=sticks_[0].y;}
  if(sticks_[1].active){pad.rx=sticks_[1].x;pad.ry=sticks_[1].y;}
  return pad;
 }
};
}
