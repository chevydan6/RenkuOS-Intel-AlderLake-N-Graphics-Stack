// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsValidatedBatch.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
 bool publicBatch=argc==8 && strcmp(argv[1],"--batch")==0;
 if(argc!=(publicBatch?8:7)){
  fprintf(stderr,"usage: %s [--batch] x0 y0 x1 y1 x2 y2\n",argv[0]);return 2;
 }
 int coordinates[6]={};
 for(int i=0;i<6;i++){
  char* end=NULL;errno=0;long value=strtol(argv[i+1+(publicBatch?1:0)],&end,10);
  if(errno || !end || *end || value<4 || value>28)return 2;
  coordinates[i]=int(value);
 }
 uint32_t vertices[12]={};
 for(int i=0;i<3;i++){
  float x=(coordinates[2*i]-16)/16.0f,y=(coordinates[2*i+1]-16)/16.0f;
  float z=0.5f,w=1.0f;
  memcpy(&vertices[4*i],&x,4);memcpy(&vertices[4*i+1],&y,4);
  memcpy(&vertices[4*i+2],&z,4);memcpy(&vertices[4*i+3],&w,4);
 }
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open native GPU bridge");return 1;}
 native_gpu_resource_command resource={};resource.version=1;
 resource.action=NATIVE_GPU_RESOURCE_CREATE;resource.size=4096;
 int status=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource));
 if(status!=0){printf("resource_create status=%d\n",status);close(fd);return 1;}
 uint32_t handle=resource.handle;uint64_t gpu=resource.gpu;
 resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_WRITE;
 resource.handle=handle;resource.bytes=sizeof(vertices);
 memcpy(resource.data,vertices,sizeof(vertices));
 status=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource));
 if(status!=0){printf("resource_write status=%d\n",status);close(fd);return 1;}
 uint32_t batchHandle=0;uint64_t batchGpu=0;
 if(publicBatch){
  resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_CREATE;resource.size=4096;
  status=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource));
  if(status!=0){printf("batch_create status=%d\n",status);close(fd);return 1;}
  batchHandle=resource.handle;batchGpu=resource.gpu;
  uint32_t batch[1024]={};
  memcpy(batch,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
  if(!NativeGpu::RcsValidatedBatch::PatchVertex(batch,uint32_t(gpu))){close(fd);return 1;}
  resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_WRITE;
  resource.handle=batchHandle;resource.bytes=sizeof(batch);
  memcpy(resource.data,batch,sizeof(batch));
  status=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource));
  if(status!=0){printf("batch_write status=%d\n",status);close(fd);return 1;}
 }
 native_gpu_render_command command={};command.version=publicBatch?5:4;
 command.mode=NATIVE_GPU_RENDER_DRAW;
 command.reserved=publicBatch?batchHandle:handle;
 command.variant=publicBatch?handle:0;
 status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 const auto& r=command.report;
 printf("rcs_public_%s ioctl=%d status=%d version=%u stage=%u refusal=%u sequence=%u/%u gpu=%llx batch=%llx\n",
  publicBatch?"batch":"buffer",
  status,r.status,r.version,r.stage,r.refusal,r.sequence,r.observed_sequence,
  (unsigned long long)gpu,(unsigned long long)batchGpu);
 printf("pixels=%u/%u hash=%08x engine=%u/%u csb=%u fault=%08x\n",
  r.matched_pixels,r.checked_pixels,r.target_hash,r.engine_stage,r.engine_error,r.csb_error,r.fault);
 const uint32_t* image=(const uint32_t*)command.output;
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<1024;i++)hash=(hash^image[i])*16777619u;
 bool passed=status==0 && r.status==0 && r.version==(publicBatch?5U:4U)
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.known_profile
  && r.owner_granted && r.checked_pixels>=500 && r.matched_pixels==r.checked_pixels
  && r.sequence && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce && hash==r.target_hash
  && r.engine_stage==4 && r.engine_error==0 && r.csb_error==0 && r.fault==0;
 if(publicBatch){
  resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_DESTROY;
  resource.handle=batchHandle;
  passed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource))==0 && passed;
 }
 resource={};resource.version=1;resource.action=NATIVE_GPU_RESOURCE_DESTROY;
 resource.handle=handle;
 int destroyed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&resource,sizeof(resource));
 close(fd);
 passed=passed && destroyed==0;
 puts(passed?(publicBatch?"RCS public batch hardware draw: PASS":"RCS public buffer hardware draw: PASS")
  :(publicBatch?"RCS public batch hardware draw: NOT PROVEN":"RCS public buffer hardware draw: NOT PROVEN"));
 return passed?0:1;
}
