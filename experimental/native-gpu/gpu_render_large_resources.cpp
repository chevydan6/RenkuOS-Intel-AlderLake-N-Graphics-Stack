// SPDX-License-Identifier: MIT
// Physical proof of a 64x64 draw using file-local vertex and shader BOs.
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsPublicShader.h"
#include "render/RcsUserVertices.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc,char** argv)
{
 if(argc!=2 || argv[1][0]<'0' || argv[1][0]>'1' || argv[1][1])return 2;
 const uint32_t vertices[2][12]={
  {0xbf400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0x3f400000,0x3f000000,0x3f800000},
  {0xbf300000,0xbf300000,0x3f000000,0x3f800000,
   0x3f300000,0xbf300000,0x3f000000,0x3f800000,
   0x00000000,0x3f300000,0x3f000000,0x3f800000}};
 unsigned shape=unsigned(argv[1][0]-'0');
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 uint32_t handles[2]={};
 const void* data[2]={vertices[shape],NativeGpu::RcsDrawData::Fs};
 const uint32_t bytes[2]={sizeof(vertices[shape]),sizeof(NativeGpu::RcsDrawData::Fs)};
 for(unsigned i=0;i<2;i++){
  native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;
  c.size=4096;
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 3;}
  handles[i]=c.handle;
  c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
  c.handle=handles[i];c.bytes=bytes[i];memcpy(c.data,data[i],c.bytes);
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 4;}
 }
 native_gpu_render_large_command command={};
 command.base.version=10;command.base.mode=NATIVE_GPU_RENDER_DRAW;
 command.base.variant=handles[0];command.base.reserved=handles[1];
 int status=ioctl(fd,NATIVE_GPU_RENDER_LARGE_IOCTL,&command,sizeof(command));
 const auto& r=command.base.report;
 uint32_t* pages[11]={};
 pages[6]=(uint32_t*)command.base.output;
 for(unsigned p=1;p<4;p++)pages[6+p]=(uint32_t*)(command.extra+(p-1)*4096);
 NativeGpu::RcsUserVertices::Check check={};
 bool image=NativeGpu::RcsUserVertices::Inspect64(pages,vertices[shape],check);
 bool passed=status==0 && r.status==0 && r.version==10
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.sequence
  && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce && !r.fault && r.engine_stage==4
  && !r.engine_error && !r.csb_error && image
  && r.checked_pixels==check.checked && r.matched_pixels==check.matched
  && r.target_hash==check.hash;
 printf("large_resource64 ioctl=%d status=%d stage=%u sequence=%u/%u checked=%u/%u hash=%08x fault=%08x\n",
  status,r.status,r.stage,r.sequence,r.observed_sequence,
  r.matched_pixels,r.checked_pixels,r.target_hash,r.fault);
 for(unsigned i=0;i<2;i++){
  native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_DESTROY;
  c.handle=handles[i];
  passed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0 && passed;
 }
 close(fd);
 puts(passed?"RCS 64x64 resource draw: PASS":"RCS 64x64 resource draw: NOT PROVEN");
 return passed?0:1;
}
