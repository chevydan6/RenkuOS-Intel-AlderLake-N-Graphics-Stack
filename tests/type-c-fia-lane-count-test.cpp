/* SPDX-License-Identifier: MIT */
#include "TypeCFiaLaneCount.h"
#include <stdio.h>
struct Mock {
 uint32 fia=0x12345621,buffer=0x40;unsigned port,writes=0,fail=0;
 static status_t Read(void* p,uint32 o,uint32* v){Mock& m=*(Mock*)p;*v=o==(0x161500U+(m.port-1)*4)?4:o==(0x64300U+(m.port-1)*0x100)?m.buffer:m.fia;return B_OK;}
 static status_t Write(void* p,uint32,uint32 v){Mock& m=*(Mock*)p;if(++m.writes==m.fail)return B_ERROR;m.fia=v;return B_OK;}
 AlderLakeTypeCIO Ops(){return {this,Read,Write,NULL};}
};
int main(){unsigned errors=0;
 for(unsigned port=1;port<=4;port++) {
  Mock m;m.port=port;TypeCFiaLaneCount f(m.Ops(),port);uint32 mask=0xfU<<(4*((port-1)%2));
  if(f.Apply()!=B_OK||m.fia!=(0x12345621U|mask)||f.Apply()!=B_BUSY)errors++;
  uint32 other=1U<<(4*(port%2));
  m.fia^=other;
  if(f.Restore()!=B_OK||f.NeedsCleanup()||m.fia!=(0x12345621U^other))errors++;
 }
 {Mock m;m.port=2;m.fail=1;TypeCFiaLaneCount f(m.Ops(),2);if(f.Apply()!=B_ERROR||f.NeedsCleanup()||m.fia!=0x12345621)errors++;}
 {Mock m;m.port=2;TypeCFiaLaneCount f(m.Ops(),2);if(f.Apply()!=B_OK)errors++;
  m.fail=m.writes+1;if(f.Restore()!=B_ERROR||!f.NeedsCleanup())errors++;
  m.fail=0;if(f.Restore()!=B_OK||f.NeedsCleanup()||m.fia!=0x12345621)errors++;
 }
 {Mock m;m.port=2;m.buffer|=0x80000000U;TypeCFiaLaneCount f(m.Ops(),2);if(f.Apply()!=B_BUSY||m.writes)errors++;}
 if(errors){fprintf(stderr,"%u FIA cases failed\n",errors);return 1;}
 puts("7 modular FIA four-lane ownership, preservation and rollback cases passed");return 0;
}
