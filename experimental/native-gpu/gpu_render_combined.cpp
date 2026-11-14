// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsPublicShader.h"
#include "render/RcsUserVertices.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main()
{
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 uint32_t handles[2]={};
 const void* data[2]={vertices,NativeGpu::RcsDrawData::Fs};
 const uint32_t bytes[2]={sizeof(vertices),sizeof(NativeGpu::RcsDrawData::Fs)};
 for(unsigned i=0;i<2;i++){
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 2;}
  handles[i]=c.handle;
  c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
  c.handle=handles[i];c.bytes=bytes[i];memcpy(c.data,data[i],c.bytes);
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 3;}
 }
 native_gpu_render_command reused={};reused.version=7;
 reused.mode=NATIVE_GPU_RENDER_DRAW;
 reused.variant=handles[0];reused.reserved=handles[0];
 if(ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&reused,sizeof(reused))==0
  || reused.report.sequence || reused.report.stage){close(fd);return 4;}
 native_gpu_render_command command={};command.version=7;
 command.mode=NATIVE_GPU_RENDER_DRAW;
 command.variant=handles[0];command.reserved=handles[1];
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 const auto& r=command.report;
 NativeGpu::RcsUserVertices::Check check={};
 bool image=NativeGpu::RcsUserVertices::Inspect(
  (const uint32_t*)command.output,vertices,check);
 bool passed=status==0 && r.status==0 && r.version==7
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.sequence
  && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce && r.fault==0
  && r.engine_stage==4 && r.engine_error==0 && r.csb_error==0
  && image && r.checked_pixels==check.checked
  && r.matched_pixels==check.matched && r.target_hash==check.hash;
 printf("combined ioctl=%d status=%d version=%u stage=%u sequence=%u/%u pixels=%u/%u hash=%08x fault=%08x\n",
  status,r.status,r.version,r.stage,r.sequence,r.observed_sequence,
  r.matched_pixels,r.checked_pixels,r.target_hash,r.fault);
 for(unsigned i=0;i<2;i++){
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=handles[i];
  passed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0 && passed;
 }
 close(fd);
 puts(passed?"RCS combined public vertex/shader draw: PASS":
  "RCS combined public vertex/shader draw: NOT PROVEN");
 return passed?0:1;
}
