// Authored memory and native-ray stub. No title is loaded and no target tracking is performed.
#include "input/automatic_fire.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "cpu/x86_context.h"
#include "core/input.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
namespace kernel {uint8_t* g_base=nullptr;}
extern "C" void sub_000C77D0(X86Context&,uint8_t*);
extern "C" void __imp__sub_000C77D0(X86Context& c,uint8_t*){c.esp+=4;}
int main(){
 using namespace urban::autofire;
 assert(mem::Init());kernel::g_base=mem::Base();assert(mem::Commit(0x500000,0x20000));
 auto owner=mem::AllocSystem(0x1000),target=mem::AllocSystem(0x1000),world=mem::AllocSystem(0x1000);
 auto level=mem::AllocSystem(0x3000),controls=mem::AllocSystem(0x1000),stack=mem::AllocStack(0x10000)-256;
 auto W=[](uint32_t a,uint32_t b){std::memcpy(kernel::HostPtr(a),&b,4);};
 W(0x51c4dc,world);W(0x5096e0,controls);W(world+4,level);W(world+12,1);
 W(owner,0x46fbe8);W(owner+0x54,10);W(owner+0x368,0);W(owner+0x9c,0x42c80000);
 W(owner+0x4fc,2);W(owner+0x388,20);W(owner+0x38c,1);W(target+0x9c,0x42c80000);
 W(level+0x3c+10*8,10);W(level+0x40+10*8,owner);W(level+0x3c+20*8,20);W(level+0x40+20*8,target);
 W(controls+0x14,1);W(world+0x958,22);W(world+0x95c,7);W(world+0x960,23);W(world+0x964,6);
 auto observe=[&](uint32_t caller=0xc7cb7){X86Context c{};c.ecx=owner;c.esp=stack;W(stack,caller);sub_000C77D0(c,kernel::g_base);assert(c.esp==stack+4&&c.ecx==owner);};
 auto poll=[&](unsigned manual=0,unsigned guard=0,int port=0){input::Pad p{};p.connected=true;p.analog[7]=manual;p.analog[6]=guard;p.lx=123;p.ry=-456;Pad(port,p);assert(p.lx==123&&p.ry==-456&&p.analog[6]==guard);return p.analog[7];};
 Configure("wrong revision");Enable(true);observe();assert(poll()==0);
 Configure("c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1");observe();assert(poll()==255&&poll()==255);
 W(world+12,2);observe();assert(poll()==0);W(world+12,3);observe();assert(poll()==255);
 auto blockedCase=[&](uint32_t at,uint32_t bad,uint32_t good){W(world+12,kernel::R32(world+12)+1);W(at,bad);observe();assert(poll()==0);W(at,good);};
 for(auto kind:{0u,2u,3u,4u,5u})blockedCase(owner+0x38c,kind,1);
 for(auto weapon:{0u,1u,8u,10u,11u,15u,25u})blockedCase(owner+0x4fc,weapon,2);
 blockedCase(owner+0x170,13,0);blockedCase(owner+0x170,4,0);blockedCase(owner+0x9c,0,0x42c80000);
 blockedCase(target+0x9c,0,0x42c80000);blockedCase(world+16,0x800,0);blockedCase(controls+0x14,0,1);
 blockedCase(level+0x3c+20*8,1044,20);blockedCase(target+0x5c,1,0);
 blockedCase(world+0x95c,6,7);blockedCase(world+0x964,7,6);blockedCase(world+0x948,3,0);
 W(world+12,kernel::R32(world+12)+2);observe(0xc19fb);assert(poll()==0);
 W(owner+0x38c,2);observe();W(owner+0x38c,1);assert(poll()==0);
 W(owner+0x388,0xffffffff);observe();W(owner+0x388,20);assert(poll()==0);
 observe();W(world+12,kernel::R32(world+12)+2);assert(poll()==0);observe();Cancel();assert(poll()==0);
 observe();UiAllowed(false);assert(poll()==0);UiAllowed(true);observe();Foreground(false);assert(poll()==0);Foreground(true);
 W(world+12,kernel::R32(world+12)+2);observe();assert(poll(128)==128);assert(poll(0,255)==0);assert(poll(200,255)==200);
 assert(poll(0,0,1)==0);Enable(false);observe();assert(poll()==0&&poll(255)==255);Enable(true);
 W(world+12,kernel::R32(world+12)+2);observe();auto before=Snapshot().requests;
 std::vector<std::thread> readers;for(unsigned i=0;i<8;++i)readers.emplace_back([&]{assert(poll()==255);});
 for(auto& t:readers)t.join();assert(Snapshot().requests==before+1);
 std::puts("Automatic RT: exact native ray, allied/stale/identity/life/weapon/shield/modal gates, compatible bindings, pulses, cross-thread delivery and manual precedence passed; authored memory only");
}
