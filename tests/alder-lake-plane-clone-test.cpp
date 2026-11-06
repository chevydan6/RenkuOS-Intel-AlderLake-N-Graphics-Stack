/* SPDX-License-Identifier: MIT */
#include "AlderLakePlaneClone.h"
#include <stdio.h>
static uint32 Reference(uint32 rate,uint32 latency) {
 uint64 selected;
 if(latency>=CloneCeil(2080000,rate))selected=CloneCeil((uint64)latency*rate,2080000)*16*65536;
 else selected=((uint64)latency*rate*4*65536)/(512000)+65536;
 uint32 blocks=(uint32)CloneCeil(selected,65536)+1;
 if(blocks<16)blocks=16;
 uint32 lines=(uint32)CloneCeil(selected,16*65536);
 return 0x80000000U|(lines<<14)|blocks;
}
int main(){unsigned errors=0,cases=0;uint32 floor=0;
 if(DecodeAlderLakeComboPixelFloor(0xb555a5,0x88,38400,0x41cac,0x80000,&floor)!=B_OK||floor!=138724)errors++;
 for(uint32 source=60000;source<=165000;source+=3500)for(uint32 target=60000;target<=165000;target+=3500)for(uint32 latency=1;latency<=100;latency++) {
  uint32 src=Reference(source,latency),dst=Reference(target,latency),wm=0,min=0;
  status_t s=BoundAlderLakeCloneWatermark(src,1920,2080,source,target,&wm,&min);
  if(s!=B_OK||(wm&0x1fff)<(dst&0x1fff)||((wm>>14)&255)<((dst>>14)&255)
   ||min<((wm&0x1fff)+CloneCeil(wm&0x1fff,10)+1))errors++;
  cases++;
 }
 AlderLakeCloneInput in={};in.sourceControl=0x94000008;in.sourceColor=0x2000;in.sourceStride=120;
 in.sourceSize=0x0437077f;in.sourceWm0=0x80004011;in.sourceSagv=0x8000c031;
 in.sourceHTotal=in.targetHTotal=0x081f077f;in.sourceVTotal=in.targetVTotal=0x04570437;
 in.sourcePipeSize=0x077f0437;in.sourcePixelFloorKHz=floor;in.targetPixelCeilKHz=138781;
 in.allocationBytes=7680*1080;in.apertureBytes=256*1024*1024;in.width=1920;in.height=1080;in.bytesPerRow=7680;
 in.mbusControl=0x1c000700;in.poweredSlices=15;in.activePipes=3;in.ddbCount=32;in.ddb[0]=0x07ff0000;
 AlderLakeClonePlan p={};if(BuildAlderLakeClonePlan(in,&p)!=B_OK||p.ddb!=0x07ff0000||p.wm0!=0x80008021||p.sagv!=0x80010041||p.minimumBlocks!=73)errors++;
 // Linux's physical DBUF slice union is global, while PLANE_BUF_CFG is
 // local to the selected MBUS. Decode plan as B's S3/S4 physical allocation.
 unsigned localStart=p.ddb&0xffff,localEnd=(p.ddb>>16)+1;
 unsigned physicalBase=2*1024;
 if(localStart!=0||localEnd!=2048||localStart+physicalBase!=2048
  ||localEnd+physicalBase!=4096||localStart+physicalBase<2048)errors++;
 // Identical A/B local fields are safe only with distinct, guarded routing.
 if(p.ddb!=in.ddb[0])errors++;
 in.ddb[8]=0x07ff0000;if(BuildAlderLakeClonePlan(in,&p)!=B_BUSY)errors++;in.ddb[8]=0;
 in.mbusControl|=0x80000000;if(BuildAlderLakeClonePlan(in,&p)!=B_BAD_VALUE)errors++;in.mbusControl&=~0x80000000;
 in.allocationBytes--;if(BuildAlderLakeClonePlan(in,&p)!=B_BAD_VALUE)errors++;
 if(errors){fprintf(stderr,"%u clone plan failures across %u latency/rate fixtures\n",errors,cases);return 1;}
 printf("%u Linux-method watermark envelope fixtures and clone allocation guards passed\n",cases);return 0;
}
