// SPDX-License-Identifier: MIT
#include "LifecycleReport.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
int main(int argc,char** argv)
{
 uint32_t action=0;
 if(argc==2){
  if(!strcmp(argv[1],"quiesce"))action=NATIVE_GPU_QUIESCE;
  else if(!strcmp(argv[1],"resume"))action=NATIVE_GPU_RESUME;
  else if(!strcmp(argv[1],"shutdown"))action=NATIVE_GPU_SHUTDOWN;
  else if(!strcmp(argv[1],"test-timeout"))action=NATIVE_GPU_TEST_TIMEOUT;
  else if(strcmp(argv[1],"status")){fprintf(stderr,"unknown action\n");return 2;}
 }else if(argc!=1){fprintf(stderr,"usage: gpu_lifecycle [status|quiesce|resume|shutdown|test-timeout]\n");return 2;}
 int fd=open("/dev/misc/intel_native_execution",O_RDWR);
 if(fd<0){perror("open");return 1;}
 bool ok=true;
 if(action){
  native_gpu_lifecycle_control control={1,action};
  if(ioctl(fd,NATIVE_GPU_LIFECYCLE_CONTROL,&control,sizeof(control))<0){perror("transition");ok=false;}
 }
 native_gpu_lifecycle_report r={};
 if(ioctl(fd,NATIVE_GPU_LIFECYCLE_STATUS,&r,sizeof(r))<0){perror("status");close(fd);return 1;}
 printf("version=%u state=%u clients=%u sequence=%u last_status=%" PRId32
  " generation=%" PRIu64 " completed=%" PRIu64 " failed=%" PRIu64 "\n",
  r.version,r.state,r.clients,r.sequence,r.last_status,r.generation,r.completed,r.failed);
 printf("total_us=%" PRIu64 " cache_us=%" PRIu64 " cache_calls=%u wait_us=%" PRIu64
  " polls=%u sleeps=%u submit_us=%" PRIu64 " queue_us=%" PRIu64
  " recovery_stage=%u recovery_error=%u reset_rounds=%u\n",
  r.last.total_us,r.last.cache_us,r.last.cache_calls,r.last.wait_us,r.last.polls,r.last.sleeps,
  r.last.submit_us,r.last.queue_us,r.last.recovery_stage,r.last.recovery_error,r.last.reset_rounds);
 close(fd);return ok?0:1;
}
