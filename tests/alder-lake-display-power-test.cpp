/* SPDX-License-Identifier: MIT */
#include "AlderLakeDisplayPower.h"
#include <stdio.h>
struct Mock {
 uint32 main=0x5405,ddi=0,dc=0;unsigned writes=0,failAt=0;bool stuck=false;
 static status_t Read(void* p,uint32 offset,uint32* value){Mock& m=*(Mock*)p;*value=offset==0x45504?m.dc:offset==0x45404?m.main:m.ddi;return B_OK;}
 static status_t Write(void* p,uint32 offset,uint32 value){Mock& m=*(Mock*)p;if(++m.writes==m.failAt)return B_ERROR;
  if(offset==0x45404)m.main=value;else {
   m.ddi=value;
   if(!m.stuck)m.ddi=(value&0xaaaaU)|((value&0xaaaaU)>>1);
  }return B_OK;}
 static void Pause(void*){}
 AlderLakeTypeCIO Ops(){return {this,Read,Write,Pause};}
};
static bool Empty(uint32* r){for(unsigned i=0;i<9;i++)if(r[i])return false;return true;}
int main(){unsigned errors=0;
 for(unsigned failure=0;failure<=4;failure++) {
  Mock m;uint32 refs[9]={},owned=0;m.failAt=failure;
  AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  status_t s=p.Acquire();if((failure==0)!=(s==B_OK))errors++;
  if(p.Release()!=B_OK||p.NeedsCleanup()||!Empty(refs)||owned||m.main!=0x5405||m.ddi)errors++;
 }
 {Mock m;uint32 refs[9]={},owned=0;
  AlderLakeDisplayPower a(m.Ops(),refs,&owned,1,0,3),b(m.Ops(),refs,&owned,2,1,3);
  if(a.Acquire()!=B_OK||b.Acquire()!=B_OK||refs[0]!=2||refs[1]!=2)errors++;
  if(a.Release()!=B_OK||refs[0]!=1||!(m.main&2)||!(m.main&8)||!(m.ddi&0x200))errors++;
  if(b.Release()!=B_OK||!Empty(refs)||owned||m.main!=0x5405||m.ddi)errors++;
 }
 {Mock m;m.main|=0x280a;m.ddi=0x300;uint32 refs[9]={},owned=0;uint32 before=m.main;
  AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  if(p.Acquire()!=B_OK||p.Release()!=B_OK||m.main!=before||m.ddi!=0x300||owned)errors++;
 }
 {Mock m;m.main=0;uint32 refs[9]={},owned=0;AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  if(p.Acquire()!=B_BUSY||m.writes||p.NeedsCleanup())errors++;
 }
 {Mock m;m.dc=1;uint32 refs[9]={},owned=0;AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  if(p.Acquire()!=B_BUSY||m.writes||p.NeedsCleanup())errors++;
 }
 {Mock m;m.stuck=true;uint32 refs[9]={},owned=0;AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  if(p.Acquire()!=B_TIMED_OUT||p.NeedsCleanup()||owned||!Empty(refs))errors++;
 }
 {Mock m;uint32 refs[9]={},owned=0;AlderLakeDisplayPower p(m.Ops(),refs,&owned,2,1,3);
  if(p.Acquire()!=B_OK)errors++;
  m.failAt=m.writes+1;
  if(p.Release()!=B_ERROR||!p.NeedsCleanup()||!owned||refs[0]!=1)errors++;
  m.failAt=0;if(p.Release()!=B_OK||p.NeedsCleanup()||owned||!Empty(refs))errors++;
 }
 if(errors){fprintf(stderr,"%u display power failures\n",errors);return 1;}
 puts("11 display power pinning, sharing, rollback and cleanup retry cases passed");return 0;
}
