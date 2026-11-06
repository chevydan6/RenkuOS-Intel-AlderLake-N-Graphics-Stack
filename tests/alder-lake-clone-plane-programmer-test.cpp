/* SPDX-License-Identifier: MIT */
#include "AlderLakeClonePlaneProgrammer.h"
#include <map>
#include <stdio.h>
struct Mock {
 std::map<uint32,uint32> r;unsigned writes=0,fail=0,frame=0;bool stuck=false,underrun=false,bad=false;
 Mock(){r[0x68980]=0x80000200;r[0x68a80]=0x200;r[0x71180]=0x04000008;r[0x70180]=0x94000008;r[0x7027c]=0x07ff0000;}
 static status_t Read(void* p,uint32 o,uint32* v){Mock& m=*(Mock*)p;if(o==0x71040){*v=m.stuck?m.frame:++m.frame;return B_OK;}
  if(o==0x71064&&m.underrun&&(m.r[0x71180]&0x80000000U)){*v=0x100;return B_OK;}*v=m.r[o];return B_OK;}
 static status_t Write(void* p,uint32 o,uint32 v){Mock& m=*(Mock*)p;if(++m.writes==m.fail)return B_ERROR;
  if(o==0x71180&&(v&0x80000000U)&&m.r[0x7127c]!=0x07ff0000)m.bad=true;
  if(o==0x7127c&&v==0&&(m.r[0x71180]&0x80000000U))m.bad=true;
  if(o>=0x70000&&o<0x71000)m.bad=true;
  m.r[o]=v;return B_OK;}
 static void Pause(void*){}
 AlderLakeTypeCIO IO(){return {this,Read,Write,Pause};}
 bool Restored(){Mock old;for(auto x:r){if(x.first==0x71064)continue;if(x.second!=old.r[x.first])return false;}return !bad;}
};
static AlderLakeClonePlan Plan(){AlderLakeClonePlan p={};p.control=0x94000008;p.color=0x2000;p.stride=120;p.size=0x0437077f;p.pipeSize=0x077f0437;p.ddb=0x07ff0000;p.wm0=0x80008021;p.sagv=0x80010041;p.minimumBlocks=73;return p;}
int main(){unsigned errors=0;
 for(unsigned failure=0;failure<=22;failure++){
  Mock m;m.fail=failure;AlderLakeClonePlaneProgrammer p(m.IO(),3);status_t s=p.Apply(Plan());
  if((failure==0)!=(s==B_OK))errors++;
  if(!failure&&(m.r[0x71180]!=0x94000008||m.r[0x68980]&0x80000000U))errors++;
  if(p.Restore()!=B_OK||p.NeedsCleanup()||!m.Restored())errors++;
 }
 {Mock m;m.stuck=true;AlderLakeClonePlaneProgrammer p(m.IO(),3);
  if(p.Apply(Plan())!=B_TIMED_OUT||!p.NeedsCleanup())errors++;
  m.stuck=false;if(p.Restore()!=B_OK||p.NeedsCleanup()||!m.Restored())errors++;
 }
 {Mock m;m.underrun=true;AlderLakeClonePlaneProgrammer p(m.IO(),3);
  if(p.Apply(Plan())!=B_ERROR||p.NeedsCleanup()||!m.Restored()||p.UnderrunAfter()!=0x100)errors++;
 }
 {Mock m;AlderLakeClonePlaneProgrammer p(m.IO(),3);if(p.Apply(Plan())!=B_OK)errors++;
  m.fail=m.writes+1;if(p.Restore()!=B_ERROR||!p.NeedsCleanup())errors++;
  m.fail=0;if(p.Restore()!=B_OK||p.NeedsCleanup()||!m.Restored())errors++;
 }
 {Mock m;m.r[0x71064]=0x100;AlderLakeClonePlaneProgrammer p(m.IO(),3);
  if(p.Apply(Plan())!=B_BUSY||m.writes||p.NeedsCleanup()||p.UnderrunBefore()!=0x100)errors++;
 }
 {Mock m;AlderLakeClonePlan invalid=Plan();invalid.ddb=0x0fff0800;
  AlderLakeClonePlaneProgrammer p(m.IO(),3);
  if(p.Apply(invalid)!=B_BAD_VALUE||m.writes||p.NeedsCleanup())errors++;
 }
 if(errors){fprintf(stderr,"%u clone programming failures\n",errors);return 1;}
 puts("28 clone plane programming, ordering, fault rollback and latch/underrun cases passed");return 0;
}
