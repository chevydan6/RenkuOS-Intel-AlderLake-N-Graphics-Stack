/* SPDX-License-Identifier: MIT */
#include "AlderLakeHdmiPowerPrerequisites.h"
#include <stdio.h>
struct Mock {
 uint32 dc=0, main=0xffffffffU, ddi=0xffffffffU;unsigned writes=0;
 static status_t Read(void* p,uint32 o,uint32* v) {Mock& m=*(Mock*)p;*v=o==0x45504?m.dc:o==0x45404?m.main:m.ddi;return B_OK;}
 static status_t Write(void* p,uint32,uint32){((Mock*)p)->writes++;return B_ERROR;}
 AlderLakeTypeCIO Ops(){return {this,Read,Write,NULL};}
};
int main(){unsigned errors=0,cases=0;
 for(unsigned port=1;port<=4;port++)for(unsigned pipe=0;pipe<=2;pipe++) {
  Mock m;m.main=3U|(3U<<2)|(3U<<(2*(pipe+5)));m.ddi=3U<<(2*(port+2));
  if(CheckAlderLakeHdmiPowerPrerequisites(m.Ops(),port,pipe)!=B_OK)errors++;
  for(unsigned bit=0;bit<32;bit++)if(m.main&(1U<<bit)) {
   m.main^=1U<<bit;if(CheckAlderLakeHdmiPowerPrerequisites(m.Ops(),port,pipe)!=B_BUSY)errors++;m.main^=1U<<bit;cases++;
  }
  m.dc=1;if(CheckAlderLakeHdmiPowerPrerequisites(m.Ops(),port,pipe)!=B_BUSY)errors++;
  m.dc=0x20000000U;if(CheckAlderLakeHdmiPowerPrerequisites(m.Ops(),port,pipe)!=B_BUSY)errors++;
  m.dc=8;if(CheckAlderLakeHdmiPowerPrerequisites(m.Ops(),port,pipe)!=B_BUSY)errors++;
  if(m.writes)errors++;
  cases+=4;
 }
 if(errors){fprintf(stderr,"%u power prerequisite failures\n",errors);return 1;}
 printf("%u read-only display power prerequisite cases passed\n",cases);return 0;
}
