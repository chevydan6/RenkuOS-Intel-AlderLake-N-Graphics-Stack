/* SPDX-License-Identifier: MIT */
#include "IntelSpirvRuntime.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t* read_words(const char* path,size_t* count)
{
 FILE* file=fopen(path,"rb");
 if(!file){perror(path);return NULL;}
 if(fseek(file,0,SEEK_END)){perror("seek");fclose(file);return NULL;}
 long bytes=ftell(file);
 if(bytes<20 || bytes>1048576 || bytes%4){
  fprintf(stderr,"invalid SPIR-V size %ld: %s\n",bytes,path);
  fclose(file);return NULL;
 }
 rewind(file);
 uint32_t* words=malloc((size_t)bytes);
 if(!words){perror("alloc");fclose(file);return NULL;}
 size_t got=fread(words,1,(size_t)bytes,file);
 if(got!=(size_t)bytes){
  fprintf(stderr,"short SPIR-V read %zu/%ld: %s\n",got,bytes,path);
  free(words);fclose(file);return NULL;
 }
 fclose(file);*count=(size_t)bytes/4;return words;
}

static int save(const char* path,const void* bytes,size_t count)
{
 FILE* file=fopen(path,"wb");
 if(!file)return -1;
 int ok=fwrite(bytes,1,count,file)==count && fclose(file)==0;
 return ok?0:-1;
}

int main(int argc,char** argv)
{
 if(argc!=5)return 2;
 size_t vertex_words=0,fragment_words=0;
 uint32_t* vertex=read_words(argv[1],&vertex_words);
 uint32_t* fragment=read_words(argv[2],&fragment_words);
 if(!vertex || !fragment){free(vertex);free(fragment);return 3;}
 uint8_t vertex_code[4096]={0},fragment_code[4096]={0};
 RenkuIntelCompilePair pair={0};pair.abi_version=RENKU_INTEL_COMPILER_ABI;
 pair.vertex.code=vertex_code;pair.vertex.capacity=sizeof(vertex_code);
 pair.fragment.code=fragment_code;pair.fragment.capacity=sizeof(fragment_code);
 char error[256]={0};
 int result=renku_intel_compile_vulkan_pair(vertex,vertex_words,
  fragment,fragment_words,&pair,error,sizeof(error));
 free(vertex);free(fragment);
 if(result){fprintf(stderr,"compile status=%d: %s\n",result,error);return 4;}
 printf("VS bytes=%zu grf=%u dispatch_grf=%u urb_read=%u urb_entry=%u mode=%u scratch=%u relocs=%u\n",
  pair.vertex.bytes,pair.vertex.grf_used,pair.vertex.dispatch_grf_start_reg,
  pair.vertex.urb_read_length,pair.vertex.urb_entry_size,
  pair.vertex.dispatch_mode,pair.vertex.total_scratch,pair.vertex.num_relocs);
 printf("FS bytes=%zu grf=%u dispatch8=%u dispatch16=%u dispatch32=%u grf8=%u grf16=%u offset16=%u scratch=%u relocs=%u\n",
  pair.fragment.bytes,pair.fragment.grf_used,pair.fragment.dispatch_8,
  pair.fragment.dispatch_16,pair.fragment.dispatch_32,
  pair.fragment.dispatch_grf_start_reg,
  pair.fragment.dispatch_grf_start_reg_16,pair.fragment.prog_offset_16,
  pair.fragment.total_scratch,pair.fragment.num_relocs);
 if(save(argv[3],vertex_code,pair.vertex.bytes)
  || save(argv[4],fragment_code,pair.fragment.bytes))return 5;
 return 0;
}
