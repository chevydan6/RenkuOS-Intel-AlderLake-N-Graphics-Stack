// SPDX-License-Identifier: MIT
#include "ResourceCommand.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int Run(int fd,native_gpu_resource_command& c)
{return ioctl(fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c));}
int main()
{
 int a=open("/dev/misc/intel_native_execution",O_RDONLY);
 int b=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(a<0 || b<0){perror("open native GPU resource bridge");return 1;}
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=8192;
 if(Run(a,c)!=0 || !c.handle || c.size!=8192 || c.gpu<0x410000 || (c.gpu&4095))return 1;
 const uint32_t handle=c.handle;const uint64_t gpu=c.gpu;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=handle;c.bytes=32;
 if(Run(a,c)!=0)return 1;
 for(unsigned i=0;i<32;i++)if(c.data[i])return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;c.handle=handle;c.offset=4090;c.bytes=32;
 for(unsigned i=0;i<32;i++)c.data[i]=uint8_t(i*7+3);
 if(Run(a,c)!=0)return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=handle;c.offset=4090;c.bytes=32;
 if(Run(a,c)!=0)return 1;
 for(unsigned i=0;i<32;i++)if(c.data[i]!=uint8_t(i*7+3))return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=handle;c.offset=8190;c.bytes=4;
 if(Run(a,c)==0)return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=handle;c.bytes=1;
 if(Run(b,c)==0)return 1; // A second file cannot use a forged handle.
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=handle;
 if(Run(a,c)!=0)return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=handle;c.bytes=1;
 if(Run(a,c)==0)return 1;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=8192;
 if(Run(a,c)!=0 || !c.handle || c.handle==handle || c.gpu!=gpu)return 1;
 uint32_t next=c.handle;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_READ;c.handle=next;c.offset=4090;c.bytes=32;
 if(Run(a,c)!=0)return 1;
 for(unsigned i=0;i<32;i++)if(c.data[i])return 1; // Reuse zeroes backing.
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=next;
 if(Run(a,c)!=0)return 1;
 close(b);close(a);
 printf("Native GPU resource create/read/write/destroy: PASS gpu=%llx\n",
  (unsigned long long)gpu);
 return 0;
}
