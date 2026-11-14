// SPDX-License-Identifier: MIT
#include "CompiledKnownShader.h"
#include <stdio.h>
#include <string.h>

int main()
{
 uint8_t fragment[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 if(!CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),fragment,sizeof(fragment)))return 1;
 if(memcmp(fragment,NativeGpu::RcsDrawData::Fs,sizeof(fragment)))return 2;
 uint32_t halfExpected[sizeof(NativeGpu::RcsDrawData::Fs)/4];
 memcpy(halfExpected,NativeGpu::RcsDrawData::Fs,sizeof(halfExpected));
 halfExpected[7]=0x3f000000;halfExpected[39]=0x3f000000;
 if(!CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),RenkuKnownSpirv::FragmentHalf,
    sizeof(RenkuKnownSpirv::FragmentHalf),fragment,sizeof(fragment)))return 5;
 if(memcmp(fragment,halfExpected,sizeof(fragment)))return 6;
 uint32_t green75[sizeof(RenkuKnownSpirv::Fragment)/4];
 memcpy(green75,RenkuKnownSpirv::Fragment,sizeof(green75));
 bool found=false;
 for(unsigned i=0;i<sizeof(green75)/4;i++)
  if(green75[i]==0x3e800000){green75[i]=0x3f400000;found=true;break;}
 if(!found)return 7;
 halfExpected[7]=0x3f400000;halfExpected[39]=0x3f400000;
 if(!CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),green75,sizeof(green75),
    fragment,sizeof(fragment)) || memcmp(fragment,halfExpected,sizeof(fragment)))return 8;
 green75[sizeof(green75)/4-1]=0;
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),green75,sizeof(green75),
    fragment,sizeof(fragment)))return 9;
 green75[sizeof(green75)/4-1]=RenkuKnownSpirv::Fragment[sizeof(green75)/4-1];
 for(unsigned i=0;i<sizeof(green75)/4;i++)
  if(green75[i]==0x3f400000){green75[i]=0x3f800000;break;}
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),green75,sizeof(green75),
    fragment,sizeof(fragment)))return 10;
 uint32_t altered[sizeof(RenkuKnownSpirv::Fragment)/sizeof(uint32_t)];
 memcpy(altered,RenkuKnownSpirv::Fragment,sizeof(altered));
 altered[5]^=1;
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),altered,sizeof(altered),
    fragment,sizeof(fragment)))return 3;
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),fragment,1))return 4;
 puts("compiler pipeline probe: accepted exact constant-green code shape; rejected changed instructions and invalid input");
 return 0;
}
