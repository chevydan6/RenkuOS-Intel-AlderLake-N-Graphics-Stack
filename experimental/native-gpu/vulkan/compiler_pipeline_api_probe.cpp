// SPDX-License-Identifier: MIT
// Exercise pipeline preflight without a GPU; fd=-1 makes owner allocation fail.
#include "NativeVulkan.cpp"
#include "RgbSpirvFixture.h"
#include <stdio.h>
#include <vector>

int main()
{
 VkDevice_T device={};device.fd=-1;
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
