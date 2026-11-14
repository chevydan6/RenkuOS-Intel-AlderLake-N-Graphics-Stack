// SPDX-License-Identifier: MIT
#include "ResourceCommand.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
int main() {
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0){perror("open resource bridge");return 1;}
 native_gpu_resource_command valid={};
 valid.version=1;valid.action=NATIVE_GPU_RESOURCE_CREATE;valid.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&valid,sizeof(valid))!=0)return 1;
 native_gpu_resource_command destroy={};destroy.version=1;
 destroy.action=NATIVE_GPU_RESOURCE_DESTROY;destroy.handle=valid.handle;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy))!=0)return 1;
 void* page=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 if(page==MAP_FAILED)return 1;
 auto* request=(native_gpu_resource_command*)page;
 *request={};request->version=1;request->action=NATIVE_GPU_RESOURCE_CREATE;
 request->size=4096;
 if(mprotect(page,8192,PROT_READ)!=0){perror("mprotect");return 1;}
 for(unsigned i=0;i<32;i++)
  if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,request,sizeof(*request))==0)return 1;
 valid={};valid.version=1;valid.action=NATIVE_GPU_RESOURCE_CREATE;valid.size=4096;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&valid,sizeof(valid))!=0){
  perror("create after failed copyouts");return 1;
 }
 destroy.handle=valid.handle;
 if(ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy))!=0)return 1;
 munmap(page,8192);close(fd);
 puts("Native resource failed-copyout rollback: PASS");return 0;
}
