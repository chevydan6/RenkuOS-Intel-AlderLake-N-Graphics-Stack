// SPDX-License-Identifier: MIT
#ifndef RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#define RENKU_VULKAN_COMPILED_KNOWN_SHADER_H
#include "KnownSpirv.h"
#include "../render/RcsPublicShader.h"
#include "../render/compiler/mesa-26.3.0/IntelSpirvRuntime.h"
#include <string.h>

// Compile Vulkan 1.0 SPIR-V, then admit only the fixed N100 constant-RGB
// instruction shape validated by the native owner. The vertex stage must
// compile to the exact owner-approved machine code.
static bool CompileKnownFragment(const uint32_t* vertex,size_t vertexBytes,
 const uint32_t* fragment,size_t fragmentBytes,void* output,size_t capacity)
{
 if(!vertex || !fragment || !output || !vertexBytes || (vertexBytes&3)
  || !fragmentBytes || (fragmentBytes&3)
  || capacity<sizeof(NativeGpu::RcsDrawData::Fs))return false;
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
  || memcmp(vs,NativeGpu::RcsDrawData::Vs,sizeof(vs)))return false;
 uint32_t shaderPage[1024]={};
 memcpy(shaderPage,fs,sizeof(fs));
 if(!NativeGpu::RcsPublicShader::ValidateFragment(shaderPage,1024))return false;
 memcpy(output,fs,sizeof(fs));return true;
}
#endif
