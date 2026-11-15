#include "KnownSpirv.h"
#include "FragCoordSpirvFixture.h"
#include "IntelSpirvRuntime.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
static int compile(const uint32_t* fs,size_t words,const char* output){
 uint8_t vs[4096]={},fragment[4096]={};
 RenkuIntelCompilePair pair={};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vs;pair.vertex.capacity=sizeof(vs);
 pair.fragment.code=fragment;pair.fragment.capacity=sizeof(fragment);
 char error[256]={};
 int status=renku_intel_compile_vulkan_pair(RenkuKnownSpirv::Vertex,
  sizeof(RenkuKnownSpirv::Vertex)/4,fs,words,&pair,error,sizeof(error));
 printf("%s status=%d error=%s vs=%zu fs=%zu grf=%u start=%u/%u off=%u/%u scratch=%u relocs=%u\n",
  output,status,error,pair.vertex.bytes,pair.fragment.bytes,pair.fragment.grf_used,
  pair.fragment.dispatch_grf_start_reg,pair.fragment.dispatch_grf_start_reg_16,
  pair.fragment.prog_offset_16,pair.fragment.prog_offset_32,
  pair.fragment.total_scratch,pair.fragment.num_relocs);
 if(status)return status;
 FILE* file=fopen(output,"wb");
 if(!file)return 2;
 size_t written=fwrite(fragment,1,pair.fragment.bytes,file);
 if(fclose(file) || written!=pair.fragment.bytes)return 3;
 return 0;
}
int main(){
 uint32_t changed[sizeof(RenkuFragCoordSpirv::Fragment)/4];
 memcpy(changed,RenkuFragCoordSpirv::Fragment,sizeof(changed));
 unsigned seen=0;
 for(size_t i=0;i+6<sizeof(changed)/4;i++)
  if(changed[i]==0x00070050u && changed[i+1]==7 && changed[i+2]==17
   && changed[i+3]==16 && changed[i+4]==10 && changed[i+5]==10
   && changed[i+6]==11){changed[i+3]=10;changed[i+4]=16;seen++;}
 if(seen!=1)return 4;
 if(compile(RenkuFragCoordSpirv::Fragment,
  sizeof(RenkuFragCoordSpirv::Fragment)/4,"/boot/home/test-work/green-gradient-063/red.bin"))return 5;
 if(compile(changed,sizeof(changed)/4,
  "/boot/home/test-work/green-gradient-063/green.bin"))return 6;
 return 0;
}
