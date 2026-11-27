// Authored packet sequences; never load or capture a commercial push buffer.
#include "gpu/recorded_push.h"
#include <cassert>
#include <cstdio>
using namespace urban::gpu;
static uint32_t Header(uint32_t method,uint32_t count,bool fixed=false) {
    return method|(count<<18)|(fixed?0x40000000u:0);
}
int main() {
    std::vector<IndexedDraw> draws;std::string error;
    std::vector<uint32_t> good={Header(0x17FC,1),5,Header(0x1800,2,true),0x00020001,0xFFFF0003,
        Header(0x17FC,1),0,Header(0x100,2,true),0,99,0,
        Header(0x17FC,1),6,Header(0x1800,1,true),0x00080007,Header(0x17FC,1),0};
    assert(DecodeRecordedIndices(good,draws,error));
    assert(draws.size()==2&&draws[0].primitive==5&&draws[1].primitive==6);
    assert((draws[0].indices==std::vector<uint16_t>{1,2,3,65535}));
    assert((draws[1].indices==std::vector<uint16_t>{7,8}));
    auto returning=good;returning.push_back(0x00020000u);
    assert(DecodeRecordedIndices(returning,draws,error)&&draws.size()==2);
    returning.push_back(0);assert(!DecodeRecordedIndices(returning,draws,error)&&draws.empty());
    assert(!DecodeRecordedIndices(std::vector<uint32_t>{Header(0x17FC,1),5,0x00020000u},draws,error));
    std::vector<uint32_t> indices32={Header(0x17FC,1),5,Header(0x1808,3,true),4,9,65535,Header(0x17FC,1),0};
    assert(DecodeRecordedIndices(indices32,draws,error)&&draws.size()==1);
    assert((draws[0].indices==std::vector<uint16_t>{4,9,65535}));
    indices32[4]=65536;
    assert(!DecodeRecordedIndices(indices32,draws,error)&&draws.empty());
    for(std::vector<uint32_t> bad:std::vector<std::vector<uint32_t>>{
        {Header(0x17FC,1)}, {Header(0x17FC,1),11}, {Header(0x17FC,1),5},
        {Header(0x17FC,1),0}, {Header(0x1800,1,true),1},
        {Header(0x17FC,1),5,Header(0x17FC,1),6},
        {Header(0x1808,1),1},{0x20000000},{0x00002000},{0x00000100},
        {Header(0x17FC,1),5,Header(0x1800,2),1,2,Header(0x17FC,1),0}}) {
        assert(!DecodeRecordedIndices(bad,draws,error)&&!error.empty()&&draws.empty());
    }
    auto partial=good;partial.push_back(Header(0x1808,1));partial.push_back(1);
    assert(!DecodeRecordedIndices(partial,draws,error)&&draws.empty()); // validate the whole stream before drawing
    assert(DecodeRecordedIndices({},draws,error)&&draws.empty());
    for(uint32_t linkage:{0u,9u,0xABCDE001u,0x00020000u}) {
        auto storage=good;storage.push_back(linkage);
        assert(DecodeXdkRecordedIndices(storage,draws,error)&&draws.size()==2);
        assert((draws[0].indices==std::vector<uint16_t>{1,2,3,65535}));
        assert((draws[1].indices==std::vector<uint16_t>{7,8}));
    }
    assert(!DecodeXdkRecordedIndices({},draws,error)&&draws.empty());
    auto internal_jump=good;internal_jump.push_back(9);internal_jump.push_back(0);
    assert(!DecodeXdkRecordedIndices(internal_jump,draws,error)&&draws.empty());
    auto damaged=good;damaged.push_back(Header(0x1800,2,true));damaged.push_back(0);
    assert(!DecodeXdkRecordedIndices(damaged,draws,error)&&draws.empty());
    std::puts("RECORDED_INDEX_PACKETS: two draws, bounded 32-bit conversion, terminal return, ordering, little-endian index pairs, NOPs, XDK linkage boundary, atomic validation, malformed/control/unknown packet rejection passed; authored packets only");
}
