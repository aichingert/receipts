#include "vulkan.h"

#include "device.cpp"
#include "instance.cpp"
#include "buffers.cpp"
#include "swap_chain.cpp"
#include "pipeline.cpp"

namespace vk {

void record_command_buffer(t_ren* ren, VkCommandBuffer command_buffer, uint32_t image_idx) {
    VkCommandBufferBeginInfo begin_info{};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    } 

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = ren->render_pass;
    render_pass_info.framebuffer = ren->swap_chain_framebuffers[image_idx];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = ren->swap_chain_extent;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ren->graphics_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ren->swap_chain_extent.width);
    viewport.height = static_cast<float>(ren->swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ren->swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {ren->vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, ren->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void init(t_ren* ren, const char* title) {
    init_instance(ren, title);
    init_surface(ren);
    init_device(ren);
    init_swap_chain(ren);
    init_image_views(ren);

    init_render_pass(ren);
    init_graphics_pipeline(ren);
    init_framebuffers(ren);

    t_queue_family_indices queue_family_indices = find_queue_families(ren, ren->physical_device);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if (vkCreateCommandPool(ren->device, &pool_info, nullptr, &ren->command_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    init_vertex_buffer(ren);
    init_index_buffer(ren);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ren->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    size_t size = MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer);
    ren->command_buffers = static_cast<VkCommandBuffer*>(malloc(size));

    if (vkAllocateCommandBuffers(ren->device, &alloc_info, ren->command_buffers) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command buffers!");
    }

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    size_t fences_size = sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT;
    size_t semaphores_size = sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT;

    ren->in_flight_fences = static_cast<VkFence*>(malloc(fences_size));
    ren->image_available_semaphores = static_cast<VkSemaphore*>(malloc(semaphores_size));
    ren->render_finished_semaphores = static_cast<VkSemaphore*>(malloc(semaphores_size));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(
                ren->device, 
                &semaphore_info, 
                nullptr, 
                &ren->image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(
                ren->device,
                &semaphore_info,
                nullptr,
                &ren->render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(
                ren->device,
                &fence_info,
                nullptr,
                &ren->in_flight_fences[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

}