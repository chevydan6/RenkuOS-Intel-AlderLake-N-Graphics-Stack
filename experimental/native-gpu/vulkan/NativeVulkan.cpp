// SPDX-License-Identifier: MIT
#include <vulkan/vulkan.h>
#include "../RenderReport.h"
#include "../ResourceCommand.h"
#include "KnownSpirv.h"
#include "../render/draw/RcsDrawGenerated.h"
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
struct VkCommandPool_T;
struct VkQueue_T { VkDevice_T* device; };
struct VkDevice_T {
 VkPhysicalDevice physical;int fd;VkQueue_T queue;bool trace;
 VkShaderModule_T* shaders;VkDeviceMemory_T* memory;
 VkBuffer_T* buffers;VkImage_T* images;VkImageView_T* views;
 VkRenderPass_T* passes;VkFramebuffer_T* framebuffers;
 VkPipelineLayout_T* layouts;VkPipeline_T* pipelines;VkCommandPool_T* pools;
};
struct VkShaderModule_T { VkDevice_T* device; VkShaderModule_T* next; size_t bytes; uint32_t* code; };
struct VkDeviceMemory_T {
 VkDevice_T* device;VkDeviceMemory_T* next;
 uint32_t handle;uint64_t gpu;VkDeviceSize bytes;uint8_t* host;bool mapped;
};
struct VkBuffer_T {
 VkDevice_T* device;VkBuffer_T* next;VkDeviceSize bytes;VkDeviceMemory memory;
};
struct VkImage_T { VkDevice_T* device;VkImage_T* next;VkDeviceMemory memory; };
struct VkImageView_T { VkDevice_T* device;VkImageView_T* next;VkImage image; };
struct VkRenderPass_T { VkDevice_T* device;VkRenderPass_T* next; };
struct VkFramebuffer_T {
 VkDevice_T* device;VkFramebuffer_T* next;
 VkRenderPass pass;VkImageView view;
};
struct VkPipelineLayout_T { VkDevice_T* device;VkPipelineLayout_T* next; };
struct VkPipeline_T {
 VkDevice_T* device;VkPipeline_T* next;VkRenderPass pass;uint32_t shaderHandle;
};
struct VkCommandBuffer_T;
struct VkCommandPool_T {
 VkDevice_T* device;VkCommandPool_T* next;VkCommandBuffer_T* commands;
};
struct VkCommandBuffer_T {
 VkDevice_T* device;VkCommandPool_T* pool;VkCommandBuffer_T* next;
 VkRenderPass pass;VkFramebuffer framebuffer;VkPipeline pipeline;
 VkBuffer boundVertex;VkBuffer vertex;
 bool recording;bool inRenderPass;bool drawn;bool complete;bool invalid;
};

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
  free(device->pipelines);device->pipelines=next;
 }
 while(device->layouts){
  VkPipelineLayout_T* next=device->layouts->next;
  free(device->layouts);device->layouts=next;
 }
 while(device->pools){
  VkCommandPool_T* next=device->pools->next;
  while(device->pools->commands){
   VkCommandBuffer_T* command=device->pools->commands;
   device->pools->commands=command->next;free(command);
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

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateBuffer(VkDevice device,const VkBufferCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkBuffer* buffer)
{
 if(buffer)*buffer=VK_NULL_HANDLE;
 if(!device || !info || !buffer || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
  || info->sharingMode!=VK_SHARING_MODE_EXCLUSIVE || !info->size
  || info->size>16*4096 || info->usage!=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkBuffer_T* value=(VkBuffer_T*)calloc(1,sizeof(VkBuffer_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->bytes=info->size;
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
  || info->extent.width!=32 || info->extent.height!=32 || info->extent.depth!=1
  || info->mipLevels!=1 || info->arrayLayers!=1
  || info->samples!=VK_SAMPLE_COUNT_1_BIT || info->tiling!=VK_IMAGE_TILING_LINEAR
  || info->usage!=(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
  || info->sharingMode!=VK_SHARING_MODE_EXCLUSIVE
  || info->initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED)
  return VK_ERROR_FORMAT_NOT_SUPPORTED;
 VkImage_T* value=(VkImage_T*)calloc(1,sizeof(VkImage_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->images;device->images=value;
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
 requirements->size=4096;requirements->alignment=4096;
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
  || a.samples!=VK_SAMPLE_COUNT_1_BIT || a.loadOp!=VK_ATTACHMENT_LOAD_OP_DONT_CARE
  || a.storeOp!=VK_ATTACHMENT_STORE_OP_STORE
  || a.initialLayout!=VK_IMAGE_LAYOUT_UNDEFINED
  || a.finalLayout!=VK_IMAGE_LAYOUT_GENERAL
  || s.flags || s.pipelineBindPoint!=VK_PIPELINE_BIND_POINT_GRAPHICS
  || s.inputAttachmentCount || s.colorAttachmentCount!=1
  || !s.pColorAttachments || s.pColorAttachments[0].attachment!=0
  || s.pColorAttachments[0].layout!=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  || s.pResolveAttachments || s.pDepthStencilAttachment || s.preserveAttachmentCount)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkRenderPass_T* value=(VkRenderPass_T*)calloc(1,sizeof(VkRenderPass_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->passes;device->passes=value;
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
  || info->pAttachments[0]->device!=device || info->width!=32
  || info->height!=32 || info->layers!=1)return VK_ERROR_INITIALIZATION_FAILED;
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
  || !info->allocationSize || info->allocationSize>16*4096
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
  || memory->device!=device || buffer->memory || offset
  || memory->bytes<((buffer->bytes+4095)&~VkDeviceSize(4095)))
  return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 buffer->memory=memory;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkBindImageMemory(VkDevice device,VkImage image,VkDeviceMemory memory,
 VkDeviceSize offset)
{
 if(!device || !image || !memory || image->device!=device
  || memory->device!=device || image->memory || offset || memory->bytes<4096)
  return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 image->memory=memory;return VK_SUCCESS;
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
vkCreatePipelineLayout(VkDevice device,const VkPipelineLayoutCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkPipelineLayout* layout)
{
 if(layout)*layout=VK_NULL_HANDLE;
 if(!device || !info || !layout || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
  || info->setLayoutCount || info->pushConstantRangeCount)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkPipelineLayout_T* value=(VkPipelineLayout_T*)calloc(1,sizeof(VkPipelineLayout_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->layouts;device->layouts=value;
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
  && stage.module->bytes==bytes && !memcmp(stage.module->code,code,bytes)
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
 if(!KnownStage(device,p.pStages[0],VK_SHADER_STAGE_VERTEX_BIT,
   RenkuKnownSpirv::Vertex,sizeof(RenkuKnownSpirv::Vertex))
  || !KnownStage(device,p.pStages[1],VK_SHADER_STAGE_FRAGMENT_BIT,
   RenkuKnownSpirv::Fragment,sizeof(RenkuKnownSpirv::Fragment)))return false;
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
 if(viewport.x || viewport.y || viewport.width!=32 || viewport.height!=32
  || viewport.minDepth!=0 || viewport.maxDepth!=1
  || scissor.offset.x || scissor.offset.y || scissor.extent.width!=32
  || scissor.extent.height!=32)return false;
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

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateGraphicsPipelines(VkDevice device,VkPipelineCache cache,uint32_t count,
 const VkGraphicsPipelineCreateInfo* infos,const VkAllocationCallbacks* allocator,
 VkPipeline* pipelines)
{
 if(pipelines && count)for(uint32_t i=0;i<count;i++)pipelines[i]=VK_NULL_HANDLE;
 if(!device || cache || count!=1 || !infos || !pipelines || allocator
  || !KnownGraphicsPipeline(device,infos[0]))return VK_ERROR_FEATURE_NOT_PRESENT;
 VkPipeline_T* value=(VkPipeline_T*)calloc(1,sizeof(VkPipeline_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 native_gpu_resource_command c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_CREATE;
 c.size=4096;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
  free(value);return VK_ERROR_OUT_OF_DEVICE_MEMORY;
 }
 value->shaderHandle=c.handle;
 c={};c.version=1;c.action=NATIVE_GPU_RESOURCE_WRITE;
 c.handle=value->shaderHandle;c.bytes=sizeof(NativeGpu::RcsDrawData::Fs);
 memcpy(c.data,NativeGpu::RcsDrawData::Fs,c.bytes);
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0){
  native_gpu_resource_command destroy={};destroy.version=1;
  destroy.action=NATIVE_GPU_RESOURCE_DESTROY;destroy.handle=value->shaderHandle;
  ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&destroy,sizeof(destroy));
  free(value);return VK_ERROR_DEVICE_LOST;
 }
 value->device=device;value->pass=infos[0].renderPass;
 value->next=device->pipelines;device->pipelines=value;
 *pipelines=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyPipeline(VkDevice device,VkPipeline pipeline,const VkAllocationCallbacks*)
{
 if(!device || !pipeline)return;
 VkPipeline_T** current=&device->pipelines;
 while(*current && *current!=pipeline)current=&(*current)->next;
 if(!*current)return;
 native_gpu_resource_command c={};c.version=1;
 c.action=NATIVE_GPU_RESOURCE_DESTROY;c.handle=pipeline->shaderHandle;
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&c,sizeof(c))!=0)return;
 *current=pipeline->next;free(pipeline);
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkCreateCommandPool(VkDevice device,const VkCommandPoolCreateInfo* info,
 const VkAllocationCallbacks* allocator,VkCommandPool* pool)
{
 if(pool)*pool=VK_NULL_HANDLE;
 if(!device || !info || !pool || allocator || info->pNext || info->flags
  || info->sType!=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
  || info->queueFamilyIndex)return VK_ERROR_FEATURE_NOT_PRESENT;
 VkCommandPool_T* value=(VkCommandPool_T*)calloc(1,sizeof(VkCommandPool_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->next=device->pools;device->pools=value;
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
  free(pool->commands);pool->commands=next;
 }
 *current=pool->next;free(pool);
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
  || info->level!=VK_COMMAND_BUFFER_LEVEL_PRIMARY || info->commandBufferCount!=1)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkCommandBuffer_T* value=(VkCommandBuffer_T*)calloc(1,sizeof(VkCommandBuffer_T));
 if(!value)return VK_ERROR_OUT_OF_HOST_MEMORY;
 value->device=device;value->pool=info->commandPool;
 value->next=info->commandPool->commands;info->commandPool->commands=value;
 commands[0]=value;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkFreeCommandBuffers(VkDevice device,VkCommandPool pool,uint32_t count,
 const VkCommandBuffer* commands)
{
 if(!device || !pool || pool->device!=device || (count && !commands))return;
 for(uint32_t i=0;i<count;i++){
  VkCommandBuffer_T** current=&pool->commands;
  while(*current && *current!=commands[i])current=&(*current)->next;
  if(*current){VkCommandBuffer_T* value=*current;*current=value->next;free(value);}
 }
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkBeginCommandBuffer(VkCommandBuffer command,const VkCommandBufferBeginInfo* info)
{
 if(!command || !info || info->pNext || info->flags || info->pInheritanceInfo
  || info->sType!=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  || command->recording || command->complete)return VK_ERROR_INITIALIZATION_FAILED;
 command->pass=VK_NULL_HANDLE;command->framebuffer=VK_NULL_HANDLE;
 command->pipeline=VK_NULL_HANDLE;command->boundVertex=VK_NULL_HANDLE;
 command->vertex=VK_NULL_HANDLE;
 command->recording=true;command->inRenderPass=false;
 command->drawn=false;command->invalid=false;
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
  || info->renderArea.extent.width!=32 || info->renderArea.extent.height!=32
  || info->clearValueCount || contents!=VK_SUBPASS_CONTENTS_INLINE){
  command->invalid=true;return;
 }
 command->pass=info->renderPass;command->framebuffer=info->framebuffer;
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
  || buffers[0]->device!=command->device || !buffers[0]->memory || offsets[0]){
  command->invalid=true;return;
 }
 command->boundVertex=buffers[0];
}

extern "C" VKAPI_ATTR void VKAPI_CALL
vkCmdDraw(VkCommandBuffer command,uint32_t vertices,uint32_t instances,
 uint32_t firstVertex,uint32_t firstInstance)
{
 if(!command)return;
 if(!command->recording || !command->inRenderPass || !command->pipeline
  || !command->boundVertex || command->drawn || vertices!=3 || instances!=1
  || firstVertex || firstInstance){command->invalid=true;return;}
 command->vertex=command->boundVertex;command->drawn=true;
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
  || !command->drawn || command->invalid)return VK_ERROR_INITIALIZATION_FAILED;
 command->recording=false;command->complete=true;return VK_SUCCESS;
}

extern "C" VKAPI_ATTR VkResult VKAPI_CALL
vkQueueSubmit(VkQueue queue,uint32_t count,const VkSubmitInfo* submits,VkFence fence)
{
 if(!queue || !queue->device || count!=1 || !submits || fence)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 const VkSubmitInfo& submit=submits[0];
 if(submit.sType!=VK_STRUCTURE_TYPE_SUBMIT_INFO || submit.pNext
  || submit.waitSemaphoreCount || submit.signalSemaphoreCount
  || submit.commandBufferCount!=1 || !submit.pCommandBuffers)
  return VK_ERROR_FEATURE_NOT_PRESENT;
 VkCommandBuffer command=submit.pCommandBuffers[0];
 if(!command || command->device!=queue->device || !command->complete
  || !command->framebuffer || !command->pipeline || !command->vertex
  || !command->vertex->memory || !command->framebuffer->view
  || !command->framebuffer->view->image
  || !command->framebuffer->view->image->memory)return VK_ERROR_INITIALIZATION_FAILED;
 VkDevice device=queue->device;
 native_gpu_render_command draw={};draw.version=7;draw.mode=NATIVE_GPU_RENDER_DRAW;
 draw.variant=command->vertex->memory->handle;
 draw.reserved=command->pipeline->shaderHandle;
 int status=ioctl(device->fd,NATIVE_GPU_RENDER_IOCTL,&draw,sizeof(draw));
 const auto& r=draw.report;
 uint32_t hash=2166136261u;
 const uint32_t* pixels=(const uint32_t*)draw.output;
 for(unsigned i=0;i<1024;i++)hash=(hash^pixels[i])*16777619u;
 if(device->trace)fprintf(stderr,
  "{\"event\":\"vk_queue_submit\",\"ioctl\":%d,\"version\":%u,"
  "\"stage\":%u,\"sequence\":%u,\"observed\":%u,"
  "\"vertex_bo\":%u,\"shader_bo\":%u,\"image_bo\":%u,"
  "\"checked\":%u,\"matched\":%u,\"pixel_hash\":\"%08x\","
  "\"fault\":%u,\"engine_stage\":%u,\"csb_error\":%u}\n",
  status,r.version,r.stage,r.sequence,r.observed_sequence,
  draw.variant,draw.reserved,command->framebuffer->view->image->memory->handle,
  r.checked_pixels,r.matched_pixels,hash,r.fault,r.engine_stage,r.csb_error);
 if(status!=0 || r.status || r.version!=7 || r.stage!=NATIVE_GPU_RENDER_STAGE_COMPLETE
  || !r.known_profile || !r.owner_granted || !r.sequence
  || r.observed_sequence!=r.sequence || r.nonce!=r.observed_nonce
  || r.observed_complement!=~r.nonce || r.checked_pixels<500
  || r.matched_pixels!=r.checked_pixels || hash!=r.target_hash
  || r.fault || r.engine_stage!=4 || r.engine_error || r.csb_error)
  return VK_ERROR_DEVICE_LOST;
 native_gpu_resource_command write={};write.version=1;
 write.action=NATIVE_GPU_RESOURCE_WRITE;
 write.handle=command->framebuffer->view->image->memory->handle;
 write.bytes=sizeof(draw.output);memcpy(write.data,draw.output,write.bytes);
 if(ioctl(device->fd,NATIVE_GPU_RESOURCE_IOCTL,&write,sizeof(write))!=0)
  return VK_ERROR_DEVICE_LOST;
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
 VK_PROC(vkCreatePipelineLayout);
 VK_PROC(vkDestroyPipelineLayout);
 VK_PROC(vkCreateGraphicsPipelines);
 VK_PROC(vkDestroyPipeline);
 VK_PROC(vkCreateCommandPool);
 VK_PROC(vkDestroyCommandPool);
 VK_PROC(vkAllocateCommandBuffers);
 VK_PROC(vkFreeCommandBuffers);
 VK_PROC(vkBeginCommandBuffer);
 VK_PROC(vkCmdBeginRenderPass);
 VK_PROC(vkCmdBindPipeline);
 VK_PROC(vkCmdBindVertexBuffers);
 VK_PROC(vkCmdDraw);
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
 VK_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
 VK_PROC(vkGetPhysicalDeviceMemoryProperties);
 VK_PROC(vkCreateDevice);
 VK_PROC(vkGetDeviceProcAddr);
 return nullptr;
}
