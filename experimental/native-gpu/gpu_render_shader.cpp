// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsPublicShader.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
 if(argc!=2 || argv[1][0]<'0' || argv[1][0]>'3' || argv[1][1]){
  fprintf(stderr,"usage: %s VARIANT(0..3)\n",argv[0]);return 2;
 }
 unsigned variant=unsigned(argv[1][0]-'0');
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open native GPU bridge");return 1;}
 native_gpu_resource_command resource={};resource.version=1;
 resource.action=NATIVE_GPU_RESOURCE_CREATE;resource.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource))!=0){close(fd);return 1;}
 uint32_t handle=resource.handle;uint64_t gpu=resource.gpu;
 resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_WRITE;
 resource.handle=handle;resource.bytes=sizeof(NativeGpu::RcsDrawData::Fs);
 memcpy(resource.data,NativeGpu::RcsDrawData::Fs,resource.bytes);
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource))!=0){close(fd);return 1;}
 native_gpu_render_command command={};command.version=6;
 command.mode=NATIVE_GPU_RENDER_DRAW;command.variant=variant;command.reserved=handle;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 const auto& r=command.report;
 uint32_t hash=2166136261u;
 const uint32_t* image=(const uint32_t*)command.output;
 for(unsigned i=0;i<1024;i++)hash=(hash^image[i])*16777619u;
 printf("rcs_public_shader ioctl=%d status=%d version=%u stage=%u sequence=%u/%u variant=%u gpu=%llx\n",
  status,r.status,r.version,r.stage,r.sequence,r.observed_sequence,variant,
  (unsigned long long)gpu);
 printf("checked=%u matched=%u target_hash=%08x fault=%08x engine=%u/%u csb=%u\n",
  r.checked_pixels,r.matched_pixels,r.target_hash,r.fault,
  r.engine_stage,r.engine_error,r.csb_error);
 bool passed=status==0 && r.status==0 && r.version==6
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.known_profile
  && r.owner_granted && r.checked_pixels==654 && r.matched_pixels==654
  && r.sequence && r.observed_sequence==r.sequence
  && r.nonce==r.observed_nonce && r.observed_complement==~r.nonce
  && hash==r.target_hash && r.fault==0 && r.engine_stage==4
  && r.engine_error==0 && r.csb_error==0;
 resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_DESTROY;
 resource.handle=handle;
 passed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource))==0 && passed;
 close(fd);
 puts(passed?"RCS public shader hardware draw: PASS":"RCS public shader hardware draw: NOT PROVEN");
 return passed?0:1;
}
