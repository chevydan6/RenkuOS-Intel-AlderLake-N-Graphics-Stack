// SPDX-License-Identifier: MIT
#include "KnownSpirv.h"
#include "IntelSpirvRuntime.h"
#include <stdio.h>
#include <stdint.h>
#include <vector>

int main(int argc,char** argv)
{
 if(argc!=3)return 2;
 FILE* input=fopen(argv[1],"rb");if(!input)return 3;
 if(fseek(input,0,SEEK_END) || ftell(input)<0)return 4;
 long bytes=ftell(input);
 if(bytes<20 || bytes>1024*1024 || (bytes&3) || fseek(input,0,SEEK_SET))return 5;
 std::vector<uint32_t> spirv(size_t(bytes)/4);
 if(fread(spirv.data(),1,size_t(bytes),input)!=size_t(bytes) || fclose(input))return 6;
 char error[512]={};
 int validated=renku_intel_validate_vulkan10(spirv.data(),spirv.size(),error,sizeof(error));
 printf("validate=%d error=%s\n",validated,error);fflush(stdout);
 if(validated)return 7;
 uint8_t vs[4096]={},fs[4096]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 int compiled=renku_intel_compile_vulkan_pair(RenkuKnownSpirv::Vertex,
  sizeof(RenkuKnownSpirv::Vertex)/4,spirv.data(),spirv.size(),
  &pair,error,sizeof(error));
 printf("compile=%d error=%s vs_bytes=%zu fs_bytes=%zu fs_grf=%u "
  "fs_dispatch=%u/%u/%u fs_offset=%u/%u fs_start=%u/%u/%u fs_scratch=%u fs_relocs=%u\n",
  compiled,error,pair.vertex.bytes,pair.fragment.bytes,pair.fragment.grf_used,
  pair.fragment.dispatch_8,pair.fragment.dispatch_16,pair.fragment.dispatch_32,
  pair.fragment.prog_offset_16,pair.fragment.prog_offset_32,
  pair.fragment.dispatch_grf_start_reg,pair.fragment.dispatch_grf_start_reg_16,
  pair.fragment.dispatch_grf_start_reg_32,
  pair.fragment.total_scratch,pair.fragment.num_relocs);
 if(compiled)return 8;
 FILE* output=fopen(argv[2],"wb");if(!output)return 9;
 bool saved=fwrite(fs,1,pair.fragment.bytes,output)==pair.fragment.bytes;
 if(fclose(output) || !saved)return 10;
 return 0;
}
