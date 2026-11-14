/* SPDX-License-Identifier: MIT
 * Copyright 2026 Native GPU validation contributors
 */
#include "IntelSpirvRuntime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t* read_spv(const char* path, size_t* words)
{
 FILE* f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long size=ftell(f);if(size<20 || size>1048576 || size%4){fclose(f);return NULL;}
 rewind(f);uint32_t* data=malloc((size_t)size);
 if(!data || fread(data,1,(size_t)size,f)!=(size_t)size){free(data);fclose(f);return NULL;}
 fclose(f);*words=(size_t)size/4;return data;
}

int main(int argc,char** argv)
{
 if(argc!=5){fprintf(stderr,"usage: runtime_smoke VS.spv FS.spv VS.bin FS.bin\n");return 2;}
 size_t vn=0,fn=0;uint32_t* vs=read_spv(argv[1],&vn);
 uint32_t* fs=read_spv(argv[2],&fn);
 if(!vs || !fs){fprintf(stderr,"cannot read SPIR-V\n");return 3;}
 unsigned char vc[4096]={0},fc[4096]={0};char error[256]={0};
 RenkuIntelCompilePair result={0};result.abi_version=RENKU_INTEL_COMPILER_ABI;
 result.vertex.code=vc;result.vertex.capacity=sizeof(vc);
 result.fragment.code=fc;result.fragment.capacity=sizeof(fc);
 uint32_t invalid[5]={0};
 if(renku_intel_compile_vulkan_pair(invalid,5,fs,fn,&result,error,sizeof(error))
  !=RENKU_INTEL_COMPILE_INVALID || !error[0])return 9;
 uint32_t* malformed=malloc(vn*4);
 if(!malformed)return 11;
 memcpy(malformed,vs,vn*4);
 malformed[5]=0x00000011; /* zero-word OpCapability, header remains valid */
 if(renku_intel_validate_vulkan10(vs,vn,error,sizeof(error))
  !=RENKU_INTEL_COMPILE_OK || error[0])return 12;
 if(renku_intel_validate_vulkan10(malformed,vn,error,sizeof(error))
  !=RENKU_INTEL_COMPILE_INVALID || !error[0])return 13;
 printf("validator rejected malformed SPIR-V: %s\n",error);
 if(renku_intel_compile_vulkan_pair(malformed,vn,fs,fn,&result,error,sizeof(error))
  !=RENKU_INTEL_COMPILE_INVALID || !error[0])return 14;
 free(malformed);
 int status=renku_intel_compile_vulkan_pair(vs,vn,fs,fn,&result,error,sizeof(error));
 if(status){fprintf(stderr,"compile status=%d: %s\n",status,error);return 4;}
 RenkuIntelCompilePair tooSmall=result;tooSmall.fragment.capacity=1;
 if(renku_intel_compile_vulkan_pair(vs,vn,fs,fn,&tooSmall,error,sizeof(error))
  !=RENKU_INTEL_COMPILE_OUTPUT_TOO_SMALL || !error[0])return 10;
 free(vs);free(fs);
 if(result.vertex.bytes!=128 || result.fragment.bytes!=208
  || result.vertex.total_scratch || result.fragment.total_scratch
  || result.vertex.num_relocs || result.fragment.num_relocs
  || result.vertex.urb_entry_size!=1 || result.fragment.dispatch_8!=1
  || result.fragment.dispatch_16!=1 || result.fragment.prog_offset_16!=128){
  fprintf(stderr,"unexpected Intel program metadata\n");return 5;
 }
 FILE* vf=fopen(argv[3],"wb");FILE* ff=fopen(argv[4],"wb");
 if(!vf || !ff)return 6;
 if(fwrite(vc,1,result.vertex.bytes,vf)!=result.vertex.bytes
  || fwrite(fc,1,result.fragment.bytes,ff)!=result.fragment.bytes)return 7;
 if(fclose(vf) || fclose(ff))return 8;
 printf("native Intel compiler API: VS=%zu FS=%zu\n",result.vertex.bytes,result.fragment.bytes);
 return 0;
}
