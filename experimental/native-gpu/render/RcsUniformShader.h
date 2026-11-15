// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_UNIFORM_SHADER_H
#define NATIVE_GPU_RCS_UNIFORM_SHADER_H
#include "RcsUboFragment.h"

namespace NativeGpu {
// Exact N100 fragment program compiled from a Vulkan set0/binding0 UBO load.
class RcsUniformShader {
public:
 static bool ValidatePage(const uint32_t* words,unsigned dwords) {
  if(!words || dwords!=1024)return false;
  for(unsigned i=0;i<1024;i++)
   if(words[i]!=(i<84?RcsUboFragment::Words[i]:0))return false;
  return true;
 }
 static bool ExpectedPixel(const uint32_t* rgba,uint32_t& pixel) {
  if(!rgba)return false;
  for(unsigned i=0;i<4;i++)if(rgba[i]>0x3f800000)return false;
  pixel=(Unorm8(rgba[3])<<24)|(Unorm8(rgba[0])<<16)
   |(Unorm8(rgba[1])<<8)|Unorm8(rgba[2]);
  return true;
 }
private:
 static uint32_t Unorm8(uint32_t bits) {
  const uint32_t exponent=bits>>23;
  if(exponent<=86)return 0;
  const uint32_t mantissa=(bits&0x7fffff)|0x800000;
  const unsigned shift=150-exponent;
  return uint32_t((uint64_t(mantissa)*255+(uint64_t(1)<<(shift-1)))>>shift);
 }
};
}
#endif
