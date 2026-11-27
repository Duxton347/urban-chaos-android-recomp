// Authored guest memory and native-call stubs; this is a hook contract, not enemy AI or a game test.
#include "input/automatic_fire.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "cpu/x86_context.h"
#include "core/input.h"
#include <cassert>
#include <cstdio>
#include <cstring>
namespace kernel {uint8_t* g_base=nullptr;}
extern "C" void sub_000C77D0(X86Context&,uint8_t*);extern "C" void sub_000567F0(X86Context&,uint8_t*);
static double physical=0;
extern "C" void __imp__sub_000C77D0(X86Context& c,uint8_t*){c.eax=kernel::R32(c.ecx+0x38c);c.esp+=4;}
extern "C" void __imp__sub_000567F0(X86Context& c,uint8_t*){c.esp+=16;c.top=(c.top+7)&7;c.st[c.top]=physical;}
int main(){using namespace urban::autofire;assert(mem::Init());kernel::g_base=mem::Base();assert(mem::Commit(0x500000,0x20000));auto owner=mem::AllocSystem(0x1000),target=mem::AllocSystem(0x1000),world=mem::AllocSystem(0x1000),level=mem::AllocSystem(0x3000),controls=mem::AllocSystem(0x1000),stack=mem::AllocStack(0x10000)-256;
 auto W=[](uint32_t a,uint32_t b){std::memcpy(kernel::HostPtr(a),&b,4);};
 W(0x51c4dc,world);W(0x5096e0,controls);W(world+4,level);W(world+12,1);W(owner,0x46fbe8);W(owner+0x54,10);W(owner+0x368,0);W(owner+0x9c,0x42c80000);W(owner+0x4fc,2);W(owner+0x388,20);W(owner+0x38c,1);W(owner+0x448,0xffffffff);W(target+0x9c,0x42c80000);W(level+0x3c+10*8,10);W(level+0x40+10*8,owner);W(level+0x3c+20*8,20);W(level+0x40+20*8,target);W(controls+0x14,1);
 auto observe=[&](uint32_t caller=0xc7cb7){X86Context c{};c.ecx=owner;c.esp=stack;W(stack,caller);sub_000C77D0(c,kernel::g_base);assert(c.esp==stack+4&&c.ecx==owner);};
 auto query=[&](unsigned mode=3,unsigned action=7){X86Context c{};c.esp=stack;W(stack,0xbe953);W(stack+4,0);W(stack+8,action);W(stack+12,mode);c.ecx=world;c.eax=0x12345678;sub_000567F0(c,kernel::g_base);assert(c.esp==stack+16&&c.eax==0x12345678);return c.st[c.top&7];};
 Configure("wrong revision");Enable(true);observe();assert(query()==0);
 Configure("c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1");observe();assert(query()==1);assert(query(4)==0&&query(0)==1&&query(1)==0&&query(2)==1);
 W(world+12,2);observe();assert(query()==0&&query(4)==1);W(world+12,3);observe();assert(query()==1);
 auto blockedCase=[&](uint32_t at,uint32_t bad,uint32_t good){W(world+12,kernel::R32(world+12)+1);W(at,bad);observe();assert(query()==0&&query(2)==0);W(at,good);};
 // Aim assistance may coexist with an exact hit; only its candidate alone is insufficient.
 W(owner+0x448,20);W(world+12,kernel::R32(world+12)+2);observe();assert(query()==1);W(owner+0x448,0xffffffff);
 blockedCase(owner+0x38c,2,1);blockedCase(owner+0x38c,3,1);blockedCase(owner+0x38c,4,1);blockedCase(owner+0x38c,5,1);blockedCase(owner+0x38c,0,1);blockedCase(owner+0x170,13,0);blockedCase(owner+0x170,4,0);blockedCase(owner+0x4fc,0,2);blockedCase(owner+0x4fc,8,2);blockedCase(owner+0x4fc,10,2);blockedCase(owner+0x4fc,11,2);blockedCase(owner+0x9c,0,0x42c80000);blockedCase(target+0x9c,0,0x42c80000);blockedCase(world+16,0x800,0);blockedCase(controls+0x14,0,1);blockedCase(level+0x3c+20*8,1044,20);blockedCase(target+0x5c,1,0);
 W(world+12,kernel::R32(world+12)+1);observe(0xc19fb);assert(query()==0);
 W(owner+0x38c,2);observe();W(owner+0x38c,1);assert(query()==0);
 W(owner+0x38c,0);observe();W(owner+0x38c,1);assert(query()==0);
 W(owner+0x388,0xffffffff);observe();W(owner+0x388,20);assert(query()==0);observe();W(world+12,kernel::R32(world+12)+2);assert(query()==0);observe();Cancel();assert(query()==0);observe();UiAllowed(false);assert(query()==0);UiAllowed(true);observe();Foreground(false);assert(query()==0);Foreground(true);
 W(world+12,kernel::R32(world+12)+2);observe();physical=.75;assert(query()==.75);physical=1;assert(query(3,6)==1);physical=0;assert(query()==0);assert(query(3,6)==0);
 input::Pad p{};p.analog[6]=255;Pad(0,p);observe();assert(query()==0);physical=.75;assert(query()==.75);physical=0;p.analog[6]=0;Pad(0,p);Enable(false);observe();assert(query()==0);physical=1;assert(query()==1);physical=0;
 std::puts("Automatic fire hook: hostile current ray, allied/incapacitated/scenery rejection, stale/identity/weapon/shield/menu/death gates, pulses, context preservation and manual precedence passed; authored memory only");}
