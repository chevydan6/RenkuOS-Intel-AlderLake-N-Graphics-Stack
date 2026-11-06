/* SPDX-License-Identifier: MIT */
#include "AlderLakeHdmiContext.h"
#include <vector>
#include <stdio.h>
int main() {
 unsigned errors=0;
 for(unsigned scenario=0;scenario<6;scenario++) {
  std::vector<uint32> words(0x16b000/4,0);intel_shared_info shared={};
  words[0x45404/4]=0xfc0f;
  AlderLakeHdmiFirmware fw={2,0,148500,38400,false,false};
  unsigned pipe=1;
  if(scenario==0)words[0x45404/4]=0;
  if(scenario==1)words[0x71008/4]=0x80000000U;
  if(scenario==2)words[0x71280/4]=0x80000000U;
  if(scenario==3)words[0x71080/4]=7;
  if(scenario==4)words[0x45504/4]=1;
  if(scenario==5)pipe=3;
  auto before=words;
  AlderLakeHdmiContext context((volatile uint8*)words.data(),words.size()*4,shared,fw,pipe);
  AlderLakeHdmiTiming t={1920,2008,2052,2200,1080,1084,1089,1125};
  if(context.Start(t,true,true)==B_OK||context.NeedsCleanup()||context.IsActive()
   ||context.Stop()!=B_OK||words!=before||shared.type_c_port_leases||shared.display_pipe_leases)errors++;
 }
 if(errors){fprintf(stderr,"%u context gate failures\n",errors);return 1;}
 puts("6 mapped RAM raster context preflight cases passed without register writes");return 0;
}
