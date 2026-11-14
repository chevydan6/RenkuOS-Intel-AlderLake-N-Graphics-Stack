// SPDX-License-Identifier: MIT
// Hardware proof for the separately versioned 64x64 RCS draw request.
#include "RenderReport.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc,char** argv)
{
 if(argc!=2 || argv[1][0]<'0' || argv[1][0]>'3' || argv[1][1])return 2;
 native_gpu_render_large_command command={};
 command.base.version=9;command.base.mode=NATIVE_GPU_RENDER_DRAW;
 command.base.variant=unsigned(argv[1][0]-'0');
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 int status=ioctl(fd,NATIVE_GPU_RENDER_LARGE_IOCTL,&command,sizeof(command));
 close(fd);
 const auto& r=command.base.report;
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<4096;i++){
  const uint8_t* page=i<1024?command.base.output:command.extra;
  unsigned offset=i<1024?i:i-1024;
  uint32_t pixel=((const uint32_t*)page)[offset];
  hash=(hash^pixel)*16777619u;
 }
 const uint32_t first=((const uint32_t*)command.base.output)[0];
 const uint32_t inside=((const uint32_t*)command.extra)[20*64+40-1024];
 bool passed=status==0 && r.status==0 && r.version==9
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.sequence
  && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce && r.checked_pixels>=2000
  && r.matched_pixels==r.checked_pixels && hash==r.target_hash
  && !r.fault && r.engine_stage==4 && !r.engine_error && !r.csb_error
  && first==0x19537bad && (command.base.variant!=0 || inside==0xffff4000);
 printf("large64 ioctl=%d status=%d stage=%u sequence=%u/%u checked=%u/%u hash=%08x fault=%08x\n",
  status,r.status,r.stage,r.sequence,r.observed_sequence,
  r.matched_pixels,r.checked_pixels,hash,r.fault);
 puts(passed?"RCS 64x64 N100 draw: PASS":"RCS 64x64 N100 draw: NOT PROVEN");
 return passed?0:1;
}
