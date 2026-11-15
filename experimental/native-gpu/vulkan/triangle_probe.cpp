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
   && strcmp(argv[4],"multipass") && strcmp(argv[4],"loadpass")
   && strcmp(argv[4],"loadsubmit")
   && strcmp(argv[4],"indexed") && strcmp(argv[4],"indexbad")
   && strcmp(argv[4],"indexalias")
   && strcmp(argv[4],"indexpair") && strcmp(argv[4],"indexpairref")
   && strcmp(argv[4],"indexsix") && strcmp(argv[4],"indexsixbad")
   && strcmp(argv[4],"indexsixpage")
   && strcmp(argv[4],"indexmesh") && strcmp(argv[4],"indexmeshbad")
   && strcmp(argv[4],"indexmeshclear")
   && strcmp(argv[4],"indexmeshplus") && strcmp(argv[4],"indexmeshminus")
   && strcmp(argv[4],"copyvertex")
   && strcmp(argv[4],"copyreadback")
   && strcmp(argv[4],"copybackground")
   && strcmp(argv[4],"clearpass")
   && strcmp(argv[4],"coloronly")
   && strcmp(argv[4],"multidraw")
   && strcmp(argv[4],"batchgpu")
   && strcmp(argv[4],"draw6")
   && strcmp(argv[4],"draw9") && strcmp(argv[4],"draw3x3")
   && strcmp(argv[4],"draw12") && strcmp(argv[4],"draw3x4")
   && strcmp(argv[4],"drawpage")
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
 bool loadSubmit=argc==5 && !strcmp(argv[4],"loadsubmit");
 bool indexBad=argc==5 && !strcmp(argv[4],"indexbad");
 bool indexAlias=argc==5 && !strcmp(argv[4],"indexalias");
 bool indexPair=argc==5 && !strcmp(argv[4],"indexpair");
 bool indexPairReference=argc==5 && !strcmp(argv[4],"indexpairref");
 bool indexSixBad=argc==5 && !strcmp(argv[4],"indexsixbad");
 bool indexSixPage=argc==5 && !strcmp(argv[4],"indexsixpage");
 bool indexMeshBad=argc==5 && !strcmp(argv[4],"indexmeshbad");
 bool indexMeshClear=argc==5 && !strcmp(argv[4],"indexmeshclear");
 bool indexMeshPlus=argc==5 && !strcmp(argv[4],"indexmeshplus");
 bool indexMeshMinus=argc==5 && !strcmp(argv[4],"indexmeshminus");
 bool copyVertex=argc==5 && !strcmp(argv[4],"copyvertex");
 bool copyReadback=argc==5 && !strcmp(argv[4],"copyreadback");
 bool copyBackground=argc==5 && !strcmp(argv[4],"copybackground");
 bool indexMesh=argc==5 && (!strcmp(argv[4],"indexmesh")
  || indexMeshBad || indexMeshClear || indexMeshPlus || indexMeshMinus);
 bool clearPass=argc==5 && (!strcmp(argv[4],"clearpass") || indexMeshClear);
 bool indexSix=argc==5 && (!strcmp(argv[4],"indexsix")
  || indexSixBad || indexSixPage);
 bool indexed=argc==5 && (!strcmp(argv[4],"indexed") || indexBad
  || indexAlias || indexPair || indexSix || indexMesh);
 bool loadPass=argc==5 && (!strcmp(argv[4],"loadpass") || loadSubmit
  || copyBackground);
 bool multiDraw=argc==5 && !strcmp(argv[4],"multidraw");
 bool batchGpu=argc==5 && !strcmp(argv[4],"batchgpu");
 bool drawSix=argc==5 && !strcmp(argv[4],"draw6");
 bool drawPage=argc==5 && !strcmp(argv[4],"drawpage");
 bool twoTriangles=batchGpu || drawSix || drawPage;
 bool drawNine=argc==5 && !strcmp(argv[4],"draw9");
 bool drawThree=argc==5 && !strcmp(argv[4],"draw3x3");
 bool threeTriangles=drawNine || drawThree;
 bool drawTwelve=argc==5 && !strcmp(argv[4],"draw12");
 bool drawFour=argc==5 && !strcmp(argv[4],"draw3x4");
 bool fourTriangles=drawTwelve || drawFour;
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
 bool twoPipelines=dual || multiPass || loadPass || (multiDraw && !large);
 if(twoTriangles && (shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats!=2))return 40;
 if(threeTriangles && (shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats!=3))return 40;
 if(fourTriangles && (shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats!=4))return 40;
 if(large && (half || green75 || green90 || named
  || (twoPipelines && !loadPass)
  || batch || (depth && shape!=0)))return 40;
 if(fragcoord && (!large || shape!=0 || rotate || depth || twoPipelines
  || batch || named || colorOnly))return 40;
 if(rotate && (shape!=0 || (argc>=3 && strcmp(argv[2],"original"))))return 40;
 if((twoPipelines || multiDraw) && (shape!=0 || argc!=5
  || strcmp(argv[2],"original")
  || repeats!=2))return 40;
 if((highVa || endVa) && (shape!=0 || argc!=5
  || strcmp(argv[2],"original") || repeats>4 || (endVa && large)))return 40;
 if(indexed && (shape!=0 || argc!=5 || strcmp(argv[2],"original")
  || repeats>2 || ((indexBad || indexAlias) && repeats!=1)))return 40;
 if((indexPair || indexPairReference) && (shape!=0 || repeats!=2
  || strcmp(argv[2],"original")))return 40;
 if(indexSix && (shape!=0 || repeats!=1
  || strcmp(argv[2],"original")))return 40;
 if(indexMesh && (shape!=0 || repeats!=1
  || strcmp(argv[2],"original")))return 40;
 if(copyVertex && (shape!=0 || repeats!=1
  || strcmp(argv[2],"original")))return 40;
 if(copyReadback && (shape!=0 || repeats!=1
  || strcmp(argv[2],"original")))return 40;
 if(copyBackground && (shape!=0 || repeats!=2
  || strcmp(argv[2],"original")))return 40;
 if(clearPass && (shape!=0 || repeats!=1
  || strcmp(argv[2],"original")))return 40;
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
 VkPhysicalDeviceProperties properties={};
 vkGetPhysicalDeviceProperties(physical,&properties);
 REQUIRE(properties.limits.maxDrawIndexedIndexValue==255
  && properties.limits.maxVertexInputAttributes==1
  && properties.limits.maxVertexInputBindings==1
  && properties.limits.maxVertexInputBindingStride==16,88);
 VkFormatProperties format={};
 vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_B8G8R8A8_UNORM,&format);
 REQUIRE(format.linearTilingFeatures==(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
  |VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT),56);
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
 bufferInfo.size=(pageOffset || drawPage)?8192:
  (indexMeshPlus || indexMeshMinus)?160:indexMesh?144:
  indexPairReference?96:indexed?64:
  fourTriangles?192:threeTriangles?144:
  (selectedVertexRange || twoTriangles)?96:48;
 bufferInfo.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  |(copyVertex?VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    |VK_BUFFER_USAGE_TRANSFER_DST_BIT:0);
 bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkBuffer vertex=VK_NULL_HANDLE;
 REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&vertex)==VK_SUCCESS,7);
 VkMemoryRequirements requirement={};vkGetBufferMemoryRequirements(device,vertex,&requirement);
 REQUIRE(requirement.size==((pageOffset || drawPage)?8192u:4096u)
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
 const uint32_t fourthVertices[12]={
  0xbf000000,0x3e800000,0x3f000000,0x3f800000,
  0x3f000000,0x3e800000,0x3f000000,0x3f800000,
  0x00000000,0x3f400000,0x3f000000,0x3f800000};
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
 }else if(drawPage){
  memcpy((uint8_t*)mapped+4096-48,positions[shape],48);
  memcpy((uint8_t*)mapped+4096,positions[2],48);
 }else if(indexMesh){
  uint8_t* mesh=(uint8_t*)mapped+(indexMeshPlus || indexMeshMinus?16:0);
  if(indexMeshPlus || indexMeshMinus)memcpy(mapped,positions[0],16);
  memcpy(mesh,positions[0],48);
  memcpy(mesh+48,positions[2],48);
  memcpy(mesh+96,positions[1],48);
  uint8_t saved[16];memcpy(saved,mesh+16,16);
  memcpy(mesh+16,mesh+32,16);
  memcpy(mesh+32,saved,16);
 }else if(indexPairReference){
  const uint32_t complement[12]={
   0xbf400000,0xbf400000,0x3f000000,0x3f800000,
   0x3f400000,0x3f400000,0x3f000000,0x3f800000,
   0xbf400000,0x3f400000,0x3f000000,0x3f800000};
  memcpy(mapped,positions[0],48);
  memcpy((uint8_t*)mapped+48,complement,48);
 }else{
  memcpy(mapped,positions[shape],48);
  if(indexed){
   uint8_t saved[16];memcpy(saved,(uint8_t*)mapped+16,16);
   memcpy((uint8_t*)mapped+16,(uint8_t*)mapped+32,16);
   memcpy((uint8_t*)mapped+32,saved,16);
   const uint32_t alternative[4]={indexPair || indexSix?0xbf400000u:0u,
    indexPair || indexSix?0x3f400000u:0xbf400000u,0x3f000000,0x3f800000};
   memcpy((uint8_t*)mapped+48,alternative,16);
  }
  if(twoTriangles)memcpy((uint8_t*)mapped+48,positions[2],48);
  if(threeTriangles || fourTriangles){
   memcpy((uint8_t*)mapped+48,positions[2],48);
   memcpy((uint8_t*)mapped+96,positions[1],48);
   if(fourTriangles)memcpy((uint8_t*)mapped+144,fourthVertices,48);
  }
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
 memcpy(mapped,positions[(multiDraw || loadPass)?2:(shape+1)%4],48);
 range.memory=laterMemory;
 REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,45);
 vkUnmapMemory(device,laterMemory);

 VkBuffer index=VK_NULL_HANDLE;
 VkDeviceMemory indexMemory=VK_NULL_HANDLE;
 if(indexed){
  bufferInfo.size=indexSixPage?4102:indexMesh?18:indexPair || indexSix?12:6;
  bufferInfo.usage=VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&index)==VK_SUCCESS,80);
  vkGetBufferMemoryRequirements(device,index,&requirement);
  allocation.allocationSize=requirement.size;
  REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&indexMemory)==VK_SUCCESS,81);
  REQUIRE(vkBindBufferMemory(device,index,
   indexAlias?vertexMemory:indexMemory,0)==VK_SUCCESS,82);
  void* indexMapped=nullptr;
  REQUIRE(vkMapMemory(device,indexMemory,0,bufferInfo.size,0,&indexMapped)==VK_SUCCESS
   && indexMapped,83);
  const uint16_t order[6]={0,uint16_t(indexBad?4:2),1,0,1,
   uint16_t(indexSixBad?4:3)};
  if(indexMesh){
   const uint16_t mesh[9]={0,2,1,3,4,5,6,7,
    uint16_t(indexMeshBad?9:8)};
   memcpy(indexMapped,mesh,sizeof(mesh));
  }else memcpy((uint8_t*)indexMapped+(indexSixPage?4090:0),order,
   indexPair || indexSix?12:6);
  range.memory=indexMemory;range.size=bufferInfo.size;
  REQUIRE(vkFlushMappedMemoryRanges(device,1,&range)==VK_SUCCESS,84);
  vkUnmapMemory(device,indexMemory);
 }

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
  |(colorOnly?0:VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
  |(copyBackground?VK_IMAGE_USAGE_TRANSFER_DST_BIT:0);
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
 VkBuffer readback=VK_NULL_HANDLE;
 VkDeviceMemory readbackMemory=VK_NULL_HANDLE;
 if(copyReadback){
  bufferInfo.size=imageBytes;bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&readback)==VK_SUCCESS,88);
  vkGetBufferMemoryRequirements(device,readback,&requirement);
  allocation.allocationSize=requirement.size;
  REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&readbackMemory)==VK_SUCCESS,89);
  REQUIRE(vkBindBufferMemory(device,readback,readbackMemory,0)==VK_SUCCESS,90);
 }
 VkBuffer background=VK_NULL_HANDLE;
 VkDeviceMemory backgroundMemory=VK_NULL_HANDLE;
 if(copyBackground){
  bufferInfo.size=imageBytes;bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  REQUIRE(vkCreateBuffer(device,&bufferInfo,nullptr,&background)==VK_SUCCESS,92);
  vkGetBufferMemoryRequirements(device,background,&requirement);
  allocation.allocationSize=requirement.size;
  REQUIRE(vkAllocateMemory(device,&allocation,nullptr,&backgroundMemory)==VK_SUCCESS,93);
  REQUIRE(vkBindBufferMemory(device,background,backgroundMemory,0)==VK_SUCCESS,94);
  void* pixels=nullptr;
  REQUIRE(vkMapMemory(device,backgroundMemory,0,imageBytes,0,&pixels)==VK_SUCCESS
   && pixels,95);
  for(uint32_t i=0;i<dimension*dimension;i++)((uint32_t*)pixels)[i]=0xff336699;
  VkMappedMemoryRange upload={};upload.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  upload.memory=backgroundMemory;upload.size=imageBytes;
  REQUIRE(vkFlushMappedMemoryRanges(device,1,&upload)==VK_SUCCESS,96);
  vkUnmapMemory(device,backgroundMemory);
 }
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
 attachment.samples=VK_SAMPLE_COUNT_1_BIT;
 attachment.loadOp=clearPass?VK_ATTACHMENT_LOAD_OP_CLEAR:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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
 VkRenderPass loadRenderPass=VK_NULL_HANDLE;
 VkFramebuffer loadFramebuffer=VK_NULL_HANDLE;
 if(loadPass){
  attachment.loadOp=VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.initialLayout=VK_IMAGE_LAYOUT_GENERAL;
  REQUIRE(vkCreateRenderPass(device,&passInfo,nullptr,&loadRenderPass)
   ==VK_SUCCESS && loadRenderPass,78);
  fbInfo.renderPass=loadRenderPass;
  REQUIRE(vkCreateFramebuffer(device,&fbInfo,nullptr,&loadFramebuffer)
   ==VK_SUCCESS && loadFramebuffer,79);
 }

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
  if(!large){
   moduleInfo.pCode=RenkuKnownSpirv::FragmentHalf;
   moduleInfo.codeSize=sizeof(RenkuKnownSpirv::FragmentHalf);
   REQUIRE(vkCreateShaderModule(device,&moduleInfo,nullptr,&halfFs)==VK_SUCCESS,56);
   secondStages[1].module=halfFs;
  }
  pipelineInfos[1].pStages=secondStages;
  if(loadPass)pipelineInfos[1].renderPass=loadRenderPass;
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
 commandInfo.commandBufferCount=(batch || dual || semaphoreBatch || loadSubmit)?2:1;
 VkCommandBuffer commands[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 REQUIRE(vkAllocateCommandBuffers(device,&commandInfo,commands)==VK_SUCCESS
  && commands[0] && (!(batch || dual || semaphoreBatch || loadSubmit)
   || (commands[1] && commands[1]!=commands[0])),23);
 VkCommandBuffer command=commands[0];
 VkCommandBufferBeginInfo begin={};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 REQUIRE(vkBeginCommandBuffer(command,&begin)==VK_SUCCESS,24);
 if(copyVertex){
  VkBufferCopy region={0,0,48};
  vkCmdCopyBuffer(command,vertex,laterVertex,1,&region);
 }
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=pass;render.framebuffer=framebuffer;
 render.renderArea={{0,0},{dimension,dimension}};
 VkClearValue clearValue={};
 clearValue.color.float32[0]=0.2f;
 clearValue.color.float32[1]=0.4f;
 clearValue.color.float32[2]=0.6f;
 clearValue.color.float32[3]=1.0f;
 if(clearPass){render.clearValueCount=1;render.pClearValues=&clearValue;}
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
 if(ubo)vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
  layout,0,1,&descriptor,0,nullptr);
 VkDeviceSize offset=pageOffset?4096:drawPage?4096-48:bindOffset?48:
  indexMeshMinus?32:0;
 VkBuffer drawVertex=copyVertex?laterVertex:vertex;
 vkCmdBindVertexBuffers(command,0,1,&drawVertex,&offset);
 const uint32_t drawVertices=drawTwelve?12:drawNine?9:
  (drawSix || drawPage || indexPairReference)?6:3;
 if(indexed){
  vkCmdBindIndexBuffer(command,index,0,VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(command,indexMesh?9:indexSix?6:3,1,
   indexSixPage?2045:0,indexMeshPlus?1:indexMeshMinus?-1:0,0);
  if(indexPair)vkCmdDrawIndexed(command,3,1,3,0,0);
 }else vkCmdDraw(command,drawVertices,1,firstVertex?3:0,0);
 if(!twoTriangles && !threeTriangles && !fourTriangles)
  vkCmdBindVertexBuffers(command,0,1,&laterVertex,&offset);
 if(multiDraw){
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,
   twoPipelines?secondPipeline:pipeline);
  vkCmdDraw(command,3,1,0,0);
 }
 if(batchGpu)vkCmdDraw(command,3,1,3,0);
 if(drawThree){
  vkCmdDraw(command,3,1,3,0);
  vkCmdDraw(command,3,1,6,0);
 }
 if(drawFour){
  vkCmdDraw(command,3,1,3,0);
  vkCmdDraw(command,3,1,6,0);
  vkCmdDraw(command,3,1,9,0);
 }
 vkCmdEndRenderPass(command);
 if(copyBackground){
  VkBufferImageCopy region={};
  region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
  region.imageExtent={dimension,dimension,1};
  vkCmdCopyBufferToImage(command,background,image,VK_IMAGE_LAYOUT_GENERAL,
   1,&region);
 }
 if(copyReadback){
  VkBufferImageCopy region={};
  region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
  region.imageExtent={dimension,dimension,1};
  vkCmdCopyImageToBuffer(command,image,VK_IMAGE_LAYOUT_GENERAL,
   readback,1,&region);
 }
 if(multiPass){
  vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,secondPipeline);
  vkCmdBindVertexBuffers(command,0,1,&vertex,&offset);
  vkCmdDraw(command,3,1,0,0);
  vkCmdEndRenderPass(command);
 }
 if(loadPass && !loadSubmit){
  VkRenderPassBeginInfo renderLoad=render;
  renderLoad.renderPass=loadRenderPass;
  renderLoad.framebuffer=loadFramebuffer;
  vkCmdBeginRenderPass(command,&renderLoad,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,secondPipeline);
  vkCmdBindVertexBuffers(command,0,1,&laterVertex,&offset);
  vkCmdDraw(command,3,1,0,0);
  vkCmdEndRenderPass(command);
 }
 REQUIRE(vkEndCommandBuffer(command)==VK_SUCCESS,25);
 VkCommandBuffer second=commands[1];
 if(batch || dual || semaphoreBatch || loadSubmit){
  REQUIRE(vkBeginCommandBuffer(second,&begin)==VK_SUCCESS,53);
  VkRenderPassBeginInfo secondRender=render;
  if(loadSubmit){secondRender.renderPass=loadRenderPass;
   secondRender.framebuffer=loadFramebuffer;}
  vkCmdBeginRenderPass(second,&secondRender,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(second,VK_PIPELINE_BIND_POINT_GRAPHICS,
   (dual || loadSubmit)?secondPipeline:pipeline);
  VkBuffer secondVertex=loadSubmit?laterVertex:vertex;
  vkCmdBindVertexBuffers(second,0,1,&secondVertex,&offset);
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
 if(loadSubmit){
  REQUIRE(vkQueueSubmit(queue,1,&submit,fence)==VK_SUCCESS,26);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
  REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
   && vkGetFenceStatus(device,fence)==VK_NOT_READY,50);
  submit.pCommandBuffers=&second;
  REQUIRE(vkQueueSubmit(queue,1,&submit,fence)==VK_SUCCESS,26);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
 }else if(batch || dual || semaphoreBatch){
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
 }else for(unsigned i=0;i<((multiPass || loadPass || multiDraw || twoTriangles
  || threeTriangles || fourTriangles || indexPair
  || indexPairReference)?1:repeats);i++){
  if(i)REQUIRE(vkResetFences(device,1,&fence)==VK_SUCCESS
   && vkGetFenceStatus(device,fence)==VK_NOT_READY,50);
  if(indexed && i){
   void* indexMapped=nullptr;
   REQUIRE(vkMapMemory(device,indexMemory,0,6,0,&indexMapped)==VK_SUCCESS
    && indexMapped,85);
   const uint16_t alternateOrder[3]={0,3,1};
   memcpy(indexMapped,alternateOrder,sizeof(alternateOrder));
   VkMappedMemoryRange indexRange={};
   indexRange.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
   indexRange.memory=indexMemory;indexRange.size=6;
   REQUIRE(vkFlushMappedMemoryRanges(device,1,&indexRange)==VK_SUCCESS,86);
   vkUnmapMemory(device,indexMemory);
  }
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
  VkResult submitted=vkQueueSubmit(queue,1,&submit,fence);
  if(indexBad || indexAlias){
   REQUIRE(submitted==VK_ERROR_DEVICE_LOST
    && vkGetFenceStatus(device,fence)==VK_NOT_READY,87);
   puts(indexAlias?"Native Vulkan N100 aliased index BO rejected before draw: PASS":
    "Native Vulkan N100 invalid GPU index rejected before draw: PASS");
   return 0;
  }
  REQUIRE(submitted==VK_SUCCESS,26);
  REQUIRE(vkGetFenceStatus(device,fence)==VK_SUCCESS
   && vkWaitForFences(device,1,&fence,VK_TRUE,0)==VK_SUCCESS,27);
 }

 mapped=nullptr;
 VkDeviceMemory outputMemory=copyReadback?readbackMemory:imageMemory;
 REQUIRE(vkMapMemory(device,outputMemory,copyReadback?0:imageOffset?4096:0,
  imageBytes,0,&mapped)
  ==VK_SUCCESS && mapped,28);
 range.memory=outputMemory;range.offset=copyReadback?0:imageOffset?4096:0;
 range.size=imageBytes;
 REQUIRE(vkInvalidateMappedMemoryRanges(device,1,&range)==VK_SUCCESS,29);
 uint32_t hash=2166136261u;
 const uint32_t* pixels=(const uint32_t*)mapped;
 for(unsigned i=0;i<dimension*dimension;i++)hash=(hash^pixels[i])*16777619u;
 if(copyReadback && getenv("RENKU_VK_READBACK_CAPTURE")){
  FILE* capture=fopen(getenv("RENKU_VK_READBACK_CAPTURE"),"wb");
  REQUIRE(capture && fwrite(mapped,1,imageBytes,capture)==imageBytes
   && fclose(capture)==0,91);
 }
 const unsigned expectedShape=shape;
 const uint32_t expected[4]={0x28616d2d,0xd40eb9c3,0xe24aed05,0x984ae945};
 if(copyBackground){
  REQUIRE(pixels[0]==0xff336699
   && pixels[(dimension/2)*dimension+dimension/2]
    ==(large?0xffff4000:0xffff8000),30);
 }else if(clearPass){
  REQUIRE(pixels[0]==0xff336699
   && pixels[(large?20:10)*dimension+(large?40:20)]==0xffff4000,30);
 }else if(indexPair || indexPairReference || indexSix){
  REQUIRE(hash==(large?0x554d3ec5:0x261a8285)
   && pixels[0]==0x19537bad,30);
 }else if(indexMesh){
  REQUIRE(hash==(large?0x9bd44344:0xcdad4a41)
   && pixels[0]==0x19537bad,30);
 }else if(indexed && repeats==2){
  REQUIRE(hash==(large?0x5fdea115:0x459d92c5)
   && pixels[0]==0x19537bad,30);
 }else if(fourTriangles){
  REQUIRE(hash==(large?0x61162dae:0x5bf758b9)
   && pixels[0]==0x19537bad,30);
 }else if(threeTriangles){
  REQUIRE(hash==(large?0x9bd44344:0xcdad4a41)
   && pixels[0]==0x19537bad,30);
 }else if(twoTriangles){
  REQUIRE(hash==(large?0xbb1f873b:0xd23b1ba1)
   && pixels[0]==0x19537bad
   && pixels[(large?20:10)*dimension+(large?50:25)]==0xffff4000
   && pixels[(large?40:20)*dimension+(large?20:10)]==0xffff4000,30);
 }else if(multiDraw || loadPass){
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
 vkUnmapMemory(device,outputMemory);
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
 if(indexPair || indexPairReference || indexSix)
  printf("Native Vulkan N100 %ux%u %s pair: PASS hash=%08x\n",
   dimension,dimension,indexPair?"indexed":indexSix?"six-index":"reference",hash);
 else if(copyVertex)printf("Native Vulkan N100 %ux%u copied GPU vertex triangle: PASS hash=%08x\n",
  dimension,dimension,hash);
 else if(copyReadback)printf("Native Vulkan N100 %ux%u image-to-buffer readback: PASS hash=%08x\n",
  dimension,dimension,hash);
 else if(copyBackground)printf("Native Vulkan N100 %ux%u buffer-to-image background: PASS hash=%08x\n",
  dimension,dimension,hash);
 else if(indexed)printf("Native Vulkan N100 %ux%u GPU indexed triangle: PASS hash=%08x\n",
  dimension,dimension,hash);
 else if(batchGpu)printf("Native Vulkan N100 %ux%u two triangles: PASS hash=%08x draws=2\n",
  dimension,dimension,hash);
 else if(drawSix || drawPage)printf("Native Vulkan N100 %ux%u two triangles: PASS hash=%08x calls=1\n",
  dimension,dimension,hash);
 else if(threeTriangles)
  printf("Native Vulkan N100 %ux%u three triangles: PASS hash=%08x calls=%u\n",
   dimension,dimension,hash,drawNine?1u:3u);
 else if(fourTriangles)
  printf("Native Vulkan N100 %ux%u four triangles: PASS hash=%08x calls=%u\n",
   dimension,dimension,hash,drawTwelve?1u:4u);
 else if(multiDraw)printf("Native Vulkan N100 %ux%u two draws in one render pass: PASS shape=%u hash=%08x draws=%lu\n",dimension,dimension,shape,hash,repeats);
 else if(loadPass)printf("Native Vulkan N100 %ux%u load across %s: PASS shape=%u hash=%08x draws=%lu\n",dimension,dimension,loadSubmit?"submissions":"render passes",shape,hash,repeats);
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

 if(batch || dual || semaphoreBatch || loadSubmit){VkCommandBuffer both[2]={command,second};vkFreeCommandBuffers(device,pool,2,both);}
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
 if(loadFramebuffer)vkDestroyFramebuffer(device,loadFramebuffer,nullptr);
 if(loadRenderPass)vkDestroyRenderPass(device,loadRenderPass,nullptr);
 vkDestroyFramebuffer(device,framebuffer,nullptr);
 vkDestroyRenderPass(device,pass,nullptr);
 vkDestroyImageView(device,view,nullptr);
 vkDestroyImage(device,image,nullptr);
 vkFreeMemory(device,imageMemory,nullptr);
 if(copyReadback){vkDestroyBuffer(device,readback,nullptr);
  vkFreeMemory(device,readbackMemory,nullptr);}
 if(copyBackground){vkDestroyBuffer(device,background,nullptr);
  vkFreeMemory(device,backgroundMemory,nullptr);}
 vkDestroyBuffer(device,vertex,nullptr);
 vkFreeMemory(device,vertexMemory,nullptr);
 vkDestroyBuffer(device,laterVertex,nullptr);
 vkFreeMemory(device,laterMemory,nullptr);
 if(indexed){vkDestroyBuffer(device,index,nullptr);
  vkFreeMemory(device,indexMemory,nullptr);}
 if(padding)vkFreeMemory(device,padding,nullptr);
 vkDestroyDevice(device,nullptr);vkDestroyInstance(instance,nullptr);
 return 0;
}
