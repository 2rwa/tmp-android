#include "globe.h"
#include <stdexcept>
#include <android/log.h>

bool Engine::initGraphics(){
    VkAttachmentDescription color{};
    color.format=format;color.samples=VK_SAMPLE_COUNT_1_BIT;
    color.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;color.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;color.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;
    VkSubpassDependency dependency{};
    dependency.srcSubpass=VK_SUBPASS_EXTERNAL;dependency.dstSubpass=0;
    dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount=1;rp.pAttachments=&color;rp.subpassCount=1;rp.pSubpasses=&sub;
    rp.dependencyCount=1;rp.pDependencies=&dependency;
    vkCheck(vkCreateRenderPass(device,&rp,nullptr,&renderPass),"vkCreateRenderPass");
    for(auto view:views){
        VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fc.renderPass=renderPass;fc.attachmentCount=1;fc.pAttachments=&view;
        fc.width=size.width;fc.height=size.height;fc.layers=1;
        VkFramebuffer fb=VK_NULL_HANDLE;
        vkCheck(vkCreateFramebuffer(device,&fc,nullptr,&fb),"vkCreateFramebuffer");buffers.push_back(fb);
    }
    VkDescriptorSetLayoutBinding binding{};
    binding.binding=0;binding.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount=1;binding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dl.bindingCount=1;dl.pBindings=&binding;
    vkCheck(vkCreateDescriptorSetLayout(device,&dl,nullptr,&descLayout),"descriptor layout");
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};
    VkDescriptorPoolCreateInfo dc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dc.maxSets=1;dc.poolSizeCount=1;dc.pPoolSizes=&poolSize;
    vkCheck(vkCreateDescriptorPool(device,&dc,nullptr,&descPool),"descriptor pool");
    VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ds.descriptorPool=descPool;ds.descriptorSetCount=1;ds.pSetLayouts=&descLayout;
    vkCheck(vkAllocateDescriptorSets(device,&ds,&desc),"descriptor allocation");
    VkDescriptorBufferInfo ub{uniformBuffer,0,sizeof(Uniforms)};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet=desc;write.dstBinding=0;write.descriptorCount=1;
    write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&ub;
    vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount=1;pl.pSetLayouts=&descLayout;
    vkCheck(vkCreatePipelineLayout(device,&pl,nullptr,&pipelineLayout),"pipeline layout");
    auto vsCode=loadSpv(assets,"shaders/globe.vert.spv");
    auto fsCode=loadSpv(assets,"shaders/globe.frag.spv");
    VkShaderModuleCreateInfo vsInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vsInfo.codeSize=vsCode.size()*4;vsInfo.pCode=vsCode.data();
    VkShaderModuleCreateInfo fsInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fsInfo.codeSize=fsCode.size()*4;fsInfo.pCode=fsCode.data();
    VkShaderModule vs=VK_NULL_HANDLE,fs=VK_NULL_HANDLE;
    vkCheck(vkCreateShaderModule(device,&vsInfo,nullptr,&vs),"vert shader module");
    vkCheck(vkCreateShaderModule(device,&fsInfo,nullptr,&fs),"frag shader module");
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vs;stages[0].pName="main";
    stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=fs;stages[1].pName="main";
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport vp{0,0,float(size.width),float(size.height),0,1};
    VkRect2D rect{{0,0},size};
    VkPipelineViewportStateCreateInfo vpInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpInfo.viewportCount=1;vpInfo.pViewports=&vp;vpInfo.scissorCount=1;vpInfo.pScissors=&rect;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;
    rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1.f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState cb{};cb.colorWriteMask=0xf;
    VkPipelineColorBlendStateCreateInfo cbInfo{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbInfo.attachmentCount=1;cbInfo.pAttachments=&cb;
    VkGraphicsPipelineCreateInfo pc{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pc.stageCount=2;pc.pStages=stages;pc.pVertexInputState=&vi;
    pc.pInputAssemblyState=&ia;pc.pViewportState=&vpInfo;
    pc.pRasterizationState=&rs;pc.pMultisampleState=&ms;
    pc.pColorBlendState=&cbInfo;pc.layout=pipelineLayout;pc.renderPass=renderPass;
    VkResult result=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pc,nullptr,&pipeline);
    vkDestroyShaderModule(device,vs,nullptr);vkDestroyShaderModule(device,fs,nullptr);
    vkCheck(result,"vkCreateGraphicsPipelines");
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;cp.queueFamilyIndex=queueIndex;
    vkCheck(vkCreateCommandPool(device,&cp,nullptr,&commandPool),"vkCreateCommandPool");
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ca.commandPool=commandPool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;
    vkCheck(vkAllocateCommandBuffers(device,&ca,&command),"vkAllocateCommandBuffers");
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCheck(vkCreateSemaphore(device,&sem,nullptr,&acquired),"acquire semaphore");
    vkCheck(vkCreateSemaphore(device,&sem,nullptr,&completed),"render semaphore");
    VkFenceCreateInfo fe{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fe.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    vkCheck(vkCreateFence(device,&fe,nullptr,&fence),"vkCreateFence");
    return true;
}
bool Engine::draw(){
    if(!device || !swap)return false;
    VkResult result=vkWaitForFences(device,1,&fence,VK_TRUE,1000000000ull);
    if(result!=VK_SUCCESS){setMessage("GPU fence timed out");return false;}
    uint32_t index=0;
    result=vkAcquireNextImageKHR(device,swap,1000000000ull,acquired,VK_NULL_HANDLE,&index);
    if(result==VK_ERROR_OUT_OF_DATE_KHR || result==VK_SUBOPTIMAL_KHR){setMessage("Surface resized; recreating");return false;}
    if(result!=VK_SUCCESS){setMessage("Swapchain acquire failed");return false;}
    vkResetFences(device,1,&fence);
    vkResetCommandBuffer(command,0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if(vkBeginCommandBuffer(command,&begin)!=VK_SUCCESS){setMessage("Command buffer begin failed");return false;}
    VkClearValue clear{};clear.color.float32[0]=.02f;clear.color.float32[1]=.035f;
    clear.color.float32[2]=.065f;clear.color.float32[3]=1.f;
    VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render.renderPass=renderPass;render.framebuffer=buffers[index];
    render.renderArea.extent=size;render.clearValueCount=1;render.pClearValues=&clear;
    vkCmdBeginRenderPass(command,&render,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
    vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&desc,0,nullptr);
    vkCmdDraw(command,3,1,0,0);vkCmdEndRenderPass(command);
    if(vkEndCommandBuffer(command)!=VK_SUCCESS){setMessage("Command buffer end failed");return false;}
    VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;
    submit.pWaitDstStageMask=&stage;submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&completed;
    if(vkQueueSubmit(queue,1,&submit,fence)!=VK_SUCCESS){setMessage("GPU submit failed");return false;}
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount=1;present.pWaitSemaphores=&completed;
    present.swapchainCount=1;present.pSwapchains=&swap;present.pImageIndices=&index;
    result=vkQueuePresentKHR(queue,&present);
    // Intentionally serialize frames in this first prototype; no host-write/render hazard.
    vkQueueWaitIdle(queue);
    if(result!=VK_SUCCESS){setMessage("GPU present interrupted");return false;}
    return true;
}
