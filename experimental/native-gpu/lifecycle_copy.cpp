// SPDX-License-Identifier: MIT
// Narrow native check: independent client patterns, session lifetime and
// unchanged handles across a rejected quiesced request and driver resume.
#include "BufferCopy.h"
#include "LifecycleReport.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool Transition(int fd,uint32_t action)
{native_gpu_lifecycle_control c={1,action};return ioctl(fd,NATIVE_GPU_LIFECYCLE_CONTROL,&c,sizeof(c))==0;}
int main(int argc,char** argv)
{
 unsigned count=argc>1?unsigned(strtoul(argv[1],NULL,10)):8;
 unsigned seed=argc>2?unsigned(strtoul(argv[2],NULL,10)):19;
 bool cycle=argc>3 && !strcmp(argv[3],"cycle");
 if(!count || count>1000000 || argc>4)return 2;
 int fd=open("/dev/misc/intel_native_execution",O_RDWR);
 if(fd<0){perror("open");return 1;}
 native_gpu_buffer_create a={4096,0},b=a;
 if(ioctl(fd,NATIVE_GPU_BUFFER_CREATE,&a,sizeof(a)) || ioctl(fd,NATIVE_GPU_BUFFER_CREATE,&b,sizeof(b)))return 1;
 native_gpu_buffer_io source={a.handle,0,4096,{}},destination={b.handle,0,4096,{}};
 native_gpu_copy c={a.handle,b.handle,1024,1,4096,4096,{}};
 if(cycle){
  if(!Transition(fd,NATIVE_GPU_QUIESCE))return 1;
  if(ioctl(fd,NATIVE_GPU_BUFFER_COPY,&c,sizeof(c))==0 || c.report.stage!=0)return 1;
  if(ioctl(fd,NATIVE_GPU_BUFFER_READ,&source,sizeof(source)))return 1;
  if(!Transition(fd,NATIVE_GPU_RESUME))return 1;
  puts("quiesce rejection retained CPU ownership; resume kept handles usable");
 }
 uint32_t previous=0;
 for(unsigned n=0;n<count;n++){
  for(unsigned i=0;i<4096;i++)source.data[i]=uint8_t(seed+i*37+n*11+(i>>5));
  memset(destination.data,0xa5,sizeof(destination.data));
  if(ioctl(fd,NATIVE_GPU_BUFFER_WRITE,&source,sizeof(source))
   || ioctl(fd,NATIVE_GPU_BUFFER_WRITE,&destination,sizeof(destination)))return 1;
  if(ioctl(fd,NATIVE_GPU_BUFFER_COPY,&c,sizeof(c))){
   printf("copy failed seed=%u iteration=%u status=%d stage=%u sequence=%u load=%u\n",
    seed,n,c.report.status,c.report.stage,c.report.sequence,c.report.load_attempted);return 1;
  }
  if(c.report.sequence<=previous || c.report.stage!=14 || c.report.copy_matched!=1024)return 1;
  previous=c.report.sequence;
  if(ioctl(fd,NATIVE_GPU_BUFFER_READ,&destination,sizeof(destination))
   || memcmp(source.data,destination.data,4096))return 1;
 }
 close(fd);
 printf("client seed=%u copies=%u final_sequence=%u verified all bytes: PASS\n",seed,count,previous);
 return 0;
}
