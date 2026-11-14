// SPDX-License-Identifier: MIT
#include "IntelSpirvRuntime.h"
#include "KnownSpirv.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char** argv)
{
 if(argc!=3)return 2;
 FILE* input=fopen(argv[1],"rb");
 if(!input)return 3;
 std::vector<uint8_t> bytes;
 uint8_t chunk[1024];
 size_t n;
 while((n=fread(chunk,1,sizeof(chunk),input)))
  bytes.insert(bytes.end(),chunk,chunk+n);
 fclose(input);
 if(bytes.empty() || bytes.size()%4)return 4;
 std::vector<uint32_t> fragment(bytes.size()/4);
 memcpy(fragment.data(),bytes.data(),bytes.size());
 std::array<uint8_t,4096> vertexCode={},fragmentCode={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vertexCode.data();pair.vertex.capacity=vertexCode.size();
 pair.fragment.code=fragmentCode.data();pair.fragment.capacity=fragmentCode.size();
 char error[512]={};
 int status=renku_intel_compile_vulkan_pair(RenkuKnownSpirv::Vertex,
  sizeof(RenkuKnownSpirv::Vertex)/4,fragment.data(),fragment.size(),
  &pair,error,sizeof(error));
 printf("{\"compile_status\":%d,\"error\":\"%s\",\"vertex_bytes\":%zu,"
        "\"fragment_bytes\":%zu,\"fragment_grf\":%u,\"fragment_scratch\":%u,"
        "\"fragment_relocs\":%u,\"dispatch8\":%u,\"dispatch16\":%u,"
        "\"dispatch32\":%u,\"offset16\":%u,\"offset32\":%u,"
        "\"grf_start8\":%u,\"grf_start16\":%u}\n",status,error,pair.vertex.bytes,
        pair.fragment.bytes,pair.fragment.grf_used,pair.fragment.total_scratch,
        pair.fragment.num_relocs,pair.fragment.dispatch_8,pair.fragment.dispatch_16,
        pair.fragment.dispatch_32,pair.fragment.prog_offset_16,
        pair.fragment.prog_offset_32,pair.fragment.dispatch_grf_start_reg,
        pair.fragment.dispatch_grf_start_reg_16);
 if(status)return 5;
 FILE* output=fopen(argv[2],"wb");
 if(!output)return 6;
 bool written=fwrite(fragmentCode.data(),1,pair.fragment.bytes,output)
  ==pair.fragment.bytes;
 fclose(output);
 return written?0:7;
}
