// SPDX-License-Identifier: MIT
#include "BufferCopy.h"
#include "LifecycleReport.h"
#include "RenderReport.h"
#include <stdio.h>
constexpr uint32_t operations[]={NATIVE_GPU_EXECUTION_IOCTL,NATIVE_GPU_BUFFER_CREATE,
 NATIVE_GPU_BUFFER_WRITE,NATIVE_GPU_BUFFER_READ,NATIVE_GPU_BUFFER_COPY,
 NATIVE_GPU_LIFECYCLE_STATUS,NATIVE_GPU_LIFECYCLE_CONTROL,NATIVE_GPU_RENDER_IOCTL};
constexpr bool UniqueOperations()
{
 for(unsigned i=0;i<sizeof(operations)/sizeof(operations[0]);i++)
  for(unsigned j=0;j<i;j++)if(operations[i]==operations[j])return false;
 return true;
}
static_assert(UniqueOperations(),"native bridge ioctl operation collision");
int main(){puts("Native bridge protocol ids: PASS");}
