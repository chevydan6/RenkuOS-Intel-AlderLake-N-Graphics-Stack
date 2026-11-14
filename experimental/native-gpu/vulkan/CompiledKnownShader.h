// SPDX-License-Identifier: MIT
#ifndef RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#define RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#include "KnownSpirv.h"
#include "../render/RcsPublicShader.h"
#include "../render/RcsDepthVertex.h"
#include "../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include <string.h>

// Compile Vulkan 1.0 SPIR-V, then admit only owner-approved N100 programs.
// The owner uses version 7 for the original VS and version 8 for the depth VS.
static bool CompileKnownPipeline(const uint32_t* vertex,size_t vertexBytes,
 const uint32_t* fragment,size_t fragmentBytes,void* vertexCode,
 size_t vertexCapacity,size_t* vertexCodeBytes,void* fragmentCode,
 size_t fragmentCapacity)
{
 if(!vertex || !fragment || !vertexCode || !vertexCodeBytes || !fragmentCode
  || !vertexBytes || (vertexBytes&3)
  || !fragmentBytes || (fragmentBytes&3)
  || vertexCapacity<sizeof(NativeGpu::RcsDepthVertex::Words)
  || fragmentCapacity<sizeof(NativeGpu::RcsDrawData::Fs))return false;
 uint8_t vs[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 uint8_t fs[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 char error[256]={};
 if(renku_intel_compile_vulkan_pair(vertex,vertexBytes/4,
   fragment,fragmentBytes/4,&pair,error,sizeof(error))!=RENKU_INTEL_COMPILE_OK
  || pair.fragment.bytes!=sizeof(fs)
  || pair.vertex.total_scratch || pair.fragment.total_scratch
  || pair.vertex.num_relocs || pair.fragment.num_relocs
  || pair.vertex.grf_used!=128 || pair.vertex.dispatch_grf_start_reg!=2
  || pair.vertex.urb_read_length!=0 || pair.vertex.urb_entry_size!=1
  || pair.vertex.dispatch_mode!=3)return false;
 const bool original=pair.vertex.bytes==sizeof(NativeGpu::RcsDrawData::Vs)
  && !memcmp(vs,NativeGpu::RcsDrawData::Vs,pair.vertex.bytes);
 const bool depth=pair.vertex.bytes==sizeof(NativeGpu::RcsDepthVertex::Words)
  && !memcmp(vs,NativeGpu::RcsDepthVertex::Words,pair.vertex.bytes);
 if(!original && !depth)return false;
 uint32_t shaderPage[1024]={};
 memcpy(shaderPage,fs,sizeof(fs));
 if(!NativeGpu::RcsPublicShader::ValidateFragment(shaderPage,1024))return false;
 memcpy(vertexCode,vs,pair.vertex.bytes);
 *vertexCodeBytes=pair.vertex.bytes;
 memcpy(fragmentCode,fs,sizeof(fs));return true;
}

static inline bool CompileKnownFragment(const uint32_t* vertex,size_t vertexBytes,
 const uint32_t* fragment,size_t fragmentBytes,void* output,size_t capacity)
{
 uint8_t vs[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 size_t vsBytes=0;
 return CompileKnownPipeline(vertex,vertexBytes,fragment,fragmentBytes,
  vs,sizeof(vs),&vsBytes,output,capacity);
}
#endif
