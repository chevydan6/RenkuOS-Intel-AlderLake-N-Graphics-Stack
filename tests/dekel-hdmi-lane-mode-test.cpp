/* SPDX-License-Identifier: MIT */
#include "DekelHdmiLaneMode.h"
#include <stdio.h>
#include <vector>
struct Mock {
 uint32 bank=0,lanes[2]={0x12345640,0x23456740},buffer=0x40,ready=4;
 unsigned writes=0,fail=0,drops=0;std::vector<unsigned> order;
 static status_t Read(void* p,uint32 o,uint32* v){Mock& m=*(Mock*)p;*v=o==0x64400?m.buffer:o==0x161504?m.ready:m.lanes[m.bank];return B_OK;}
 static status_t Write(void* p,uint32 o,uint32 v){Mock& m=*(Mock*)p;if(++m.writes==m.fail)return B_ERROR;
  if(o==0x1010a0)m.bank=v>>8;else {m.order.push_back(m.bank);if(m.drops)m.drops--;else m.lanes[m.bank]=v;}return B_OK;}
 AlderLakeTypeCIO Ops(){return {this,Read,Write,NULL};}
};
int main(){unsigned errors=0;
 for(unsigned failure=0;failure<=8;failure++) {
  Mock m;m.fail=failure;DekelHdmiLaneMode mode(m.Ops(),2);
  status_t s=mode.Apply();if((failure==0)!=(s==B_OK))errors++;
  if(mode.Restore()!=B_OK||mode.NeedsCleanup()||m.lanes[0]!=0x12345640||m.lanes[1]!=0x23456740)errors++;
 }
 {Mock m;m.drops=2;DekelHdmiLaneMode mode(m.Ops(),2);
  if(mode.Apply()!=B_OK||m.order.size()!=4||m.order[0]!=0||m.order[1]!=1||m.order[2]!=0||m.order[3]!=1
   ||m.lanes[0]!=0x12345680||m.lanes[1]!=0x23456780||mode.Restore()!=B_OK)errors++;
 }
 {Mock m;m.buffer|=0x80000000U;DekelHdmiLaneMode mode(m.Ops(),2);if(mode.Apply()!=B_BUSY||m.writes)errors++;}
 {Mock m;DekelHdmiLaneMode mode(m.Ops(),2);if(mode.Apply()!=B_OK)errors++;
  m.fail=m.writes+1;if(mode.Restore()!=B_ERROR||!mode.NeedsCleanup())errors++;
  m.fail=0;if(mode.Restore()!=B_OK||mode.NeedsCleanup())errors++;
 }
 if(errors){fprintf(stderr,"%u lane mode failures\n",errors);return 1;}
 puts("12 legacy PHY lane mode, masked preservation, rewrite and rollback cases passed");return 0;
}
