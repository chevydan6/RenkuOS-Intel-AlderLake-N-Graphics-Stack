// SPDX-License-Identifier: MIT
#include "RenderReport.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
 uint32_t mode=NATIVE_GPU_RENDER_PROBE,variant=0;
 if(argc==3 && strcmp(argv[1],"draw")==0 && argv[2][0]>='0' && argv[2][0]<='3' && !argv[2][1]){
  mode=NATIVE_GPU_RENDER_DRAW;variant=uint32_t(argv[2][0]-'0');
 }
 else if(argc==2 && strcmp(argv[1],"bootstrap")==0)mode=NATIVE_GPU_RENDER_BOOTSTRAP;
 else if(argc>2 || (argc==2 && strcmp(argv[1],"probe")!=0)){
  fprintf(stderr,"usage: %s [probe|bootstrap|draw VARIANT]\n",argv[0]);return 2;
 }
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open native GPU execution bridge");return 1;}
 native_gpu_render_command command={};command.version=1;command.mode=mode;command.variant=variant;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));close(fd);
 const auto& r=command.report;
 printf("native_render version=%u mode=%u ioctl=%d status=%d stage=%u refusal=%u\n",
  r.version,r.mode,status,r.status,r.stage,r.refusal);
 printf("identity=%08x revision=%u known_profile=%u owner_granted=%u retained=%u\n",
  r.identity,r.revision,r.known_profile,r.owner_granted,r.retained);
 printf("slice=%08x DSS=%08x EU_disable=%08x media_disable=%08x mirror_fuse3=%08x RPCS=%08x\n",
  r.slice_enable,r.geometry_dss_enable,r.eu_disable,r.media_disable,r.mirror_fuse3,r.rpcs);
 printf("ring_ctl=%08x mode=%08x EL=%08x/%08x EIR=%08x ESR=%08x fault=%08x\n",
  r.ring_control,r.ring_mode,r.el_status_hi,r.el_status_lo,r.eir,r.esr,r.fault);
 printf("mapping=%d dma=%d bootstrap=%d group=%d pci_error=%u forcewake=%u/%u group_detail=%u/%u safety=%u\n",
  r.mapping,r.dma,r.bootstrap,r.group,r.pci_error,r.bootstrap_stage,r.bootstrap_error,r.group_stage,r.group_error,r.safety_reason);
 printf("state_flags=%u cursor=%u checked=%u matched=%u target_hash=%08x interior=%u,%u exterior=%u,%u CSB=%016llx/%016llx\n",
  r.state_flags,r.csb_cursor,r.checked_pixels,r.matched_pixels,r.target_hash,
  r.interior_xy&65535,r.interior_xy>>16,r.exterior_xy&65535,r.exterior_xy>>16,
  (unsigned long long)r.activation,(unsigned long long)r.completion);
 if(r.refusal==NATIVE_GPU_RENDER_NEEDS_BOOTSTRAP)puts("DRAW requires explicit bootstrap after initialization or resume.");
 if(r.refusal==NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED)puts("RCS default context is already bootstrapped.");
 if(r.refusal==NATIVE_GPU_RENDER_NO_HANDOFF)
  puts("RCS publication refused: trusted display profile/generation handoff is unavailable.");
 printf("nonce=%08x/%08x complement=%08x sequence=%u/%u tail=%u reset=%u/%u engine=%u/%u csb=%u\n",
  r.nonce,r.observed_nonce,r.observed_complement,r.sequence,r.observed_sequence,r.ring_tail,
  r.reset_stage,r.reset_error,r.engine_stage,r.engine_error,r.csb_error);
 bool passed=status==0 && r.version==1 && r.status==0 && r.known_profile
  && ((mode==NATIVE_GPU_RENDER_PROBE && r.stage==6)
   || (mode!=NATIVE_GPU_RENDER_PROBE && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE && r.sequence
    && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce && ~r.nonce==r.observed_complement));
 if(mode==NATIVE_GPU_RENDER_DRAW){
  uint32_t hash=2166136261u;
  const uint32_t* pixels=(const uint32_t*)command.output;
  for(unsigned i=0;i<1024;i++)hash=(hash^pixels[i])*16777619u;
  passed=passed && r.checked_pixels && r.checked_pixels==r.matched_pixels && hash==r.target_hash;
 }
 if(mode==NATIVE_GPU_RENDER_DRAW)puts(passed?"RCS fixed-pipeline shader draw: PASS":"RCS fixed-pipeline shader draw: NOT PROVEN");
 else if(mode==NATIVE_GPU_RENDER_PROBE)puts(passed?"RCS read-only probe: PASS":"RCS read-only probe: NOT PROVEN");
 else puts(passed?"RCS command-streamer bootstrap: PASS (no shader draw)":"RCS command-streamer execution: NOT PROVEN");
 return passed?0:1;
}
