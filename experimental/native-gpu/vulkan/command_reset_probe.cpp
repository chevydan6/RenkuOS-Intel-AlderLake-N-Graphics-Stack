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
 VkImage_T image={};image.device=&device;
 VkImageView_T view={};view.device=&device;view.image=&image;
 VkFramebuffer_T framebuffer={};framebuffer.device=&device;framebuffer.pass=&pass;
 framebuffer.view=&view;
 VkPipeline_T pipeline={};pipeline.device=&device;pipeline.pass=&pass;
 VkDeviceMemory_T memory={};memory.device=&device;
 VkBuffer_T vertex={};vertex.device=&device;vertex.memory=&memory;
 VkRenderPassBeginInfo render={};render.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
 render.renderPass=&pass;render.framebuffer=&framebuffer;
 render.renderArea.extent={32,32};
 vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,&pipeline);
 VkBuffer buffers[1]={&vertex};VkDeviceSize offsets[1]={0};
 vkCmdBindVertexBuffers(command,0,1,buffers,offsets);
 vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
 if(vkEndCommandBuffer(command)!=VK_SUCCESS)return 7;
 if(reset(&device,pool,0)!=VK_SUCCESS)return 8;
 if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS)return 9;
 vkDestroyCommandPool(&device,pool,nullptr);
 puts("Vulkan command-pool reset: reused recording and executable buffers");
 return 0;
}
