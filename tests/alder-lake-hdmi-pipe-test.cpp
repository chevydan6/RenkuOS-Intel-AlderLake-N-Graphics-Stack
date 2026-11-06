/* SPDX-License-Identifier: MIT */
#include "AlderLakeHdmiPipe.h"
#include <map>
#include <stdio.h>
struct Mock {
 std::map<uint32,uint32> regs;unsigned writes=0,fail=0;bool stuck=false;
 static status_t Read(void* p,uint32 o,uint32* v){*v=((Mock*)p)->regs[o];return B_OK;}
 static status_t Write(void* p,uint32 o,uint32 v){Mock& m=*(Mock*)p;if(++m.writes==m.fail)return B_ERROR;
  if(o==0x71008 && !m.stuck) v=(v&~0x40000000U)|((v&0x80000000U)>>1);
  if(o==0x71008 && m.stuck) v=(v&~0x40000000U)|(m.regs[o]&0x40000000U);
  m.regs[o]=v;return B_OK;}
 static void Pause(void*){}
 AlderLakeTypeCIO Ops(){return {this,Read,Write,Pause};}
};
int main(){unsigned errors=0;AlderLakeHdmiTiming t={1920,2008,2052,2200,1080,1084,1089,1125};
 for(unsigned fail=0;fail<=8;fail++) {
  Mock m;m.fail=fail;AlderLakeHdmiPipe pipe(m.Ops(),1,3);
  status_t s=pipe.Prepare(t);
  if(fail>0&&fail<=7){if(s==B_OK||pipe.NeedsCleanup())errors++;}
  else {if(s!=B_OK)errors++;if(fail==8){if(pipe.Enable()==B_OK)errors++;}else if(pipe.Enable()!=B_OK)errors++;
   if(pipe.Release()!=B_OK||pipe.NeedsCleanup())errors++;}
  for(auto pair:m.regs)if(pair.second!=0)errors++;
 }
 {Mock m;m.regs[0x71008]=0x80000000;AlderLakeHdmiPipe p(m.Ops(),1,3);if(p.Prepare(t)!=B_BUSY||m.writes)errors++;}
 {Mock m;AlderLakeHdmiPipe p(m.Ops(),1,3);t.hTotal=100;if(p.Prepare(t)!=B_BAD_VALUE||m.writes)errors++;}
 {Mock m;AlderLakeHdmiPipe p(m.Ops(),1,3);if(p.Prepare(t)==B_OK)errors++;}
 t.hTotal=2200;
 {Mock m;m.stuck=true;AlderLakeHdmiPipe p(m.Ops(),1,3);if(p.Prepare(t)!=B_OK||p.Enable()!=B_TIMED_OUT||p.NeedsCleanup())errors++;}
 {Mock m;AlderLakeHdmiPipe p(m.Ops(),1,3);if(p.Prepare(t)!=B_OK||p.Enable()!=B_OK)errors++;
  m.stuck=true;if(p.Release()!=B_TIMED_OUT||!p.NeedsCleanup())errors++;
  m.stuck=false;if(p.Release()!=B_OK||p.NeedsCleanup())errors++;}
 {Mock m;AlderLakeHdmiPipe p(m.Ops(),1,3);if(p.Prepare(t)!=B_OK||p.Enable()!=B_OK)errors++;
  m.fail=m.writes+1;if(p.Release()!=B_ERROR||!p.NeedsCleanup())errors++;
  m.fail=0;if(p.Release()!=B_OK||p.NeedsCleanup())errors++;}
 if(errors){fprintf(stderr,"%u pipe failures\n",errors);return 1;}puts("15 staged HDMI pipe validation, ownership and rollback cases passed; simulated registers only");return 0;
}
