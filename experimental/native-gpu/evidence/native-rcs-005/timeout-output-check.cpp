#include "RenderReport.h"
#include "LifecycleReport.h"
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
int main() {
 int fd=open("/dev/misc/intel_native_execution",O_RDWR);if(fd<0)return 1;
 native_gpu_lifecycle_report before={};if(ioctl(fd,NATIVE_GPU_LIFECYCLE_STATUS,&before,sizeof before))return 2;
 native_gpu_render_command command={};command.version=1;command.mode=NATIVE_GPU_RENDER_DRAW;
 memset(command.output,0xa5,sizeof command.output);
 bigtime_t start=system_time();int result=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof command);
 bigtime_t elapsed=system_time()-start;bool zero=true;
 for(auto b:command.output)zero=zero && b==0;
 const auto failed=command.report;
 native_gpu_lifecycle_report lost={};if(ioctl(fd,NATIVE_GPU_LIFECYCLE_STATUS,&lost,sizeof lost))return 3;
 command={};command.version=1;command.mode=NATIVE_GPU_RENDER_DRAW;
 int rejected=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof command);
 native_gpu_lifecycle_report after={};if(ioctl(fd,NATIVE_GPU_LIFECYCLE_STATUS,&after,sizeof after))return 4;
 printf("timeout ioctl=%d status=%d stage=%u sequence=%u csb_error=%u retained=%u output_zero=%u elapsed_us=%lld\n",result,failed.status,failed.stage,failed.sequence,failed.csb_error,failed.retained,zero,(long long)elapsed);
 printf("lifecycle before=%u/%u lost=%u/%u after=%u/%u completed=%llu failed=%llu rejected_ioctl=%d rejected_status=%d\n",before.state,before.sequence,lost.state,lost.sequence,after.state,after.sequence,(unsigned long long)lost.completed,(unsigned long long)lost.failed,rejected,command.report.status);
 close(fd);
 bool ok=result<0 && failed.status==B_TIMED_OUT && failed.stage==10 && failed.csb_error
  && failed.retained && zero && elapsed<1000000 && lost.state==5 && after.state==5
  && lost.sequence==before.sequence+1 && after.sequence==lost.sequence
  && lost.completed==before.completed && lost.failed==before.failed+1 && rejected<0;
 puts(ok?"RCS uncertain-completion/output-retention: PASS":"RCS uncertain-completion/output-retention: FAIL");return ok?0:1;
}
