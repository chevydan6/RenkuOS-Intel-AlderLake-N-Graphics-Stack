// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
static bool Rejected(int fd,uint32_t handle,unsigned trial)
{
 native_gpu_render_command c={};c.version=4;c.mode=NATIVE_GPU_RENDER_DRAW;c.reserved=handle;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&c,sizeof(c));
 bool clear=true;for(uint8_t byte:c.output)clear&=byte==0;
 bool rejected=status!=0 && c.report.status<0 && c.report.version==4
  && c.report.stage==0 && c.report.sequence==0 && c.report.observed_sequence==0 && clear;
 printf("public_buffer_invalid trial=%u status=%d refusal=%u sequence=%u clear=%u result=%s\n",
  trial,status,c.report.refusal,c.report.sequence,unsigned(clear),rejected?"PASS":"FAIL");
 return rejected;
}
int main()
{
 int a=open("/dev/misc/intel_native_execution",O_RDONLY);
 int b=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(a<0 || b<0)return 1;
 native_gpu_resource_command r={};r.version=1;r.action=NATIVE_GPU_RESOURCE_CREATE;r.size=4096;
 if(ioctl(a,NATIVE_GPU_RESOURCE_IOCTL,&r,sizeof(r))!=0)return 1;
 uint32_t handle=r.handle;
 if(!Rejected(a,handle,0) || !Rejected(b,handle,1))return 1;
 r={};r.version=1;r.action=NATIVE_GPU_RESOURCE_DESTROY;r.handle=handle;
 if(ioctl(a,NATIVE_GPU_RESOURCE_IOCTL,&r,sizeof(r))!=0)return 1;
 if(!Rejected(a,handle,2))return 1;
 close(b);close(a);return 0;
}
