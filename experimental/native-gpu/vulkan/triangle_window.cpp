// SPDX-License-Identifier: MIT
// Present one compiler-backed native Vulkan N100 triangle through Haiku BBitmap.
#include <vulkan/vulkan.h>
#include "KnownSpirv.h"
#include "../PresentationFrame.h"
#include <Application.h>
#include <Bitmap.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>
#include <Window.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"BGRX presentation requires little endian");
#define REQUIRE(condition) do {if(!(condition)){ \
 fprintf(stderr,"Vulkan window failed at line %d\n",__LINE__);return false; \
}} while(0)

static bool RenderTriangle(uint32_t* pixels)
{
 if(!pixels)return false;
 VkApplicationInfo app={};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
 app.apiVersion=VK_API_VERSION_1_0;
 VkInstanceCreateInfo instanceInfo={};instanceInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
 instanceInfo.pApplicationInfo=&app;
 VkInstance instance=VK_NULL_HANDLE;
 REQUIRE(vkCreateInstance(&instanceInfo,nullptr,&instance)==VK_SUCCESS);
 uint32_t count=1;VkPhysicalDevice physical=VK_NULL_HANDLE;
 REQUIRE(vkEnumeratePhysicalDevices(instance,&count,&physical)==VK_SUCCESS
  && count==1 && physical);
 VkPhysicalDeviceProperties properties={};vkGetPhysicalDeviceProperties(physical,&properties);
 REQUIRE(properties.vendorID==0x8086 && properties.deviceID==0x46d1);
 float priority=1;
 VkDeviceQueueCreateInfo queueInfo={};queueInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
 queueInfo.queueFamilyIndex=0;queueInfo.queueCount=1;queueInfo.pQueuePriorities=&priority;
 VkDeviceCreateInfo deviceInfo={};deviceInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
 deviceInfo.queueCreateInfoCount=1;deviceInfo.pQueueCreateInfos=&queueInfo;
 VkDevice device=VK_NULL_HANDLE;
 REQUIRE(vkCreateDevice(physical,&deviceInfo,nullptr,&device)==VK_SUCCESS);
 VkQueue queue=VK_NULL_HANDLE;vkGetDeviceQueue(device,0,0,&queue);REQUIRE(queue);

 VkShaderModuleCreateInfo moduleInfo={};moduleInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 moduleInfo.pCode=RenkuKnownSpirv::Vertex;moduleInfo.codeSize=sizeof(RenkuKnownSpirv::Vertex);
 VkShaderModule vs=VK_NULL_HANDLE,fs=VK_NULL_HANDLE;
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&vs)==VK_SUCCESS);
 moduleInfo.pCode=RenkuKnownSpirv::Fragment;
 moduleInfo.codeSize=sizeof(RenkuKnownSpirv::Fragment);
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&fs)==VK_SUCCESS);

 VkBufferCreateInfo bufferInfo={};bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
 bufferInfo.size=48;bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkBuffer vertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&vertex)==VK_SUCCESS);
 VkMemoryRequirements requirement={};vkGetBufferMemoryRequirements(device,vertex,&requirement);
 REQUIRE(requirement.size==4096 && requirement.memoryTypeBits==1);
 VkMemoryAllocateInfo allocation={};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
 allocation.allocationSize=4096;allocation.memoryTypeIndex=0;
 VkDeviceMemory vertexMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&vertexMemory)==VK_SUCCESS);
 REQUIRE(vkBindBufferMemory(device,vertex,vertexMemory,0)==VK_SUCCESS);
 const uint32_t position[12]={
  0xbf400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0xbf400000,0x3f000000,0x3f800000,
  0x3f400000,0x3f400000,0x3f000000,0x3f800000};
 void* mapped=nullptr;
 REQUIRE(vkMapMemory(device,vertexMemory,0,48,0,&mapped)==VK_SUCCESS && mapped);
 memcpy(mapped,position,sizeof(position));
 VkMappedMemoryRange range={};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
 range.memory=vertexMemory;range.size=48;
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS);
 vkUnmapMemory(device,vertexMemory);

 VkImageCreateInfo imageInfo={};imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
 imageInfo.imageType=VK_IMAGE_TYPE_2D;imageInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 imageInfo.extent={32,32,1};imageInfo.mipLevels=1;imageInfo.arrayLayers=1;
 imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;imageInfo.tiling=VK_IMAGE_TILING_LINEAR;
 imageInfo.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
 imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkImage image=VK_NULL_HANDLE;
 REQUIRE(vkCreateImage(device,&imageInfo,nullptr,&image)==VK_SUCCESS);
 vkGetImageMemoryRequirements(device,image,&requirement);
 REQUIRE(requirement.size==4096 && requirement.memoryTypeBits==1);
 VkDeviceMemory imageMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&imageMemory)==VK_SUCCESS);
 REQUIRE(vkBindImageMemory(device,image,imageMemory,0)==VK_SUCCESS);
 VkImageViewCreateInfo viewInfo={};viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
 viewInfo.image=image;viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
 viewInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
 viewInfo.subresourceRange.levelCount=1;viewInfo.subresourceRange.layerCount=1;
 VkImageView view=VK_NULL_HANDLE;
 REQUIRE(vkCreateImageView(device,&viewInfo,nullptr,&view)==VK_SUCCESS);
 VkAttachmentDescription attachment={};attachment.format=VK_FORMAT_B8G8R8A8_UNORM;
 attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
 attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
 attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_GENERAL;
 VkAttachmentReference color={};color.attachment=0;
 color.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
 VkSubpassDescription subpass={};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
 subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
 VkRenderPassCreateInfo passInfo={};passInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
 passInfo.attachmentCount=1;passInfo.pAttachments=&attachment;
 passInfo.subpassCount=1;passInfo.pSubpasses=&subpass;
 VkRenderPass pass=VK_NULL_HANDLE;
 REQUIRE(vkCreateRenderPass(device,&passInfo,nullptr,&pass)==VK_SUCCESS);
 VkFramebufferCreateInfo framebufferInfo={};framebufferInfo.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
 framebufferInfo.renderPass=pass;framebufferInfo.attachmentCount=1;
 framebufferInfo.pAttachments=&view;framebufferInfo.width=32;framebufferInfo.height=32;
 framebufferInfo.layers=1;
 VkFramebuffer framebuffer=VK_NULL_HANDLE;
 REQUIRE(vkCreateFramebuffer(device,&framebufferInfo,nullptr,&framebuffer)==VK_SUCCESS);

 VkPipelineLayoutCreateInfo layoutInfo={};layoutInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
 VkPipelineLayout layout=VK_NULL_HANDLE;
 REQUIRE(vkCreatePipelineLayout(device,&layoutInfo,nullptr,&layout)==VK_SUCCESS);
 VkPipelineShaderStageCreateInfo stages[2]={};
 for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage.pName="main";}
 stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;
 stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;
 VkVertexInputBindingDescription binding={0,16,VK_VERTEX_INPUT_RATE_VERTEX};
 VkVertexInputAttributeDescription attribute={0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
 VkPipelineVertexInputStateCreateInfo input={};input.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
 input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
 input.vertexAttributeDescriptionCount=1;input.pVertexAttributeDescriptions=&attribute;
 VkPipelineInputAssemblyStateCreateInfo assembly={};assembly.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
 assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 VkViewport viewport={0,0,32,32,0,1};VkRect2D scissor={{0,0},{32,32}};
 VkPipelineViewportStateCreateInfo viewportState={};viewportState.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
 viewportState.viewportCount=1;viewportState.pViewports=&viewport;
 viewportState.scissorCount=1;viewportState.pScissors=&scissor;
 VkPipelineRasterizationStateCreateInfo raster={};raster.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
 raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;
 raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo samples={};samples.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
 samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineColorBlendAttachmentState blendAttachment={};blendAttachment.colorWriteMask=0xf;
 VkPipelineColorBlendStateCreateInfo blend={};blend.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
 blend.attachmentCount=1;blend.pAttachments=&blendAttachment;
 VkGraphicsPipelineCreateInfo pipelineInfo={};pipelineInfo.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
 pipelineInfo.stageCount=2;pipelineInfo.pStages=stages;pipelineInfo.pVertexInputState=&input;
 pipelineInfo.pInputAssemblyState=&assembly;pipelineInfo.pViewportState=&viewportState;
 pipelineInfo.pRasterizationState=&raster;pipelineInfo.pMultisampleState=&samples;
 pipelineInfo.pColorBlendState=&blend;pipelineInfo.layout=layout;pipelineInfo.renderPass=pass;
 VkPipeline pipeline=VK_NULL_HANDLE;
 REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&pipeline)
  ==VK_SUCCESS && pipeline);
 vkDestroyShaderModule(device,fs,nullptr);vkDestroyShaderModule(device,vs,nullptr);

 VkCommandPoolCreateInfo poolInfo={};poolInfo.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
 VkCommandPool pool=VK_NULL_HANDLE;
 REQUIRE(vkCreateCommandPool(device,&poolInfo,nullptr,&pool)==VK_SUCCESS);
 VkCommandBufferAllocateInfo commandInfo={};commandInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
 commandInfo.commandPool=pool;commandInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
 commandInfo.commandBufferCount=1;
 VkCommandBuffer command=VK_NULL_HANDLE;
 REQUIRE(vkAllocateCommandBuffers(device,&commandInfo,&command)==VK_SUCCESS);
 VkCommandBufferBeginInfo begin={};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 REQUIRE(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS);
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=pass;render.framebuffer=framebuffer;render.renderArea={{0,0},{32,32}};
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
 VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);
 vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 REQUIRE(vkEndCommandBuffer(command)==VK_SUCCESS);
 VkFenceCreateInfo fenceInfo={};fenceInfo.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
 VkFence fence=VK_NULL_HANDLE;
 REQUIRE(vkCreateFence(device,&fenceInfo,nullptr,&fence)==VK_SUCCESS);
 VkSubmitInfo submit={};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
 submit.commandBufferCount=1;submit.pCommandBuffers=&command;
 REQUIRE(vkQueueSubmit(queue,1,&submit,fence)==VK_SUCCESS);
 REQUIRE(vkWaitForFences(device,1,&fence,VK_TRUE,1000000000ULL)==VK_SUCCESS);
 mapped=nullptr;REQUIRE(vkMapMemory(device,imageMemory,0,4096,0,&mapped)==VK_SUCCESS && mapped);
 range.memory=imageMemory;range.size=4096;
 REQUIRE(vkInvalidateMappedMemoryRanges(device,1,&range)==VK_SUCCESS);
 memcpy(pixels,mapped,4096);vkUnmapMemory(device,imageMemory);
 uint32_t hash=2166136261u;
 for(unsigned i=0;i<1024;i++)hash=(hash^pixels[i])*16777619u;
 REQUIRE(hash==0x28616d2d && pixels[10*32+20]==0xffff4000
  && pixels[20*32+10]==0x19537bad);
 printf("vulkan_render_complete hash=%08x color=%08x\n",hash,pixels[10*32+20]);fflush(stdout);
 vkDestroyFence(device,fence,nullptr);vkFreeCommandBuffers(device,pool,1,&command);
 vkDestroyCommandPool(device,pool,nullptr);vkDestroyPipeline(device,pipeline,nullptr);
 vkDestroyPipelineLayout(device,layout,nullptr);vkDestroyFramebuffer(device,framebuffer,nullptr);
 vkDestroyRenderPass(device,pass,nullptr);vkDestroyImageView(device,view,nullptr);
 vkDestroyImage(device,image,nullptr);vkFreeMemory(device,imageMemory,nullptr);
 vkDestroyBuffer(device,vertex,nullptr);vkFreeMemory(device,vertexMemory,nullptr);
 vkDestroyDevice(device,nullptr);vkDestroyInstance(instance,nullptr);
 return true;
}

class TriangleView:public BView {
 BBitmap fBitmap;
 bool fReady=false;
public:
 explicit TriangleView(const uint32_t* pixels):
  BView(BRect(0,0,639,479),"Vulkan pixels",B_FOLLOW_ALL,B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE),
  fBitmap(BRect(0,0,31,31),B_RGB32)
 {
  SetViewColor(245,245,245);
  NativeGpu::FrameDescription shape={32,32,128,NativeGpu::OpaqueBgrx32};
  fReady=fBitmap.InitCheck()==B_OK && NativeGpu::CopyFrameRows(shape,pixels,4096,
   fBitmap.Bits(),size_t(fBitmap.BitsLength()),fBitmap.BytesPerRow());
 }
 bool Ready() const{return fReady;}
 void Draw(BRect) override {
  if(!fReady)return;
  SetDrawingMode(B_OP_COPY);SetHighColor(20,20,20);
  DrawString("Native Vulkan on Intel N100",BPoint(20,28));
  DrawBitmap(&fBitmap,fBitmap.Bounds(),BRect(60,70,443,453));
  Sync();
  printf("vulkan_presented bitmap_stride=%ld\n",(long)fBitmap.BytesPerRow());fflush(stdout);
 }
};

class TriangleWindow:public BWindow {
 bool fReady=false;
public:
 explicit TriangleWindow(const uint32_t* pixels):
  BWindow(BRect(100,100,739,579),"RenkuOS native Vulkan triangle",B_TITLED_WINDOW,0)
 {
  TriangleView* view=new TriangleView(pixels);
  if(!view->Ready()){delete view;fprintf(stderr,"bitmap setup failed\n");return;}
  AddChild(view);fReady=true;
  Show();
 }
 bool Ready() const{return fReady;}
 bool QuitRequested() override {be_app->PostMessage(B_QUIT_REQUESTED);return true;}
};

int main(int argc,char** argv)
{
 uint32_t closeMs=0;
 if(argc==3 && !strcmp(argv[1],"--close-after-ms")){
  char* end=nullptr;unsigned long value=strtoul(argv[2],&end,10);
  if(!*argv[2] || *end || !value || value>60000)return 2;
  closeMs=uint32_t(value);
 }else if(argc!=1)return 2;
 uint32_t pixels[1024]={};
 if(!RenderTriangle(pixels))return 1;
 BApplication application("application/x-vnd.renku-native-vulkan-triangle");
 TriangleWindow* window=new TriangleWindow(pixels);
 if(!window->Ready())return 3;
 BMessage quit(B_QUIT_REQUESTED);
 BMessageRunner* timer=closeMs?new BMessageRunner(BMessenger(window),quit,
  bigtime_t(closeMs)*1000,1):nullptr;
 if(timer && timer->InitCheck()!=B_OK){delete timer;return 3;}
 application.Run();delete timer;
 puts("vulkan_window_closed");return 0;
}
