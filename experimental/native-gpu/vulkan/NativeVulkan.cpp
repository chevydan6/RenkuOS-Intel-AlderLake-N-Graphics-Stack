// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>
#include "../RenderReport.h"
#include "../ResourceCommand.h"
#include "KnownSpirv.h"
#include "../render/draw/RcsDrawGenerated.h"
#include "../render/RcsResourceRange.h"
#include <OS.h>
#ifdef RENKU_VULKAN_USE_INTEL_COMPILER
#include "CompiledKnownShader.h"
#endif
#include <fcntl.h>
#include <stdio.h>
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
struct VkDeviceMemory_T;
struct VkBuffer_T;
struct VkImage_T;
struct VkImageView_T;
struct VkRenderPass_T;
struct VkFramebuffer_T;
struct VkPipelineLayout_T;
struct VkPipeline_T;
struct VkDescriptorSetLayout_T;
struct VkDescriptorPool_T;
struct VkDescriptorSet_T;
struct VkFence_T;
struct VkSemaphore_T;
struct VkCommandPool_T;
struct VkQueue_T { VkDevice_T* device; };
struct VkDevice_T {
 VkPhysicalDevice physical;int fd;VkQueue_T queue;bool trace;
 VkShaderModule_T* shaders;VkDeviceMemory_T* memory;
 VkBuffer_T* buffers;VkImage_T* images;VkImageView_T* views;
 VkRenderPass_T* passes;VkFramebuffer_T* framebuffers;
 VkPipelineLayout_T* layouts;VkPipeline_T* pipelines;VkCommandPool_T* pools;
 VkDescriptorSetLayout_T* setLayouts;VkDescriptorPool_T* descriptorPools;
 VkFence_T* fences;VkSemaphore_T* semaphores;
};
struct VkShaderModule_T { VkDevice_T* device; VkShaderModule_T* next; size_t bytes; uint32_t* code; };
struct VkDeviceMemory_T {
 VkDevice_T* device;VkDeviceMemory_T* next;
 uint32_t handle;uint64_t gpu;VkDeviceSize bytes;uint8_t* host;bool mapped;
};
struct VkBuffer_T {
 VkDevice_T* device;VkBuffer_T* next;VkDeviceSize bytes;VkBufferUsageFlags usage;
 VkDeviceMemory memory;uint32_t memoryOffset;
};
struct VkImage_T {
 VkDevice_T* device;VkImage_T* next;VkDeviceMemory memory;
 uint32_t width,memoryOffset;VkImageUsageFlags usage;
};
struct VkImageView_T { VkDevice_T* device;VkImageView_T* next;VkImage image; };
struct VkRenderPass_T { VkDevice_T* device;VkRenderPass_T* next;bool load,clear; };
struct VkFramebuffer_T {
 VkDevice_T* device;VkFramebuffer_T* next;
 VkRenderPass pass;VkImageView view;
};
struct VkDescriptorSetLayout_T {VkDevice_T* device;VkDescriptorSetLayout_T* next;};
struct VkDescriptorPool_T {
 VkDevice_T* device;VkDescriptorPool_T* next;VkDescriptorSet_T* sets;
 uint32_t maxSets,used;
};
struct VkDescriptorSet_T {
 VkDevice_T* device;VkDescriptorPool_T* pool;VkDescriptorSet_T* next;
 VkDescriptorSetLayout layout;VkBuffer buffer;uint32_t bufferOffset;bool valid;
};
struct VkPipelineLayout_T {
 VkDevice_T* device;VkPipelineLayout_T* next;VkDescriptorSetLayout setLayout;
};
struct VkPipeline_T {
 VkDevice_T* device;VkPipeline_T* next;VkRenderPass pass;
 VkPipelineLayout layout;
 uint32_t shaderHandle,vertexShaderHandle,width;uint64_t shaderGpu;
 bool fragcoordGradient,uniformBuffer;
};
struct VkFence_T { VkDevice_T* device;VkFence_T* next;bool signaled; };
struct VkSemaphore_T { VkDevice_T* device;VkSemaphore_T* next;bool signaled; };
struct VkCommandBuffer_T;
struct VkCommandPool_T {
 VkDevice_T* device;VkCommandPool_T* next;VkCommandBuffer_T* commands;
 VkCommandPoolCreateFlags flags;
};
struct VkCommandBuffer_T {
 struct Draw {VkFramebuffer framebuffer;VkPipeline pipeline;VkBuffer vertex;
  VkDescriptorSet descriptor;
  uint32_t vertexOffset;int32_t vertexBias;bool preserve;
  VkBuffer index;uint32_t indexOffset;bool indexed;uint32_t indexCount;
  uint32_t vertexCount;
  bool clear;uint32_t clearColor;};
 struct Copy {VkDeviceMemory source,destination;
  VkDeviceSize sourceOffset,destinationOffset,size;
  bool sourceImage,destinationImage;};
 struct Event {bool copy;size_t index;};
 VkDevice_T* device;VkCommandPool_T* pool;VkCommandBuffer_T* next;
 VkRenderPass pass;VkFramebuffer framebuffer;VkPipeline pipeline;
 VkBuffer boundVertex;VkDeviceSize boundVertexOffset;
 VkBuffer boundIndex;VkDeviceSize boundIndexOffset;
 VkDescriptorSet boundDescriptor;
 Draw* draws;size_t drawCount,drawCapacity;
 Copy* copies;size_t copyCount,copyCapacity;
 Event* events;size_t eventCount,eventCapacity;
 uint32_t clearColor;
 bool recording;bool inRenderPass;bool drawn;bool complete;bool invalid;
};

static bool AppendEvent(VkCommandBuffer command,bool copy,size_t index)
{
 if(command->eventCount==command->eventCapacity){
  size_t next=command->eventCapacity?command->eventCapacity*2:4;
  if(next<command->eventCapacity || next>SIZE_MAX/sizeof(VkCommandBuffer_T::Event))
   return false;
  void* storage=realloc(command->events,next*sizeof(VkCommandBuffer_T::Event));
  if(!storage)return false;
  command->events=(VkCommandBuffer_T::Event*)storage;
  command->eventCapacity=next;
 }
 command->events[command->eventCount++]={copy,index};return true;
}

static bool AppendCopy(VkCommandBuffer command,const VkCommandBuffer_T::Copy& copy)
{
 if(command->copyCount==command->copyCapacity){
  size_t next=command->copyCapacity?command->copyCapacity*2:4;
  if(next<command->copyCapacity || next>SIZE_MAX/sizeof(VkCommandBuffer_T::Copy))
   return false;
  void* storage=realloc(command->copies,next*sizeof(VkCommandBuffer_T::Copy));
  if(!storage)return false;
  command->copies=(VkCommandBuffer_T::Copy*)storage;
  command->copyCapacity=next;
 }
 size_t index=command->copyCount++;
 command->copies[index]=copy;
 return AppendEvent(command,true,index);
}

static bool DisjointCopy(VkDeviceMemory source,VkDeviceSize sourceOffset,
 VkDeviceMemory destination,VkDeviceSize destinationOffset,VkDeviceSize size)
{
 return source!=destination || sourceOffset>=destinationOffset+size
  || destinationOffset>=sourceOffset+size;
}

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

static bool EnsureBootstrap(int fd)
{
 native_gpu_render_command command={};command.version=1;
 command.mode=NATIVE_GPU_RENDER_BOOTSTRAP;
 int status=ioctl(fd,NATIVE_GPU_RENDER_IOCTL,&command,sizeof(command));
 return (status==0 && command.report.status==0
  && command.report.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE)
  || command.report.refusal==NATIVE_GPU_RENDER_ALREADY_BOOTSTRAPPED;
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
 properties->limits.maxImageDimension2D=64;
 properties->limits.maxFramebufferWidth=64;
 properties->limits.maxFramebufferHeight=64;
 properties->limits.maxColorAttachments=1;
 properties->limits.maxDrawIndexedIndexValue=255;
 properties->limits.maxVertexInputAttributes=1;
 properties->limits.maxVertexInputBindings=1;
 properties->limits.maxVertexInputBindingStride=16;
 properties->limits.maxMemoryAllocationCount=16;
 properties->limits.maxBoundDescriptorSets=1;
 properties->limits.maxPerStageDescriptorUniformBuffers=1;
 properties->limits.maxDescriptorSetUniformBuffers=1;
 properties->limits.maxUniformBufferRange=16;
 properties->limits.minUniformBufferOffsetAlignment=16;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceFeatures(VkPhysicalDevice,VkPhysicalDeviceFeatures* features)
{if(features)memset(features,0,sizeof(*features));}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physical,VkFormat format,
 VkFormatProperties* properties)
{
 if(!properties)return;
 memset(properties,0,sizeof(*properties));
 if(physical && format==VK_FORMAT_B8G8R8A8_UNORM)
  properties->linearTilingFeatures=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
   |VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
 if(physical && format==VK_FORMAT_R32G32B32A32_SFLOAT)
  properties->bufferFeatures=VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physical,VkFormat format,
 VkImageType type,VkImageTiling tiling,VkImageUsageFlags usage,
 VkImageCreateFlags flags,VkImageFormatProperties* properties)
{
 if(!properties)return VK_ERROR_INITIALIZATION_FAILED;
 memset(properties,0,sizeof(*properties));
 if(!physical || format!=VK_FORMAT_B8G8R8A8_UNORM
  || type!=VK_IMAGE_TYPE_2D || tiling!=VK_IMAGE_TILING_LINEAR
  || !(usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
  || (usage&~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT
   |VK_IMAGE_USAGE_TRANSFER_DST_BIT))
  || flags)return VK_ERROR_FORMAT_NOT_SUPPORTED;
 properties->maxExtent={64,64,1};
 properties->maxMipLevels=1;properties->maxArrayLayers=1;
 properties->sampleCounts=VK_SAMPLE_COUNT_1_BIT;
 properties->maxResourceSize=16384;
 return VK_SUCCESS;
}

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
 memory->memoryHeaps[0].size=NativeGpu::RcsResourceRange::Pages*4096;
 memory->memoryHeaps[0].flags=VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
 memory->memoryTypeCount=1;
 memory->memoryTypes[0].heapIndex=0;
 memory->memoryTypes[0].propertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  |VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
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
 if(!EnsureBootstrap(fd)){close(fd);return VK_ERROR_INITIALIZATION_FAILED;}
 VkDevice_T* value=(VkDevice_T*)calloc(1,sizeof(VkDevice_T));
 if(!value){close(fd);return VK_ERROR_OUT_OF_HOST_MEMORY;}
 value->physical=physical;value->fd=fd;value->queue.device=value;
 value->trace=getenv("RENKU_VK_TRACE")!=nullptr;
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
 while(device->buffers){
  VkBuffer_T* next=device->buffers->next;
  free(device->buffers);device->buffers=next;
 }
 while(device->images){
  VkImage_T* next=device->images->next;
  free(device->images);device->images=next;
 }
 while(device->views){
  VkImageView_T* next=device->views->next;
  free(device->views);device->views=next;
 }
 while(device->passes){
  VkRenderPass_T* next=device->passes->next;
  free(device->passes);device->passes=next;
 }
 while(device->framebuffers){
  VkFramebuffer_T* next=device->framebuffers->next;
  free(device->framebuffers);device->framebuffers=next;
 }
 while(device->pipelines){
  VkPipeline_T* next=device->pipelines->next;
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=device->pipelines->shaderHandle;
  ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c));
  if(device->pipelines->vertexShaderHandle){
   c.handle=device->pipelines->vertexShaderHandle;
   ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c));
  }
  free(device->pipelines);device->pipelines=next;
 }
 while(device->layouts){
  VkPipelineLayout_T* next=device->layouts->next;
  free(device->layouts);device->layouts=next;
 }
 while(device->descriptorPools){
  VkDescriptorPool_T* next=device->descriptorPools->next;
  while(device->descriptorPools->sets){
   VkDescriptorSet_T* set=device->descriptorPools->sets;
   device->descriptorPools->sets=set->next;free(set);
  }
  free(device->descriptorPools);device->descriptorPools=next;
 }
 while(device->setLayouts){
  VkDescriptorSetLayout_T* next=device->setLayouts->next;
  free(device->setLayouts);device->setLayouts=next;
 }
 while(device->pools){
  VkCommandPool_T* next=device->pools->next;
  while(device->pools->commands){
   VkCommandBuffer_T* command=device->pools->commands;
   device->pools->commands=command->next;free(command->draws);free(command);
  }
  free(device->pools);device->pools=next;
 }
 while(device->memory){
  VkDeviceMemory_T* next=device->memory->next;
  native_gpu_resource_command c={};c.version=1;
  c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=device->memory->handle;
  ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c));
  free(device->memory->host);free(device->memory);device->memory=next;
 }
 while(device->fences){
  VkFence_T* next=device->fences->next;
  free(device->fences);device->fences=next;
 }
 while(device->semaphores){
  VkSemaphore_T* next=device->semaphores->next;
  free(device->semaphores);device->semaphores=next;
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
vkCreateSemaphore(VkDevice device,const VkSemaphoreCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkSemaphore* semaphore)
{
 if(semaphore)*semaphore=VK_NULL_HANDLE;
 if(!device || !info || !semaphore || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
  return VK_ERROR_INITIALIZATION_FAILED;
 VkSemaphore_T* value=(VkSemaphore_T*)calloc(1,sizeof(VkSemaphore_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->semaphores;
 device->semaphores=value;*semaphore=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroySemaphore(VkDevice device,VkSemaphore semaphore,const VkAllocationCallbacks*)
{
 if(!device || !semaphore)return;
 VkSemaphore_T** current=&device->semaphores;
 while(*current && *current!=semaphore)current=&(*current)->next;
 if(*current){*current=semaphore->next;free(semaphore);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFence(VkDevice device,const VkFenceCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkFence* fence)
{
 if(fence)*fence=VK_NULL_HANDLE;
 if(!device || !info || !fence || allocator || info->pNext
  || info->sType!=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
  || (info->flags&~VK_FENCE_CREATE_SIGNALED_BIT))return VK_ERROR_INITIALIZATION_FAILED;
 VkFence_T* value=(VkFence_T*)calloc(1,sizeof(VkFence_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->signaled=bool(info->flags&VK_FENCE_CREATE_SIGNALED_BIT);
 value->next=device->fences;device->fences=value;*fence=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyFence(VkDevice device,VkFence fence,const VkAllocationCallbacks*)
{
 if(!device || !fence)return;
 VkFence_T** current=&device->fences;
 while(*current && *current!=fence)current=&(*current)->next;
 if(*current){*current=fence->next;free(fence);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkGetFenceStatus(VkDevice device,VkFence fence)
{
 if(!device || !fence || fence->device!=device)return VK_ERROR_INITIALIZATION_FAILED;
 return __atomic_load_n(&fence->signaled,__ATOMIC_ACQUIRE)?VK_SUCCESS:VK_NOT_READY;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkResetFences(VkDevice device,uint32_t count,const VkFence* fences)
{
 if(!device || !count || !fences)return VK_ERROR_INITIALIZATION_FAILED;
 for(uint32_t i=0;i<count;i++)
  if(!fences[i] || fences[i]->device!=device)return VK_ERROR_INITIALIZATION_FAILED;
 for(uint32_t i=0;i<count;i++)
  __atomic_store_n(&fences[i]->signaled,false,__ATOMIC_RELEASE);
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkWaitForFences(VkDevice device,uint32_t count,const VkFence* fences,
 VkBool32 waitAll,uint64_t timeout)
{
 if(!device || !count || !fences)return VK_ERROR_INITIALIZATION_FAILED;
 for(uint32_t i=0;i<count;i++)
  if(!fences[i] || fences[i]->device!=device)return VK_ERROR_INITIALIZATION_FAILED;
 const bigtime_t start=system_time();
 for(;;){
  uint32_t signaled=0;
  for(uint32_t i=0;i<count;i++)
   signaled+=__atomic_load_n(&fences[i]->signaled,__ATOMIC_ACQUIRE)?1:0;
  if(waitAll?signaled==count:signaled!=0)return VK_SUCCESS;
  if(!timeout)return VK_TIMEOUT;
  const uint64_t elapsed=uint64_t(system_time()-start)*1000;
  if(timeout!=UINT64_MAX && elapsed>=timeout)return VK_TIMEOUT;
  const uint64_t remaining=timeout==UINT64_MAX?1000000:timeout-elapsed;
  bigtime_t sleepUs=bigtime_t(remaining/1000+(remaining%1000!=0));
  if(sleepUs>1000)sleepUs=1000;
  snooze(sleepUs);
 }
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateShaderModule(VkDevice device,const VkShaderModuleCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkShaderModule* shader)
{
 if(shader)*shader=VK_NULL_HANDLE;
 if(!device || !info || !shader || info->sType!=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
  || info->pNext || info->flags || allocator || !info->pCode || info->codeSize<20
  || info->codeSize>1024*1024 || (info->codeSize&3)
  || info->pCode[0]!=0x07230203)return VK_ERROR_INITIALIZATION_FAILED;
#ifdef RENKU_VULKAN_USE_INTEL_COMPILER
 char validationError[256]={};
 int validation=renku_intel_validate_vulkan10(info->pCode,info->codeSize/4,
  validationError,sizeof(validationError));
 if(validation==RENKU_INTEL_COMPILE_OUT_OF_MEMORY)return VK_ERROR_OUT_OF_HOST_MEMORY;
 if(validation!=RENKU_INTEL_COMPILE_OK)return VK_ERROR_INITIALIZATION_FAILED;
#endif
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

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateBuffer(VkDevice device,const VkBufferCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkBuffer* buffer)
{
 if(buffer)*buffer=VK_NULL_HANDLE;
 if(!device || !info || !buffer || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
  || info->sharingMode!=VK_SHARING_MODE_EXCLUSIVE || !info->size
  || info->size>NativeGpu::RcsResourceRange::Pages*4096
  || !info->usage
  || (info->usage&~(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
   |VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
   |VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT)))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkBuffer_T* value=(VkBuffer_T*)calloc(1,sizeof(VkBuffer_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->bytes=info->size;value->usage=info->usage;
 value->next=device->buffers;device->buffers=value;*buffer=value;
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyBuffer(VkDevice device,VkBuffer buffer,const VkAllocationCallbacks*)
{
 if(!device || !buffer)return;
 VkBuffer_T** current=&device->buffers;
 while(*current && *current!=buffer)current=&(*current)->next;
 if(*current){*current=buffer->next;free(buffer);}
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetBufferMemoryRequirements(VkDevice device,VkBuffer buffer,
 VkMemoryRequirements* requirements)
{
 if(!requirements)return;
 memset(requirements,0,sizeof(*requirements));
 if(!device || !buffer || buffer->device!=device)return;
 requirements->size=(buffer->bytes+4095)&~VkDeviceSize(4095);
 requirements->alignment=4096;requirements->memoryTypeBits=1;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImage(VkDevice device,const VkImageCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkImage* image)
{
 if(image)*image=VK_NULL_HANDLE;
 if(!device || !info || !image || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
  || info->imageType!=VK_IMAGE_TYPE_2D || info->format!=VK_FORMAT_B8G8R8A8_UNORM
  || (info->extent.width!=32 && info->extent.width!=64)
  || info->extent.height!=info->extent.width || info->extent.depth!=1
  || info->mipLevels!=1 || info->arrayLayers!=1
  || info->samples!=VK_SAMPLE_COUNT_1_BIT || info->tiling!=VK_IMAGE_TILING_LINEAR
  || !(info->usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
  || (info->usage&~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
   |VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT))
  || info->sharingMode!=VK_SHARING_MODE_EXCLUSIVE
  || info->initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED)
  return VK_ERROR_FORMAT_NOT_SUPPORTED;
 VkImage_T* value=(VkImage_T*)calloc(1,sizeof(VkImage_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->width=info->extent.width;value->usage=info->usage;
 value->next=device->images;device->images=value;
 *image=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyImage(VkDevice device,VkImage image,const VkAllocationCallbacks*)
{
 if(!device || !image)return;
 VkImage_T** current=&device->images;
 while(*current && *current!=image)current=&(*current)->next;
 if(*current){*current=image->next;free(image);}
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkGetImageMemoryRequirements(VkDevice device,VkImage image,
 VkMemoryRequirements* requirements)
{
 if(!requirements)return;
 memset(requirements,0,sizeof(*requirements));
 if(!device || !image || image->device!=device)return;
 requirements->size=VkDeviceSize(image->width)*image->width*4;
 requirements->alignment=4096;
 requirements->memoryTypeBits=1;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateImageView(VkDevice device,const VkImageViewCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkImageView* view)
{
 if(view)*view=VK_NULL_HANDLE;
 if(!device || !info || !view || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
  || !info->image || info->image->device!=device
  || info->viewType!=VK_IMAGE_VIEW_TYPE_2D
  || info->format!=VK_FORMAT_B8G8R8A8_UNORM
  || info->components.r || info->components.g || info->components.b
  || info->components.a
  || info->subresourceRange.aspectMask!=VK_IMAGE_ASPECT_COLOR_BIT
  || info->subresourceRange.baseMipLevel || info->subresourceRange.levelCount!=1
  || info->subresourceRange.baseArrayLayer || info->subresourceRange.layerCount!=1)
  return VK_ERROR_FORMAT_NOT_SUPPORTED;
 VkImageView_T* value=(VkImageView_T*)calloc(1,sizeof(VkImageView_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->image=info->image;
 value->next=device->views;device->views=value;*view=value;
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyImageView(VkDevice device,VkImageView view,const VkAllocationCallbacks*)
{
 if(!device || !view)return;
 VkImageView_T** current=&device->views;
 while(*current && *current!=view)current=&(*current)->next;
 if(*current){*current=view->next;free(view);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRenderPass(VkDevice device,const VkRenderPassCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkRenderPass* pass)
{
 if(pass)*pass=VK_NULL_HANDLE;
 if(!device || !info || !pass || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
  || info->attachmentCount!=1 || !info->pAttachments
  || info->subpassCount!=1 || !info->pSubpasses || info->dependencyCount)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 const VkAttachmentDescription& a=info->pAttachments[0];
 const VkSubpassDescription& s=info->pSubpasses[0];
 if(a.flags || a.format!=VK_FORMAT_B8G8R8A8_UNORM
  || a.samples!=VK_SAMPLE_COUNT_1_BIT
  || !((a.loadOp==VK_ATTACHMENT_LOAD_OP_DONT_CARE
   && a.initialLayout==VK_IMAGE_LAYOUT_UNDEFINED)
   || (a.loadOp==VK_ATTACHMENT_LOAD_OP_CLEAR
   && (a.initialLayout==VK_IMAGE_LAYOUT_UNDEFINED
    || a.initialLayout==VK_IMAGE_LAYOUT_GENERAL))
   || (a.loadOp==VK_ATTACHMENT_LOAD_OP_LOAD
   && a.initialLayout==VK_IMAGE_LAYOUT_GENERAL))
  || a.storeOp!=VK_ATTACHMENT_STORE_OP_STORE
  || a.finalLayout!=VK_IMAGE_LAYOUT_GENERAL
  || s.flags || s.pipelineBindPoint!=VK_PIPELINE_BIND_POINT_GRAPHICS
  || s.inputAttachmentCount || s.colorAttachmentCount!=1
  || !s.pColorAttachments || s.pColorAttachments[0].attachment!=0
  || s.pColorAttachments[0].layout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  || s.pResolveAttachments || s.pDepthStencilAttachment || s.preserveAttachmentCount)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkRenderPass_T* value=(VkRenderPass_T*)calloc(1,sizeof(VkRenderPass_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->load=a.loadOp==VK_ATTACHMENT_LOAD_OP_LOAD;
 value->clear=a.loadOp==VK_ATTACHMENT_LOAD_OP_CLEAR;
 value->next=device->passes;device->passes=value;
 *pass=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyRenderPass(VkDevice device,VkRenderPass pass,const VkAllocationCallbacks*)
{
 if(!device || !pass)return;
 VkRenderPass_T** current=&device->passes;
 while(*current && *current!=pass)current=&(*current)->next;
 if(*current){*current=pass->next;free(pass);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateFramebuffer(VkDevice device,const VkFramebufferCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkFramebuffer* framebuffer)
{
 if(framebuffer)*framebuffer=VK_NULL_HANDLE;
 if(!device || !info || !framebuffer || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
  || !info->renderPass || info->renderPass->device!=device
  || info->attachmentCount!=1 || !info->pAttachments || !info->pAttachments[0]
  || info->pAttachments[0]->device!=device
  || info->width!=info->pAttachments[0]->image->width
  || info->height!=info->width || info->layers!=1)
  return VK_ERROR_INITIALIZATION_FAILED;
 VkFramebuffer_T* value=(VkFramebuffer_T*)calloc(1,sizeof(VkFramebuffer_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->pass=info->renderPass;
 value->view=info->pAttachments[0];
 value->next=device->framebuffers;device->framebuffers=value;
 *framebuffer=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyFramebuffer(VkDevice device,VkFramebuffer framebuffer,
 const VkAllocationCallbacks*)
{
 if(!device || !framebuffer)return;
 VkFramebuffer_T** current=&device->framebuffers;
 while(*current && *current!=framebuffer)current=&(*current)->next;
 if(*current){*current=framebuffer->next;free(framebuffer);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkAllocateMemory(VkDevice device,const VkMemoryAllocateInfo* info,
 const VkAllocationCallbacks* allocator,VkDeviceMemory* memory)
{
 if(memory)*memory=VK_NULL_HANDLE;
 if(!device || !info || !memory || allocator || info->pNext
  || info->sType!=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
  || !info->allocationSize
  || info->allocationSize>NativeGpu::RcsResourceRange::Pages*4096
  || info->memoryTypeIndex!=0)return VK_ERROR_FEATURE_NOT_PRESENT;
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;
 c.size=info->allocationSize;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)
  return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 VkDeviceMemory_T* value=(VkDeviceMemory_T*)calloc(1,sizeof(VkDeviceMemory_T));
 if(value)value->host=(uint8_t*)calloc(1,(size_t)c.size);
 if(!value || !value->host){
  native_gpu_resource_command destroy={};destroy.version=1;
  destroy.action=NATIVE_GPU_RESOURCE_DESTROY;destroy.handle=c.handle;
  ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
  free(value);return VK_ERROR_OUT_OF_HOST_MEMORY;
 }
 value->device=device;value->handle=c.handle;value->gpu=c.gpu;
 value->bytes=info->allocationSize;
 value->next=device->memory;device->memory=value;*memory=value;
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkFreeMemory(VkDevice device,VkDeviceMemory memory,const VkAllocationCallbacks*)
{
 if(!device || !memory)return;
 for(VkBuffer_T* b=device->buffers;b;b=b->next)if(b->memory==memory)return;
 for(VkImage_T* i=device->images;i;i=i->next)if(i->memory==memory)return;
 VkDeviceMemory_T** current=&device->memory;
 while(*current && *current!=memory)current=&(*current)->next;
 if(!*current)return;
 native_gpu_resource_command c={};c.version=1;
 c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=memory->handle;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return;
 *current=memory->next;free(memory->host);free(memory);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkBindBufferMemory(VkDevice device,VkBuffer buffer,VkDeviceMemory memory,
 VkDeviceSize offset)
{
 if(!device || !buffer || !memory || buffer->device!=device
  || memory->device!=device || buffer->memory || (offset&4095)
  || offset>memory->bytes
  || ((buffer->bytes+4095)&~VkDeviceSize(4095))>memory->bytes-offset)
  return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 buffer->memory=memory;buffer->memoryOffset=uint32_t(offset);
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory(VkDevice device,VkImage image,VkDeviceMemory memory,
 VkDeviceSize offset)
{
 if(!device || !image || !memory || image->device!=device
  || memory->device!=device || image->memory || (offset&4095)
  || offset>memory->bytes
  || VkDeviceSize(image->width)*image->width*4>memory->bytes-offset)
  return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 image->memory=memory;image->memoryOffset=uint32_t(offset);
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkMapMemory(VkDevice device,VkDeviceMemory memory,VkDeviceSize offset,
 VkDeviceSize size,VkMemoryMapFlags flags,void** data)
{
 if(data)*data=nullptr;
 if(!device || !memory || !data || memory->device!=device || flags
  || memory->mapped || offset>=memory->bytes)
  return VK_ERROR_MEMORY_MAP_FAILED;
 VkDeviceSize length=size==VK_WHOLE_SIZE?memory->bytes-offset:size;
 if(!length || length>memory->bytes-offset)return VK_ERROR_MEMORY_MAP_FAILED;
 memory->mapped=true;*data=memory->host+offset;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkUnmapMemory(VkDevice device,VkDeviceMemory memory)
{if(device && memory && memory->device==device)memory->mapped=false;}

static bool ValidMappedRange(VkDevice device,const VkMappedMemoryRange& range,
 VkDeviceSize& bytes)
{
 if(range.sType!=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE || range.pNext
  || !range.memory || range.memory->device!=device || !range.memory->mapped
  || range.offset>=range.memory->bytes)return false;
 bytes=range.size==VK_WHOLE_SIZE?range.memory->bytes-range.offset:range.size;
 return bytes && bytes<=range.memory->bytes-range.offset;
}

static VkResult TransferMapped(VkDevice device,uint32_t count,
 const VkMappedMemoryRange* ranges,bool write)
{
 if(!device || (count && !ranges))return VK_ERROR_MEMORY_MAP_FAILED;
 for(uint32_t i=0;i<count;i++){
  VkDeviceSize bytes=0;
  if(!ValidMappedRange(device,ranges[i],bytes))return VK_ERROR_MEMORY_MAP_FAILED;
  VkDeviceSize copied=0;
  while(copied<bytes){
   native_gpu_resource_command c={};c.version=1;
   c.action=write?NATIVE_GPU_RESOURCE_WRITE:NATIVE_GPU_RESOURCE_READ;
   c.handle=ranges[i].memory->handle;c.offset=ranges[i].offset+copied;
   c.bytes=uint32_t(bytes-copied<4096?bytes-copied:4096);
   uint8_t* host=ranges[i].memory->host+c.offset;
   if(write)memcpy(c.data,host,c.bytes);
   if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)
    return VK_ERROR_DEVICE_LOST;
   if(!write)memcpy(host,c.data,c.bytes);
   copied+=c.bytes;
  }
 }
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkFlushMappedMemoryRanges(VkDevice device,uint32_t count,
 const VkMappedMemoryRange* ranges)
{return TransferMapped(device,count,ranges,true);}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkInvalidateMappedMemoryRanges(VkDevice device,uint32_t count,
 const VkMappedMemoryRange* ranges)
{return TransferMapped(device,count,ranges,false);}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDescriptorSetLayout(VkDevice device,const VkDescriptorSetLayoutCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkDescriptorSetLayout* layout)
{
 if(layout)*layout=VK_NULL_HANDLE;
 if(!device || !info || !layout || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
  || info->bindingCount!=1 || !info->pBindings)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 const auto& binding=info->pBindings[0];
 if(binding.binding || binding.descriptorType!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
  || binding.descriptorCount!=1 || binding.stageFlags!=VK_SHADER_STAGE_FRAGMENT_BIT
  || binding.pImmutableSamplers)return VK_ERROR_FEATURE_NOT_PRESENT;
 VkDescriptorSetLayout_T* value=(VkDescriptorSetLayout_T*)calloc(1,sizeof(*value));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->setLayouts;device->setLayouts=value;
 *layout=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyDescriptorSetLayout(VkDevice device,VkDescriptorSetLayout layout,
 const VkAllocationCallbacks*)
{
 if(!device || !layout)return;
 for(VkPipelineLayout_T* p=device->layouts;p;p=p->next)
  if(p->setLayout==layout)return;
 for(VkDescriptorPool_T* p=device->descriptorPools;p;p=p->next)
  for(VkDescriptorSet_T* s=p->sets;s;s=s->next)if(s->layout==layout)return;
 VkDescriptorSetLayout_T** current=&device->setLayouts;
 while(*current && *current!=layout)current=&(*current)->next;
 if(*current){*current=layout->next;free(layout);}
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDescriptorPool(VkDevice device,const VkDescriptorPoolCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkDescriptorPool* pool)
{
 if(pool)*pool=VK_NULL_HANDLE;
 if(!device || !info || !pool || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
  || !info->maxSets || info->maxSets>64 || info->poolSizeCount!=1
  || !info->pPoolSizes || info->pPoolSizes[0].type!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
  || info->pPoolSizes[0].descriptorCount<info->maxSets)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkDescriptorPool_T* value=(VkDescriptorPool_T*)calloc(1,sizeof(*value));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->maxSets=info->maxSets;
 value->next=device->descriptorPools;device->descriptorPools=value;
 *pool=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyDescriptorPool(VkDevice device,VkDescriptorPool pool,
 const VkAllocationCallbacks*)
{
 if(!device || !pool)return;
 VkDescriptorPool_T** current=&device->descriptorPools;
 while(*current && *current!=pool)current=&(*current)->next;
 if(!*current)return;
 while(pool->sets){VkDescriptorSet_T* set=pool->sets;
  pool->sets=set->next;free(set);}
 *current=pool->next;free(pool);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkAllocateDescriptorSets(VkDevice device,const VkDescriptorSetAllocateInfo* info,
 VkDescriptorSet* sets)
{
 if(!device || !info || !sets || info->pNext
  || info->sType!=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
  || !info->descriptorPool || info->descriptorPool->device!=device
  || !info->descriptorSetCount || !info->pSetLayouts)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkDescriptorPool pool=info->descriptorPool;
 if(info->descriptorSetCount>pool->maxSets-pool->used)return VK_ERROR_OUT_OF_POOL_MEMORY;
 for(uint32_t i=0;i<info->descriptorSetCount;i++){
  sets[i]=VK_NULL_HANDLE;
  if(!info->pSetLayouts[i] || info->pSetLayouts[i]->device!=device)
   return VK_ERROR_FEATURE_NOT_PRESENT;
 }
 VkDescriptorSet_T* pending=nullptr;
 for(uint32_t i=0;i<info->descriptorSetCount;i++){
  VkDescriptorSet_T* set=(VkDescriptorSet_T*)calloc(1,sizeof(*set));
  if(!set){while(pending){VkDescriptorSet_T* next=pending->next;
   free(pending);pending=next;}
   for(uint32_t j=0;j<i;j++)sets[j]=VK_NULL_HANDLE;
   return VK_ERROR_OUT_OF_HOST_MEMORY;}
  set->device=device;set->pool=pool;set->layout=info->pSetLayouts[i];
  set->next=pending;pending=set;sets[i]=set;
 }
 while(pending){VkDescriptorSet_T* next=pending->next;
  pending->next=pool->sets;pool->sets=pending;pending=next;}
 pool->used+=info->descriptorSetCount;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkUpdateDescriptorSets(VkDevice device,uint32_t writeCount,
 const VkWriteDescriptorSet* writes,uint32_t copyCount,const VkCopyDescriptorSet*)
{
 if(!device || (writeCount && !writes) || copyCount)return;
 for(uint32_t i=0;i<writeCount;i++){
  const auto& w=writes[i];
  if(!w.dstSet || w.dstSet->device!=device)continue;
  w.dstSet->valid=false;w.dstSet->buffer=VK_NULL_HANDLE;
  if(w.sType!=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET || w.pNext
   || w.dstBinding || w.dstArrayElement || w.descriptorCount!=1
   || w.descriptorType!=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || !w.pBufferInfo)
   continue;
  const auto& b=w.pBufferInfo[0];
  if(!b.buffer || b.buffer->device!=device
   || !(b.buffer->usage&VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
   || !b.buffer->memory || b.range!=16 || b.buffer->bytes<16
   || (b.offset&15) || b.offset>b.buffer->bytes-16)
   continue;
  w.dstSet->buffer=b.buffer;w.dstSet->bufferOffset=uint32_t(b.offset);
  w.dstSet->valid=true;
 }
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreatePipelineLayout(VkDevice device,const VkPipelineLayoutCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkPipelineLayout* layout)
{
 if(layout)*layout=VK_NULL_HANDLE;
 if(!device || !info || !layout || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
  || info->setLayoutCount>1 || info->pushConstantRangeCount
  || (info->setLayoutCount && (!info->pSetLayouts || !info->pSetLayouts[0]
   || info->pSetLayouts[0]->device!=device)))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkPipelineLayout_T* value=(VkPipelineLayout_T*)calloc(1,sizeof(VkPipelineLayout_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;
 value->setLayout=info->setLayoutCount?info->pSetLayouts[0]:VK_NULL_HANDLE;
 value->next=device->layouts;device->layouts=value;
 *layout=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyPipelineLayout(VkDevice device,VkPipelineLayout layout,
 const VkAllocationCallbacks*)
{
 if(!device || !layout)return;
 VkPipelineLayout_T** current=&device->layouts;
 while(*current && *current!=layout)current=&(*current)->next;
 if(*current){*current=layout->next;free(layout);}
}

static bool KnownStage(VkDevice device,const VkPipelineShaderStageCreateInfo& stage,
 VkShaderStageFlagBits expected,const uint32_t* code,size_t bytes)
{
 return stage.sType==VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
  && !stage.pNext && !stage.flags && stage.stage==expected
  && stage.module && stage.module->device==device
  && stage.module->code && (code
    ? stage.module->bytes==bytes && !memcmp(stage.module->code,code,bytes)
    : stage.module->bytes && !(stage.module->bytes&3))
  && stage.pName && !strcmp(stage.pName,"main") && !stage.pSpecializationInfo;
}

static bool KnownGraphicsPipeline(VkDevice device,const VkGraphicsPipelineCreateInfo& p)
{
 if(p.sType!=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO || p.pNext || p.flags
  || p.stageCount!=2 || !p.pStages || !p.layout || p.layout->device!=device
  || !p.renderPass || p.renderPass->device!=device || p.subpass
  || p.pTessellationState || p.pDepthStencilState || p.pDynamicState
  || !p.pVertexInputState || !p.pInputAssemblyState || !p.pViewportState
  || !p.pRasterizationState || !p.pMultisampleState || !p.pColorBlendState)
  return false;
#ifdef RENKU_VULKAN_USE_INTEL_COMPILER
 bool knownVertex=KnownStage(device,p.pStages[0],VK_SHADER_STAGE_VERTEX_BIT,
  nullptr,0);
 bool knownFragment=KnownStage(device,p.pStages[1],VK_SHADER_STAGE_FRAGMENT_BIT,
  nullptr,0);
#else
 bool knownVertex=KnownStage(device,p.pStages[0],VK_SHADER_STAGE_VERTEX_BIT,
  RenkuKnownSpirv::Vertex,sizeof(RenkuKnownSpirv::Vertex));
 bool knownFragment=KnownStage(device,p.pStages[1],VK_SHADER_STAGE_FRAGMENT_BIT,
  RenkuKnownSpirv::Fragment,sizeof(RenkuKnownSpirv::Fragment));
#endif
 if(!knownVertex || !knownFragment)return false;
 const auto& in=*p.pVertexInputState;
 if(in.sType!=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
  || in.pNext || in.flags || in.vertexBindingDescriptionCount!=1
  || in.vertexAttributeDescriptionCount!=1 || !in.pVertexBindingDescriptions
  || !in.pVertexAttributeDescriptions)return false;
 const auto& binding=in.pVertexBindingDescriptions[0];
 const auto& attr=in.pVertexAttributeDescriptions[0];
 if(binding.binding || binding.stride!=16 || binding.inputRate!=VK_VERTEX_INPUT_RATE_VERTEX
  || attr.location || attr.binding || attr.format!=VK_FORMAT_R32G32B32A32_SFLOAT
  || attr.offset)return false;
 const auto& assembly=*p.pInputAssemblyState;
 if(assembly.sType!=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
  || assembly.pNext || assembly.flags
  || assembly.topology!=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
  || assembly.primitiveRestartEnable)return false;
 const auto& view=*p.pViewportState;
 if(view.sType!=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
  || view.pNext || view.flags || view.viewportCount!=1 || !view.pViewports
  || view.scissorCount!=1 || !view.pScissors)return false;
 const auto& viewport=view.pViewports[0];
 const auto& scissor=view.pScissors[0];
 if(viewport.x || viewport.y || (viewport.width!=32 && viewport.width!=64)
  || viewport.height!=viewport.width
  || viewport.minDepth!=0 || viewport.maxDepth!=1
  || scissor.offset.x || scissor.offset.y
  || scissor.extent.width!=viewport.width
  || scissor.extent.height!=viewport.width)return false;
 const auto& raster=*p.pRasterizationState;
 if(raster.sType!=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
  || raster.pNext || raster.flags || raster.depthClampEnable
  || raster.rasterizerDiscardEnable || raster.depthBiasEnable
  || raster.polygonMode!=VK_POLYGON_MODE_FILL || raster.cullMode!=VK_CULL_MODE_NONE
  || raster.frontFace!=VK_FRONT_FACE_COUNTER_CLOCKWISE || raster.lineWidth!=1)
  return false;
 const auto& samples=*p.pMultisampleState;
 if(samples.sType!=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
  || samples.pNext || samples.flags || samples.rasterizationSamples!=VK_SAMPLE_COUNT_1_BIT
  || samples.sampleShadingEnable || samples.alphaToCoverageEnable
  || samples.alphaToOneEnable || samples.pSampleMask)return false;
 const auto& blend=*p.pColorBlendState;
 if(blend.sType!=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
  || blend.pNext || blend.flags || blend.logicOpEnable
  || blend.attachmentCount!=1 || !blend.pAttachments
  || blend.pAttachments[0].blendEnable
  || blend.pAttachments[0].colorWriteMask!=
   (VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT
    |VK_COLOR_COMPONENT_A_BIT))return false;
 return true;
}

static VkResult CreateGraphicsPipelineOne(VkDevice device,
 const VkGraphicsPipelineCreateInfo& info,VkPipeline* pipeline)
{
 size_t fragmentCodeBytes=sizeof(NativeGpu::RcsDrawData::Fs);
 bool fragcoordGradient=false,uniformBuffer=false;
#ifdef RENKU_VULKAN_USE_INTEL_COMPILER
 uint8_t compiledVs[sizeof(NativeGpu::RcsDepthVertex::Words)]={};
 size_t compiledVsBytes=0;
 uint8_t compiledFs[sizeof(NativeGpu::RcsFragCoordRed::Words)]={};
 if(!CompileKnownPipeline(info.pStages[0].module->code,
    info.pStages[0].module->bytes,info.pStages[1].module->code,
    info.pStages[1].module->bytes,compiledVs,sizeof(compiledVs),
    &compiledVsBytes,compiledFs,sizeof(compiledFs),
    &fragmentCodeBytes,&fragcoordGradient,&uniformBuffer))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 const void* fragmentCode=compiledFs;
 const bool customVertex=compiledVsBytes!=sizeof(NativeGpu::RcsDrawData::Vs);
#else
 const void* fragmentCode=NativeGpu::RcsDrawData::Fs;
 const bool customVertex=false;
#endif
 if(fragcoordGradient && (customVertex
  || info.pViewportState->pViewports[0].width!=64))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 if(uniformBuffer && (customVertex || !info.layout->setLayout))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkPipeline_T* value=(VkPipeline_T*)calloc(1,sizeof(VkPipeline_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;
 c.size=4096;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
  free(value);return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 }
 value->shaderHandle=c.handle;
 value->shaderGpu=c.gpu;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
 c.handle=value->shaderHandle;c.bytes=fragmentCodeBytes;
 memcpy(c.data,fragmentCode,c.bytes);
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
  native_gpu_resource_command destroy={};destroy.version=1;
  destroy.action=NATIVE_GPU_RESOURCE_DESTROY;destroy.handle=value->shaderHandle;
  ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
  free(value);return VK_ERROR_DEVICE_LOST;
 }
 if(customVertex){
  c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;c.size=4096;
  if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
   native_gpu_resource_command destroy={};destroy.version=1;
   destroy.action=NATIVE_GPU_RESOURCE_DESTROY;destroy.handle=value->shaderHandle;
   ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
   free(value);
   return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }
  value->vertexShaderHandle=c.handle;
  c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
  c.handle=value->vertexShaderHandle;
#ifdef RENKU_VULKAN_USE_INTEL_COMPILER
  c.bytes=compiledVsBytes;memcpy(c.data,compiledVs,c.bytes);
#endif
  if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
   native_gpu_resource_command destroy={};destroy.version=1;
   destroy.action=NATIVE_GPU_RESOURCE_DESTROY;
   destroy.handle=value->vertexShaderHandle;
   ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
   destroy.handle=value->shaderHandle;
   ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
   free(value);
   return VK_ERROR_DEVICE_LOST;
  }
 }
 value->device=device;value->pass=info.renderPass;value->layout=info.layout;
 value->width=uint32_t(info.pViewportState->pViewports[0].width);
 value->fragcoordGradient=fragcoordGradient;
 value->uniformBuffer=uniformBuffer;
 value->next=device->pipelines;device->pipelines=value;
 *pipeline=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateGraphicsPipelines(VkDevice device,VkPipelineCache cache,uint32_t count,
 const VkGraphicsPipelineCreateInfo* infos,const VkAllocationCallbacks* allocator,
 VkPipeline* pipelines)
{
 if(pipelines && count)for(uint32_t i=0;i<count;i++)pipelines[i]=VK_NULL_HANDLE;
 if(!device || cache || !count || !infos || !pipelines || allocator)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 for(uint32_t i=0;i<count;i++)
  if(!KnownGraphicsPipeline(device,infos[i]))return VK_ERROR_FEATURE_NOT_PRESENT;
 for(uint32_t i=0;i<count;i++){
  VkResult result=CreateGraphicsPipelineOne(device,infos[i],&pipelines[i]);
  if(result!=VK_SUCCESS){
   for(uint32_t j=0;j<i;j++){
    vkDestroyPipeline(device,pipelines[j],nullptr);
    pipelines[j]=VK_NULL_HANDLE;
   }
   return result;
  }
 }
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device,VkPipeline pipeline,const VkAllocationCallbacks*)
{
 if(!device || !pipeline)return;
 VkPipeline_T** current=&device->pipelines;
 while(*current && *current!=pipeline)current=&(*current)->next;
 if(!*current)return;
 native_gpu_resource_command c={};c.version=1;
 c.action=NATIVE_GPU_RESOURCE_DESTROY;
 if(pipeline->vertexShaderHandle){
  c.handle=pipeline->vertexShaderHandle;
  if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return;
  pipeline->vertexShaderHandle=0;
 }
 c.handle=pipeline->shaderHandle;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return;
 *current=pipeline->next;free(pipeline);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateCommandPool(VkDevice device,const VkCommandPoolCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkCommandPool* pool)
{
 if(pool)*pool=VK_NULL_HANDLE;
 if(!device || !info || !pool || allocator || info->pNext
  || (info->flags&~VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
  || info->sType!=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
  || info->queueFamilyIndex)return VK_ERROR_FEATURE_NOT_PRESENT;
 VkCommandPool_T* value=(VkCommandPool_T*)calloc(1,sizeof(VkCommandPool_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->flags=info->flags;
 value->next=device->pools;device->pools=value;
 *pool=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyCommandPool(VkDevice device,VkCommandPool pool,const VkAllocationCallbacks*)
{
 if(!device || !pool)return;
 VkCommandPool_T** current=&device->pools;
 while(*current && *current!=pool)current=&(*current)->next;
 if(!*current)return;
 while(pool->commands){
  VkCommandBuffer_T* next=pool->commands->next;
  free(pool->commands->draws);free(pool->commands->copies);
  free(pool->commands->events);free(pool->commands);pool->commands=next;
 }
 *current=pool->next;free(pool);
}

static void ResetCommandState(VkCommandBuffer_T* command)
{
 command->pass=VK_NULL_HANDLE;command->framebuffer=VK_NULL_HANDLE;
 command->pipeline=VK_NULL_HANDLE;command->boundVertex=VK_NULL_HANDLE;
 command->boundIndex=VK_NULL_HANDLE;command->boundIndexOffset=0;
 command->boundDescriptor=VK_NULL_HANDLE;
 command->boundVertexOffset=0;
 command->clearColor=0;
 command->drawCount=0;command->copyCount=0;command->eventCount=0;
 command->recording=false;command->inRenderPass=false;
 command->drawn=false;command->complete=false;command->invalid=false;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkResetCommandPool(VkDevice device,VkCommandPool pool,VkCommandPoolResetFlags flags)
{
 if(!device || !pool || pool->device!=device
  || (flags&~VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT))
  return VK_ERROR_INITIALIZATION_FAILED;
 for(VkCommandBuffer_T* command=pool->commands;command;command=command->next){
  ResetCommandState(command);
  if(flags&VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT){
   free(command->draws);command->draws=nullptr;command->drawCapacity=0;
   free(command->copies);command->copies=nullptr;command->copyCapacity=0;
   free(command->events);command->events=nullptr;command->eventCapacity=0;
  }
 }
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkResetCommandBuffer(VkCommandBuffer command,VkCommandBufferResetFlags flags)
{
 if(!command || !command->pool || !command->device
  || (flags&~VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT))
  return VK_ERROR_INITIALIZATION_FAILED;
 if(!(command->pool->flags&VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
  return VK_ERROR_FEATURE_NOT_PRESENT;
 ResetCommandState(command);
 if(flags&VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT){
  free(command->draws);command->draws=nullptr;command->drawCapacity=0;
  free(command->copies);command->copies=nullptr;command->copyCapacity=0;
  free(command->events);command->events=nullptr;command->eventCapacity=0;
 }
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkAllocateCommandBuffers(VkDevice device,const VkCommandBufferAllocateInfo* info,
 VkCommandBuffer* commands)
{
 if(commands && info && info->commandBufferCount)
  for(uint32_t i=0;i<info->commandBufferCount;i++)commands[i]=VK_NULL_HANDLE;
 if(!device || !info || !commands || info->pNext
  || info->sType!=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
  || !info->commandPool || info->commandPool->device!=device
  || info->level!=VK_COMMAND_BUFFER_LEVEL_PRIMARY || !info->commandBufferCount)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 for(uint32_t i=0;i<info->commandBufferCount;i++){
  VkCommandBuffer_T* value=(VkCommandBuffer_T*)calloc(1,sizeof(VkCommandBuffer_T));
  if(!value){
   for(uint32_t j=0;j<i;j++){
    VkCommandBuffer_T* allocated=info->commandPool->commands;
    info->commandPool->commands=allocated->next;free(allocated);
    commands[j]=VK_NULL_HANDLE;
   }
   return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
  value->device=device;value->pool=info->commandPool;
  value->next=info->commandPool->commands;info->commandPool->commands=value;
  commands[i]=value;
 }
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkFreeCommandBuffers(VkDevice device,VkCommandPool pool,uint32_t count,
 const VkCommandBuffer* commands)
{
 if(!device || !pool || pool->device!=device || (count && !commands))return;
 for(uint32_t i=0;i<count;i++){
  VkCommandBuffer_T** current=&pool->commands;
  while(*current && *current!=commands[i])current=&(*current)->next;
  if(*current){VkCommandBuffer_T* value=*current;*current=value->next;
   free(value->draws);free(value->copies);free(value->events);free(value);}
 }
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkBeginCommandBuffer(VkCommandBuffer command,const VkCommandBufferBeginInfo* info)
{
 if(!command || !info || info->pNext || info->flags || info->pInheritanceInfo
  || info->sType!=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  || command->recording || (command->complete &&
   !(command->pool->flags&VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)))
  return VK_ERROR_INITIALIZATION_FAILED;
 ResetCommandState(command);command->recording=true;
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdBeginRenderPass(VkCommandBuffer command,const VkRenderPassBeginInfo* info,
 VkSubpassContents contents)
{
 if(!command)return;
 if(!command->recording || command->inRenderPass || !info || info->pNext
  || info->sType!=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
  || !info->renderPass || !info->framebuffer
  || info->renderPass->device!=command->device
  || info->framebuffer->device!=command->device
  || info->framebuffer->pass!=info->renderPass
  || info->renderArea.offset.x || info->renderArea.offset.y
  || info->renderArea.extent.width!=info->framebuffer->view->image->width
  || info->renderArea.extent.height!=info->renderArea.extent.width
  || info->clearValueCount!=(info->renderPass->clear?1u:0u)
  || (info->renderPass->clear && !info->pClearValues)
  || contents!=VK_SUBPASS_CONTENTS_INLINE){
  command->invalid=true;return;
 }
 if(info->renderPass->clear){
  const float* rgba=info->pClearValues[0].color.float32;
  auto unorm8=[](float value)->uint32_t {
   if(!(value>0))return 0;
   if(value>=1)return 255;
   return uint32_t(value*255.0f+0.5f);
  };
  command->clearColor=(unorm8(rgba[3])<<24)|(unorm8(rgba[0])<<16)
   |(unorm8(rgba[1])<<8)|unorm8(rgba[2]);
 }
 command->pass=info->renderPass;command->framebuffer=info->framebuffer;
 command->pipeline=VK_NULL_HANDLE;command->boundVertex=VK_NULL_HANDLE;
 command->boundIndex=VK_NULL_HANDLE;command->boundIndexOffset=0;
 command->boundDescriptor=VK_NULL_HANDLE;
 command->boundVertexOffset=0;
 command->drawn=false;
 command->inRenderPass=true;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdBindPipeline(VkCommandBuffer command,VkPipelineBindPoint bindPoint,
 VkPipeline pipeline)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass
  || bindPoint!=VK_PIPELINE_BIND_POINT_GRAPHICS || !pipeline
  || pipeline->device!=command->device || pipeline->pass!=command->pass){
  command->invalid=true;return;
 }
 command->pipeline=pipeline;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdBindVertexBuffers(VkCommandBuffer command,uint32_t first,uint32_t count,
 const VkBuffer* buffers,const VkDeviceSize* offsets)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || first || count!=1
  || !buffers || !offsets || !buffers[0]
  || buffers[0]->device!=command->device || !buffers[0]->memory
  || !(buffers[0]->usage&VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
  || (offsets[0]&15) || offsets[0]>buffers[0]->bytes){
  command->invalid=true;return;
 }
 command->boundVertex=buffers[0];command->boundVertexOffset=offsets[0];
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdBindIndexBuffer(VkCommandBuffer command,VkBuffer buffer,
 VkDeviceSize offset,VkIndexType type)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || !buffer
  || buffer->device!=command->device || !buffer->memory
  || !(buffer->usage&VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
  || type!=VK_INDEX_TYPE_UINT16 || (offset&1) || offset>buffer->bytes){
  command->invalid=true;return;
 }
 command->boundIndex=buffer;command->boundIndexOffset=offset;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdCopyBuffer(VkCommandBuffer command,VkBuffer source,VkBuffer destination,
 uint32_t count,const VkBufferCopy* regions)
{
 if(!command)return;
 if(!command->recording || command->inRenderPass || !source || !destination
  || source->device!=command->device || destination->device!=command->device
  || !source->memory || !destination->memory
  || !(source->usage&VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
  || !(destination->usage&VK_BUFFER_USAGE_TRANSFER_DST_BIT)
  || !count || !regions){command->invalid=true;return;}
 for(uint32_t i=0;i<count;i++){
  const auto& region=regions[i];
  if(!region.size || region.size>NativeGpu::RcsResourceRange::Pages*4096
   || ((region.srcOffset|region.dstOffset|region.size)&3)
   || region.srcOffset>source->bytes
   || region.size>source->bytes-region.srcOffset
   || region.dstOffset>destination->bytes
   || region.size>destination->bytes-region.dstOffset){
   command->invalid=true;return;
  }
  if(!DisjointCopy(source->memory,source->memoryOffset+region.srcOffset,
    destination->memory,destination->memoryOffset+region.dstOffset,
    region.size)){command->invalid=true;return;}
 }
 for(uint32_t i=0;i<count;i++){
  VkCommandBuffer_T::Copy copy={source->memory,destination->memory,
   source->memoryOffset+regions[i].srcOffset,
   destination->memoryOffset+regions[i].dstOffset,regions[i].size,false,false};
  if(!AppendCopy(command,copy)){command->invalid=true;return;}
 }
}

static bool FullImageCopyRegion(VkImage image,VkBuffer buffer,
 const VkBufferImageCopy& region)
{
 const VkDeviceSize bytes=VkDeviceSize(image->width)*image->width*4;
 return !(region.bufferOffset&3) && region.bufferOffset<=buffer->bytes
  && bytes<=buffer->bytes-region.bufferOffset
  && (!region.bufferRowLength || region.bufferRowLength==image->width)
  && (!region.bufferImageHeight || region.bufferImageHeight==image->width)
  && region.imageSubresource.aspectMask==VK_IMAGE_ASPECT_COLOR_BIT
  && !region.imageSubresource.mipLevel
  && !region.imageSubresource.baseArrayLayer
  && region.imageSubresource.layerCount==1
  && !region.imageOffset.x && !region.imageOffset.y && !region.imageOffset.z
  && region.imageExtent.width==image->width
  && region.imageExtent.height==image->width && region.imageExtent.depth==1;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdCopyImageToBuffer(VkCommandBuffer command,VkImage source,
 VkImageLayout layout,VkBuffer destination,uint32_t count,
 const VkBufferImageCopy* regions)
{
 if(!command)return;
 if(!command->recording || command->inRenderPass || !source || !destination
  || source->device!=command->device || destination->device!=command->device
  || !source->memory || !destination->memory
  || !(source->usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
  || !(destination->usage&VK_BUFFER_USAGE_TRANSFER_DST_BIT)
  || layout!=VK_IMAGE_LAYOUT_GENERAL || count!=1 || !regions){
  command->invalid=true;return;
 }
 const VkBufferImageCopy& region=regions[0];
 const VkDeviceSize bytes=VkDeviceSize(source->width)*source->width*4;
 if(!FullImageCopyRegion(source,destination,region)){
  command->invalid=true;return;
 }
 if(!DisjointCopy(source->memory,source->memoryOffset,
  destination->memory,destination->memoryOffset+region.bufferOffset,bytes)){
  command->invalid=true;return;
 }
 VkCommandBuffer_T::Copy copy={source->memory,destination->memory,
  source->memoryOffset,destination->memoryOffset+region.bufferOffset,
  bytes,true,false};
 if(!AppendCopy(command,copy))command->invalid=true;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdCopyBufferToImage(VkCommandBuffer command,VkBuffer source,
 VkImage destination,VkImageLayout layout,uint32_t count,
 const VkBufferImageCopy* regions)
{
 if(!command)return;
 if(!command->recording || command->inRenderPass || !source || !destination
  || source->device!=command->device || destination->device!=command->device
  || !source->memory || !destination->memory
  || !(source->usage&VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
  || !(destination->usage&VK_IMAGE_USAGE_TRANSFER_DST_BIT)
  || layout!=VK_IMAGE_LAYOUT_GENERAL || count!=1 || !regions){
  command->invalid=true;return;
 }
 const VkBufferImageCopy& region=regions[0];
 const VkDeviceSize bytes=VkDeviceSize(destination->width)*destination->width*4;
 if(!FullImageCopyRegion(destination,source,region)
  || !DisjointCopy(source->memory,
   source->memoryOffset+region.bufferOffset,destination->memory,
   destination->memoryOffset,bytes)){
  command->invalid=true;return;
 }
 VkCommandBuffer_T::Copy copy={source->memory,destination->memory,
  source->memoryOffset+region.bufferOffset,destination->memoryOffset,
  bytes,false,true};
 if(!AppendCopy(command,copy))command->invalid=true;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdBindDescriptorSets(VkCommandBuffer command,VkPipelineBindPoint bindPoint,
 VkPipelineLayout layout,uint32_t first,uint32_t count,
 const VkDescriptorSet* sets,uint32_t dynamicCount,const uint32_t*)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass
  || bindPoint!=VK_PIPELINE_BIND_POINT_GRAPHICS || !layout
  || layout->device!=command->device || !layout->setLayout || first
  || count!=1 || !sets || !sets[0] || sets[0]->device!=command->device
  || sets[0]->layout!=layout->setLayout || dynamicCount){
  command->invalid=true;return;
 }
 command->boundDescriptor=sets[0];
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdDraw(VkCommandBuffer command,uint32_t vertices,uint32_t instances,
 uint32_t firstVertex,uint32_t firstInstance)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || !command->pass
  || !command->pipeline
  || !command->boundVertex || vertices<3 || vertices%3 || instances!=1
  || firstInstance){command->invalid=true;return;}
 if(command->pipeline->uniformBuffer && (!command->boundDescriptor
  || !command->boundDescriptor->valid
  || command->boundDescriptor->layout!=command->pipeline->layout->setLayout)){
  command->invalid=true;return;
 }
 uint64_t start=command->boundVertexOffset+uint64_t(firstVertex)*16;
 const uint64_t vertexBytes=uint64_t(vertices)*16;
 if(start>UINT32_MAX || start>command->boundVertex->bytes
  || vertexBytes>command->boundVertex->bytes-start){
  command->invalid=true;return;
 }
 const size_t added=vertices/3;
 for(size_t i=0;i<added;i++)
  if(((start+i*48)&4095)>4096-48){command->invalid=true;return;}
 start+=command->boundVertex->memoryOffset;
 if(command->drawCount>SIZE_MAX-added){command->invalid=true;return;}
 if(command->drawCount+added>command->drawCapacity){
  size_t next=command->drawCapacity?command->drawCapacity*2:4;
  if(next<command->drawCount+added)next=command->drawCount+added;
  if(next<command->drawCapacity || next>SIZE_MAX/sizeof(VkCommandBuffer_T::Draw)){
   command->invalid=true;return;
  }
  void* storage=realloc(command->draws,next*sizeof(VkCommandBuffer_T::Draw));
  if(!storage){command->invalid=true;return;}
  command->draws=(VkCommandBuffer_T::Draw*)storage;
  command->drawCapacity=next;
 }
 for(size_t i=0;i<added;i++){
  size_t index=command->drawCount++;
  command->draws[index]={command->framebuffer,
   command->pipeline,command->boundVertex,command->boundDescriptor,
   uint32_t(start+i*48),0,command->pass->load || command->pass->clear
    || command->drawn || i!=0,
   VK_NULL_HANDLE,0,false,0,0,
   command->pass->clear && !command->drawn && i==0,command->clearColor};
  if(!AppendEvent(command,false,index)){command->invalid=true;return;}
 }
 command->drawn=true;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdDrawIndexed(VkCommandBuffer command,uint32_t indexCount,
 uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,
 uint32_t firstInstance)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || !command->pass
  || !command->pipeline
  || !command->boundVertex || !command->boundIndex
  || command->pipeline->vertexShaderHandle
  || command->pipeline->uniformBuffer || command->pipeline->fragcoordGradient
  || indexCount<3 || indexCount>255 || indexCount%3
  || instanceCount!=1 || firstInstance){
  command->invalid=true;return;
 }
 uint64_t vertexStart=command->boundVertexOffset;
 // The owner fetches an index relative to the supplied vertex address.
 // Shift that address to implement Vulkan's signed vertexOffset exactly.
 if(vertexStart>command->boundVertex->bytes){
  command->invalid=true;return;
 }
 if(vertexOffset>=0){
  const uint64_t advance=uint64_t(vertexOffset)*16;
  if(advance>command->boundVertex->bytes-vertexStart){
   command->invalid=true;return;
  }
  vertexStart+=advance;
 }else{
  const uint64_t retreat=uint64_t(-int64_t(vertexOffset))*16;
  if(retreat>vertexStart){command->invalid=true;return;}
  vertexStart-=retreat;
 }
 uint64_t indexStart=command->boundIndexOffset+uint64_t(firstIndex)*2;
 if(vertexStart>command->boundVertex->bytes
  || indexStart>command->boundIndex->bytes
  || uint64_t(indexCount)*2>command->boundIndex->bytes-indexStart
  || (indexStart&4095)>4096-indexCount*2){
  command->invalid=true;return;
 }
 uint64_t vertexBytes=command->boundVertex->bytes-vertexStart;
 if(vertexBytes<48 || vertexBytes>4096 || (vertexBytes&15)
  || (vertexStart&4095)>4096-vertexBytes){
  command->invalid=true;return;
 }
 vertexStart+=command->boundVertex->memoryOffset;
 indexStart+=command->boundIndex->memoryOffset;
 if(vertexStart>UINT32_MAX || indexStart>UINT32_MAX
  || command->drawCount==SIZE_MAX){command->invalid=true;return;}
 if(command->drawCount==command->drawCapacity){
  size_t next=command->drawCapacity?command->drawCapacity*2:4;
  if(next<command->drawCapacity || next>SIZE_MAX/sizeof(VkCommandBuffer_T::Draw)){
   command->invalid=true;return;
  }
  void* storage=realloc(command->draws,next*sizeof(VkCommandBuffer_T::Draw));
  if(!storage){command->invalid=true;return;}
  command->draws=(VkCommandBuffer_T::Draw*)storage;
  command->drawCapacity=next;
 }
 auto& draw=command->draws[command->drawCount++];
 draw={command->framebuffer,command->pipeline,command->boundVertex,
  command->boundDescriptor,uint32_t(vertexStart),vertexOffset,
  command->pass->load || command->pass->clear || command->drawn,
  command->boundIndex,uint32_t(indexStart),true,indexCount,
  uint32_t(vertexBytes/16),
  command->pass->clear && !command->drawn,command->clearColor};
 if(!AppendEvent(command,false,command->drawCount-1)){
  command->invalid=true;return;
 }
 command->drawn=true;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdEndRenderPass(VkCommandBuffer command)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || !command->drawn){
  command->invalid=true;return;
 }
 command->inRenderPass=false;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkEndCommandBuffer(VkCommandBuffer command)
{
 if(!command || !command->recording || command->inRenderPass
  || !command->eventCount || command->invalid)return VK_ERROR_INITIALIZATION_FAILED;
 command->recording=false;command->complete=true;return VK_SUCCESS;
}

// Queue execution is synchronous today. Simulate all binary transitions before
// the first draw so a bad later submit cannot partially execute the call.
static VkResult ValidateSemaphoreSubmits(VkDevice device,uint32_t count,
 const VkSubmitInfo* submits)
{
 size_t operations=0;
 for(uint32_t i=0;i<count;i++){
  const auto& submit=submits[i];
  if((submit.waitSemaphoreCount && (!submit.pWaitSemaphores
    || !submit.pWaitDstStageMask))
   || (submit.signalSemaphoreCount && !submit.pSignalSemaphores))
   return VK_ERROR_FEATURE_NOT_PRESENT;
  if(operations>SIZE_MAX-submit.waitSemaphoreCount
   || operations+submit.waitSemaphoreCount>SIZE_MAX-submit.signalSemaphoreCount)
   return VK_ERROR_OUT_OF_HOST_MEMORY;
  operations+=size_t(submit.waitSemaphoreCount)+submit.signalSemaphoreCount;
 }
 if(!operations)return VK_SUCCESS;
 struct State {VkSemaphore semaphore;bool signaled;};
 if(operations>SIZE_MAX/sizeof(State))return VK_ERROR_OUT_OF_HOST_MEMORY;
 State* states=(State*)calloc(operations,sizeof(State));
 if(!states)return VK_ERROR_OUT_OF_HOST_MEMORY;
 size_t used=0;
 auto state=[&](VkSemaphore semaphore)->State* {
  if(!semaphore || semaphore->device!=device)return nullptr;
  for(size_t i=0;i<used;i++)if(states[i].semaphore==semaphore)return &states[i];
  State* found=&states[used++];found->semaphore=semaphore;
  found->signaled=__atomic_load_n(&semaphore->signaled,__ATOMIC_ACQUIRE);
  return found;
 };
 VkResult result=VK_SUCCESS;
 for(uint32_t i=0;i<count && result==VK_SUCCESS;i++){
  const auto& submit=submits[i];
  for(uint32_t j=0;j<submit.waitSemaphoreCount;j++){
   State* found=state(submit.pWaitSemaphores[j]);
   if(!found || !submit.pWaitDstStageMask[j] || !found->signaled){
    result=VK_ERROR_INITIALIZATION_FAILED;break;
   }
   found->signaled=false;
  }
  if(result!=VK_SUCCESS)break;
  for(uint32_t j=0;j<submit.signalSemaphoreCount;j++){
   State* found=state(submit.pSignalSemaphores[j]);
   if(!found || found->signaled){result=VK_ERROR_INITIALIZATION_FAILED;break;}
   found->signaled=true;
  }
 }
 free(states);return result;
}

static VkResult ClearImage(VkDevice device,VkImage image,uint32_t color)
{
 const unsigned pages=image->width==64?4u:1u;
 const bigtime_t start=device->trace?system_time():0;
 for(unsigned page=0;page<pages;page++){
  native_gpu_resource_command write={};
  write.version=1;write.action=NATIVE_GPU_RESOURCE_WRITE;
  write.handle=image->memory->handle;
  write.offset=image->memoryOffset+uint64_t(page)*4096;
  write.bytes=4096;
  uint32_t* pixels=(uint32_t*)write.data;
  for(unsigned i=0;i<1024;i++)pixels[i]=color;
  int status=ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&write,sizeof(write));
  if(status!=0){
   if(device->trace)fprintf(stderr,
    "{\"event\":\"vk_clear_write\",\"image_bo\":%u,\"page\":%u,"
    "\"status\":%d,\"us\":%lld}\n",
    image->memory->handle,page,status,(long long)(system_time()-start));
   return VK_ERROR_DEVICE_LOST;
  }
 }
 if(device->trace)fprintf(stderr,
  "{\"event\":\"vk_clear_write\",\"image_bo\":%u,\"pages\":%u,"
  "\"color\":\"%08x\",\"status\":0,\"us\":%lld}\n",
  image->memory->handle,pages,color,(long long)(system_time()-start));
 return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkQueueSubmit(VkQueue queue,uint32_t count,const VkSubmitInfo* submits,VkFence fence)
{
 if(!queue || !queue->device || (fence && (fence->device!=queue->device
  || __atomic_load_n(&fence->signaled,__ATOMIC_ACQUIRE))))
  return VK_ERROR_INITIALIZATION_FAILED;
 if(!count){
  if(fence)__atomic_store_n(&fence->signaled,true,__ATOMIC_RELEASE);
  return VK_SUCCESS;
 }
 if(!submits)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkDevice device=queue->device;
 const char* directSetting=getenv("RENKU_VK_DIRECT_TARGET");
 const bool directTarget=!directSetting || strcmp(directSetting,"0")!=0;
 const char* batchSetting=getenv("RENKU_VK_NATIVE_BATCH");
 const bool batchEnabled=!batchSetting || strcmp(batchSetting,"0")!=0;
 for(uint32_t i=0;i<count;i++){
  const VkSubmitInfo& submit=submits[i];
  if(submit.sType!=VK_STRUCTURE_TYPE_SUBMIT_INFO || submit.pNext
   || (submit.commandBufferCount && !submit.pCommandBuffers))
   return VK_ERROR_FEATURE_NOT_PRESENT;
  for(uint32_t j=0;j<submit.commandBufferCount;j++){
   VkCommandBuffer command=submit.pCommandBuffers[j];
   if(!command || command->device!=device || !command->complete
    || !command->eventCount || !command->events
    || (command->drawCount && !command->draws)
    || (command->copyCount && !command->copies))
    return VK_ERROR_INITIALIZATION_FAILED;
   for(size_t k=0;k<command->copyCount;k++){
    const auto& recorded=command->copies[k];
    if(!recorded.source || !recorded.destination
     || recorded.source->device!=device || recorded.destination->device!=device
     || recorded.sourceOffset>recorded.source->bytes
     || recorded.size>recorded.source->bytes-recorded.sourceOffset
     || recorded.destinationOffset>recorded.destination->bytes
     || recorded.size>recorded.destination->bytes-recorded.destinationOffset)
     return VK_ERROR_INITIALIZATION_FAILED;
   }
   for(size_t k=0;k<command->drawCount;k++){
    const auto& recorded=command->draws[k];
    if(!recorded.framebuffer || recorded.framebuffer->device!=device
     || !recorded.framebuffer->view || !recorded.framebuffer->view->image
     || !recorded.framebuffer->view->image->memory
     || !recorded.pipeline || recorded.pipeline->device!=device
     || recorded.pipeline->pass!=recorded.framebuffer->pass
     || !recorded.vertex || recorded.vertex->device!=device
     || !recorded.vertex->memory)return VK_ERROR_INITIALIZATION_FAILED;
    if(recorded.pipeline->width!=recorded.framebuffer->view->image->width)
     return VK_ERROR_FEATURE_NOT_PRESENT;
    if(recorded.preserve && !directTarget)return VK_ERROR_FEATURE_NOT_PRESENT;
    if(recorded.indexed && (!directTarget || !recorded.index
     || recorded.index->device!=device || !recorded.index->memory
     || !(recorded.index->usage&VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
     || recorded.framebuffer->view->image->memoryOffset))
     return VK_ERROR_FEATURE_NOT_PRESENT;
    if(recorded.pipeline->uniformBuffer && (!directTarget
     || !recorded.descriptor || !recorded.descriptor->valid
     || recorded.descriptor->device!=device
     || recorded.descriptor->layout!=recorded.pipeline->layout->setLayout
     || !recorded.descriptor->buffer
     || !recorded.descriptor->buffer->memory))
     return VK_ERROR_INITIALIZATION_FAILED;
   }
  }
 }
 VkResult semaphoreState=ValidateSemaphoreSubmits(device,count,submits);
 if(semaphoreState!=VK_SUCCESS)return semaphoreState;
 for(uint32_t i=0;i<count;i++){
 for(uint32_t j=0;j<submits[i].waitSemaphoreCount;j++)
  __atomic_store_n(&submits[i].pWaitSemaphores[j]->signaled,false,__ATOMIC_RELEASE);
 for(uint32_t j=0;j<submits[i].commandBufferCount;j++){
 VkCommandBuffer command=submits[i].pCommandBuffers[j];
 for(size_t eventIndex=0;eventIndex<command->eventCount;eventIndex++){
 const auto& event=command->events[eventIndex];
 if(event.copy){
  const auto& copy=command->copies[event.index];
  native_gpu_resource_command request={};request.version=1;
  request.action=NATIVE_GPU_RESOURCE_COPY;
  request.handle=copy.destination->handle;
  request.gpu=copy.source->handle;
  request.size=copy.sourceOffset;
  request.offset=copy.destinationOffset;
  request.bytes=uint32_t(copy.size);
  const bigtime_t started=device->trace?system_time():0;
  int status=ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&request,sizeof(request));
  if(device->trace)fprintf(stderr,
   "{\"event\":\"vk_copy_buffer\",\"src_kind\":\"%s\",\"dst_kind\":\"%s\",\"src_bo\":%u,\"dst_bo\":%u,"
   "\"src_offset\":%llu,\"dst_offset\":%llu,\"bytes\":%u,"
   "\"status\":%d,\"us\":%lld}\n",
   copy.sourceImage?"image":"buffer",
   copy.destinationImage?"image":"buffer",
   copy.source->handle,copy.destination->handle,
   (unsigned long long)request.size,(unsigned long long)request.offset,
   request.bytes,status,(long long)(system_time()-started));
  if(status!=0)return VK_ERROR_DEVICE_LOST;
  continue;
 }
 size_t k=event.index;
 const auto& recorded=command->draws[k];
 const bool large=recorded.framebuffer->view->image->width==64;
 const bool uniform=recorded.pipeline->uniformBuffer;
 const bool generalIndexed=recorded.indexed
  && (recorded.indexCount>6 || recorded.vertexCount!=4
   || (recorded.indexCount==6 && recorded.preserve));
 const bool nativeBatch=!recorded.indexed && eventIndex+1<command->eventCount
  && !command->events[eventIndex+1].copy
  && command->events[eventIndex+1].index==k+1
  && !command->draws[k+1].indexed && directTarget
  && !uniform && !recorded.pipeline->vertexShaderHandle
  && !recorded.pipeline->fragcoordGradient
  && !recorded.framebuffer->view->image->memoryOffset
  && command->draws[k+1].preserve
  && command->draws[k+1].framebuffer==recorded.framebuffer
  && command->draws[k+1].pipeline==recorded.pipeline
  && command->draws[k+1].vertex==recorded.vertex
  && uint64_t(recorded.vertexOffset)+48==command->draws[k+1].vertexOffset
  && (recorded.vertexOffset&4095)<=4096-96
  && batchEnabled;
 const uint32_t uniformOffset=uniform
  ?recorded.descriptor->buffer->memoryOffset+recorded.descriptor->bufferOffset:0;
 native_gpu_render_large_command largeDraw={};
 native_gpu_render_command smallDraw={};
 native_gpu_render_command& draw=large?largeDraw.base:smallDraw;
 draw.version=recorded.indexed?(generalIndexed?25:recorded.indexCount==6?24:
  recorded.preserve?23:22):
  nativeBatch?(recorded.preserve?21:20):recorded.preserve?19:
  directTarget && recorded.framebuffer->view->image->memoryOffset?18:
  uniform?(uniformOffset?17:16):directTarget?15:recorded.vertexOffset
  ?(recorded.pipeline->vertexShaderHandle?14:13)
  :large?(recorded.pipeline->fragcoordGradient?12:
  recorded.pipeline->vertexShaderHandle?11:10):
  recorded.pipeline->vertexShaderHandle?8:7;
 draw.mode=NATIVE_GPU_RENDER_DRAW;
 draw.variant=recorded.vertex->memory->handle;
 draw.reserved=recorded.pipeline->shaderHandle;
 if(draw.version==13 || draw.version==14 || draw.version==15
  || draw.version==16 || draw.version==17 || draw.version==18
  || draw.version==19 || draw.version==20 || draw.version==21
  || draw.version==22 || draw.version==23 || draw.version==24
  || draw.version==25)
  memcpy(draw.output,&recorded.vertexOffset,
  sizeof(recorded.vertexOffset));
 if(draw.version==8 || draw.version==11)
  memcpy(draw.output,&recorded.pipeline->vertexShaderHandle,
   sizeof(recorded.pipeline->vertexShaderHandle));
 if(draw.version==14)memcpy(draw.output+4,
  &recorded.pipeline->vertexShaderHandle,
  sizeof(recorded.pipeline->vertexShaderHandle));
 if(draw.version==15 || draw.version==16 || draw.version==17
  || draw.version==18 || draw.version==19 || draw.version==20
  || draw.version==21 || draw.version==22 || draw.version==23
  || draw.version==24 || draw.version==25){
  memcpy(draw.output+4,&recorded.pipeline->vertexShaderHandle,
   sizeof(recorded.pipeline->vertexShaderHandle));
  memcpy(draw.output+8,&recorded.framebuffer->view->image->memory->handle,
   sizeof(recorded.framebuffer->view->image->memory->handle));
  if(uniform)memcpy(draw.output+12,
   &recorded.descriptor->buffer->memory->handle,
   sizeof(recorded.descriptor->buffer->memory->handle));
  if(draw.version==17 || draw.version==18 || draw.version==19)
   memcpy(draw.output+16,
   &uniformOffset,sizeof(uniformOffset));
  if(draw.version==18 || draw.version==19)memcpy(draw.output+20,
   &recorded.framebuffer->view->image->memoryOffset,
   sizeof(recorded.framebuffer->view->image->memoryOffset));
  if(draw.version==25){
   memcpy(draw.output+12,&recorded.vertexCount,
    sizeof(recorded.vertexCount));
   memcpy(draw.output+16,&recorded.indexCount,
    sizeof(recorded.indexCount));
   uint32_t flags=recorded.preserve?1u:0u;
   memcpy(draw.output+20,&flags,sizeof(flags));
  }
  if(draw.version==22 || draw.version==23 || draw.version==24
   || draw.version==25){
   memcpy(draw.output+24,&recorded.index->memory->handle,
    sizeof(recorded.index->memory->handle));
   memcpy(draw.output+28,&recorded.indexOffset,sizeof(recorded.indexOffset));
  }
 }
 if(recorded.clear){
  VkResult clear=ClearImage(device,recorded.framebuffer->view->image,
   recorded.clearColor);
  if(clear!=VK_SUCCESS)return clear;
 }
 const bigtime_t submitStart=device->trace?system_time():0;
 int status=large
  ?ioctl(device->fd,NATIVE_GPU_RENDER_LARGE_IOCTL,&largeDraw,sizeof(largeDraw))
  :ioctl(device->fd,NATIVE_GPU_RENDER_IOCTL,&smallDraw,sizeof(smallDraw));
 if((large && draw.version==12) || (directTarget && draw.version>=15)){
  const char* capture=getenv("RENKU_VK_CAPTURE_PIXELS");
  if(capture){
   FILE* file=fopen(capture,"wb");
   size_t saved=0;int closed=-1;
   if(file){
    saved=fwrite(draw.output,1,sizeof(draw.output),file);
    if(large)saved+=fwrite(largeDraw.extra,1,sizeof(largeDraw.extra),file);
    closed=fclose(file);
   }
   fprintf(stderr,"{\"event\":\"vk_pixel_capture\",\"bytes\":%zu,"
    "\"closed\":%d,\"ioctl\":%d}\n",saved,closed,status);
  }
 }
 const bigtime_t renderDone=device->trace?system_time():0;
 const auto& r=draw.report;
 uint32_t hash=2166136261u;
 const uint32_t* pixels=(const uint32_t*)draw.output;
 const uint32_t* extraPixels=(const uint32_t*)largeDraw.extra;
 for(unsigned i=0;i<(large?4096u:1024u);i++)
  hash=(hash^(i<1024?pixels[i]:extraPixels[i-1024]))*16777619u;
 const bool valid=status==0 && !r.status && r.version==draw.version
  && r.stage==NATIVE_GPU_RENDER_STAGE_COMPLETE
  && r.known_profile && r.owner_granted && r.sequence
  && r.observed_sequence==r.sequence && r.nonce==r.observed_nonce
  && r.observed_complement==~r.nonce
  && (draw.version==25
   ?r.checked_pixels==0 && r.matched_pixels==0
   :r.checked_pixels>=(large?2000u:500u)
    && r.matched_pixels==r.checked_pixels)
  && hash==r.target_hash
  && !r.fault && r.engine_stage==4 && !r.engine_error && !r.csb_error;
 int writeStatus=directTarget?0:-1;
 bigtime_t writeStart=renderDone,writeDone=renderDone;
 if(valid && !directTarget){
  if(device->trace)writeStart=system_time();
  for(unsigned p=0;p<(large?4u:1u);p++){
   native_gpu_resource_command write={};write.version=1;
   write.action=NATIVE_GPU_RESOURCE_WRITE;
   write.handle=recorded.framebuffer->view->image->memory->handle;
   write.offset=recorded.framebuffer->view->image->memoryOffset
    +uint64_t(p)*4096;
   write.bytes=4096;
   memcpy(write.data,p?largeDraw.extra+(p-1)*4096:draw.output,4096);
   writeStatus=ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&write,sizeof(write));
   if(writeStatus!=0)break;
  }
  if(device->trace)writeDone=system_time();
 }
 if(device->trace)fprintf(stderr,
  "{\"event\":\"vk_queue_submit\",\"ioctl\":%d,\"write_ioctl\":%d,"
  "\"version\":%u,\"stage\":%u,\"sequence\":%u,\"observed\":%u,"
  "\"draw_index\":%zu,\"draw_count\":%zu,\"gpu_draws\":%u,\"vertex_offset\":%u,\"vertex_bias\":%d,\"direct_target\":%u,"
  "\"vertex_bo\":%u,\"shader_bo\":%u,\"vertex_shader_bo\":%u,\"image_bo\":%u,\"image_offset\":%u,\"uniform_bo\":%u,\"uniform_offset\":%u,\"index_bo\":%u,\"index_offset\":%u,\"index_count\":%u,\"vertex_count\":%u,\"pixel_oracle\":%u,"
  "\"vertex_gpu_va\":\"%llx\",\"shader_gpu_va\":\"%llx\",\"image_gpu_va\":\"%llx\","
  "\"checked\":%u,\"matched\":%u,\"pixel_hash\":\"%08x\","
  "\"fault\":%u,\"engine_stage\":%u,\"csb_error\":%u,"
  "\"render_ioctl_us\":%lld,\"write_ioctl_us\":%lld,\"submit_us\":%lld}\n",
  status,writeStatus,r.version,r.stage,r.sequence,r.observed_sequence,
  k,command->drawCount,nativeBatch?2u:1u,recorded.vertexOffset,
  recorded.vertexBias,
  unsigned(directTarget),draw.variant,draw.reserved,
  recorded.pipeline->vertexShaderHandle,
  recorded.framebuffer->view->image->memory->handle,
  recorded.framebuffer->view->image->memoryOffset,
  uniform?recorded.descriptor->buffer->memory->handle:0,
  uniformOffset,
  recorded.indexed?recorded.index->memory->handle:0,
  recorded.indexed?recorded.indexOffset:0,
  recorded.indexed?recorded.indexCount:0,
  recorded.indexed?recorded.vertexCount:0,
  unsigned(draw.version!=25),
  (unsigned long long)recorded.vertex->memory->gpu,
  (unsigned long long)recorded.pipeline->shaderGpu,
  (unsigned long long)recorded.framebuffer->view->image->memory->gpu,
  r.checked_pixels,r.matched_pixels,hash,r.fault,r.engine_stage,r.csb_error,
  (long long)(renderDone-submitStart),(long long)(writeDone-writeStart),
  (long long)(system_time()-submitStart));
 if(!valid || writeStatus!=0)return VK_ERROR_DEVICE_LOST;
 if(nativeBatch)++eventIndex;
 }
 }
 for(uint32_t j=0;j<submits[i].signalSemaphoreCount;j++)
  __atomic_store_n(&submits[i].pSignalSemaphores[j]->signaled,true,__ATOMIC_RELEASE);
 }
 if(fence)__atomic_store_n(&fence->signaled,true,__ATOMIC_RELEASE);
 return VK_SUCCESS;
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
 VK_PROC(vkCreateFence);
 VK_PROC(vkCreateSemaphore);
 VK_PROC(vkDestroySemaphore);
 VK_PROC(vkDestroyFence);
 VK_PROC(vkGetFenceStatus);
 VK_PROC(vkResetFences);
 VK_PROC(vkWaitForFences);
 VK_PROC(vkCreateShaderModule);
 VK_PROC(vkDestroyShaderModule);
 VK_PROC(vkCreateBuffer);
 VK_PROC(vkDestroyBuffer);
 VK_PROC(vkGetBufferMemoryRequirements);
 VK_PROC(vkCreateImage);
 VK_PROC(vkDestroyImage);
 VK_PROC(vkGetImageMemoryRequirements);
 VK_PROC(vkCreateImageView);
 VK_PROC(vkDestroyImageView);
 VK_PROC(vkCreateRenderPass);
 VK_PROC(vkDestroyRenderPass);
 VK_PROC(vkCreateFramebuffer);
 VK_PROC(vkDestroyFramebuffer);
 VK_PROC(vkAllocateMemory);
 VK_PROC(vkFreeMemory);
 VK_PROC(vkBindBufferMemory);
 VK_PROC(vkBindImageMemory);
 VK_PROC(vkMapMemory);
 VK_PROC(vkUnmapMemory);
 VK_PROC(vkFlushMappedMemoryRanges);
 VK_PROC(vkInvalidateMappedMemoryRanges);
 VK_PROC(vkCreateDescriptorSetLayout);
 VK_PROC(vkDestroyDescriptorSetLayout);
 VK_PROC(vkCreateDescriptorPool);
 VK_PROC(vkDestroyDescriptorPool);
 VK_PROC(vkAllocateDescriptorSets);
 VK_PROC(vkUpdateDescriptorSets);
 VK_PROC(vkCreatePipelineLayout);
 VK_PROC(vkDestroyPipelineLayout);
 VK_PROC(vkCreateGraphicsPipelines);
 VK_PROC(vkDestroyPipeline);
 VK_PROC(vkCreateCommandPool);
 VK_PROC(vkDestroyCommandPool);
 VK_PROC(vkResetCommandPool);
 VK_PROC(vkResetCommandBuffer);
 VK_PROC(vkAllocateCommandBuffers);
 VK_PROC(vkFreeCommandBuffers);
 VK_PROC(vkBeginCommandBuffer);
 VK_PROC(vkCmdBeginRenderPass);
 VK_PROC(vkCmdBindPipeline);
 VK_PROC(vkCmdBindDescriptorSets);
 VK_PROC(vkCmdBindVertexBuffers);
 VK_PROC(vkCmdBindIndexBuffer);
 VK_PROC(vkCmdCopyBuffer);
 VK_PROC(vkCmdCopyImageToBuffer);
 VK_PROC(vkCmdCopyBufferToImage);
 VK_PROC(vkCmdDraw);
 VK_PROC(vkCmdDrawIndexed);
 VK_PROC(vkCmdEndRenderPass);
 VK_PROC(vkEndCommandBuffer);
 VK_PROC(vkQueueSubmit);
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
 VK_PROC(vkGetPhysicalDeviceFormatProperties);
 VK_PROC(vkGetPhysicalDeviceImageFormatProperties);
 VK_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
 VK_PROC(vkGetPhysicalDeviceMemoryProperties);
 VK_PROC(vkCreateDevice);
 VK_PROC(vkGetDeviceProcAddr);
 return nullptr;
}
