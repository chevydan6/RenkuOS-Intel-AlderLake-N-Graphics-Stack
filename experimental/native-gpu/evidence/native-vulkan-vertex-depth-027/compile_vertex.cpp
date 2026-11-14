// SPDX-License-Identifier: MIT
// Experimental compiler-output capture for the N100 vertex shader.
#include "IntelSpirvRuntime.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
static std::vector<uint32_t> read_spv(const char* path) {
 FILE* f=fopen(path,"rb");if(!f)exit(2);
 if(fseek(f,0,SEEK_END))exit(3);
 long n=ftell(f);if(n<20 || n%4 || n>1048576)exit(4);
 rewind(f);std::vector<uint32_t> words(n/4);
 if(fread(words.data(),1,n,f)!=(size_t)n)exit(5);
 fclose(f);return words;
}
static void write_bin(const char* path,const void* data,size_t bytes){
 FILE* f=fopen(path,"wb");if(!f)exit(6);
 if(fwrite(data,1,bytes,f)!=bytes)exit(7);
 if(fclose(f))exit(8);
}
int main(int argc,char** argv){
 if(argc!=5)return 1;
 auto v=read_spv(argv[1]),f=read_spv(argv[2]);
 uint8_t vs[4096]={},fs[4096]={};char error[512]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fs;pair.fragment.capacity=sizeof(fs);
 int result=renku_intel_compile_vulkan_pair(v.data(),v.size(),f.data(),f.size(),&pair,error,sizeof(error));
 printf("result=%d error=%s vs_bytes=%zu vs_scratch=%u vs_relocs=%u vs_grf=%u vs_grf_start=%u vs_urb_read=%u vs_urb_entry=%u vs_dispatch=%u fs_bytes=%zu\n",
  result,error,pair.vertex.bytes,pair.vertex.total_scratch,pair.vertex.num_relocs,
  pair.vertex.grf_used,pair.vertex.dispatch_grf_start_reg,pair.vertex.urb_read_length,pair.vertex.urb_entry_size,
  pair.vertex.dispatch_mode,pair.fragment.bytes);
 if(result)return 9;
 write_bin(argv[3],vs,pair.vertex.bytes);
 write_bin(argv[4],fs,pair.fragment.bytes);
}
