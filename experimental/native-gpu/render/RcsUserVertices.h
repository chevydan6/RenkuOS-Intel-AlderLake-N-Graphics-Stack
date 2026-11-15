// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_USER_VERTICES_H
#define NATIVE_GPU_RCS_USER_VERTICES_H
#include "RcsDrawImage.h"
#include <stdint.h>
#include <string.h>

namespace NativeGpu {
// RCS v2's first user resource: three position vec4s copied into the owned
// vertex page. Coordinates are exact 1/16 NDC steps, mapped to pixels 4..28.
// The batch, shaders, surfaces, GPU addresses and page ownership stay private.
class RcsUserVertices {
public:
 struct Coord { int x, y; };
 struct Check { uint32_t checked, matched, hash, interior, exterior; };

 static bool Validate(const uint32_t* words, Coord* result=nullptr) {
  if(!words)return false;
  Coord points[3]={};
  for(unsigned i=0;i<3;i++){
   if(!Decode(words[4*i],points[i].x) || !Decode(words[4*i+1],points[i].y)
    || words[4*i+2]!=0x3f000000 || words[4*i+3]!=0x3f800000)return false;
  }
  int area=(points[1].x-points[0].x)*(points[2].y-points[0].y)
   -(points[1].y-points[0].y)*(points[2].x-points[0].x);
  if(area<100)return false; // Positive winding and a useful rasterized area.
  unsigned checked=0,inside=0;
  for(int y=0;y<32;y++)for(int x=0;x<32;x++){
   int kind=Classify(points,x,y);
   if(kind>=0){checked++;inside+=kind==1;}
  }
  if(checked<500 || inside<8)return false;
  if(result)memcpy(result,points,sizeof(points));
  return true;
 }

 static bool Build(uint32_t* const* pages,const uint32_t* words) {
  if(!Validate(words) || !RcsDrawImage::Build(pages,0))return false;
  memcpy(pages[RcsDrawImage::Vertices],words,12*sizeof(uint32_t));
  return true;
 }

 static bool Inspect(const volatile uint32_t* target,const uint32_t* words,Check& check,
  uint32_t renderedPixel=RcsDrawImage::RenderedPixel,
  const uint32_t* background=nullptr) {
  Coord points[3]={};
  if(!target || !Validate(words,points))return false;
  Check found={0,0,2166136261u,0,0};
  bool haveInterior=false,haveExterior=false;
  for(int y=0;y<32;y++)for(int x=0;x<32;x++){
   uint32_t observed=target[y*32+x];
   found.hash=(found.hash^observed)*16777619u;
   int kind=Classify(points,x,y);
   if(kind<0)continue;
   uint32_t expected=kind==1?renderedPixel:
    background?background[y*32+x]:RcsDrawImage::Poison;
   found.checked++;found.matched+=observed==expected;
   if(kind==1 && !haveInterior){found.interior=(uint32_t(y)<<16)|uint32_t(x);haveInterior=true;}
   if(kind==0 && !haveExterior){found.exterior=(uint32_t(y)<<16)|uint32_t(x);haveExterior=true;}
  }
  check=found;
  return haveInterior && haveExterior && found.matched==found.checked;
 }

 static bool InspectPair(const volatile uint32_t* target,const uint32_t* first,
  const uint32_t* second,Check& check,uint32_t renderedPixel) {
  Coord a[3]={},b[3]={};
  if(!target || !Validate(first,a) || !Validate(second,b))return false;
  Check found={0,0,2166136261u,0,0};
  bool haveInterior=false,haveExterior=false;
  for(int y=0;y<32;y++)for(int x=0;x<32;x++){
   uint32_t observed=target[y*32+x];
   found.hash=(found.hash^observed)*16777619u;
   int firstKind=Classify(a,x,y),secondKind=Classify(b,x,y);
   if(firstKind<0 || secondKind<0)continue;
   bool inside=firstKind==1 || secondKind==1;
   uint32_t expected=inside?renderedPixel:RcsDrawImage::Poison;
   found.checked++;found.matched+=observed==expected;
   if(inside && !haveInterior){found.interior=(uint32_t(y)<<16)|uint32_t(x);haveInterior=true;}
   if(!inside && !haveExterior){found.exterior=(uint32_t(y)<<16)|uint32_t(x);haveExterior=true;}
  }
  check=found;
  return haveInterior && haveExterior && found.checked>=500
   && found.matched==found.checked;
 }

 static bool Inspect64(uint32_t* const* pages,const uint32_t* words,Check& check,
  uint32_t renderedPixel=RcsDrawImage::RenderedPixel,
  unsigned fragcoordDenominator=0,bool fragcoordGreen=false,
  const uint32_t* background=nullptr) {
  Coord points[3]={};
  if(!pages || !Validate(words,points)
   || (fragcoordDenominator && fragcoordDenominator!=32
    && fragcoordDenominator!=64 && fragcoordDenominator!=128))return false;
  for(unsigned p=6;p<10;p++)if(!pages[p])return false;
  Check found={0,0,2166136261u,0,0};
  bool haveInterior=false,haveExterior=false;
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){
   unsigned index=unsigned(y*64+x);
   uint32_t observed=pages[6+index/1024][index%1024];
   found.hash=(found.hash^observed)*16777619u;
   int kind=Classify(points,x,y,2);
   if(kind<0)continue;
   // FragCoord.x is the pixel-center coordinate x+0.5. The accepted
   // power-of-two divisors give exact float values at these pixel centers.
   uint32_t channel=fragcoordDenominator
    ?((2*x+1)*255+fragcoordDenominator)/(2*fragcoordDenominator):0;
   if(channel>255)channel=255;
   uint32_t expected=kind
    ?fragcoordDenominator?0xff000000u|(channel<<(fragcoordGreen?8:16)):renderedPixel
    :background?background[index]:RcsDrawImage::Poison;
   found.matched+=observed==expected;
   found.checked++;
   if(kind==1 && !haveInterior){found.interior=(uint32_t(y)<<16)|uint32_t(x);haveInterior=true;}
   if(kind==0 && !haveExterior){found.exterior=(uint32_t(y)<<16)|uint32_t(x);haveExterior=true;}
  }
  check=found;
  return haveInterior && haveExterior && found.checked>=2000
   && found.matched==found.checked;
 }

private:
 static bool Decode(uint32_t bits,int& pixel) {
  static constexpr uint32_t encoded[25]={
   0xbf400000,0xbf300000,0xbf200000,0xbf100000,0xbf000000,
   0xbee00000,0xbec00000,0xbea00000,0xbe800000,0xbe400000,
   0xbe000000,0xbd800000,0x00000000,0x3d800000,0x3e000000,
   0x3e400000,0x3e800000,0x3ea00000,0x3ec00000,0x3ee00000,
   0x3f000000,0x3f100000,0x3f200000,0x3f300000,0x3f400000};
  for(unsigned i=0;i<25;i++)if(bits==encoded[i]){pixel=int(i)+4;return true;}
  return false;
 }
 static int Abs(int value){return value<0?-value:value;}
 static int Classify(const Coord* p,int x,int y,int scale=1) {
  if(x<3*scale || x>=29*scale || y<3*scale || y>=29*scale)return 0;
  bool inside=true;
  for(unsigned i=0;i<3;i++){
   unsigned j=(i+1)%3;
   int ax=2*scale*p[i].x,ay=2*scale*p[i].y;
   int bx=2*scale*p[j].x,by=2*scale*p[j].y;
   int edge=(bx-ax)*(2*y+1-ay)-(by-ay)*(2*x+1-ax);
   int extent=Abs(bx-ax)>Abs(by-ay)?Abs(bx-ax):Abs(by-ay);
   if(Abs(edge)<=4*extent)return -1; // Ignore two-pixel raster edge band.
   if(edge<0)inside=false;
  }
  return inside?1:0;
 }
};
}
#endif
