// SPDX-License-Identifier: MIT
// Exercise host fence state on a VM without a GPU owner.
#include "NativeVulkan.cpp"
#include <stdio.h>

int main()
{
 VkDevice_T device={};device.fd=-1;device.queue.device=&device;
 auto create=(PFN_vkCreateFence)vkGetDeviceProcAddr(&device,"vkCreateFence");
 auto destroy=(PFN_vkDestroyFence)vkGetDeviceProcAddr(&device,"vkDestroyFence");
 auto status=(PFN_vkGetFenceStatus)vkGetDeviceProcAddr(&device,"vkGetFenceStatus");
 auto wait=(PFN_vkWaitForFences)vkGetDeviceProcAddr(&device,"vkWaitForFences");
 auto reset=(PFN_vkResetFences)vkGetDeviceProcAddr(&device,"vkResetFences");
 if(!create || !destroy || !status || !wait || !reset)return 1;
 VkFenceCreateInfo info={};info.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
 VkFence fence=VK_NULL_HANDLE;
 if(create(&device,&info,nullptr,&fence)!=VK_SUCCESS || !fence)return 2;
 if(status(&device,fence)!=VK_NOT_READY || wait(&device,1,&fence,VK_TRUE,0)!=VK_TIMEOUT)
  return 3;
 if(vkQueueSubmit(&device.queue,0,nullptr,fence)!=VK_SUCCESS)return 4;
 if(status(&device,fence)!=VK_SUCCESS || wait(&device,1,&fence,VK_TRUE,0)!=VK_SUCCESS)
  return 5;
 if(reset(&device,1,&fence)!=VK_SUCCESS || status(&device,fence)!=VK_NOT_READY)
  return 6;
 VkFence signaled=VK_NULL_HANDLE;
 info.flags=VK_FENCE_CREATE_SIGNALED_BIT;
 if(create(&device,&info,nullptr,&signaled)!=VK_SUCCESS || !signaled
  || status(&device,signaled)!=VK_SUCCESS)return 7;
 VkFence both[2]={fence,signaled};
 if(wait(&device,2,both,VK_FALSE,0)!=VK_SUCCESS
  || wait(&device,2,both,VK_TRUE,0)!=VK_TIMEOUT)return 8;
 const bigtime_t started=system_time();
 if(wait(&device,1,&fence,VK_TRUE,2000000)!=VK_TIMEOUT
  || system_time()-started<1000)return 9;
 destroy(&device,fence,nullptr);destroy(&device,signaled,nullptr);
 puts("Vulkan fence probe: timed wait; empty submit signal; reset; signaled creation; wait-any/all");
 return 0;
}
