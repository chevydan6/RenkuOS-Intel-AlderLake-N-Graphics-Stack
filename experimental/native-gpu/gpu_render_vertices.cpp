// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc,char** argv)
{
 if(argc!=7){fprintf(stderr,"usage: %s x0 y0 x1 y1 x2 y2 (pixel grid 4..28)\n",argv[0]);return 2;}
 int coordinates[6]={};
 for(int i=0;i<6;i++){
  char* end=NULL;errno=0;long value=strtol(argv[i+1],&end,10);
  if(errno || !end || *end || value<4 || value>28){fprintf(stderr,"invalid coordinate %s\n",argv[i+1]);return 2;}
  coordinates[i]=int(value);
 }
 uint32_t vertices[12]={};
 for(int i=0;i<3;i++){
  float x=(coordinates[2*i]-16)/16.0f,y=(coordinates[2*i+1]-16)/16.0f;
  float z=0.5f,w=1.0f;
  memcpy(&vertices[4*i],&x,4);memcpy(&vertices[4*i+1],&y,4);
  memcpy(&vertices[4*i+2],&z,4);memcpy(&vertices[4*i+3],&w,4);
 }
 native_gpu_render_command command={};
 command.version=2;command.mode=NATIVE_GPU_RENDER_DRAW;
 memcpy(command.output,vertices,sizeof(vertices));
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open native GPU execution bridge");return 1;}
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));close(fd);
 const auto& report=command.report;
 printf("rcs_user_vertices ioctl=%d status=%d version=%u stage=%u refusal=%u sequence=%u/%u\n",
  status,report.status,report.version,report.stage,report.refusal,report.sequence,report.observed_sequence);
 printf("pixels=%u/%u hash=%08x interior=%u,%u exterior=%u,%u engine=%u/%u csb=%u fault=%08x\n",
  report.matched_pixels,report.checked_pixels,report.target_hash,
  report.interior_xy&65535,report.interior_xy>>16,report.exterior_xy&65535,report.exterior_xy>>16,
  report.engine_stage,report.engine_error,report.csb_error,report.fault);
 const uint32_t* image=(const uint32_t*)command.output;
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<1024;i++)hash=(hash^image[i])*16777619u;
 unsigned inside=(report.interior_xy>>16)*32+(report.interior_xy&65535);
 unsigned outside=(report.exterior_xy>>16)*32+(report.exterior_xy&65535);
 bool passed=status==0 && report.status==0 && report.version==2
  && report.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && report.known_profile
  && report.owner_granted && report.checked_pixels>=500
  && report.matched_pixels==report.checked_pixels && report.sequence
  && report.observed_sequence==report.sequence && report.nonce==report.observed_nonce
  && report.observed_complement==~report.nonce && hash==report.target_hash
  && inside<1024 && outside<1024 && image[inside]==0xffff4000 && image[outside]==0x19537bad;
 puts(passed?"RCS user vertex hardware draw: PASS":"RCS user vertex hardware draw: NOT PROVEN");
 return passed?0:1;
}
