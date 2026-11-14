// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include "ResourceCommand.h"
#include "render/RcsPublicShader.h"
#include "render/RcsDepthVertex.h"
#include "render/RcsRotateVertex.h"
#include "render/RcsUserVertices.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc,char** argv)
{
 if(argc>2 || (argc==2 && strcmp(argv[1],"rotate")))return 9;
 const bool rotate=argc==2;
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 uint32_t handles[3]={};
 const void* vertexCode=rotate?(const void*)NativeGpu::RcsRotateVertex::Words:
  (const void*)NativeGpu::RcsDepthVertex::Words;
 const uint32_t vertexBytes=rotate?sizeof(NativeGpu::RcsRotateVertex::Words):
  sizeof(NativeGpu::RcsDepthVertex::Words);
 const void* data[3]={vertices,NativeGpu::RcsDrawData::Fs,vertexCode};
 const uint32_t bytes[3]={sizeof(vertices),sizeof(NativeGpu::RcsDrawData::Fs),
  vertexBytes};
 for(unsigned i=0;i<3;i++){
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 2;}
  handles[i]=c.handle;
  c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
  c.handle=handles[i];c.bytes=bytes[i];memcpy(c.data,data[i],c.bytes);
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){close(fd);return 3;}
 }
 native_gpu_render_command reused={};reused.version=8;
 reused.mode=NATIVE_GPU_RENDER_DRAW;
 reused.variant=handles[0];reused.reserved=handles[1];
 memcpy(reused.output,&handles[1],sizeof(handles[1]));
 if(ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&reused,sizeof(reused))==0
  || reused.report.sequence || reused.report.stage){close(fd);return 4;}
 uint32_t altered[sizeof(NativeGpu::RcsDepthVertex::Words)/4]={};
 memcpy(altered,vertexCode,vertexBytes);altered[vertexBytes/4-1]^=1;
 native_gpu_resource_command write={};write.version=1;
 write.action=NATIVE_GPU_RESOURCE_WRITE;write.handle=handles[2];
 write.bytes=vertexBytes;memcpy(write.data,altered,vertexBytes);
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&write,sizeof(write))!=0){close(fd);return 6;}
 native_gpu_render_command corrupt={};corrupt.version=8;
 corrupt.mode=NATIVE_GPU_RENDER_DRAW;
 corrupt.variant=handles[0];corrupt.reserved=handles[1];
 memcpy(corrupt.output,&handles[2],sizeof(handles[2]));
 if(ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&corrupt,sizeof(corrupt))==0
  || corrupt.report.sequence || corrupt.report.stage){close(fd);return 7;}
 write={};write.version=1;write.action=NATIVE_GPU_RESOURCE_WRITE;
 write.handle=handles[2];write.bytes=vertexBytes;
 memcpy(write.data,vertexCode,write.bytes);
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&write,sizeof(write))!=0){close(fd);return 8;}
 native_gpu_render_command command={};command.version=8;
 command.mode=NATIVE_GPU_RENDER_DRAW;
 command.variant=handles[0];command.reserved=handles[1];
 memcpy(command.output,&handles[2],sizeof(handles[2]));
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 const auto& r=command.report;
 NativeGpu::RcsUserVertices::Check check={};
 uint32_t expectedVertices[12]={};
 auto program=rotate?NativeGpu::RcsPublicShader::RotateHalfturn:
  NativeGpu::RcsPublicShader::DepthQuarter;
 bool geometry=NativeGpu::RcsPublicShader::ExpectedVertices(
  program,vertices,expectedVertices);
 bool image=NativeGpu::RcsUserVertices::Inspect(
  (const uint32_t*)command.output,expectedVertices,check);
 bool passed=status==0 && r.status==0 && r.version==8
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.sequence
  && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce && r.fault==0
  && r.engine_stage==4 && r.engine_error==0 && r.csb_error==0
  && geometry && image && r.checked_pixels==check.checked
  && r.matched_pixels==check.matched && r.target_hash==check.hash;
 if(rotate)passed=passed && r.target_hash==0x77323fd5;
 printf("combined ioctl=%d status=%d version=%u stage=%u sequence=%u/%u pixels=%u/%u hash=%08x fault=%08x\n",
  status,r.status,r.version,r.stage,r.sequence,r.observed_sequence,
  r.matched_pixels,r.checked_pixels,r.target_hash,r.fault);
 for(unsigned i=0;i<3;i++){
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=handles[i];
  passed=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))==0 && passed;
 }
 close(fd);
 printf("RCS public %s vertex/shader draw: %s\n",
  rotate?"rotate":"depth",passed?"PASS":"NOT PROVEN");
 return passed?0:1;
}
