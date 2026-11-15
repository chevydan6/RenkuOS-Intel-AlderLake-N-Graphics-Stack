// SPDX-License-Identifier: MIT
// Physical N100 probe for an owner-bound UBO driving fragment output.
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsUboFragment.h"
#include "render/RcsUniformShader.h"
#include "render/RcsUserVertices.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static bool Create(int fd,unsigned size,uint32_t& handle)
{
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;
 c.size=size;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return false;
 handle=c.handle;return handle!=0;
}
static bool Write(int fd,uint32_t handle,const void* data,unsigned bytes)
{
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
 c.handle=handle;c.bytes=bytes;memcpy(c.data,data,bytes);
 return ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0;
}
static bool ReadImage(int fd,uint32_t handle,const uint8_t* expected)
{
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;
 c.handle=handle;c.bytes=4096;
 return ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0
  && memcmp(c.data,expected,4096)==0;
}

int main()
{
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 const uint32_t colors[2][4]={
  {0x3f800000,0x3e800000,0,0x3f800000},
  {0,0x3f800000,0x3f000000,0x3f800000}};
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 native_gpu_render_command bootstrap={};bootstrap.version=1;
 bootstrap.mode=NATIVE_GPU_RENDER_BOOTSTRAP;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&bootstrap,sizeof(bootstrap));
 if(!((status==0 && bootstrap.report.status==0
   && bootstrap.report.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE)
   || bootstrap.report.refusal==NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED)){
  close(fd);return 2;
 }
 uint32_t handles[4]={};
 bool ready=Create(fd,4096,handles[0]) && Create(fd,4096,handles[1])
  && Create(fd,4096,handles[2]) && Create(fd,4096,handles[3])
  && Write(fd,handles[0],vertices,sizeof(vertices))
  && Write(fd,handles[1],NativeGpu::RcsUboFragment::Words,
   sizeof(NativeGpu::RcsUboFragment::Words));
 if(!ready){close(fd);return 3;}
 native_gpu_resource_command shaderRead={};shaderRead.version=1;
 shaderRead.action=NATIVE_GPU_RESOURCE_READ;shaderRead.handle=handles[1];
 shaderRead.bytes=4096;
 int shaderReadResult=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&shaderRead,sizeof(shaderRead));
 bool shaderExact=shaderReadResult==0
  && NativeGpu::RcsUniformShader::ValidatePage((const uint32_t*)shaderRead.data,1024);
 printf("shader_read=%d exact=%u\n",shaderReadResult,unsigned(shaderExact));
 if(!shaderExact){close(fd);return 4;}
 bool passed=true;
 for(unsigned i=0;i<2;i++){
  if(!Write(fd,handles[3],colors[i],sizeof(colors[i]))){passed=false;break;}
  native_gpu_render_command c={};c.version=16;c.mode=NATIVE_GPU_RENDER_DRAW;
  c.variant=handles[0];c.reserved=handles[1];
  memcpy(c.output+8,&handles[2],sizeof(handles[2]));
  memcpy(c.output+12,&handles[3],sizeof(handles[3]));
  int result=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&c,sizeof(c));
  const auto& r=c.report;
  uint32_t expected=0;
  NativeGpu::RcsUserVertices::Check check={};
  bool pixels=NativeGpu::RcsUniformShader::ExpectedPixel(colors[i],expected)
   && NativeGpu::RcsUserVertices::Inspect((uint32_t*)c.output,
    vertices,check,expected);
  bool okay=result==0 && r.status==0 && r.version==16
   && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE
   && r.sequence && r.observed_sequence==r.sequence
   && r.nonce==r.observed_nonce && r.observed_complement==~r.nonce
   && !r.fault && !r.engine_error && !r.csb_error
   && pixels && r.checked_pixels==check.checked
   && r.matched_pixels==check.matched && r.target_hash==check.hash
   && ReadImage(fd,handles[2],c.output);
  printf("ubo_draw=%u ioctl=%d status=%d stage=%u seq=%u/%u "
   "version=%u refusal=%u handles=%u,%u,%u,%u "
   "expected=%08x checked=%u/%u hash=%08x fault=%08x\n",
   i,result,r.status,r.stage,r.sequence,r.observed_sequence,
   r.version,r.refusal,handles[0],handles[1],handles[2],handles[3],expected,
   r.matched_pixels,r.checked_pixels,r.target_hash,r.fault);
  if(!okay){passed=false;break;}
 }
 for(uint32_t handle:handles)if(handle){
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=handle;
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)passed=false;
 }
 close(fd);
 puts(passed?"RCS UBO color draw: PASS":"RCS UBO color draw: NOT PROVEN");
 return passed?0:1;
}
