// SPDX-License-Identifier: MIT
// Read-only Mesa compiler matrix for Vulkan constant-color fragment programs.
#include "../../vulkan/KnownSpirv.h"
#include "../../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include "../../render/draw/RcsDrawGenerated.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

static bool SetFloat(std::vector<uint32_t>& words,uint32_t id,uint32_t bits)
{
 for(size_t i=5;i<words.size();){
  unsigned count=words[i]>>16;
  if(!count || i+count>words.size())return false;
  if((words[i]&0xffff)==43 && count==4 && words[i+2]==id){
   words[i+3]=bits;return true;
  }
  i+=count;
 }
 return false;
}

static bool SplitAlpha(std::vector<uint32_t>& words)
{
 for(size_t i=5;i<words.size();){
  unsigned count=words[i]>>16;
  if(!count || i+count>words.size())return false;
  if((words[i]&0xffff)==44 && count==7){
   uint32_t alphaId=words[3]++;
   words[i+6]=alphaId;
   words.insert(words.begin()+i,{0x0004002b,5,alphaId,0x3f800000});
   return true;
  }
  i+=count;
 }
 return false;
}

static void Run(const char* name,uint32_t red,uint32_t green,uint32_t blue,
 bool separateAlpha)
{
 std::vector<uint32_t> spv(std::begin(RenkuKnownSpirv::Fragment),
  std::end(RenkuKnownSpirv::Fragment));
 if(separateAlpha && !SplitAlpha(spv))return;
 if(!SetFloat(spv,8,red) || !SetFloat(spv,9,green)
  || !SetFloat(spv,10,blue))return;
 uint8_t vs[1024]={},fs[1024]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 char error[256]={};
 int status=renku_intel_compile_vulkan_pair(RenkuKnownSpirv::Vertex,
  sizeof(RenkuKnownSpirv::Vertex)/4,spv.data(),spv.size(),
  &pair,error,sizeof(error));
 unsigned diffs=0;
 printf("%s status=%d error=%s spv=%zu vs=%zu fs=%zu scratch=%u/%u reloc=%u/%u vs_exact=%d",
  name,status,error,spv.size(),pair.vertex.bytes,pair.fragment.bytes,
  pair.vertex.total_scratch,pair.fragment.total_scratch,
  pair.vertex.num_relocs,pair.fragment.num_relocs,
  pair.vertex.bytes==sizeof(NativeGpu::RcsDrawData::Vs)
   && !memcmp(vs,NativeGpu::RcsDrawData::Vs,sizeof(NativeGpu::RcsDrawData::Vs)));
 for(unsigned i=0;i<std::min(pair.fragment.bytes,sizeof(fs))/4;i++){
  uint32_t word;memcpy(&word,fs+4*i,4);
  if(i>=sizeof(NativeGpu::RcsDrawData::Fs)/4
   || word!=NativeGpu::RcsDrawData::Fs[i]){
   if(diffs<24)printf(" %u:%08x",i,word);
   diffs++;
  }
 }
 printf(" diffs=%u\n",diffs);
}

int main()
{
 Run("baseline",0x3f800000,0x3e800000,0x00000000,false);
 Run("blue_half",0x3f800000,0x3e800000,0x3f000000,false);
 Run("blue_one",0x3f800000,0x3e800000,0x3f800000,false);
 Run("red_half",0x3f000000,0x3e800000,0x00000000,true);
 Run("red_zero",0x00000000,0x3e800000,0x00000000,true);
 Run("rgb_mixed",0x3f000000,0x3f400000,0x3f000000,true);
 Run("rgb_distinct",0x3f000000,0x3f400000,0x3e800000,true);
 Run("rgb_equal_half",0x3f000000,0x3f000000,0x3f000000,true);
 Run("rgb_equal_zero",0x00000000,0x00000000,0x00000000,true);
 Run("rgb_equal_one",0x3f800000,0x3f800000,0x3f800000,false);
 Run("green_one",0x3f800000,0x3f800000,0x00000000,false);
 Run("red_blue_equal",0x3e800000,0x3f000000,0x3e800000,true);
}
