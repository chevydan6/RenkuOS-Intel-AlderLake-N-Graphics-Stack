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
 if(memory.memoryHeapCount!=1 || memory.memoryTypeCount!=1
  || !(memory.memoryTypes[0].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  || (memory.memoryTypes[0].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))return 10;
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
 VkBufferCreateInfo bufferInfo={};bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
 bufferInfo.size=48;bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkBuffer buffer=VK_NULL_HANDLE;
 if(vkCreateBuffer(device,&bufferInfo,nullptr,&buffer)!=VK_SUCCESS || !buffer)return 17;
 VkMemoryRequirements requirements={};vkGetBufferMemoryRequirements(device,buffer,&requirements);
 if(requirements.size!=4096 || requirements.alignment!=4096
  || requirements.memoryTypeBits!=1)return 18;
 VkMemoryAllocateInfo allocation={};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
 allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=0;
 VkDeviceMemory memoryObject=VK_NULL_HANDLE;
 if(vkAllocateMemory(device,&allocation,nullptr,&memoryObject)!=VK_SUCCESS
  || !memoryObject)return 19;
 if(vkBindBufferMemory(device,buffer,memoryObject,0)!=VK_SUCCESS)return 20;
 void* mapped=nullptr;
 if(vkMapMemory(device,memoryObject,0,48,0,&mapped)!=VK_SUCCESS || !mapped)return 21;
 const uint32_t vertices[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 memcpy(mapped,vertices,sizeof(vertices));
 VkMappedMemoryRange range={};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
 range.memory=memoryObject;range.offset=0;range.size=sizeof(vertices);
 if(vkFlushMappedMemoryRanges(device,1,&range)!=VK_SUCCESS)return 22;
 memset(mapped,0,sizeof(vertices));
 if(vkInvalidateMappedMemoryRanges(device,1,&range)!=VK_SUCCESS
  || memcmp(mapped,vertices,sizeof(vertices)))return 23;
 vkUnmapMemory(device,memoryObject);
 VkBuffer unbound=VK_NULL_HANDLE;
 if(vkCreateBuffer(device,&bufferInfo,nullptr,&unbound)!=VK_SUCCESS)return 24;
 allocation.allocationSize=1;
 VkDeviceMemory tiny=VK_NULL_HANDLE;
 if(vkAllocateMemory(device,&allocation,nullptr,&tiny)!=VK_SUCCESS)return 25;
 void* invalidMap=nullptr;
 if(vkBindBufferMemory(device,unbound,tiny,0)==VK_SUCCESS
  || vkMapMemory(device,tiny,1,1,0,&invalidMap)==VK_SUCCESS)return 26;
 vkDestroyBuffer(device,unbound,nullptr);
 vkFreeMemory(device,tiny,nullptr);
 vkDestroyBuffer(device,buffer,nullptr);
 vkFreeMemory(device,memoryObject,nullptr);
 VkImageCreateInfo imageInfo={};imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
 imageInfo.imageType=VK_IMAGE_TYPE_2D;imageInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 imageInfo.extent={32,32,1};imageInfo.mipLevels=1;imageInfo.arrayLayers=1;
 imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;imageInfo.tiling=VK_IMAGE_TILING_LINEAR;
 imageInfo.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
 imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkImage image=VK_NULL_HANDLE;
 if(vkCreateImage(device,&imageInfo,nullptr,&image)!=VK_SUCCESS || !image)return 27;
 VkMemoryRequirements imageRequirements={};
 vkGetImageMemoryRequirements(device,image,&imageRequirements);
 if(imageRequirements.size!=4096 || imageRequirements.alignment!=4096
  || imageRequirements.memoryTypeBits!=1)return 28;
 allocation.allocationSize=imageRequirements.size;
 VkDeviceMemory imageMemory=VK_NULL_HANDLE;
 if(vkAllocateMemory(device,&allocation,nullptr,&imageMemory)!=VK_SUCCESS
  || vkBindImageMemory(device,image,imageMemory,0)!=VK_SUCCESS)return 29;
 imageInfo.extent.width=33;
 VkImage unsupportedImage=VK_NULL_HANDLE;
 if(vkCreateImage(device,&imageInfo,nullptr,&unsupportedImage)==VK_SUCCESS
  || unsupportedImage)return 30;
 vkDestroyImage(device,image,nullptr);
 vkFreeMemory(device,imageMemory,nullptr);
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
