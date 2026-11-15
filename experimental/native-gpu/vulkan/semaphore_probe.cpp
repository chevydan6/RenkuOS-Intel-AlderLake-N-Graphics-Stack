// SPDX-License-Identifier: MIT
// Exercise binary queue dependencies without requiring an N100 on the VM.
#include "NativeVulkan.cpp"
#include <stdio.h>

int main()
{
 VkDevice_T device={};device.fd=-1;device.queue.device=&device;
 VkDevice_T other={};other.fd=-1;other.queue.device=&other;
 auto create=(PFN_vkCreateSemaphore)vkGetDeviceProcAddr(&device,"vkCreateSemaphore");
 auto destroy=(PFN_vkDestroySemaphore)vkGetDeviceProcAddr(&device,"vkDestroySemaphore");
 if(!create || !destroy)return 1;
 VkSemaphoreCreateInfo info={};info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
 VkSemaphore semaphore=VK_NULL_HANDLE;
 if(create(&device,&info,nullptr,&semaphore)!=VK_SUCCESS || !semaphore)return 2;
 VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
 VkSubmitInfo signal={};signal.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
 signal.signalSemaphoreCount=1;signal.pSignalSemaphores=&semaphore;
 VkSubmitInfo wait={};wait.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
 wait.waitSemaphoreCount=1;wait.pWaitSemaphores=&semaphore;
 wait.pWaitDstStageMask=&stage;
 if(vkQueueSubmit(&device.queue,1,&wait,VK_NULL_HANDLE)!=VK_ERROR_INITIALIZATION_FAILED)
  return 3;
 VkSubmitInfo pair[2]={signal,wait};
 if(vkQueueSubmit(&device.queue,2,pair,VK_NULL_HANDLE)!=VK_SUCCESS)return 4;
 if(vkQueueSubmit(&device.queue,1,&wait,VK_NULL_HANDLE)!=VK_ERROR_INITIALIZATION_FAILED)
  return 5;
 pair[1]=signal;
 if(vkQueueSubmit(&device.queue,2,pair,VK_NULL_HANDLE)!=VK_ERROR_INITIALIZATION_FAILED)
  return 6;
 if(vkQueueSubmit(&device.queue,1,&signal,VK_NULL_HANDLE)!=VK_SUCCESS)return 7;
 if(vkQueueSubmit(&other.queue,1,&wait,VK_NULL_HANDLE)!=VK_ERROR_INITIALIZATION_FAILED)
  return 8;
 if(vkQueueSubmit(&device.queue,1,&wait,VK_NULL_HANDLE)!=VK_SUCCESS)return 9;
 VkSubmitInfo invalid=wait;invalid.pWaitDstStageMask=nullptr;
 if(vkQueueSubmit(&device.queue,1,&invalid,VK_NULL_HANDLE)!=VK_ERROR_FEATURE_NOT_PRESENT)
  return 10;
 info.flags=1;VkSemaphore rejected=VK_NULL_HANDLE;
 if(create(&device,&info,nullptr,&rejected)!=VK_ERROR_INITIALIZATION_FAILED
  || rejected)return 11;
 destroy(&device,semaphore,nullptr);
 puts("Vulkan binary semaphore probe: signal/wait order, preflight, ownership: PASS");
 return 0;
}
