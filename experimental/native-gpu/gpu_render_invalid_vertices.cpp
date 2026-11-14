// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main()
{
 const uint32_t valid[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open native GPU execution bridge");return 1;}
 for(unsigned trial=0;trial<3;trial++){
  uint32_t words[12];memcpy(words,valid,sizeof(words));
  if(trial==0)words[0]=0x7fc00000; // NaN x coordinate.
  if(trial==1)words[3]=0; // Non-unit homogeneous w.
  if(trial==2)memcpy(words+4,words,4*sizeof(uint32_t)); // Zero-area edge.
  native_gpu_render_command command={};
  command.version=2;command.mode=NATIVE_GPU_RENDER_DRAW;
  memcpy(command.output,words,sizeof(words));
  int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
  bool cleared=true;
  for(unsigned i=0;i<sizeof(command.output);i++)cleared&=command.output[i]==0;
  bool rejected=status!=0 && command.report.status<0
   && command.report.version==2 && command.report.stage==0
   && command.report.sequence==0 && command.report.observed_sequence==0 && cleared;
  printf("invalid_vertex_trial=%u ioctl=%d status=%d version=%u sequence=%u/%u cleared=%u result=%s\n",
   trial,status,command.report.status,command.report.version,
   command.report.sequence,command.report.observed_sequence,cleared,
   rejected?"PASS":"FAIL");
  if(!rejected){close(fd);return 1;}
 }
 close(fd);
 return 0;
}
