// SPDX-License-Identifier: MIT
// Exercise pipeline preflight without a GPU; fd=-1 makes owner allocation fail.
#include "NativeVulkan.cpp"
#include "RgbSpirvFixture.h"
#include <stdio.h>
#include <vector>

int main()
{
 VkInstance_T instance={};instance.physical.owner=&instance;
 VkPhysicalDevice physical=&instance.physical;
 auto getFormat=(PFN_vkGetPhysicalDeviceFormatProperties)
  vkGetInstanceProcAddr(&instance,"vkGetPhysicalDeviceFormatProperties");
 auto getImageFormat=(PFN_vkGetPhysicalDeviceImageFormatProperties)
  vkGetInstanceProcAddr(&instance,"vkGetPhysicalDeviceImageFormatProperties");
 if(!getFormat || !getImageFormat)return 16;
 VkFormatProperties format={};
 getFormat(physical,VK_FORMAT_B8G8R8A8_UNORM,&format);
 if(!(format.linearTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
  || !(format.linearTilingFeatures&VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
  || format.optimalTilingFeatures || format.bufferFeatures)return 17;
 getFormat(physical,VK_FORMAT_R32G32B32A32_SFLOAT,&format);
 if(format.bufferFeatures!=VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT
  || format.linearTilingFeatures || format.optimalTilingFeatures)return 23;
 getFormat(physical,VK_FORMAT_R8G8B8A8_UNORM,&format);
 if(format.linearTilingFeatures || format.optimalTilingFeatures
  || format.bufferFeatures)return 18;
 VkImageFormatProperties imageFormat={};
 if(getImageFormat(physical,VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_TYPE_2D,
   VK_IMAGE_TILING_LINEAR,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
   |VK_IMAGE_USAGE_TRANSFER_SRC_BIT,0,&imageFormat)!=VK_SUCCESS
  || imageFormat.maxExtent.width!=32 || imageFormat.maxExtent.height!=32
  || imageFormat.maxExtent.depth!=1 || imageFormat.maxMipLevels!=1
  || imageFormat.maxArrayLayers!=1 || imageFormat.sampleCounts!=VK_SAMPLE_COUNT_1_BIT
  || imageFormat.maxResourceSize!=4096)return 19;
 if(getImageFormat(physical,VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_TYPE_2D,
   VK_IMAGE_TILING_LINEAR,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,0,
   &imageFormat)!=VK_SUCCESS)return 21;
 if(getImageFormat(physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_TYPE_2D,
   VK_IMAGE_TILING_LINEAR,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,0,
   &imageFormat)!=VK_ERROR_FORMAT_NOT_SUPPORTED)return 20;
 VkDevice_T device={};device.fd=-1;
 VkImageCreateInfo imageInfo={};imageInfo.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
 imageInfo.imageType=VK_IMAGE_TYPE_2D;imageInfo.format=VK_FORMAT_B8G8R8A8_UNORM;
 imageInfo.extent={32,32,1};imageInfo.mipLevels=1;imageInfo.arrayLayers=1;
 imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;imageInfo.tiling=VK_IMAGE_TILING_LINEAR;
 imageInfo.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
 imageInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
 VkImage colorOnly=VK_NULL_HANDLE;
 if(vkCreateImage(&device,&imageInfo,nullptr,&colorOnly)!=VK_SUCCESS
  || !colorOnly)return 22;
 vkDestroyImage(&device,colorOnly,nullptr);
 VkShaderModuleCreateInfo create={};create.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 create.pCode=RenkuKnownSpirv::Vertex;create.codeSize=sizeof(RenkuKnownSpirv::Vertex);
 VkShaderModule checked=VK_NULL_HANDLE;
 if(vkCreateShaderModule(&device,&create,nullptr,&checked)!=VK_SUCCESS || !checked)return 3;
 vkDestroyShaderModule(&device,checked,nullptr);
 uint32_t malformed[sizeof(RenkuKnownSpirv::Vertex)/4];
 memcpy(malformed,RenkuKnownSpirv::Vertex,sizeof(malformed));malformed[5]=0x00000011;
 create.pCode=malformed;
 if(vkCreateShaderModule(&device,&create,nullptr,&checked)
  !=VK_ERROR_INITIALIZATION_FAILED || checked)return 4;
 create.pCode=RenkuKnownSpirv::FragmentHalf;
 create.codeSize=sizeof(RenkuKnownSpirv::FragmentHalf);
 if(vkCreateShaderModule(&device,&create,nullptr,&checked)!=VK_SUCCESS || !checked)return 5;
 vkDestroyShaderModule(&device,checked,nullptr);
 VkShaderModule_T vs={};vs.device=&device;vs.bytes=sizeof(RenkuKnownSpirv::Vertex);
 vs.code=const_cast<uint32_t*>(RenkuKnownSpirv::Vertex);
 VkShaderModule_T fs={};fs.device=&device;fs.bytes=sizeof(RenkuKnownSpirv::Fragment);
 fs.code=const_cast<uint32_t*>(RenkuKnownSpirv::Fragment);
 VkRenderPass_T pass={};pass.device=&device;
 VkPipelineLayout_T layout={};layout.device=&device;
 VkPipelineShaderStageCreateInfo stages[2]={};
 for(auto& s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}
 stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=&vs;
 stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=&fs;
 VkVertexInputBindingDescription binding={0,16,VK_VERTEX_INPUT_RATE_VERTEX};
 VkVertexInputAttributeDescription attribute={0,0,VK_FORMAT_R32G32B32A32_SFLOAT,0};
 VkPipelineVertexInputStateCreateInfo input={};input.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
 input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;
 input.vertexAttributeDescriptionCount=1;input.pVertexAttributeDescriptions=&attribute;
 VkPipelineInputAssemblyStateCreateInfo assembly={};assembly.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
 assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 VkViewport viewport={0,0,32,32,0,1};VkRect2D scissor={{0,0},{32,32}};
 VkPipelineViewportStateCreateInfo view={};view.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
 view.viewportCount=1;view.pViewports=&viewport;view.scissorCount=1;view.pScissors=&scissor;
 VkPipelineRasterizationStateCreateInfo raster={};raster.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
 raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;
 raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo samples={};samples.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
 samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineColorBlendAttachmentState color={};color.colorWriteMask=0xf;
 VkPipelineColorBlendStateCreateInfo blend={};blend.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
 blend.attachmentCount=1;blend.pAttachments=&color;
 VkGraphicsPipelineCreateInfo info={};info.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
 info.stageCount=2;info.pStages=stages;info.pVertexInputState=&input;
 info.pInputAssemblyState=&assembly;info.pViewportState=&view;
 info.pRasterizationState=&raster;info.pMultisampleState=&samples;
 info.pColorBlendState=&blend;info.layout=&layout;info.renderPass=&pass;
 VkPipeline pipeline=VK_NULL_HANDLE;
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pipeline)return 1;
 VkGraphicsPipelineCreateInfo pair[2]={info,info};
 VkPipeline pairOutput[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,2,pair,nullptr,pairOutput)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pairOutput[0] || pairOutput[1])return 14;
 VkPipelineRasterizationStateCreateInfo badRaster=raster;
 badRaster.lineWidth=2;
 pair[1].pRasterizationState=&badRaster;
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,2,pair,nullptr,pairOutput)
  !=VK_ERROR_FEATURE_NOT_PRESENT || pairOutput[0] || pairOutput[1])return 15;
 std::vector<uint32_t> namedVertex(std::begin(RenkuKnownSpirv::Vertex),
  std::end(RenkuKnownSpirv::Vertex));
 namedVertex.insert(namedVertex.begin()+17,
  {0x00040005u,1u,0x72746e65u,0x00000079u}); // OpName %main "entry"
 create.pCode=namedVertex.data();create.codeSize=namedVertex.size()*4;
 if(vkCreateShaderModule(&device,&create,nullptr,&checked)!=VK_SUCCESS || !checked)
  return 12;
 vkDestroyShaderModule(&device,checked,nullptr);
 vs.code=namedVertex.data();vs.bytes=namedVertex.size()*4;
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pipeline)return 13;
 vs.code=const_cast<uint32_t*>(RenkuKnownSpirv::Vertex);
 vs.bytes=sizeof(RenkuKnownSpirv::Vertex);
 fs.code=const_cast<uint32_t*>(RenkuKnownSpirv::FragmentHalf);
 fs.bytes=sizeof(RenkuKnownSpirv::FragmentHalf);
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pipeline)return 6;
 auto rgb=RgbSpirvFixture(0x3f000000,0x3f400000,0x3e800000,true);
 if(rgb.empty())return 9;
 create.pCode=rgb.data();create.codeSize=rgb.size()*4;
 if(vkCreateShaderModule(&device,&create,nullptr,&checked)!=VK_SUCCESS || !checked)
  return 10;
 vkDestroyShaderModule(&device,checked,nullptr);
 fs.code=rgb.data();fs.bytes=rgb.size()*4;
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pipeline)return 11;
 fs.code=const_cast<uint32_t*>(RenkuKnownSpirv::Fragment);
 fs.bytes=sizeof(RenkuKnownSpirv::Fragment);
 uint32_t altered[sizeof(RenkuKnownSpirv::Fragment)/4];
 memcpy(altered,RenkuKnownSpirv::Fragment,sizeof(altered));
 bool changed=false;
 for(unsigned i=0;i<sizeof(altered)/4;i++)
  if(altered[i]==0x3e800000){altered[i]=0x3f400000;changed=true;break;}
 if(!changed)return 7;
 fs.code=altered;
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_OUT_OF_DEVICE_MEMORY || pipeline)return 2;
 for(unsigned i=0;i<sizeof(altered)/4;i++)
  if(altered[i]==0x3f400000){altered[i]=0x3f800000;break;}
 if(vkCreateGraphicsPipelines(&device,VK_NULL_HANDLE,1,&info,nullptr,&pipeline)
  !=VK_ERROR_FEATURE_NOT_PRESENT || pipeline)return 8;
 puts("Vulkan API probe: accepted bounded RGB pipeline; rejected 1.0 green pipeline");
 return 0;
}
