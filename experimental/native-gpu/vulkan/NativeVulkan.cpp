// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>
#include "../RenderReport.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Direct-linked experimental Vulkan ABI. Dispatchable objects stay stable for
// their owning instance; no Linux DRM/GEM object is involved.
struct VkInstance_T;
struct VkPhysicalDevice_T { VkInstance_T* owner; uint32_t revision; };
struct VkInstance_T { VkPhysicalDevice_T physical; };
struct VkDevice_T;
struct VkShaderModule_T;
struct VkQueue_T { VkDevice_T* device; };
struct VkDevice_T { VkPhysicalDevice physical; int fd; VkQueue_T queue; VkShaderModule_T* shaders; };
struct VkShaderModule_T { VkDevice_T* device; VkShaderModule_T* next; size_t bytes; uint32_t* code; };

static bool ProbeN100(VkPhysicalDevice_T* physical)
{
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0)return false;
 native_gpu_render_command command={};command.version=1;
 command.mode=NATIVE_GPU_RENDER_PROBE;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 close(fd);
 const auto& report=command.report;
 if(status!=0 || report.status!=0 || report.stage!=6 || !report.known_profile
  || !report.owner_granted || report.identity!=0x46d18086 || report.revision!=0)
  return false;
 physical->revision=report.revision;return true;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceVersion(uint32_t* version)
{
 if(!version)return VK_ERROR_INITIALIZATION_FAILED;
 *version=VK_API_VERSION_1_0;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char* layer,uint32_t* count,
 VkExtensionProperties*)
{
 if(!count)return VK_ERROR_INITIALIZATION_FAILED;
 *count=0;return layer?VK_ERROR_LAYER_NOT_PRESENT:VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t* count,VkLayerProperties*)
{
 if(!count)return VK_ERROR_INITIALIZATION_FAILED;
 *count=0;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(const VkInstanceCreateInfo* info,const VkAllocationCallbacks* allocator,
 VkInstance* instance)
{
 if(instance)*instance=VK_NULL_HANDLE;
 if(!info || !instance || info->sType!=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
  || info->pNext || info->flags || allocator)return VK_ERROR_INITIALIZATION_FAILED;
 if(info->enabledLayerCount)return VK_ERROR_LAYER_NOT_PRESENT;
 if(info->enabledExtensionCount)return VK_ERROR_EXTENSION_NOT_PRESENT;
 if(info->pApplicationInfo &&
  (info->pApplicationInfo->sType!=VK_STRUCTURE_TYPE_APPLICATION_INFO
   || info->pApplicationInfo->pNext))return VK_ERROR_INITIALIZATION_FAILED;
 if(info->pApplicationInfo && info->pApplicationInfo->apiVersion>VK_API_VERSION_1_0)
  return VK_ERROR_INCOMPATIBLE_DRIVER;
 VkInstance_T* value=(VkInstance_T*)calloc(1,sizeof(VkInstance_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->physical.owner=value;*instance=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance,const VkAllocationCallbacks*)
{free(instance);}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkEnumeratePhysicalDevices(VkInstance instance,uint32_t* count,VkPhysicalDevice* devices)
{
 if(!instance || !count)return VK_ERROR_INITIALIZATION_FAILED;
 bool available=ProbeN100(&instance->physical);
 if(!devices){*count=available?1:0;return VK_SUCCESS;}
 uint32_t capacity=*count;*count=available && capacity?1:0;
 if(*count)devices[0]=&instance->physical;
 return available && !capacity?VK_INCOMPLETE:VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceProperties(VkPhysicalDevice physical,VkPhysicalDeviceProperties* properties)
{
 if(!properties)return;
 memset(properties,0,sizeof(*properties));
 if(!physical)return;
 properties->apiVersion=VK_API_VERSION_1_0;
 properties->driverVersion=VK_MAKE_API_VERSION(0,0,1,0);
 properties->vendorID=0x8086;properties->deviceID=0x46d1;
 properties->deviceType=VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
 strcpy(properties->deviceName,"RenkuOS Intel N100 experimental Vulkan");
 properties->limits.maxImageDimension2D=32;
 properties->limits.maxFramebufferWidth=32;
 properties->limits.maxFramebufferHeight=32;
 properties->limits.maxColorAttachments=1;
 properties->limits.maxMemoryAllocationCount=16;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceFeatures(VkPhysicalDevice,VkPhysicalDeviceFeatures* features)
{if(features)memset(features,0,sizeof(*features));}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physical,uint32_t* count,
 VkQueueFamilyProperties* families)
{
 if(!count)return;
 if(!physical){*count=0;return;}
 if(!families){*count=1;return;}
 if(*count){
  memset(families,0,sizeof(*families));
  families[0].queueFlags=VK_QUEUE_GRAPHICS_BIT;
  families[0].queueCount=1;
  families[0].minImageTransferGranularity={1,1,1};
  *count=1;
 }else *count=0;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physical,
 VkPhysicalDeviceMemoryProperties* memory)
{
 if(!memory)return;
 memset(memory,0,sizeof(*memory));
 if(!physical)return;
 memory->memoryHeapCount=1;
 memory->memoryHeaps[0].size=16*4096;
 memory->memoryHeaps[0].flags=VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
 memory->memoryTypeCount=1;
 memory->memoryTypes[0].heapIndex=0;
 memory->memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  |VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDevice(VkPhysicalDevice physical,const VkDeviceCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkDevice* device)
{
 if(device)*device=VK_NULL_HANDLE;
 if(!physical || !info || !device || info->sType!=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
  || info->pNext || allocator
  || info->queueCreateInfoCount!=1 || !info->pQueueCreateInfos)
  return VK_ERROR_INITIALIZATION_FAILED;
 if(info->enabledLayerCount)return VK_ERROR_LAYER_NOT_PRESENT;
 if(info->enabledExtensionCount)return VK_ERROR_EXTENSION_NOT_PRESENT;
 if(info->pEnabledFeatures){
  const VkBool32* fields=(const VkBool32*)info->pEnabledFeatures;
  for(unsigned i=0;i<sizeof(VkPhysicalDeviceFeatures)/sizeof(VkBool32);i++)
   if(fields[i])return VK_ERROR_FEATURE_NOT_PRESENT;
 }
 const VkDeviceQueueCreateInfo& queue=info->pQueueCreateInfos[0];
 if(queue.sType!=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO || queue.pNext
  || queue.flags || queue.queueFamilyIndex!=0 || queue.queueCount!=1
  || !queue.pQueuePriorities || queue.pQueuePriorities[0]<0
  || queue.pQueuePriorities[0]>1 || !ProbeN100(physical))
  return VK_ERROR_INITIALIZATION_FAILED;
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);
 if(fd<0)return VK_ERROR_INITIALIZATION_FAILED;
 VkDevice_T* value=(VkDevice_T*)calloc(1,sizeof(VkDevice_T));
 if(!value){close(fd);return VK_ERROR_OUT_OF_HOST_MEMORY;}
 value->physical=physical;value->fd=fd;value->queue.device=value;
 *device=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(VkDevice device,const VkAllocationCallbacks*)
{
 if(!device)return;
 while(device->shaders){
  VkShaderModule_T* next=device->shaders->next;
  free(device->shaders);device->shaders=next;
 }
 close(device->fd);free(device);
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetDeviceQueue(VkDevice device,uint32_t family,uint32_t index,VkQueue* queue)
{if(queue)*queue=device && family==0 && index==0?&device->queue:VK_NULL_HANDLE;}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkQueueWaitIdle(VkQueue queue)
{return queue && queue->device?VK_SUCCESS:VK_ERROR_DEVICE_LOST;}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkDeviceWaitIdle(VkDevice device)
{return device?VK_SUCCESS:VK_ERROR_DEVICE_LOST;}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateShaderModule(VkDevice device,const VkShaderModuleCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkShaderModule* shader)
{
 if(shader)*shader=VK_NULL_HANDLE;
 if(!device || !info || !shader || info->sType!=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
  || info->pNext || info->flags || allocator || !info->pCode || info->codeSize<20
  || info->codeSize>1024*1024 || (info->codeSize&3)
  || info->pCode[0]!=0x07230203)return VK_ERROR_INITIALIZATION_FAILED;
 VkShaderModule_T* value=(VkShaderModule_T*)malloc(sizeof(VkShaderModule_T)+info->codeSize);
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->shaders;value->bytes=info->codeSize;
 value->code=(uint32_t*)(value+1);
 memcpy(value->code,info->pCode,info->codeSize);
 device->shaders=value;*shader=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyShaderModule(VkDevice device,VkShaderModule shader,const VkAllocationCallbacks*)
{
 if(!device || !shader)return;
 VkShaderModule_T** current=&device->shaders;
 while(*current && *current!=shader)current=&(*current)->next;
 if(*current){*current=shader->next;free(shader);}
}

#define VK_PROC(name) if(!strcmp(pName,#name))return (PFN_vkVoidFunction)name
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetDeviceProcAddr(VkDevice device,const char* pName)
{
 if(!device || !pName)return nullptr;
 VK_PROC(vkGetDeviceProcAddr);
 VK_PROC(vkDestroyDevice);
 VK_PROC(vkGetDeviceQueue);
 VK_PROC(vkQueueWaitIdle);
 VK_PROC(vkDeviceWaitIdle);
 VK_PROC(vkCreateShaderModule);
 VK_PROC(vkDestroyShaderModule);
 return nullptr;
}

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance,const char* pName)
{
 if(!pName)return nullptr;
 VK_PROC(vkGetInstanceProcAddr);
 VK_PROC(vkCreateInstance);
 VK_PROC(vkEnumerateInstanceVersion);
 VK_PROC(vkEnumerateInstanceExtensionProperties);
 VK_PROC(vkEnumerateInstanceLayerProperties);
 if(!instance)return nullptr;
 VK_PROC(vkDestroyInstance);
 VK_PROC(vkEnumeratePhysicalDevices);
 VK_PROC(vkGetPhysicalDeviceProperties);
 VK_PROC(vkGetPhysicalDeviceFeatures);
 VK_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
 VK_PROC(vkGetPhysicalDeviceMemoryProperties);
 VK_PROC(vkCreateDevice);
 VK_PROC(vkGetDeviceProcAddr);
 return nullptr;
}
