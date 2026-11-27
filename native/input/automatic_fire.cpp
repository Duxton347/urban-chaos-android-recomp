// Own adapter based on consumers verified in the owner's exact XBE. No Burner implementation copied.
#include "input/automatic_fire.h"
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "core/input.h"
#include <atomic>
#include <cstring>
#include <mutex>
namespace {
std::atomic<bool> enabled{false},revision{false},foreground{true},ui{true},shield{false};
std::atomic<uint64_t> epoch{0},traces{0},requests{0},manual{0},blocked{0},queries{0},fresh{0},hostile{0},friendly{0},coneBlocked{0},stateBlocked{0},lifeBlocked{0},shieldBlocked{0},identityBlocked{0},controlsBlocked{0};
struct Observation {uint32_t owner=0,id=0,world=0,level=0,target=0,tick=0,kind=0;uint64_t epoch=0;};
Observation observation;urban::autofire::Pulse pulse;std::mutex observationMutex,pulseMutex;
Observation ReadObservation(){std::lock_guard lock(observationMutex);return observation;}

bool Active(){return enabled.load()&&revision.load()&&foreground.load()&&ui.load();}
bool Range(uint32_t a,uint32_t size){if(a<mem::kUserLow||uint64_t(a)+size>(uint64_t(1)<<32))return false;for(uint64_t p=a&~uint64_t(4095),last=uint64_t(a)+size;p<last;p+=4096)if(!mem::IsCommitted(uint32_t(p)))return false;return true;}
uint32_t Word(const uint8_t* base,uint32_t a){uint32_t n;std::memcpy(&n,base+a,4);return n;}
uint8_t Byte(const uint8_t* base,uint32_t a){return base[a];}
uint32_t Object(const uint8_t* b,uint32_t level,uint32_t id){if(id==0xffffffff||!Range(level,0x203c))return 0;uint32_t slot=level+0x3c+(id&1023)*8;if(!Range(slot,8)||Word(b,slot)!=id)return 0;return Word(b,slot+4);}
bool Firearm(uint32_t item){return (item>=2&&item<=7)||(item>=16&&item<=24);}
bool Living(const uint8_t* b,uint32_t actor){if(!Range(actor,0xa0)||(Word(b,actor+0x5c)&1))return false;uint32_t health=Word(b,actor+0x9c);return health>0&&health<0x7f800000;}
void TargetRay(const uint8_t* b,uint32_t owner,uint32_t caller){
 std::lock_guard lock(observationMutex);observation={};if(!Active())return;
 // Entry immediately after the exact native ray classification, before broader aim assistance.
 if(caller!=0x000c7cb7||!Range(owner,0x890)||Word(b,owner)!=0x46fbe8||Word(b,owner+0x368)!=0||!Range(0x51c4dc,4))return;
 uint32_t world=Word(b,0x51c4dc);if(!Range(world,20))return;uint32_t level=Word(b,world+4),id=Word(b,owner+0x54);
 if(Object(b,level,id)!=owner)return;
 observation={owner,id,world,level,Word(b,owner+0x388),Word(b,world+12),Word(b,owner+0x38c),epoch.load()};++traces;auto kind=Word(b,owner+0x38c);if(kind==1)++hostile;else if(kind==2)++friendly;
}
bool Eligible(const uint8_t* b,uint32_t world,uint32_t tick,const Observation& o){
 if(shield.load()){++shieldBlocked;return false;}
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
// Feed the existing physical-controller route; do not replace the game's action query/ABI.
void Pad(int port,input::Pad& p){
 if(port!=0)return;
 shield=p.analog[6]>0;
 if(!p.connected||!Active()||!kernel::g_base||!Range(0x51c4dc,4))return;
 auto* b=kernel::g_base;auto world=Word(b,0x51c4dc);if(!Range(world,0xa18))return;
 std::lock_guard lock(pulseMutex);auto o=ReadObservation();
 ++queries;auto tick=Word(b,world+12);if(o.world==world&&uint32_t(tick-o.tick)<=1)++fresh;
 auto scheme=Word(b,world+0x948);bool fire=false,guard=false,conflict=false;
 if(scheme<3&&Range(world+0x958+scheme*192,192))for(unsigned i=0;i<24;++i){
  auto at=world+0x958+scheme*192+i*8,physical=Word(b,at),action=Word(b,at+4);
  if(physical==22){fire|=action==7;conflict|=action!=7;}
  if(physical==23){guard|=action==6;conflict|=action!=6;}
 }
 if(!fire||!guard||conflict)++controlsBlocked;
 bool eligible=fire&&guard&&!conflict&&Eligible(b,world,tick,o);
 auto weapon=Range(o.owner,0x500)?Word(b,o.owner+0x4fc):~0u;
 bool prior=pulse.current;auto priorTick=pulse.tick;pulse.Update(world,o.id,weapon,tick,eligible);
 if(p.analog[7]){++manual;return;}
 if(pulse.current){p.analog[7]=255;if(!prior||priorTick!=tick)++requests;}
 else if(!eligible)++blocked;
}
Stats Snapshot(){return {traces.load(),requests.load(),manual.load(),blocked.load(),queries.load(),fresh.load(),hostile.load(),friendly.load(),coneBlocked.load(),stateBlocked.load(),lifeBlocked.load(),shieldBlocked.load(),identityBlocked.load(),controlsBlocked.load(),enabled.load()};}
}
extern "C" void __imp__sub_000C77D0(X86Context&,uint8_t*);
extern "C" void sub_000C77D0(X86Context& c,uint8_t* b){uint32_t caller=Range(c.esp,4)?Word(b,c.esp):0;TargetRay(b,c.ecx,caller);__imp__sub_000C77D0(c,b);}
