#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


namespace VulkanCore {

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);

    VkSemaphore CreateSemaphore(VkDevice Device);

}
