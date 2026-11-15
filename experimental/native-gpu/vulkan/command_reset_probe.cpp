// SPDX-License-Identifier: MIT
// Command-pool reset must make an allocated command buffer recordable again.
#include "NativeVulkan.cpp"
#include <stdio.h>

int main()
{
 VkDevice_T device={};device.fd=-1;
 auto reset=(PFN_vkResetCommandPool)vkGetDeviceProcAddr(&device,"vkResetCommandPool");
 if(!reset)return 1;
 VkCommandPoolCreateInfo create={};create.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
 VkCommandPool pool=VK_NULL_HANDLE;
 if(vkCreateCommandPool(&device,&create,nullptr,&pool)!=VK_SUCCESS || !pool)return 2;
 VkCommandBufferAllocateInfo allocation={};
 allocation.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
 allocation.commandPool=pool;allocation.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
 allocation.commandBufferCount=1;
 VkCommandBuffer command=VK_NULL_HANDLE;
 if(vkAllocateCommandBuffers(&device,&allocation,&command)!=VK_SUCCESS || !command)return 3;
 VkCommandBufferBeginInfo begin={};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 4;
 if(reset(&device,pool,VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT)!=VK_SUCCESS)return 5;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 6;
 VkRenderPass_T pass={};pass.device=&device;
 VkImage_T image={};image.device=&device;image.width=32;
 VkImageView_T view={};view.device=&device;view.image=&image;
 VkFramebuffer_T framebuffer={};framebuffer.device=&device;framebuffer.pass=&pass;
 framebuffer.view=&view;
 VkFramebuffer_T secondFramebuffer=framebuffer;
 VkPipeline_T pipeline={};pipeline.device=&device;pipeline.pass=&pass;
 VkPipeline_T secondPipeline={};secondPipeline.device=&device;secondPipeline.pass=&pass;
 VkDeviceMemory_T memory={};memory.device=&device;
 VkBuffer_T vertex={};vertex.device=&device;vertex.memory=&memory;vertex.bytes=48;
 vertex.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 VkBuffer_T secondVertex={};secondVertex.device=&device;
 secondVertex.memory=&memory;secondVertex.bytes=48;
 secondVertex.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=&pass;render.framebuffer=&framebuffer;
 render.renderArea.extent={32,32};
 VkRenderPassBeginInfo secondRender=render;secondRender.framebuffer=&secondFramebuffer;
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,&pipeline);
 VkBuffer buffers[1]={&vertex};VkDeviceSize offsets[1]={0};
 vkCmdBindVertexBuffers(command,0,1,buffers,offsets);
 vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 vkCmdBeginRenderPass(command,&secondRender,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,&secondPipeline);
 VkBuffer secondBuffers[1]={&secondVertex};
 vkCmdBindVertexBuffers(command,0,1,secondBuffers,offsets);
 vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 for(unsigned i=0;i<3;i++){
  vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,&pipeline);
  vkCmdBindVertexBuffers(command,0,1,buffers,offsets);
  vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 }
 if(vkEndCommandBuffer(command)!=VK_SUCCESS)return 7;
 if(command->drawCount!=5 || command->drawCapacity<5
  || command->draws[0].framebuffer!=&framebuffer
  || command->draws[1].framebuffer!=&secondFramebuffer
  || command->draws[0].pipeline!=&pipeline
  || command->draws[0].vertex!=&vertex
  || command->draws[1].pipeline!=&secondPipeline
  || command->draws[1].vertex!=&secondVertex)return 16;
 if(reset(&device,pool,0)!=VK_SUCCESS)return 8;
 if(command->drawCount || !command->draws)return 17;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 9;
 auto resetOne=(PFN_vkResetCommandBuffer)vkGetDeviceProcAddr(&device,"vkResetCommandBuffer");
 if(!resetOne)return 10;
 if(resetOne(command,0)==VK_SUCCESS)return 11;
 vkDestroyCommandPool(&device,pool,nullptr);
 create.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
 if(vkCreateCommandPool(&device,&create,nullptr,&pool)!=VK_SUCCESS)return 12;
 allocation.commandPool=pool;command=VK_NULL_HANDLE;
 if(vkAllocateCommandBuffers(&device,&allocation,&command)!=VK_SUCCESS || !command)return 13;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS
  || resetOne(command,VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)!=VK_SUCCESS
  || vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 14;
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,&pipeline);
 vkCmdBindVertexBuffers(command,0,1,buffers,offsets);
 vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 if(vkEndCommandBuffer(command)!=VK_SUCCESS
  || vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 15;
 if(command->drawCount || !command->draws)return 18;
 if(resetOne(command,VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT)!=VK_SUCCESS
  || command->draws || command->drawCount || command->drawCapacity)return 19;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 20;
 vkDestroyCommandPool(&device,pool,nullptr);
 create.flags=0;
 if(vkCreateCommandPool(&device,&create,nullptr,&pool)!=VK_SUCCESS)return 21;
 allocation.commandPool=pool;allocation.commandBufferCount=2;
 VkCommandBuffer pair[2]={VK_NULL_HANDLE,VK_NULL_HANDLE};
 if(vkAllocateCommandBuffers(&device,&allocation,pair)!=VK_SUCCESS
  || !pair[0] || !pair[1] || pair[0]==pair[1])return 22;
 vkFreeCommandBuffers(&device,pool,1,pair);
 if(pool->commands!=pair[1] || pair[1]->pool!=pool)return 23;
 vkDestroyCommandPool(&device,pool,nullptr);
 puts("Vulkan command reset and two-buffer allocation: PASS");
 return 0;
}
