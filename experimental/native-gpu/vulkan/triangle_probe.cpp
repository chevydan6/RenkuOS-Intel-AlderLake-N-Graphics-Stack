// SPDX-License-Identifier: MIT
// One bounded Vulkan triangle through the native N100 owner and image BO.
#include <vulkan/vulkan.h>
#include "KnownSpirv.h"
#include "RgbSpirvFixture.h"
#include "DepthSpirvFixture.h"
#include "RotateSpirvFixture.h"
#include "FragCoordSpirvFixture.h"
#include "UboSpirvFixture.h"
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
   && strcmp(argv[2],"fragcoord32") && strcmp(argv[2],"fragcoord128")
   && strcmp(argv[2],"fragcoordgreen")
   && strcmp(argv[2],"ubo") && strcmp(argv[2],"ubooffset")
   && strcmp(argv[2],"ubomemoffset")
   && strcmp(argv[2],"fragcoordgreen32")
   && strcmp(argv[2],"fragcoordgreen128"))
  || (argc==5 && strcmp(argv[4],"batch")
   && strcmp(argv[4],"named") && strcmp(argv[4],"depth")
   && strcmp(argv[4],"rotate") && strcmp(argv[4],"dual")
   && strcmp(argv[4],"semaphore")
   && strcmp(argv[4],"highva")
   && strcmp(argv[4],"endva")
   && strcmp(argv[4],"multipass") && strcmp(argv[4],"coloronly")
   && strcmp(argv[4],"multidraw")
   && strcmp(argv[4],"batchgpu")
   && strcmp(argv[4],"offset") && strcmp(argv[4],"firstvertex")
   && strcmp(argv[4],"pageoffset")
   && strcmp(argv[4],"memoryoffset")
   && strcmp(argv[4],"imageoffset")
   && strcmp(argv[4],"rotateimageoffset")
   && strcmp(argv[4],"rotateoffset") && strcmp(argv[4],"rotatepage")
   && strcmp(argv[4],"rotatefirst")
   && strcmp(argv[4],"depthoffset")))return 40;
 unsigned shape=unsigned(argv[1][0]-'0');
 const bool large=getenv("RENKU_VK_TARGET_64")!=nullptr;
 const uint32_t dimension=large?64:32;
 const uint32_t imageBytes=dimension*dimension*4;
 bool half=argc>=3 && !strcmp(argv[2],"half");
 bool green75=argc>=3 && !strcmp(argv[2],"green75");
 bool green90=argc>=3 && !strcmp(argv[2],"green90");
 bool rgb=argc>=3 && !strcmp(argv[2],"rgb");
 bool uboOffset=argc>=3 && !strcmp(argv[2],"ubooffset");
 bool uboMemOffset=argc>=3 && !strcmp(argv[2],"ubomemoffset");
 bool ubo=argc>=3 && (!strcmp(argv[2],"ubo") || uboOffset || uboMemOffset);
 bool fragcoordGreen=argc>=3 && (!strcmp(argv[2],"fragcoordgreen")
  || !strcmp(argv[2],"fragcoordgreen32")
  || !strcmp(argv[2],"fragcoordgreen128"));
 unsigned fragcoordDenominator=argc>=3 && (!strcmp(argv[2],"fragcoord32")
  || !strcmp(argv[2],"fragcoordgreen32"))?32:
  argc>=3 && (!strcmp(argv[2],"fragcoord128")
  || !strcmp(argv[2],"fragcoordgreen128"))?128:
  argc>=3 && (!strcmp(argv[2],"fragcoord") || fragcoordGreen)?64:0;
 bool fragcoord=fragcoordDenominator!=0;
 char* repeatEnd=nullptr;
 unsigned long repeats=argc>=4?strtoul(argv[3],&repeatEnd,10):1;
 bool batch=argc==5 && !strcmp(argv[4],"batch");
 bool semaphoreBatch=argc==5 && !strcmp(argv[4],"semaphore");
 bool highVa=argc==5 && !strcmp(argv[4],"highva");
 bool endVa=argc==5 && !strcmp(argv[4],"endva");
 bool named=argc==5 && !strcmp(argv[4],"named");
 bool depth=argc==5 && (!strcmp(argv[4],"depth")
  || !strcmp(argv[4],"depthoffset"));
 bool rotate=argc==5 && (!strcmp(argv[4],"rotate")
  || !strcmp(argv[4],"rotateoffset") || !strcmp(argv[4],"rotatepage")
  || !strcmp(argv[4],"rotatefirst") || !strcmp(argv[4],"rotateimageoffset"));
 bool dual=argc==5 && !strcmp(argv[4],"dual");
 bool multiPass=argc==5 && !strcmp(argv[4],"multipass");
 bool multiDraw=argc==5 && !strcmp(argv[4],"multidraw");
 bool batchGpu=argc==5 && !strcmp(argv[4],"batchgpu");
 bool colorOnly=argc==5 && !strcmp(argv[4],"coloronly");
 bool bindOffset=argc==5 && (!strcmp(argv[4],"offset")
  || !strcmp(argv[4],"rotateoffset") || !strcmp(argv[4],"depthoffset"));
 bool firstVertex=argc==5 && (!strcmp(argv[4],"firstvertex")
  || !strcmp(argv[4],"rotatefirst"));
 bool pageOffset=argc==5 && (!strcmp(argv[4],"pageoffset")
  || !strcmp(argv[4],"rotatepage"));
 bool vertexMemOffset=argc==5 && !strcmp(argv[4],"memoryoffset");
 bool imageOffset=argc==5 && (!strcmp(argv[4],"imageoffset")
  || !strcmp(argv[4],"rotateimageoffset"));
 bool selectedVertexRange=bindOffset || firstVertex || pageOffset;
 bool twoPipelines=dual || multiPass || (multiDraw && !large);
 if(batchGpu && (large || shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats!=2))return 40;
 if(large && (half || green75 || green90 || named || twoPipelines
  || batch || (depth && shape!=0)))return 40;
 if(fragcoord && (!large || shape!=0 || rotate || depth || twoPipelines
  || batch || named || colorOnly))return 40;
 if(rotate && (shape!=0 || (argc>=3 && strcmp(argv[2],"original"))))return 40;
 if((twoPipelines || multiDraw) && (shape!=0 || argc!=5
  || strcmp(argv[2],"original")
  || repeats!=2))return 40;
 if((highVa || endVa) && (shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats>4 || (endVa && large)))return 40;
 if(selectedVertexRange && (shape!=0 || repeats>8))return 40;
 if(vertexMemOffset && (shape!=0 || repeats>8 || argc<3
  || strcmp(argv[2],"original")))return 40;
 if(imageOffset && (shape!=0 || repeats!=2))return 40;
 if(!repeats || repeats>64 || (argc>=4 && (!repeatEnd || *repeatEnd))
  || ((batch || semaphoreBatch) && repeats!=2))return 40;
 if(ubo && (shape!=0 || repeats!=2 || (argc==5 && !imageOffset)))return 40;
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
 if(fragcoordGreen){
  unsigned changed=0;
  for(size_t i=0;i+6<sizeof(fragcoordCode)/4;i++)
   if(fragcoordCode[i]==0x00070050u && fragcoordCode[i+1]==7
    && fragcoordCode[i+2]==17 && fragcoordCode[i+3]==16
    && fragcoordCode[i+4]==10 && fragcoordCode[i+5]==10
    && fragcoordCode[i+6]==11){
    fragcoordCode[i+3]=10;fragcoordCode[i+4]=16;changed++;
   }
  REQUIRE(changed==1,57);
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
 moduleInfo.pCode=ubo?RenkuUboSpirv::Fragment:fragcoord?fragcoordCode:
  rgb?rgbCode.data():(green75 || green90)?alternateCode:
  half?RenkuKnownSpirv::FragmentHalf:RenkuKnownSpirv::Fragment;
 moduleInfo.codeSize=ubo?sizeof(RenkuUboSpirv::Fragment):
  fragcoord?sizeof(RenkuFragCoordSpirv::Fragment):
  rgb?rgbCode.size()*4:
  half?sizeof(RenkuKnownSpirv::FragmentHalf):sizeof(RenkuKnownSpirv::Fragment);
 REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&fs)==VK_SUCCESS,6);

 VkDeviceMemory padding=VK_NULL_HANDLE;
 if(highVa || endVa){
  VkMemoryAllocateInfo padInfo={};padInfo.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  padInfo.allocationSize=(endVa?60:16)*4096;padInfo.memoryTypeIndex=0;
  REQUIRE(vkAllocateMemory(device,&padInfo,nullptr,&padding)==VK_SUCCESS
   && padding,64);
 }

 VkBufferCreateInfo bufferInfo={};bufferInfo.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
 bufferInfo.size=pageOffset?8192:selectedVertexRange || batchGpu?96:48;
 bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkBuffer vertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&vertex)==VK_SUCCESS,7);
 VkMemoryRequirements requirement={};vkGetBufferMemoryRequirements(device,vertex,&requirement);
 REQUIRE(requirement.size==(pageOffset?8192u:4096u)
  && requirement.memoryTypeBits==1,8);
 VkMemoryAllocateInfo allocation={};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
 allocation.memoryTypeIndex=0;
 allocation.allocationSize=requirement.size+(vertexMemOffset?4096:0);
 VkDeviceMemory vertexMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&vertexMemory)==VK_SUCCESS,9);
 REQUIRE(vkBindBufferMemory(device,vertex,vertexMemory,
  vertexMemOffset?4096:0)==VK_SUCCESS,10);
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
 REQUIRE(vkMapMemory(device,vertexMemory,0,
  bufferInfo.size+(vertexMemOffset?4096:0),0,&mapped)==VK_SUCCESS
  && mapped,11);
 if(selectedVertexRange){
  memcpy(mapped,positions[(shape+1)%4],48);
  memcpy((uint8_t*)mapped+(pageOffset?4096:48),positions[shape],48);
 }else if(vertexMemOffset){
  memcpy(mapped,positions[(shape+1)%4],48);
  memcpy((uint8_t*)mapped+4096,positions[shape],48);
 }else{
  memcpy(mapped,positions[shape],48);
  if(batchGpu)memcpy((uint8_t*)mapped+48,positions[2],48);
 }
 VkMappedMemoryRange range={};range.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
 range.memory=vertexMemory;range.size=bufferInfo.size+(vertexMemOffset?4096:0);
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,12);
 vkUnmapMemory(device,vertexMemory);
 VkBuffer laterVertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&laterVertex)==VK_SUCCESS,41);
 VkDeviceMemory laterMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&laterMemory)==VK_SUCCESS,42);
 REQUIRE(vkBindBufferMemory(device,laterVertex,laterMemory,0)==VK_SUCCESS,43);
 mapped=nullptr;
 REQUIRE(vkMapMemory(device,laterMemory,0,48,0,&mapped)==VK_SUCCESS && mapped,44);
 memcpy(mapped,positions[multiDraw?2:(shape+1)%4],48);
 range.memory=laterMemory;
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,45);
 vkUnmapMemory(device,laterMemory);

 VkBuffer uniform=VK_NULL_HANDLE;
 VkDeviceMemory uniformMemory=VK_NULL_HANDLE;
 void* uniformMapped=nullptr;
 if(ubo){
  bufferInfo.size=uboOffset?32:16;
  bufferInfo.usage=VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&uniform)==VK_SUCCESS,65);
  vkGetBufferMemoryRequirements(device,uniform,&requirement);
  allocation.allocationSize=requirement.size+(uboMemOffset?4096:0);
  REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&uniformMemory)==VK_SUCCESS,66);
  REQUIRE(vkBindBufferMemory(device,uniform,uniformMemory,
   uboMemOffset?4096:0)==VK_SUCCESS,67);
  REQUIRE(vkMapMemory(device,uniformMemory,0,
   bufferInfo.size+(uboMemOffset?4096:0),0,&uniformMapped)==VK_SUCCESS
   && uniformMapped,68);
 }

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
 allocation.allocationSize=requirement.size+(imageOffset?4096:0);
 VkDeviceMemory imageMemory=VK_NULL_HANDLE;
 REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&imageMemory)==VK_SUCCESS,15);
 REQUIRE(vkBindImageMemory(device,image,imageMemory,
  imageOffset?4096:0)==VK_SUCCESS,16);
 if(imageOffset){
  void* guard=nullptr;
  REQUIRE(vkMapMemory(device,imageMemory,0,4096,0,&guard)==VK_SUCCESS
   && guard,73);
  memset(guard,0xa5,4096);
  VkMappedMemoryRange guardRange={};
  guardRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  guardRange.memory=imageMemory;guardRange.size=4096;
  REQUIRE(vkFlushMappedMemoryRanges(device,1,&guardRange)==VK_SUCCESS,74);
  vkUnmapMemory(device,imageMemory);
 }
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

 VkDescriptorSetLayout setLayout=VK_NULL_HANDLE;
 VkDescriptorPool descriptorPool=VK_NULL_HANDLE;
 VkDescriptorSet descriptor=VK_NULL_HANDLE;
 if(ubo){
  VkDescriptorSetLayoutBinding uniformBinding={};uniformBinding.binding=0;
  uniformBinding.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBinding.descriptorCount=1;uniformBinding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo setInfo={};
  setInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setInfo.bindingCount=1;setInfo.pBindings=&uniformBinding;
  REQUIRE(vkCreateDescriptorSetLayout(device,&setInfo,nullptr,&setLayout)
   ==VK_SUCCESS && setLayout,69);
  VkDescriptorPoolSize poolSize={VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};
  VkDescriptorPoolCreateInfo poolInfo={};
  poolInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets=1;poolInfo.poolSizeCount=1;poolInfo.pPoolSizes=&poolSize;
  REQUIRE(vkCreateDescriptorPool(device,&poolInfo,nullptr,&descriptorPool)
   ==VK_SUCCESS && descriptorPool,70);
  VkDescriptorSetAllocateInfo setAllocation={};
  setAllocation.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocation.descriptorPool=descriptorPool;
  setAllocation.descriptorSetCount=1;setAllocation.pSetLayouts=&setLayout;
  REQUIRE(vkAllocateDescriptorSets(device,&setAllocation,&descriptor)
   ==VK_SUCCESS && descriptor,71);
  VkDescriptorBufferInfo uniformInfo={uniform,uboOffset?16u:0u,16};
  VkWriteDescriptorSet write={};write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet=descriptor;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.descriptorCount=1;write.pBufferInfo=&uniformInfo;
  vkUpdateDescriptorSets(device,1,&write,0,nullptr);
 }
 VkPipelineLayoutCreateInfo layoutInfo={};layoutInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
 if(ubo){layoutInfo.setLayoutCount=1;layoutInfo.pSetLayouts=&setLayout;}
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
   VkGraphicsPipelineCreateInfo overflow[65]={};
   VkPipeline rolledBack[65]={};
   for(unsigned i=0;i<65;i++)overflow[i]=pipelineInfo;
   REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,65,
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
 commandInfo.commandBufferCount=(batch || dual || semaphoreBatch)?2:1;
 VkCommandBuffer commands[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 REQUIRE(vkAllocateCommandBuffers(device,&commandInfo,commands)==VK_SUCCESS
  && commands[0] && (!(batch || dual || semaphoreBatch)
   || (commands[1] && commands[1]!=commands[0])),23);
 VkCommandBuffer command=commands[0];
 VkCommandBufferBeginInfo begin={};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 REQUIRE(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS,24);
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=pass;render.framebuffer=framebuffer;
 render.renderArea={{0,0},{dimension,dimension}};
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
 if(ubo)vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
  layout,0,1,&descriptor,0,nullptr);
 VkDeviceSize offset=pageOffset?4096:bindOffset?48:0;
 vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);
 vkCmdDraw(command,3,1,firstVertex?3:0,0);
 if(!batchGpu)vkCmdBindVertexBuffers(command,0,1,&laterVertex,&offset);
 if(multiDraw){
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
   twoPipelines?secondPipeline:pipeline);
  vkCmdDraw(command,3,1,0,0);
 }
 if(batchGpu)vkCmdDraw(command,3,1,3,0);
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
 if(batch || dual || semaphoreBatch){
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
 if(batch || dual || semaphoreBatch){
  VkSubmitInfo submits[2]={submit,submit};
  submits[1].pCommandBuffers=&second;
  VkSemaphore dependency=VK_NULL_HANDLE;
  VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  if(semaphoreBatch){
   VkSemaphoreCreateInfo semInfo={};semInfo.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   REQUIRE(vkCreateSemaphore(device,&semInfo,nullptr,&dependency)==VK_SUCCESS
    && dependency,61);
   submits[0].signalSemaphoreCount=1;
   submits[0].pSignalSemaphores=&dependency;
   submits[1].waitSemaphoreCount=1;
   submits[1].pWaitSemaphores=&dependency;
   submits[1].pWaitDstStageMask=&stage;
   VkSemaphore unsignaled=VK_NULL_HANDLE;
   REQUIRE(vkCreateSemaphore(device,&semInfo,nullptr,&unsignaled)==VK_SUCCESS
    && unsignaled,62);
   submits[1].pWaitSemaphores=&unsignaled;
   REQUIRE(vkQueueSubmit(queue,2,submits,fence)==VK_ERROR_INITIALIZATION_FAILED
    && vkGetFenceStatus(device,fence)==VK_NOT_READY,63);
   submits[1].pWaitSemaphores=&dependency;
   vkDestroySemaphore(device,unsignaled,nullptr);
  }
  REQUIRE(vkQueueSubmit(queue,2,submits,fence)==VK_SUCCESS,55);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
  if(dependency)vkDestroySemaphore(device,dependency,nullptr);
 }else for(unsigned i=0;i<((multiPass || multiDraw || batchGpu)?1:repeats);i++){
  if(i)REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
   && vkGetFenceStatus(device,fence)==VK_NOT_READY,50);
  if(ubo){
   const uint32_t colors[2][4]={{0x3f800000,0x3e800000,0,0x3f800000},
    {0,0x3f800000,0x3f000000,0x3f800000}};
   if(uboOffset || uboMemOffset){
    const uint32_t decoy[4]={0,0,0,0x3f800000};
    memcpy(uniformMapped,decoy,16);
   }
   memcpy((uint8_t*)uniformMapped+(uboMemOffset?4096:uboOffset?16:0),colors[i],16);
   VkMappedMemoryRange uniformRange={};
   uniformRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   uniformRange.memory=uniformMemory;
   uniformRange.size=uboMemOffset?4112:uboOffset?32:16;
   REQUIRE(vkFlushMappedMemoryRanges(device,1,&uniformRange)==VK_SUCCESS,72);
  }
  REQUIRE(vkQueueSubmit(queue,1,&submit,fence)==VK_SUCCESS,26);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
 }

 mapped=nullptr;
 REQUIRE(vkMapMemory(device,imageMemory,imageOffset?4096:0,imageBytes,0,&mapped)
  ==VK_SUCCESS && mapped,28);
 range.memory=imageMemory;range.offset=imageOffset?4096:0;range.size=imageBytes;
 REQUIRE(vkInvalidateMappedMemoryRanges(device,1,&range)==VK_SUCCESS,29);
 uint32_t hash=2166136261u;
 const uint32_t* pixels=(const uint32_t*)mapped;
 for(unsigned i=0;i<dimension*dimension;i++)hash=(hash^pixels[i])*16777619u;
 const unsigned expectedShape=shape;
 const uint32_t expected[4]={0x28616d2d,0xd40eb9c3,0xe24aed05,0x984ae945};
 if(batchGpu){
  REQUIRE(hash==0xd23b1ba1 && pixels[0]==0x19537bad
   && pixels[10*32+25]==0xffff4000
   && pixels[20*32+10]==0xffff4000,30);
 }else if(multiDraw){
  REQUIRE(pixels[0]==0x19537bad
   && pixels[(large?20:10)*dimension+(large?40:20)]==0xffff4000
   && pixels[(large?40:20)*dimension+(large?20:10)]
    ==(large?0xffff4000:0xffff8000),30);
 }else if(large){
  const uint32_t expectedLarge[4]={
   0x8aaee2f5,0x9ca44bcd,0x23ba4705,0xb0d153c5};
  const uint32_t expectedLargeRgb[4]={
   0x7a1ac675,0xe178394d,0x49942685,0x9eff5945};
  REQUIRE((ubo?hash==0x8df12ff5:
   fragcoord?(hash==(fragcoordGreen
    ?(fragcoordDenominator==32?0x6e59daf5:
      fragcoordDenominator==64?0xf25bcaf5:0x649956f5):
    fragcoordDenominator==32?0x655762f5:
    fragcoordDenominator==64?0x7b2f62f5:0xa0a362f5)
   && pixels[10*64+20]==(fragcoordGreen
    ?(fragcoordDenominator==32?0xff00a300:
      fragcoordDenominator==64?0xff005200:0xff002900):
    fragcoordDenominator==32?0xffa30000:
    fragcoordDenominator==64?0xff520000:0xff290000)):
   hash==(rotate?0x7b48f3e5:
   rgb?expectedLargeRgb[shape]:expectedLarge[shape])
   ) && pixels[0]==0x19537bad,30);
 }else if(ubo){
  REQUIRE(hash==0x4979062d && pixels[10*32+20]==0xff00ff80
   && pixels[0]==0x19537bad,30);
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
 if(imageOffset){
  void* guard=nullptr;
  REQUIRE(vkMapMemory(device,imageMemory,0,4096,0,&guard)==VK_SUCCESS
   && guard,75);
  VkMappedMemoryRange guardRange={};
  guardRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  guardRange.memory=imageMemory;guardRange.size=4096;
  REQUIRE(vkInvalidateMappedMemoryRanges(device,1,&guardRange)==VK_SUCCESS,76);
  for(unsigned i=0;i<4096;i++)REQUIRE(((uint8_t*)guard)[i]==0xa5,77);
  vkUnmapMemory(device,imageMemory);
 }
 if(batchGpu)printf("Native Vulkan N100 two triangles: PASS hash=%08x draws=2\n",hash);
 else if(multiDraw)printf("Native Vulkan N100 %ux%u two draws in one render pass: PASS shape=%u hash=%08x draws=%lu\n",dimension,dimension,shape,hash,repeats);
 else if(large)printf("Native Vulkan N100 64x64 %striangle: PASS shape=%u hash=%08x draws=%lu\n",
  ubo?"UBO ":fragcoord?(fragcoordGreen?"FragCoord-green ":"FragCoord-red "):
  rotate?"rotated-vertex ":
  depth?"depth-vertex ":rgb?"RGB ":"",
  shape,hash,repeats);
 else if(multiPass)printf("Native Vulkan N100 two passes in one command buffer: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(dual)printf("Native Vulkan N100 dual pipeline: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(semaphoreBatch)printf("Native Vulkan N100 binary semaphore: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(ubo)printf("Native Vulkan N100 UBO triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(rgb)printf("Native Vulkan N100 RGB triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(green90)printf("Native Vulkan N100 0.9-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(green75)printf("Native Vulkan N100 0.75-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else if(half)printf("Native Vulkan N100 half-green triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);
 else printf("Native Vulkan N100 triangle: PASS shape=%u hash=%08x draws=%lu\n",shape,hash,repeats);

 if(batch || dual || semaphoreBatch){VkCommandBuffer both[2]={command,second};vkFreeCommandBuffers(device,pool,2,both);}
 else vkFreeCommandBuffers(device,pool,1,&command);
 vkDestroyCommandPool(device,pool,nullptr);
 REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
  && vkGetFenceStatus(device,fence)==VK_NOT_READY
  && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_TIMEOUT,49);
 vkDestroyFence(device,fence,nullptr);
 if(secondPipeline)vkDestroyPipeline(device,secondPipeline,nullptr);
 vkDestroyPipeline(device,pipeline,nullptr);
 vkDestroyPipelineLayout(device,layout,nullptr);
 if(ubo){
  vkDestroyDescriptorPool(device,descriptorPool,nullptr);
  vkDestroyDescriptorSetLayout(device,setLayout,nullptr);
  vkUnmapMemory(device,uniformMemory);
  vkDestroyBuffer(device,uniform,nullptr);
  vkFreeMemory(device,uniformMemory,nullptr);
 }
 vkDestroyFramebuffer(device,framebuffer,nullptr);
 vkDestroyRenderPass(device,pass,nullptr);
 vkDestroyImageView(device,view,nullptr);
 vkDestroyImage(device,image,nullptr);
 vkFreeMemory(device,imageMemory,nullptr);
 vkDestroyBuffer(device,vertex,nullptr);
 vkFreeMemory(device,vertexMemory,nullptr);
 vkDestroyBuffer(device,laterVertex,nullptr);
 vkFreeMemory(device,laterMemory,nullptr);
 if(padding)vkFreeMemory(device,padding,nullptr);
 vkDestroyDevice(device,nullptr);vkDestroyInstance(instance,nullptr);
 return 0;
}
