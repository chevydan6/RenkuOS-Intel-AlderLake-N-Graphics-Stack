// SPDX-License-Identifier: MIT
// Offline image construction check against the Mesa 26.3 64x64 packet artifact.
#include "RcsDrawImage64.h"
#include "draw/RcsDrawGenerated64.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
 using NativeGpu::RcsDrawImage64;
 namespace Data=NativeGpu::RcsDrawData64;
 alignas(4096) uint32_t storage[11][1024];
 uint32_t* pages[11];
 for(unsigned i=0;i<11;i++)pages[i]=storage[i];
 memset(storage,0xa5,sizeof(storage));
 assert(!RcsDrawImage64::Build(pages,4));
 assert(!RcsDrawImage64::Build(pages,0,0x500000));
 assert(!RcsDrawImage64::Build(nullptr,0));
 uint32_t* saved=pages[9];pages[9]=pages[8];
 assert(!RcsDrawImage64::Build(pages,0));pages[9]=saved;
 pages[10]++;assert(!RcsDrawImage64::Build(pages,0));pages[10]--;
 for(unsigned i=0;i<11*1024;i++)assert((&storage[0][0])[i]==0xa5a5a5a5);
 for(unsigned variant=0;variant<4;variant++){
  assert(RcsDrawImage64::Build(pages,variant));
  assert(memcmp(pages[0],Data::Batch,sizeof(Data::Batch))==0);
  assert(memcmp(pages[1],Data::Dynamic,sizeof(Data::Dynamic))==0);
  assert(memcmp(pages[2],Data::Surface,sizeof(Data::Surface))==0);
  assert(memcmp(pages[4],Data::Vs,sizeof(Data::Vs))==0);
  assert(memcmp(pages[5],Data::Fs,sizeof(Data::Fs))==0);
  for(unsigned page=6;page<10;page++)
   for(unsigned word=0;word<1024;word++)assert(pages[page][word]==0x19537bad);
  for(unsigned word=0;word<1024;word++)assert(pages[10][word]==0);
  assert(pages[3][3]==0x3f800000 && pages[3][7]==0x3f800000
   && pages[3][11]==0x3f800000);
 }
 puts("RCS 64x64 CPU packet image: PASS (no hardware submission)");
}
