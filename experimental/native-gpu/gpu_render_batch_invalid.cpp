// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsValidatedBatch.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static bool Transfer(int fd,uint32_t handle,const void* bytes,unsigned count)
{
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
 c.handle=handle;c.bytes=count;memcpy(c.data,bytes,count);
 return ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0;
}
static bool Rejected(int fd,uint32_t vertex,uint32_t batch,unsigned trial)
{
 native_gpu_render_command c={};c.version=5;c.mode=NATIVE_GPU_RENDER_DRAW;
 c.variant=vertex;c.reserved=batch;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&c,sizeof(c));
 bool cleared=true;for(uint8_t byte:c.output)cleared&=byte==0;
 bool good=status!=0 && c.report.status<0 && c.report.version==5
  && c.report.stage==0 && c.report.sequence==0 && cleared;
 printf("public_batch_invalid trial=%u status=%d sequence=%u clear=%u result=%s\n",
  trial,status,c.report.sequence,unsigned(cleared),good?"PASS":"FAIL");
 return good;
}
int main()
{
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 int other=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0 || other<0)return 1;
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 uint32_t vertex=c.handle;uint32_t vertexGpu=uint32_t(c.gpu);
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 uint32_t batch=c.handle;
 const uint32_t positions[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 if(!Transfer(fd,vertex,positions,sizeof(positions)))return 1;
 uint32_t words[1024]={};
 memcpy(words,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 if(!NativeGpu::RcsValidatedBatch::PatchVertex(words,vertexGpu))return 1;
 words[0]^=1;
 if(!Transfer(fd,batch,words,sizeof(words)) || !Rejected(fd,vertex,batch,0))return 1;
 words[0]^=1;words[1023]=1;
 if(!Transfer(fd,batch,words,sizeof(words)) || !Rejected(fd,vertex,batch,1))return 1;
 words[1023]=0;
 if(!Transfer(fd,batch,words,sizeof(words)) || !Rejected(other,vertex,batch,2))return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=batch;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 if(!Rejected(fd,vertex,batch,3))return 1;
 c.handle=vertex;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return 1;
 close(other);close(fd);return 0;
}
