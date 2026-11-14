// SPDX-License-Identifier: MIT
// One bounded Vulkan triangle through the native N100 owner and image BO.
#include <vulkan/vulkan.h>
#include "KnownSpirv.h"
#include "RgbSpirvFixture.h"
#include "DepthSpirvFixture.h"
#include "RotateSpirvFixture.h"
#include "FragCoordSpirvFixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(RenkuKnownSpirv::Vertex[1]==0x00010000
 && RenkuKnownSpirv::Fragment[1]==0x00010000
 && RenkuKnownSpirv::FragmentHalf[1]==0x00010000,
 "Advertised Vulkan 1.0 requires the SPIR-V 1.0 fixture");

#define REQUIRE(condition,code) do { if(!(condition)) { \
 fprintf(stderr,"Vulkan triangle failed at %s:%d (code %d)\n",__FILE__,__LINE__,code); \
 return code; } } while(0)

int main(int argc,char** argv)
{
 if((argc<2 || argc>5) || argv[1][0]<'0' || argv[1][0]>'3' || argv[1][1]
  || (argc>=3 && strcmp(argv[2],"original") && strcmp(argv[2],"half")
   && strcmp(argv[2],"green75") && strcmp(argv[2],"green90")
   && strcmp(argv[2],"rgb") && strcmp(argv[2],"fragcoord")
   && strcmp(argv[2],"fragcoord32") && strcmp(argv[2],"fragcoord128"))
  || (argc==5 && strcmp(argv[4],"batch")
   && strcmp(argv[4],"named") && strcmp(argv[4],"depth")
   && strcmp(argv[4],"rotate") && strcmp(argv[4],"dual")
   && strcmp(argv[4],"multipass") && strcmp(argv[4],"coloronly")))return 40;
 unsigned shape=unsigned(argv[1][0]-'0');
 const bool large=getenv("RENKU_VK_TARGET_64")!=nullptr;
 const uint32_t dimension=large?64:32;
 const uint32_t imageBytes=dimension*dimension*4;
 bool half=argc>=3 && !strcmp(argv[2],"half");
 bool green75=argc>=3 && !strcmp(argv[2],"green75");
 bool green90=argc>=3 && !strcmp(argv[2],"green90");
 bool rgb=argc>=3 && !strcmp(argv[2],"rgb");
 unsigned fragcoordDenominator=argc>=3 && !strcmp(argv[2],"fragcoord32")?32:
  argc>=3 && !strcmp(argv[2],"fragcoord128")?128:
  argc>=3 && !strcmp(argv[2],"fragcoord")?64:0;
 bool fragcoord=fragcoordDenominator!=0;
 char* repeatEnd=nullptr;
 unsigned long repeats=argc>=4?strtoul(argv[3],&repeatEnd,10):1;
 bool batch=argc==5 && !strcmp(argv[4],"batch");
 bool named=argc==5 && !strcmp(argv[4],"named");
 bool depth=argc==5 && !strcmp(argv[4],"depth");
 bool rotate=argc==5 && !strcmp(argv[4],"rotate");
 bool dual=argc==5 && !strcmp(argv[4],"dual");
 bool multiPass=argc==5 && !strcmp(argv[4],"multipass");
 bool colorOnly=argc==5 && !strcmp(argv[4],"coloronly");
 bool twoPipelines=dual || multiPass;
 if(large && (half || green75 || green90 || named || twoPipelines
  || batch || (depth && shape!=0)))return 40;
 if(fragcoord && (!large || shape!=0 || rotate || depth || twoPipelines
  || batch || named || colorOnly))return 40;
 if(rotate && (shape!=0 || (argc>=3 && strcmp(argv[2],"original"))))return 40;
 if(twoPipelines && (shape!=0 || argc!=5 || strcmp(argv[2],"original")
  || repeats!=2))return 40;
 if(!repeats || repeats>64 || (argc>=4 && (!repeatEnd || *repeatEnd))
  || (batch && repeats!=2))return 40;
 uint32_t alternateCode[sizeof(RenkuKnownSpirv::Fragment)/4];
 memcpy(alternateCode,RenkuKnownSpirv::Fragment,sizeof(alternateCode));
 bool alternateChanged=false;
 for(uint32_t& word:alternateCode)
  if(word==0x3e800000){word=green90?0x3f666666:0x3f400000;alternateChanged=true;break;}
 REQUIRE(alternateChanged,46);
 auto rgbCode=rgb?RgbSpirvFixture(0x3f000000,0x3f400000,0x3e800000,true):
  std::vector<uint32_t>{};
 REQUIRE(!rgb || !rgbCode.empty(),51);
 uint32_t fragcoordCode[sizeof(RenkuFragCoordSpirv::Fragment)/4];
 memcpy(fragcoordCode,RenkuFragCoordSpirv::Fragment,sizeof(fragcoordCode));
 if(fragcoord && fragcoordDenominator!=64){
  bool changed=false;
  const uint32_t scale=fragcoordDenominator==32?0x3d000000:0x3c000000;
  for(uint32_t& word:fragcoordCode)
   if(word==0x3c800000){word=scale;changed=true;break;}
  REQUIRE(changed,56);
 }
 std::vector<uint32_t> namedVertex;
 if(named){
  namedVertex.assign(std::begin(RenkuKnownSpirv::Vertex),
   std::end(RenkuKnownSpirv::Vertex));
  namedVertex.insert(namedVertex.begin()+17,
   {0x00040005u,1u,0x72746e65u,0x00000079u}); // OpName %main "entry"
 }
 VkApplicationInfo app={};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
 app.apiVersion=VK_API_VERSION_1_0;
 VkInstanceCreateInfo instanceInfo={};instanceInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
 instanceInfo.pApplicationInfo=&app;
 VkInstance instance=VK_NULL_HANDLE;
 REQUIRE(vkCreateInstance(&instanceInfo,nullptr,&instance)==VK_SUCCESS,1);
 uint32_t count=1;VkPhysicalDevice physical=VK_NULL_HANDLE;
 REQUIRE(vkEnumeratePhysicalDevices(instance,&count,&physical)==VK_SUCCESS
  && count==1 && physical,2);
 VkFormatProperties format={};
 vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_B8G8R8A8_UNORM,&format);
 REQUIRE(format.linearTilingFeatures==(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
  |VK_FORMAT_FEATURE_TRANSFER_SRC_BIT),56);
 vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_R32G32B32A32_SFLOAT,&format);
 REQUIRE(format.bufferFeatures==VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,57);
 VkImageFormatProperties imageFormat={};
 REQUIRE(vkGetPhysicalDeviceImageFormatProperties(physical,VK_FORMAT_B8G8R8A8_UNORM,
  VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_LINEAR,
  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,0,
  &imageFormat)==VK_SUCCESS && imageFormat.maxExtent.width==64
  && imageFormat.maxExtent.height==64 && imageFormat.maxResourceSize==16384,58);
 float priority=1;
 VkDeviceQueueCreateInfo queueInfo={};queueInfo.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
 queueInfo.queueFamilyIndex=0;queueInfo.queueCount=1;queueInfo.pQueuePriorities=&priority;
 VkDeviceCreateInfo deviceInfo={};deviceInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
 deviceInfo.queueCreateInfoCount=1;deviceInfo.pQueueCreateInfos=&queueInfo;
 VkDevice device=VK_NULL_HANDLE;
 REQUIRE(vkCreateDevice(physical,&deviceInfo,nullptr,&device)==VK_SUCCESS,3);
 VkQueue queue=VK_NULL_HANDLE;vkGetDeviceQueue(device,0,0,&queue);
 REQUIRE(queue,4);

 VkShaderModuleCreateInfo moduleInfo={};moduleInfo.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 moduleInfo.pCode=rotate?RenkuRotateSpirv::Vertex:
  depth?RenkuDepthSpirv::Vertex:
  named?namedVertex.data():RenkuKnownSpirv::Vertex;
 moduleInfo.codeSize=rotate?sizeof(RenkuRotateSpirv::Vertex):
  depth?sizeof(RenkuDepthSpirv::Vertex):
  named?namedVertex.size()*4:sizeof(RenkuKnownSpirv::Vertex);
 VkShaderModule vs=VK_NULL_HANDLE,fs=VK_NULL_HANDLE;
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&vs)==VK_SUCCESS,5);
 moduleInfo.pCode=fragcoord?fragcoordCode:
  rgb?rgbCode.data():(green75 || green90)?alternateCode:
  half?RenkuKnownSpirv::FragmentHalf:RenkuKnownSpirv::Fragment;
 moduleInfo.codeSize=fragcoord?sizeof(RenkuFragCoordSpirv::Fragment):
  rgb?rgbCode.size()*4:
  half?sizeof(RenkuKnownSpirv::FragmentHalf):sizeof(RenkuKnownSpirv::Fragment);
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&fs)==VK_SUCCESS,6);

 VkBufferCreateInfo bufferInfo={};bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
 bufferInfo.size=48;bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkBuffer vertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&vertex)==VK_SUCCESS,7);
 VkMemoryRequirements requirement={};vkGetBufferMemoryRequirements(device,vertex,&requirement);
 REQUIRE(requirement.size==4096 && requirement.memoryTypeBits==1,8);
 VkMemoryAllocateInfo allocation={};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
 allocation.memoryTypeIndex=0;allocation.allocationSize=requirement.size;
 VkDeviceMemory vertexMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&vertexMemory)==VK_SUCCESS,9);
 REQUIRE(vkBindBufferMemory(device,vertex,vertexMemory,0)==VK_SUCCESS,10);
 const uint32_t positions[4][12]={
  {0xbf400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0x3f400000,0x3f000000,0x3f800000},
  {0xbf300000,0xbf300000,0x3f000000,0x3f800000,
   0x3f300000,0xbf300000,0x3f000000,0x3f800000,
   0x00000000,0x3f300000,0x3f000000,0x3f800000},
  {0xbf400000,0x3f000000,0x3f000000,0x3f800000,
   0x00000000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0x3f000000,0x3f000000,0x3f800000},
  {0xbf000000,0xbf000000,0x3f000000,0x3f800000,
   0x3f000000,0xbf000000,0x3f000000,0x3f800000,
   0x00000000,0x3f000000,0x3f000000,0x3f800000}};
 void* mapped=nullptr;
 REQUIRE(vkMapMemory(device,vertexMemory,0,48,0,&mapped)==VK_SUCCESS && mapped,11);
 memcpy(mapped,positions[shape],48);
 VkMappedMemoryRange range={};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
 range.memory=vertexMemory;range.size=48;
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,12);
 vkUnmapMemory(device,vertexMemory);
 VkBuffer laterVertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&laterVertex)==VK_SUCCESS,41);
 VkDeviceMemory laterMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&laterMemory)==VK_SUCCESS,42);
 REQUIRE(vkBindBufferMemory(device,laterVertex,laterMemory,0)==VK_SUCCESS,43);
 mapped=nullptr;
 REQUIRE(vkMapMemory(device,laterMemory,0,48,0,&mapped)==VK_SUCCESS && mapped,44);
 memcpy(mapped,positions[(shape+1)%4],48);
 range.memory=laterMemory;
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,45);
 vkUnmapMemory(device,laterMemory);

 VkImageCreateInfo imageInfo={};imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
 imageInfo.imageType=VK_IMAGE_TYPE_2D;imageInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 imageInfo.extent={dimension,dimension,1};imageInfo.mipLevels=1;imageInfo.arrayLayers=1;
 imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;imageInfo.tiling=VK_IMAGE_TILING_LINEAR;
 imageInfo.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
  |(colorOnly?0:VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
 imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkImage image=VK_NULL_HANDLE;
 REQUIRE(vkCreateImage(device,&imageInfo,nullptr,&image)==VK_SUCCESS,13);
 vkGetImageMemoryRequirements(device,image,&requirement);
 REQUIRE(requirement.size==imageBytes && requirement.memoryTypeBits==1,14);
 allocation.allocationSize=requirement.size;
 VkDeviceMemory imageMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&imageMemory)==VK_SUCCESS,15);
 REQUIRE(vkBindImageMemory(device,image,imageMemory,0)==VK_SUCCESS,16);
 VkImageViewCreateInfo viewInfo={};viewInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
 viewInfo.image=image;viewInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
 viewInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
 viewInfo.subresourceRange.levelCount=1;viewInfo.subresourceRange.layerCount=1;
 VkImageView view=VK_NULL_HANDLE;
 REQUIRE(vkCreateImageView(device,&viewInfo,nullptr,&view)==VK_SUCCESS,17);
 VkAttachmentDescription attachment={};attachment.format=VK_FORMAT_B8G8R8A8_UNORM;
 attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
 attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
 attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
 attachment.finalLayout=VK_IMAGE_LAYOUT_GENERAL;
 VkAttachmentReference color={};color.attachment=0;
 color.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
 VkSubpassDescription subpass={};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
 subpass.colorAttachmentCount=1;subpass.pColorAttachments=&color;
 VkRenderPassCreateInfo passInfo={};passInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
 passInfo.attachmentCount=1;passInfo.pAttachments=&attachment;
 passInfo.subpassCount=1;passInfo.pSubpasses=&subpass;
 VkRenderPass pass=VK_NULL_HANDLE;
 REQUIRE(vkCreateRenderPass(device,&passInfo,nullptr,&pass)==VK_SUCCESS,18);
 VkFramebufferCreateInfo fbInfo={};fbInfo.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
 fbInfo.renderPass=pass;fbInfo.attachmentCount=1;fbInfo.pAttachments=&view;
 fbInfo.width=dimension;fbInfo.height=dimension;fbInfo.layers=1;
 VkFramebuffer framebuffer=VK_NULL_HANDLE;
 REQUIRE(vkCreateFramebuffer(device,&fbInfo,nullptr,&framebuffer)==VK_SUCCESS,19);

 VkPipelineLayoutCreateInfo layoutInfo={};layoutInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
 VkPipelineLayout layout=VK_NULL_HANDLE;
 REQUIRE(vkCreatePipelineLayout(device,&layoutInfo,nullptr,&layout)==VK_SUCCESS,20);
 VkPipelineShaderStageCreateInfo stages[2]={};
 for(unsigned i=0;i<2;i++){stages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[i].pName="main";}
 stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;
 stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;
 VkVertexInputBindingDescription binding={0,16,VK_VERTEX_INPUT_RATE_VERTEX};
 VkVertexInputAttributeDescription attribute={0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
 VkPipelineVertexInputStateCreateInfo input={};input.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
 input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
 input.vertexAttributeDescriptionCount=1;input.pVertexAttributeDescriptions=&attribute;
 VkPipelineInputAssemblyStateCreateInfo assembly={};assembly.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
 assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 VkViewport viewport={0,0,float(dimension),float(dimension),0,1};
 VkRect2D scissor={{0,0},{dimension,dimension}};
 VkPipelineViewportStateCreateInfo viewportState={};viewportState.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
 viewportState.viewportCount=1;viewportState.pViewports=&viewport;
 viewportState.scissorCount=1;viewportState.pScissors=&scissor;
 VkPipelineRasterizationStateCreateInfo raster={};raster.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
 raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;
 raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo multisample={};multisample.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
 multisample.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineColorBlendAttachmentState blendAttachment={};
 blendAttachment.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT
  |VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
 VkPipelineColorBlendStateCreateInfo blend={};blend.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
 blend.attachmentCount=1;blend.pAttachments=&blendAttachment;
 VkGraphicsPipelineCreateInfo pipelineInfo={};pipelineInfo.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
 pipelineInfo.stageCount=2;pipelineInfo.pStages=stages;
 pipelineInfo.pVertexInputState=&input;pipelineInfo.pInputAssemblyState=&assembly;
 pipelineInfo.pViewportState=&viewportState;pipelineInfo.pRasterizationState=&raster;
 pipelineInfo.pMultisampleState=&multisample;pipelineInfo.pColorBlendState=&blend;
 pipelineInfo.layout=layout;pipelineInfo.renderPass=pass;
 uint32_t alteredFs[sizeof(RenkuKnownSpirv::Fragment)/sizeof(uint32_t)]={};
 memcpy(alteredFs,RenkuKnownSpirv::Fragment,sizeof(alteredFs));
 bool changed=false;
 for(unsigned i=0;i<sizeof(alteredFs)/sizeof(alteredFs[0]);i++)
  if(alteredFs[i]==0x3e800000){alteredFs[i]=0x3f800000;changed=true;break;}
 REQUIRE(changed,31);
 moduleInfo.pCode=alteredFs;moduleInfo.codeSize=sizeof(alteredFs);
 VkShaderModule alternate=VK_NULL_HANDLE;
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&alternate)==VK_SUCCESS,32);
 stages[1].module=alternate;
 VkPipeline rejected=VK_NULL_HANDLE;
 REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&rejected)
  ==VK_ERROR_FEATURE_NOT_PRESENT && !rejected,33);
 vkDestroyShaderModule(device,alternate,nullptr);
 stages[1].module=fs;
 VkShaderModule halfFs=VK_NULL_HANDLE;
 VkPipelineShaderStageCreateInfo secondStages[2]={stages[0],stages[1]};
 VkGraphicsPipelineCreateInfo pipelineInfos[2]={pipelineInfo,pipelineInfo};
 if(twoPipelines){
  moduleInfo.pCode=RenkuKnownSpirv::FragmentHalf;
  moduleInfo.codeSize=sizeof(RenkuKnownSpirv::FragmentHalf);
  REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&halfFs)==VK_SUCCESS,56);
  secondStages[1].module=halfFs;
  pipelineInfos[1].pStages=secondStages;
  if(dual){
   VkGraphicsPipelineCreateInfo overflow[17]={};
   VkPipeline rolledBack[17]={};
   for(unsigned i=0;i<17;i++)overflow[i]=pipelineInfo;
   REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,17,
    overflow,nullptr,rolledBack)==VK_ERROR_OUT_OF_DEVICE_MEMORY,57);
   for(VkPipeline prior:rolledBack)REQUIRE(!prior,58);
   VkPipeline filled[13]={};
   REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,13,
    overflow,nullptr,filled)==VK_SUCCESS,59);
   for(VkPipeline prior:filled){
    REQUIRE(prior,60);
    vkDestroyPipeline(device,prior,nullptr);
   }
  }
 }
 VkPipeline pipelines[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,twoPipelines?2:1,
   pipelineInfos,nullptr,pipelines)==VK_SUCCESS && pipelines[0]
  && (!twoPipelines || pipelines[1]),21);
 VkPipeline pipeline=pipelines[0],secondPipeline=pipelines[1];
 if(halfFs)vkDestroyShaderModule(device,halfFs,nullptr);
 vkDestroyShaderModule(device,fs,nullptr);vkDestroyShaderModule(device,vs,nullptr);

 VkCommandPoolCreateInfo poolInfo={};poolInfo.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
 poolInfo.queueFamilyIndex=0;
 VkCommandPool pool=VK_NULL_HANDLE;
 REQUIRE(vkCreateCommandPool(device,&poolInfo,nullptr,&pool)==VK_SUCCESS,22);
 VkCommandBufferAllocateInfo commandInfo={};commandInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
 commandInfo.commandPool=pool;commandInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
 commandInfo.commandBufferCount=(batch || dual)?2:1;
 VkCommandBuffer commands[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 REQUIRE(vkAllocateCommandBuffers(device,&commandInfo,commands)==VK_SUCCESS
  && commands[0] && (!(batch || dual) || (commands[1] && commands[1]!=commands[0])),23);
 VkCommandBuffer command=commands[0];
 VkCommandBufferBeginInfo begin={};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 REQUIRE(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS,24);
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=pass;render.framebuffer=framebuffer;
 render.renderArea={{0,0},{dimension,dimension}};
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
 VkDeviceSize offset=0;vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);
 vkCmdDraw(command,3,1,0,0);
 vkCmdBindVertexBuffers(command,0,1,&laterVertex,&offset);
 vkCmdEndRenderPass(command);
 if(multiPass){
  vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,secondPipeline);
  vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);
  vkCmdDraw(command,3,1,0,0);
  vkCmdEndRenderPass(command);
 }
 REQUIRE(vkEndCommandBuffer(command)==VK_SUCCESS,25);
 VkCommandBuffer second=commands[1];
 if(batch || dual){
  REQUIRE(vkBeginCommandBuffer(second,&begin)==VK_SUCCESS,53);
  vkCmdBeginRenderPass(second,&render,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(second,VK_PIPELINE_BIND_POINT_GRAPHICS,
   dual?secondPipeline:pipeline);
  vkCmdBindVertexBuffers(second,0,1,&vertex,&offset);
  vkCmdDraw(second,3,1,0,0);
  vkCmdEndRenderPass(second);
  REQUIRE(vkEndCommandBuffer(second)==VK_SUCCESS,54);
 }
 VkFenceCreateInfo fenceInfo={};fenceInfo.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
 VkFence fence=VK_NULL_HANDLE;
 REQUIRE(vkCreateFence(device,&fenceInfo,nullptr,&fence)==VK_SUCCESS && fence,47);
 REQUIRE(vkGetFenceStatus(device,fence)==VK_NOT_READY
  && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_TIMEOUT,48);
 VkSubmitInfo submit={};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
 submit.commandBufferCount=1;submit.pCommandBuffers=&command;
 if(batch || dual){
  VkSubmitInfo submits[2]={submit,submit};
  submits[1].pCommandBuffers=&second;
  REQUIRE(vkQueueSubmit(queue,2,submits,fence)==VK_SUCCESS,55);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
 }else for(unsigned i=0;i<(multiPass?1:repeats);i++){
  if(i)REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
   && vkGetFenceStatus(device,fence)==VK_NOT_READY,50);
  REQUIRE(vkQueueSubmit(queue,1,&submit,fence)==VK_SUCCESS,26);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
 }

 mapped=nullptr;
 REQUIRE(vkMapMemory(device,imageMemory,0,imageBytes,0,&mapped)==VK_SUCCESS && mapped,28);
 range.memory=imageMemory;range.size=imageBytes;
 REQUIRE(vkInvalidateMappedMemoryRanges(device,1,&range)==VK_SUCCESS,29);
 uint32_t hash=2166136261u;
 const uint32_t* pixels=(const uint32_t*)mapped;
 for(unsigned i=0;i<dimension*dimension;i++)hash=(hash^pixels[i])*16777619u;
 const unsigned expectedShape=shape;
 const uint32_t expected[4]={0x28616d2d,0xd40eb9c3,0xe24aed05,0x984ae945};
 if(large){
  const uint32_t expectedLarge[4]={
   0x8aaee2f5,0x9ca44bcd,0x23ba4705,0xb0d153c5};
  const uint32_t expectedLargeRgb[4]={
   0x7a1ac675,0xe178394d,0x49942685,0x9eff5945};
  REQUIRE((fragcoord?(hash==(fragcoordDenominator==32?0x655762f5:
    fragcoordDenominator==64?0x7b2f62f5:0xa0a362f5)
   && pixels[10*64+20]==(fragcoordDenominator==32?0xffa30000:
    fragcoordDenominator==64?0xff520000:0xff290000)):
   hash==(rotate?0x7b48f3e5:
   rgb?expectedLargeRgb[shape]:expectedLarge[shape])
   ) && pixels[0]==0x19537bad,30);
 }else if(half || green75 || green90 || rgb || twoPipelines){
  const uint32_t expectedHalf[4]={0x1629ed2d,0x0bf039c3,0x48e0ed05,0x05d66945};
  const uint32_t expectedGreen75[4]={0x28e87f2d,0xa6f78dc3,0xcef9f105,0xabd7c545};
  const uint32_t expectedGreen90[4]={0x979c9f2d,0x6a3fddc3,0x44297d05,0xd034dd45};
  const uint32_t expectedRgb[4]={0x6f2bc92d,0xa4e13743,0xce445d05,0xfffe3dc5};
  const unsigned interiorX[4]={20,16,16,16},interiorY[4]={10,16,16,16};
  REQUIRE(pixels[interiorY[expectedShape]*32+interiorX[expectedShape]]
   ==(rgb?0xff80bf40:green90?0xffffe500:green75?0xffffbf00:0xffff8000)
   && pixels[0]==0x19537bad
   && hash==(rgb?expectedRgb[expectedShape]:green90?expectedGreen90[expectedShape]:
    green75?expectedGreen75[expectedShape]:expectedHalf[expectedShape]),30);
 }else REQUIRE(hash==(rotate?0x77323fd5:expected[expectedShape])
  && pixels[0]==0x19537bad
  && (!rotate || (pixels[20*32+10]==0xffff4000
   && pixels[10*32+20]==0x19537bad)),30);
 vkUnmapMemory(device,imageMemory);
 if(large)printf("Native Vulkan N100 64x64 %striangle: PASS shape=%u hash=%08x draws=%lu\n",
  fragcoord?"FragCoord-red ":rotate?"rotated-vertex ":
  depth?"depth-vertex ":rgb?"RGB ":"",
  shape,hash,repeats);
 else if(multiPass)printf("Native Vulkan N100 two passes in one command buffer: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(dual)printf("Native Vulkan N100 dual pipeline: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(rgb)printf("Native Vulkan N100 RGB triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(green90)printf("Native Vulkan N100 0.9-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(green75)printf("Native Vulkan N100 0.75-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(half)printf("Native Vulkan N100 half-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else printf("Native Vulkan N100 triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);

 if(batch || dual){VkCommandBuffer both[2]={command,second};vkFreeCommandBuffers(device,pool,2,both);}
 else vkFreeCommandBuffers(device,pool,1,&command);
 vkDestroyCommandPool(device,pool,nullptr);
 REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
  && vkGetFenceStatus(device,fence)==VK_NOT_READY
  && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_TIMEOUT,49);
 vkDestroyFence(device,fence,nullptr);
 if(secondPipeline)vkDestroyPipeline(device,secondPipeline,nullptr);
 vkDestroyPipeline(device,pipeline,nullptr);
 vkDestroyPipelineLayout(device,layout,nullptr);
 vkDestroyFramebuffer(device,framebuffer,nullptr);
 vkDestroyRenderPass(device,pass,nullptr);
 vkDestroyImageView(device,view,nullptr);
 vkDestroyImage(device,image,nullptr);
 vkFreeMemory(device,imageMemory,nullptr);
 vkDestroyBuffer(device,vertex,nullptr);
 vkFreeMemory(device,vertexMemory,nullptr);
 vkDestroyBuffer(device,laterVertex,nullptr);
 vkFreeMemory(device,laterMemory,nullptr);
 vkDestroyDevice(device,nullptr);vkDestroyInstance(instance,nullptr);
 return 0;
}
