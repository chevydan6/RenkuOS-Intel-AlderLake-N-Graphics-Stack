// SPDX-License-Identifier: MIT
#ifndef RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#define RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#include "KnownSpirv.h"
#include "../render/draw/RcsDrawGenerated.h"
#include "../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include <string.h>

// This bridge deliberately accepts only the SPIR-V pair already proven on the
// N100. SPIRV-Tools validates Vulkan 1.0 modules in the compiler, but this
// pipeline and the native owner still accept one exact fragment program.
static bool CompileKnownFragment(const uint32_t* vertex,size_t vertexBytes,
 const uint32_t* fragment,size_t fragmentBytes,void* output,size_t capacity)
{
 if(!vertex || !fragment || !output || vertexBytes!=sizeof(RenkuKnownSpirv::Vertex)
  || fragmentBytes!=sizeof(RenkuKnownSpirv::Fragment)
  || capacity<sizeof(NativeGpu::RcsDrawData::Fs)
  || memcmp(vertex,RenkuKnownSpirv::Vertex,vertexBytes)
  || memcmp(fragment,RenkuKnownSpirv::Fragment,fragmentBytes))return false;
 uint8_t vs[sizeof(NativeGpu::RcsDrawData::Vs)]={};
 uint8_t fs[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 char error[256]={};
 if(renku_intel_compile_vulkan_pair(vertex,vertexBytes/4,
   fragment,fragmentBytes/4,&pair,error,sizeof(error))!=RENKU_INTEL_COMPILE_OK
  || pair.vertex.bytes!=sizeof(vs) || pair.fragment.bytes!=sizeof(fs)
  || pair.vertex.total_scratch || pair.fragment.total_scratch
  || pair.vertex.num_relocs || pair.fragment.num_relocs
  || memcmp(vs,NativeGpu::RcsDrawData::Vs,sizeof(vs))
  || memcmp(fs,NativeGpu::RcsDrawData::Fs,sizeof(fs)))return false;
 memcpy(output,fs,sizeof(fs));return true;
}
#endif
