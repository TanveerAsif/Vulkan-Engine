#include "Wrapper.h"
#include <stdexcept>

namespace VulkanCore {

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags)
    {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = usageFlags,
            .pInheritanceInfo = nullptr
        };

        VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer!");
        }
    }

    VkSemaphore CreateSemaphore(VkDevice device)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkSemaphore semaphore;
        VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphore!");
        }

        return semaphore;
    }
}
