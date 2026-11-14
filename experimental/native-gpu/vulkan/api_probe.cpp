// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>
#include "KnownSpirv.h"
#include <stdio.h>
#include <string.h>

int main()
{
 uint32_t version=0,extensions=1;
 if(vkEnumerateInstanceVersion(&version)!=VK_SUCCESS || version!=VK_API_VERSION_1_0)
  return 1;
 if(vkEnumerateInstanceExtensionProperties(nullptr,&extensions,nullptr)!=VK_SUCCESS
  || extensions!=0)return 2;
 VkApplicationInfo app={};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
 app.apiVersion=VK_API_VERSION_1_0;
 VkInstanceCreateInfo create={};create.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
 create.pApplicationInfo=&app;
 VkInstance instance=VK_NULL_HANDLE;
 if(vkCreateInstance(&create,nullptr,&instance)!=VK_SUCCESS || !instance)return 3;
 if(!vkGetInstanceProcAddr(nullptr,"vkCreateInstance")
  || !vkGetInstanceProcAddr(instance,"vkEnumeratePhysicalDevices")
  || vkGetInstanceProcAddr(instance,"vkImaginaryFunction"))return 4;
 uint32_t count=0;
 if(vkEnumeratePhysicalDevices(instance,&count,nullptr)!=VK_SUCCESS || count!=1)return 5;
 VkPhysicalDevice physical=VK_NULL_HANDLE;
 if(vkEnumeratePhysicalDevices(instance,&count,&physical)!=VK_SUCCESS
  || count!=1 || !physical)return 6;
 VkPhysicalDeviceProperties properties={};
 vkGetPhysicalDeviceProperties(physical,&properties);
 if(properties.vendorID!=0x8086 || properties.deviceID!=0x46d1
  || properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
  || properties.apiVersion!=VK_API_VERSION_1_0)return 7;
 uint32_t queues=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&queues,nullptr);
 if(queues!=1)return 8;
 VkQueueFamilyProperties family={};
 vkGetPhysicalDeviceQueueFamilyProperties(physical,&queues,&family);
 if(queues!=1 || family.queueCount!=1 || !(family.queueFlags&VK_QUEUE_GRAPHICS_BIT))return 9;
 VkPhysicalDeviceFeatures features={};vkGetPhysicalDeviceFeatures(physical,&features);
 VkPhysicalDeviceMemoryProperties memory={};vkGetPhysicalDeviceMemoryProperties(physical,&memory);
 if(memory.memoryHeapCount!=1 || memory.memoryTypeCount!=1)return 10;
 float priority=1.0f;
 VkDeviceQueueCreateInfo queueInfo={};queueInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
 queueInfo.queueFamilyIndex=0;queueInfo.queueCount=1;queueInfo.pQueuePriorities=&priority;
 VkDeviceCreateInfo deviceInfo={};deviceInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
 deviceInfo.queueCreateInfoCount=1;deviceInfo.pQueueCreateInfos=&queueInfo;
 VkDevice device=VK_NULL_HANDLE;
 if(vkCreateDevice(physical,&deviceInfo,nullptr,&device)!=VK_SUCCESS || !device)return 12;
 VkQueue queue=VK_NULL_HANDLE;vkGetDeviceQueue(device,0,0,&queue);
 if(!queue || vkQueueWaitIdle(queue)!=VK_SUCCESS || vkDeviceWaitIdle(device)!=VK_SUCCESS
  || !vkGetDeviceProcAddr(device,"vkQueueWaitIdle"))return 13;
 VkShaderModuleCreateInfo shaderInfo={};
 shaderInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 shaderInfo.pCode=RenkuKnownSpirv::Vertex;
 shaderInfo.codeSize=sizeof(RenkuKnownSpirv::Vertex);
 VkShaderModule vs=VK_NULL_HANDLE,fs=VK_NULL_HANDLE;
 if(vkCreateShaderModule(device,&shaderInfo,nullptr,&vs)!=VK_SUCCESS || !vs)return 14;
 shaderInfo.pCode=RenkuKnownSpirv::Fragment;
 shaderInfo.codeSize=sizeof(RenkuKnownSpirv::Fragment);
 if(vkCreateShaderModule(device,&shaderInfo,nullptr,&fs)!=VK_SUCCESS || !fs)return 15;
 vkDestroyShaderModule(device,fs,nullptr);
 vkDestroyShaderModule(device,vs,nullptr);
 vkDestroyDevice(device,nullptr);
 VkPhysicalDeviceFeatures requested={};requested.geometryShader=VK_TRUE;
 deviceInfo.pEnabledFeatures=&requested;
 VkDevice unsupportedDevice=VK_NULL_HANDLE;
 VkResult unsupportedResult=vkCreateDevice(physical,&deviceInfo,nullptr,&unsupportedDevice);
 if(unsupportedDevice)vkDestroyDevice(unsupportedDevice,nullptr);
 if(unsupportedResult!=VK_ERROR_FEATURE_NOT_PRESENT || unsupportedDevice)return 16;
 printf("Native Vulkan N100 probe: PASS vendor=%04x device=%04x api=%u.%u memory=%llu\n",
  properties.vendorID,properties.deviceID,VK_API_VERSION_MAJOR(version),
  VK_API_VERSION_MINOR(version),(unsigned long long)memory.memoryHeaps[0].size);
 vkDestroyInstance(instance,nullptr);
 create.enabledExtensionCount=1;const char* unsupported="VK_EXT_unavailable";
 create.ppEnabledExtensionNames=&unsupported;
 if(vkCreateInstance(&create,nullptr,&instance)!=VK_ERROR_EXTENSION_NOT_PRESENT)return 11;
 return 0;
}
