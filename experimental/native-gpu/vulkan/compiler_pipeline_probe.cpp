// SPDX-License-Identifier: MIT
#include "CompiledKnownShader.h"
#include "RgbSpirvFixture.h"
#include "DepthSpirvFixture.h"
#include "RotateSpirvFixture.h"
#include "FragCoordSpirvFixture.h"
#include "../render/RcsDepthVertex.h"
#include "../render/RcsRotateVertex.h"
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
 struct RgbCase {uint32_t red,green,blue;bool splitAlpha,accepted;};
 const RgbCase rgb[]={
  {0x3f800000,0x3e800000,0x3f000000,false,true},
  {0x3f800000,0x3e800000,0x3f800000,false,true},
  {0x3f000000,0x3e800000,0x00000000,true,true},
  {0x3f000000,0x3f400000,0x3e800000,true,true},
  {0x3f000000,0x3f000000,0x3f000000,true,false}};
 for(const auto& c:rgb){
  auto code=RgbSpirvFixture(c.red,c.green,c.blue,c.splitAlpha);
  if(code.empty())return 11;
  bool compiled=CompileKnownFragment(RenkuKnownSpirv::Vertex,
   sizeof(RenkuKnownSpirv::Vertex),code.data(),code.size()*4,
   fragment,sizeof(fragment));
  if(compiled!=c.accepted)return 12;
  if(compiled){
   uint32_t expected[sizeof(NativeGpu::RcsDrawData::Fs)/4];
   memcpy(expected,NativeGpu::RcsDrawData::Fs,sizeof(expected));
   expected[3]=expected[35]=c.red;
   expected[7]=expected[39]=c.green;
   expected[11]=expected[43]=c.blue;
   if(memcmp(fragment,expected,sizeof(expected)))return 13;
  }
 }
 uint8_t depthVertex[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 size_t depthBytes=0;
 if(!CompileKnownPipeline(RenkuDepthSpirv::Vertex,
    sizeof(RenkuDepthSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),depthVertex,sizeof(depthVertex),
    &depthBytes,fragment,sizeof(fragment)))return 14;
 if(depthBytes!=sizeof(depthVertex)
  || memcmp(depthVertex,NativeGpu::RcsDepthVertex::Words,depthBytes))return 15;
 uint8_t rotateVertex[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 size_t rotateBytes=0;
 if(!CompileKnownPipeline(RenkuRotateSpirv::Vertex,
    sizeof(RenkuRotateSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),rotateVertex,sizeof(rotateVertex),
    &rotateBytes,fragment,sizeof(fragment)))return 16;
 if(rotateBytes!=sizeof(NativeGpu::RcsRotateVertex::Words)
  || memcmp(rotateVertex,NativeGpu::RcsRotateVertex::Words,rotateBytes))return 17;
 uint8_t gradientFs[sizeof(NativeGpu::RcsFragCoordRed::Words)]={};
 uint8_t gradientVs[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 size_t gradientVsBytes=0,gradientFsBytes=0;
 bool gradient=false;
 if(!CompileKnownPipeline(RenkuKnownSpirv::Vertex,
   sizeof(RenkuKnownSpirv::Vertex),RenkuFragCoordSpirv::Fragment,
   sizeof(RenkuFragCoordSpirv::Fragment),gradientVs,sizeof(gradientVs),
   &gradientVsBytes,gradientFs,sizeof(gradientFs),&gradientFsBytes,&gradient)
  || !gradient || gradientFsBytes!=sizeof(gradientFs)
  || memcmp(gradientFs,NativeGpu::RcsFragCoordRed::Words,
   sizeof(gradientFs)))return 18;
 puts("compiler pipeline probe: accepted bounded RGB and vertex programs; rejected changed instructions and invalid input");
 return 0;
}
