#include "../../vulkan/KnownSpirv.h"
#include "../../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include "../../render/draw/RcsDrawGenerated.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 uint32_t green=(uint32_t)strtoul(argv[1],nullptr,16);
 uint32_t fragment[sizeof(RenkuKnownSpirv::FragmentHalf)/4];
 memcpy(fragment,RenkuKnownSpirv::FragmentHalf,sizeof(fragment));
 unsigned replaced=0;
 for(uint32_t& word:fragment)if(word==0x3f000000u){word=green;replaced++;}
 if(replaced!=1)return 2;
 uint8_t vs[sizeof(NativeGpu::RcsDrawData::Vs)]={};
 uint8_t fs[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 char error[256]={};
 int status=renku_intel_compile_vulkan_pair(RenkuKnownSpirv::Vertex,
  sizeof(RenkuKnownSpirv::Vertex)/4,fragment,sizeof(fragment)/4,&pair,error,sizeof(error));
 uint32_t words[sizeof(fs)/4]={};memcpy(words,fs,sizeof(fs));
 unsigned otherDiff=0;
 for(unsigned i=0;i<sizeof(words)/4;i++)
  if(i!=7 && i!=39 && words[i]!=NativeGpu::RcsDrawData::Fs[i])otherDiff++;
 printf("green=%08x status=%d error=%s vs=%zu fs=%zu scratch=%u/%u reloc=%u/%u vs_exact=%d other_diff=%u immediates=%08x/%08x\n",
  green,status,error,pair.vertex.bytes,pair.fragment.bytes,
  pair.vertex.total_scratch,pair.fragment.total_scratch,
  pair.vertex.num_relocs,pair.fragment.num_relocs,
  !memcmp(vs,NativeGpu::RcsDrawData::Vs,sizeof(vs)),otherDiff,words[7],words[39]);
 return status?3:0;
}
