// SPDX-License-Identifier: MIT
#include "ResourceCommand.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main() {
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("resource bridge open");return 1;}
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=8192;
 errno=0;int result=ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c));
 printf("resource_create ioctl=%d errno=%d handle=%u size=%llu gpu=%llx\n",
  result,errno,c.handle,(unsigned long long)c.size,(unsigned long long)c.gpu);
 close(fd);return result==0?0:1;
}
