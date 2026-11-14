// SPDX-License-Identifier: MIT
// Test-only variation of the pinned Vulkan 1.0 constant-color SPIR-V module.
#ifndef RENKU_NATIVE_RGB_SPIRV_FIXTURE_H
#define RENKU_NATIVE_RGB_SPIRV_FIXTURE_H
#include "KnownSpirv.h"
#include <iterator>
#include <vector>

static std::vector<uint32_t> RgbSpirvFixture(uint32_t red,uint32_t green,
 uint32_t blue,bool separateAlpha)
{
 std::vector<uint32_t> code(std::begin(RenkuKnownSpirv::Fragment),
  std::end(RenkuKnownSpirv::Fragment));
 if(separateAlpha){
  bool found=false;
  for(size_t i=5;i<code.size();){
   unsigned count=code[i]>>16;
   if(!count || i+count>code.size())return {};
   if((code[i]&0xffff)==44 && count==7 && code[i+2]==11){
    uint32_t alpha=code[3]++;
    code[i+6]=alpha;
    code.insert(code.begin()+i,{0x0004002b,5,alpha,0x3f800000});
    found=true;break;
   }
   i+=count;
  }
  if(!found)return {};
 }
 unsigned changed=0;
 for(size_t i=5;i<code.size();){
  unsigned count=code[i]>>16;
  if(!count || i+count>code.size())return {};
  if((code[i]&0xffff)==43 && count==4 && code[i+1]==5){
   if(code[i+2]==8){code[i+3]=red;changed|=1;}
   if(code[i+2]==9){code[i+3]=green;changed|=2;}
   if(code[i+2]==10){code[i+3]=blue;changed|=4;}
  }
  i+=count;
 }
 return changed==7?code:std::vector<uint32_t>{};
}
#endif
