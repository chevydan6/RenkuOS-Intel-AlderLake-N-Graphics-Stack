// SPDX-License-Identifier: MIT
#include "BufferCopy.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static bool Create(int fd,uint32_t& handle)
{native_gpu_buffer_create c={4096,0};if(ioctl(fd,NATIVE_GPU_BUFFER_CREATE,&c,sizeof(c))!=0)return false;handle=c.handle;return true;}
int main()
{
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);if(fd<0){perror("open");return 1;}
 uint32_t a=0,b=0;if(!Create(fd,a)||!Create(fd,b))return 1;
 native_gpu_buffer_io source={a,0,4096,{}},dest={b,0,4096,{}};
 for(unsigned i=0;i<4096;i++){source.data[i]=uint8_t(i*37+11);dest.data[i]=0xa5;}
 if(ioctl(fd,NATIVE_GPU_BUFFER_WRITE,&source,sizeof(source))!=0
  || ioctl(fd,NATIVE_GPU_BUFFER_WRITE,&dest,sizeof(dest))!=0)return 1;
 native_gpu_copy c={a,b,17,5,80,96,{}};
 // Rejected requests must leave the session and GPU admission usable.
 native_gpu_copy bad=c;bad.destination=a;
 if(ioctl(fd,NATIVE_GPU_BUFFER_COPY,&bad,sizeof(bad))==0)return 1;
 bad=c;bad.height=100;
 if(ioctl(fd,NATIVE_GPU_BUFFER_COPY,&bad,sizeof(bad))==0)return 1;
 for(unsigned n=0;n<3;n++){
  int status=ioctl(fd,NATIVE_GPU_BUFFER_COPY,&c,sizeof(c));
  printf("buffer_copy ioctl=%d status=%d sequence=%u observed=%u words=%u matched=%u stage=%u EIR=%08x fault=%08x\n",
   status,c.report.status,c.report.sequence,c.report.observed_sequence,c.report.copy_words,c.report.copy_matched,
   c.report.stage,c.report.error_register,c.report.fault_register);
  if(status!=0 || c.report.status!=0 || c.report.copy_words!=85 || c.report.copy_matched!=85)return 1;
 }
 if(ioctl(fd,NATIVE_GPU_BUFFER_READ,&dest,sizeof(dest))!=0)return 1;
 for(unsigned i=0;i<4096;i++){
  unsigned y=i/96,x=i%96;
  uint8_t expected=y<5 && x<68?source.data[y*80+x]:0xa5;
  if(dest.data[i]!=expected){printf("mismatch byte%u\n",i);return 1;}
 }
 // Handles are session-local; a fresh open has no buffers.
 int other=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(other<0 || ioctl(other,NATIVE_GPU_BUFFER_COPY,&c,sizeof(c))==0)return 1;
 close(other);close(fd);
 puts("Owned-buffer rectangular GPU copy: PASS (17x5, pitches80/96, padding preserved)");return 0;
}
