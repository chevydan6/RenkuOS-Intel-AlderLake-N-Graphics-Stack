// SPDX-License-Identifier: MIT
#include "RcsResourceVertices.h"
#include "RcsDrawImage64.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace NativeGpu;
static_assert(RcsResourceVertices::VertexGpu>=RcsDrawImage::GpuBase
 +RcsDrawImage64::PageCount*4096,"vertex resource overlaps 64x64 draw image");
int main() {
 alignas(4096) uint32_t storage[9][1024]={};
 uint32_t* pages[8];for(unsigned i=0;i<8;i++)pages[i]=storage[i];
 const uint32_t words[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 assert(RcsUserVertices::Build(pages,words));
 uint32_t reference[1024];memcpy(reference,pages[RcsDrawImage::Batch],sizeof(reference));
 assert(!RcsResourceVertices::Build(pages,pages[0],words));
 assert(!RcsResourceVertices::Build(pages,storage[8]+1,words));
 assert(RcsResourceVertices::Build(pages,storage[8],words));
 assert(memcmp(storage[8],words,sizeof(words))==0);
 for(unsigned i=12;i<1024;i++)assert(storage[8][i]==0);
 unsigned changes=0;
 for(unsigned i=0;i<1024;i++)if(reference[i]!=pages[0][i]){
  changes++;assert(reference[i]==0x00403000);
  assert(pages[0][i]==RcsResourceVertices::VertexGpu);
  assert(i>=2 && pages[0][i-2]==0x78080003);
 }
 assert(changes==1);
 assert(RcsResourceVertices::BuildAt(pages,words,0x410000));
 assert(pages[0][RcsDrawImage::BatchDwords-1]==0x05000000);
 unsigned arenaChanges=0;
 for(unsigned i=0;i<1024;i++)if(reference[i]!=pages[0][i]){
  arenaChanges++;assert(reference[i]==0x00403000);
  assert(pages[0][i]==0x410000);
 }
 assert(arenaChanges==1);
 for(unsigned i=0;i<1024;i++)assert(pages[RcsDrawImage::Vertices][i]==0);
 assert(!RcsResourceVertices::BuildAt(pages,words,0x40f000));
 assert(!RcsResourceVertices::BuildAt(pages,words,0x410001));
 assert(!RcsResourceVertices::BuildAt(pages,words,0x420000));
 puts("RCS separate vertex resource packet: PASS (CPU only)");
}
