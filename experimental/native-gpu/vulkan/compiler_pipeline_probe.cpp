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
 const uint32_t scales[]={0x3c000000u,0x3d000000u};
 for(uint32_t scale:scales){
  uint32_t code[sizeof(RenkuFragCoordSpirv::Fragment)/4];
  memcpy(code,RenkuFragCoordSpirv::Fragment,sizeof(code));
  bool changed=false;
  for(uint32_t& word:code)
   if(word==0x3c800000){word=scale;changed=true;break;}
  if(!changed)return 19;
  gradient=false;gradientFsBytes=0;
  if(!CompileKnownPipeline(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),code,sizeof(code),
    gradientVs,sizeof(gradientVs),&gradientVsBytes,
    gradientFs,sizeof(gradientFs),&gradientFsBytes,&gradient)
   || !gradient || gradientFsBytes!=sizeof(gradientFs))return 20;
  uint32_t expected[sizeof(gradientFs)/4];
  memcpy(expected,NativeGpu::RcsFragCoordRed::Words,sizeof(expected));
  expected[29]=expected[77]=scale==0x3c000000?0x3c002002:0x3d002002;
  if(memcmp(gradientFs,expected,sizeof(expected)))return 21;
 }
 uint32_t greenCode[sizeof(RenkuFragCoordSpirv::Fragment)/4];
 memcpy(greenCode,RenkuFragCoordSpirv::Fragment,sizeof(greenCode));
 unsigned changed=0;
 for(size_t i=0;i+6<sizeof(greenCode)/4;i++)
  if(greenCode[i]==0x00070050u && greenCode[i+1]==7
   && greenCode[i+2]==17 && greenCode[i+3]==16
   && greenCode[i+4]==10 && greenCode[i+5]==10
   && greenCode[i+6]==11){
   greenCode[i+3]=10;greenCode[i+4]=16;changed++;
  }
 if(changed!=1)return 22;
 const uint32_t greenScales[]={0x3d000000u,0x3c800000u,0x3c000000u};
 for(uint32_t scale:greenScales){
  uint32_t code[sizeof(greenCode)/4];memcpy(code,greenCode,sizeof(code));
  for(uint32_t& word:code)if(word==0x3c800000u){word=scale;break;}
  gradient=false;gradientFsBytes=0;
  if(!CompileKnownPipeline(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),code,sizeof(code),
    gradientVs,sizeof(gradientVs),&gradientVsBytes,
    gradientFs,sizeof(gradientFs),&gradientFsBytes,&gradient)
   || !gradient || gradientFsBytes!=sizeof(gradientFs))return 23;
  uint32_t expected[sizeof(gradientFs)/4];
  memcpy(expected,NativeGpu::RcsFragCoordRed::Words,sizeof(expected));
  expected[5]=0x7c054220;expected[28]=0xe17d0141;
  expected[53]=0x78054220;expected[76]=0xe07a0141;
  expected[29]=expected[77]=scale==0x3d000000u?0x3d002002:
   scale==0x3c800000u?0x3c802002:0x3c002002;
  if(memcmp(gradientFs,expected,sizeof(expected)))return 24;
 }
 puts("compiler pipeline probe: accepted bounded RGB and vertex programs; rejected changed instructions and invalid input");
 return 0;
}
