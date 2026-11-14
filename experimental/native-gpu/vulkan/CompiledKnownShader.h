// SPDX-License-Identifier: MIT
#ifndef RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#define RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#include "KnownSpirv.h"
#include "../render/draw/RcsDrawGenerated.h"
#include "../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include <string.h>

// This bridge admits two exact Vulkan 1.0 fragment fixtures, both now proven
// on the N100 with the matching native owner.
static bool CompileKnownFragment(const uint32_t* vertex,size_t vertexBytes,
 const uint32_t* fragment,size_t fragmentBytes,void* output,size_t capacity)
{
 if(!vertex || !fragment || !output || vertexBytes!=sizeof(RenkuKnownSpirv::Vertex)
  || capacity<sizeof(NativeGpu::RcsDrawData::Fs)
  || memcmp(vertex,RenkuKnownSpirv::Vertex,vertexBytes))return false;
 const bool original=fragmentBytes==sizeof(RenkuKnownSpirv::Fragment)
  && !memcmp(fragment,RenkuKnownSpirv::Fragment,fragmentBytes);
 const bool half=fragmentBytes==sizeof(RenkuKnownSpirv::FragmentHalf)
  && !memcmp(fragment,RenkuKnownSpirv::FragmentHalf,fragmentBytes);
 if(!original && !half)return false;
 uint8_t vs[sizeof(NativeGpu::RcsDrawData::Vs)]={};
 uint8_t fs[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 char error[256]={};
 uint32_t expectedFs[sizeof(NativeGpu::RcsDrawData::Fs)/4];
 memcpy(expectedFs,NativeGpu::RcsDrawData::Fs,sizeof(expectedFs));
 if(half){expectedFs[7]=0x3f000000;expectedFs[39]=0x3f000000;}
 if(renku_intel_compile_vulkan_pair(vertex,vertexBytes/4,
   fragment,fragmentBytes/4,&pair,error,sizeof(error))!=RENKU_INTEL_COMPILE_OK
  || pair.vertex.bytes!=sizeof(vs) || pair.fragment.bytes!=sizeof(fs)
  || pair.vertex.total_scratch || pair.fragment.total_scratch
  || pair.vertex.num_relocs || pair.fragment.num_relocs
  || memcmp(vs,NativeGpu::RcsDrawData::Vs,sizeof(vs))
  || memcmp(fs,expectedFs,sizeof(fs)))return false;
 memcpy(output,fs,sizeof(fs));return true;
}
#endif
