#pragma once
// Xbox D3DTSS_COLORSIGN: unsigned source bytes become signed normalized
// components before spatial/mip/cube filtering. Keep unsigned components intact.
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
#include <stdexcept>
namespace urban::gpu {
inline uint32_t ColourSign(uint32_t state){return state&0xf0000000u;}
inline bool ByteColourFormat(uint32_t f){return f==0x06||f==0x12||f==0x3a||f==0x3b||f==0x3c||f==0x3f||f==0x40||f==0x41;}
inline int16_t SignedChannel16(uint8_t byte,bool sign){
 if(!sign)return int16_t((uint32_t(byte)*32767u+127u)/255u);
 int value=byte<128?int(byte):int(byte)-256;
 if(value<=-127)return -32767;
 int n=value*32767;return int16_t((n+(n<0?-63:63))/127);
}
inline void SignedTexels(std::span<const uint8_t> bgra,uint32_t sign,std::vector<uint8_t>& rgba16){
 if(bgra.size()%4||bgra.size()>rgba16.max_size()/2)throw std::runtime_error("UC-VK-008: invalid signed texture extent");
 rgba16.resize(bgra.size()*2);
 constexpr uint32_t bits[]={0x20000000u,0x40000000u,0x80000000u,0x10000000u};
 constexpr unsigned index[]={2,1,0,3};
 for(size_t at=0;at<bgra.size();at+=4)for(unsigned c=0;c<4;c++){
  int16_t value=SignedChannel16(bgra[at+index[c]],(sign&bits[c])!=0);
  std::memcpy(rgba16.data()+at*2+c*2,&value,2);
 }
}
}
