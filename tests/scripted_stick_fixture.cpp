// Authored host-input checks; does not load a title or select targets.
#include "app/app.h"
#include "core/input.h"
#include "harness.h"
#include <cassert>
#include <fstream>
#include <string>
const app::GameInfo app::kGame={};
std::filesystem::path app::ExeDir(){return std::filesystem::current_path();}
int main(){
 struct Axis {const char* name;int rx,ry;};
 for(auto a:{Axis{"RSU",0,32767},Axis{"RSD",0,-32767},Axis{"RSL",-32767,0},Axis{"RSR",32767,0}}){
  harness::g_frames+=10;{std::ofstream f("cmd");f<<a.name<<" 3\n";}
  auto p=input::Poll(0);assert(p.connected&&p.rx==a.rx&&p.ry==a.ry&&!p.buttons);
  for(auto v:p.analog)assert(v==0);
  assert(!std::filesystem::exists("cmd"));harness::g_frames+=4;
  p=input::Poll(0);assert(p.rx==0&&p.ry==0);
 }
 harness::g_frames+=10;{std::ofstream f("cmd");f<<"LS+RSR+A 3\n";}
 auto p=input::Poll(0);assert(p.ly==32767&&p.rx==32767&&p.analog[0]==255&&p.analog[7]==0);
 p=input::Poll(1);assert(!p.connected&&p.rx==0&&p.ly==0);
 std::puts("Scripted right-stick direction, expiry, consumption, combinations and no implicit fire passed");
}
