// Own adapter based on consumers verified in the owner's exact XBE. No Burner implementation copied.
#include "input/automatic_fire.h"
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "core/input.h"
#include <atomic>
#include <cstring>
#include <cmath>
namespace {
std::atomic<bool> enabled{false},revision{false},foreground{true},ui{true},shield{false};
std::atomic<uint64_t> epoch{0},traces{0},requests{0},manual{0},blocked{0},queries{0},fresh{0},hostile{0},friendly{0},coneBlocked{0},stateBlocked{0},lifeBlocked{0},shieldBlocked{0},identityBlocked{0},controlsBlocked{0};
struct Observation {uint32_t owner=0,id=0,world=0,level=0,target=0,tick=0,kind=0;uint64_t epoch=0;};
thread_local Observation observation;thread_local urban::autofire::Pulse pulse;
thread_local uint32_t shieldWorld=0,shieldTick=0;thread_local bool nativeShield=false;
bool Active(){return enabled.load()&&revision.load()&&foreground.load()&&ui.load();}
bool Range(uint32_t a,uint32_t size){if(a<mem::kUserLow||uint64_t(a)+size>(uint64_t(1)<<32))return false;for(uint64_t p=a&~uint64_t(4095),last=uint64_t(a)+size;p<last;p+=4096)if(!mem::IsCommitted(uint32_t(p)))return false;return true;}
uint32_t Word(const uint8_t* base,uint32_t a){uint32_t n;std::memcpy(&n,base+a,4);return n;}
uint8_t Byte(const uint8_t* base,uint32_t a){return base[a];}
uint32_t Object(const uint8_t* b,uint32_t level,uint32_t id){if(id==0xffffffff||!Range(level,0x203c))return 0;uint32_t slot=level+0x3c+(id&1023)*8;if(!Range(slot,8)||Word(b,slot)!=id)return 0;return Word(b,slot+4);}
bool Firearm(uint32_t item){return (item>=2&&item<=7)||(item>=16&&item<=24);}
bool Living(const uint8_t* b,uint32_t actor){if(!Range(actor,0xa0)||(Word(b,actor+0x5c)&1))return false;uint32_t health=Word(b,actor+0x9c);return health>0&&health<0x7f800000;}
void TargetRay(const uint8_t* b,uint32_t owner,uint32_t caller){
 observation={};if(!Active())return;
 // Entry immediately after the exact native ray classification, before broader aim assistance.
 if(caller!=0x000c7cb7||!Range(owner,0x890)||Word(b,owner)!=0x46fbe8||Word(b,owner+0x368)!=0||!Range(0x51c4dc,4))return;
 uint32_t world=Word(b,0x51c4dc);if(!Range(world,20))return;uint32_t level=Word(b,world+4),id=Word(b,owner+0x54);
 if(Object(b,level,id)!=owner)return;
 observation={owner,id,world,level,Word(b,owner+0x388),Word(b,world+12),Word(b,owner+0x38c),epoch.load()};++traces;auto kind=Word(b,owner+0x38c);if(kind==1)++hostile;else if(kind==2)++friendly;
}
bool Eligible(const uint8_t* b,uint32_t world,uint32_t tick){
 auto o=observation;if(shield.load()||(nativeShield&&shieldWorld==world&&uint32_t(tick-shieldTick)<=1)){++shieldBlocked;return false;}
 if(!Active()||o.world!=world||uint32_t(tick-o.tick)>1||o.epoch!=epoch.load()||!Range(o.owner,0x890)||Word(b,world+4)!=o.level){++identityBlocked;return false;}
 if(Object(b,o.level,o.id)!=o.owner||Word(b,o.owner)!=0x46fbe8||Word(b,o.owner+0x368)!=0||!Living(b,o.owner)||(Word(b,world+16)&0x800)){++lifeBlocked;return false;}
 // Idle weapon state only. Reload, scripted actions, death and shield attacks cannot start an automatic shot.
 if(Word(b,o.owner+0x170)!=0||!Firearm(Word(b,o.owner+0x4fc))){++stateBlocked;return false;}
 if(o.kind!=1||Word(b,o.owner+0x38c)!=1||Word(b,o.owner+0x388)!=o.target||o.target==o.id)return false;
 // Reject targets selected only by the broad assistance cone: the observed exact ray must match.
 if(o.target==0xffffffff){++coneBlocked;return false;}
 auto target=Object(b,o.level,o.target);if(!target||!Living(b,target)){++lifeBlocked;return false;}
 // The same native input-enabled checks used by the original action query.
 if(!Range(0x5096e0,4))return false;auto controls=Word(b,0x5096e0);if(!Range(controls,0x18))return false;
 bool allowed=Byte(b,controls+0x10)!=0xff&&Byte(b,controls+0x14)!=0&&Byte(b,controls+0x0c)!=0xff;if(!allowed)++controlsBlocked;return allowed;
}
}
namespace urban::autofire {
void Configure(const char* hash){revision=hash&&std::strcmp(hash,"c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1")==0;Cancel();}
void Enable(bool value){enabled=value;Cancel();}
void Foreground(bool value){foreground=value;Cancel();}
void UiAllowed(bool value){ui=value;Cancel();}
void Cancel(){++epoch;}
void Pad(int port,input::Pad& p){if(port==0)shield=p.analog[6]>0;}
Stats Snapshot(){return {traces.load(),requests.load(),manual.load(),blocked.load(),queries.load(),fresh.load(),hostile.load(),friendly.load(),coneBlocked.load(),stateBlocked.load(),lifeBlocked.load(),shieldBlocked.load(),identityBlocked.load(),controlsBlocked.load(),enabled.load()};}
}
extern "C" void __imp__sub_000C77D0(X86Context&,uint8_t*);
extern "C" void __imp__sub_000567F0(X86Context&,uint8_t*);
extern "C" void sub_000C77D0(X86Context& c,uint8_t* b){uint32_t caller=Range(c.esp,4)?Word(b,c.esp):0;TargetRay(b,c.ecx,caller);__imp__sub_000C77D0(c,b);}
extern "C" void sub_000567F0(X86Context& c,uint8_t* b){
 uint32_t world=c.ecx,slot=~0u,action=~0u,mode=~0u;
 if(Active()&&Range(c.esp,16)){slot=Word(b,c.esp+4);action=Word(b,c.esp+8);mode=Word(b,c.esp+12);}
 __imp__sub_000567F0(c,b);
 if(slot!=0||(action!=7&&action!=6)||mode>4||!Range(world,20)||!Range(0x51c4dc,4)||Word(b,0x51c4dc)!=world)return;
 uint32_t tick=Word(b,world+12);if(action==6){if(mode==3){nativeShield=c.st[c.top&7]>0;shieldWorld=world;shieldTick=tick;}return;}
 ++queries;if(observation.world==world&&uint32_t(tick-observation.tick)<=1)++fresh;bool eligible=Eligible(b,world,tick);auto o=observation;
 uint32_t weapon=Range(o.owner,0x500)?Word(b,o.owner+0x4fc):~0u;
 bool prior=pulse.current;uint32_t priorTick=pulse.tick;pulse.Update(world,o.id,weapon,tick,eligible);
 if(pulse.current&&(!prior||priorTick!=tick))++requests;
 double original=c.st[c.top&7];if(!std::isfinite(original)||original>0){++manual;return;}
 double automatic=pulse.Value(mode);if(automatic>0)c.st[c.top&7]=automatic;else if(!eligible)++blocked;
}
