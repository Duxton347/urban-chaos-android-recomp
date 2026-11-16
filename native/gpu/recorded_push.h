#pragma once
// Authored decoder for an explicit subset of NV2A 3D method packets.
// No title instructions, captured payloads or title-specific hooks.
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <utility>
namespace urban::gpu {
struct IndexedDraw { uint32_t primitive; std::vector<uint16_t> indices; };
inline bool DecodeRecordedIndices(std::span<const uint32_t> words, std::vector<IndexedDraw>& draws, std::string& error) {
    draws.clear(); error.clear();
    auto fail=[&](const char* reason) { draws.clear();error=reason;return false; };
    if(words.size()>4u*1024*1024)return fail("recorded buffer exceeds decoder bound");
    IndexedDraw current{};
    for(size_t p=0;p<words.size();) {
        uint32_t header=words[p++],count=(header>>18)&2047,method=header&0x1FFC;
        if(header==0x00020000u) { // NV2A return from this top-level recorded buffer
            if(current.primitive||p!=words.size())return fail("nonterminal or mid-primitive GPU return");
            return true;
        }
        if(header&0xA003E003u) {
            draws.clear();error="unsupported GPU control/subchannel flags "+std::to_string(header&0xA003E003u)+" at word "+std::to_string(p-1);
            return false;
        }
        if(count>words.size()-p)return fail("truncated method packet");
        if(!count) { if(header)return fail("unsupported empty packet");continue; }
        for(uint32_t i=0;i<count;i++) {
            uint32_t m=header&0x40000000u ? method : method+4*i,value=words[p++];
            if(m==0x0100)continue; // NV097_NO_OPERATION has no rendering side effect
            if(m==0x17FC) {
                if(value) {
                    if(current.primitive||value>10)return fail("invalid or nested begin");
                    current={value,{}};
                } else {
                    if(!current.primitive)return fail("end without begin");
                    if(!current.indices.empty())draws.push_back(std::move(current));
                    current={};
                }
            } else if(m==0x1800) { // NV097_ARRAY_ELEMENT16, two little-endian indices per word
                if(!current.primitive)return fail("indices outside primitive");
                current.indices.push_back(uint16_t(value));
                current.indices.push_back(uint16_t(value>>16));
            } else if(m==0x1808) { // NV097_ARRAY_ELEMENT32; current backend consumes 16-bit indices
                if(!current.primitive)return fail("indices outside primitive");
                if(value>65535)return fail("32-bit recorded index exceeds backend range");
                current.indices.push_back(uint16_t(value)); // lossless within the explicitly supported range
            } else {
                draws.clear();
                error="unsupported recorded rendering method "+std::to_string(m)+" at word "+std::to_string(p-1);
                return false;
            }
        }
    }
    if(current.primitive)return fail("unterminated primitive");
    return true;
}
// XDK D3DPushBuffer::Size includes a final DWORD reserved for submission linkage.
// RunPushBuffer either patches it to the caller's GPU continuation or copies Size-4 bytes.
// This adapter models the return to the host submitter; the generic packet decoder
// still rejects unsupported control flow anywhere in the executable payload.
inline bool DecodeXdkRecordedIndices(std::span<const uint32_t> storage, std::vector<IndexedDraw>& draws, std::string& error) {
    if(storage.empty()) { draws.clear();error="XDK recorded buffer is missing its linkage word";return false; }
    return DecodeRecordedIndices(storage.first(storage.size()-1),draws,error);
}

}
