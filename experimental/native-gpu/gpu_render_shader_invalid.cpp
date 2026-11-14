// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsPublicShader.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static bool WritePage(int fd,uint32_t handle,const uint32_t* page)
{
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
 c.handle=handle;c.bytes=4096;memcpy(c.data,page,4096);
 return ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0;
}
static bool Rejected(int fd,uint32_t handle,unsigned trial)
{
 native_gpu_render_command c={};c.version=6;c.mode=NATIVE_GPU_RENDER_DRAW;
 c.variant=0;c.reserved=handle;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&c,sizeof(c));
 bool clear=true;for(uint8_t byte:c.output)clear&=byte==0;
 bool good=status!=0 && c.report.status<0 && c.report.version==6
  && c.report.stage==0 && c.report.sequence==0 && clear;
 printf("public_shader_invalid trial=%u status=%d sequence=%u clear=%u result=%s\n",
  trial,status,c.report.sequence,unsigned(clear),good?"PASS":"FAIL");
 return good;
}
int main()
{
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 int other=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0 || other<0)return 1;
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 uint32_t handle=c.handle;uint32_t shader[1024]={};
 memcpy(shader,NativeGpu::RcsDrawData::Fs,sizeof(NativeGpu::RcsDrawData::Fs));
 shader[0]^=1;
 if(!WritePage(fd,handle,shader) || !Rejected(fd,handle,0))return 1;
 shader[0]^=1;shader[1023]=1;
 if(!WritePage(fd,handle,shader) || !Rejected(fd,handle,1))return 1;
 shader[1023]=0;
 if(!WritePage(fd,handle,shader) || !Rejected(other,handle,2))return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=handle;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 if(!Rejected(fd,handle,3))return 1;
 close(other);close(fd);return 0;
}
