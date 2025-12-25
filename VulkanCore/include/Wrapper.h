#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


namespace VulkanCore {

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);

    VkSemaphore CreateSemaphore(VkDevice Device);

    void imageMemBarrier(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format,
                         VkImageLayout OldLayout, VkImageLayout NewLayout);

    VkImageView createImageView(VkDevice Device, VkImage Image, VkFormat Format,
                                VkImageAspectFlags AspectFlags);

    VkSampler createTextureSampler(VkDevice Device, VkFilter MinFilter,
                                   VkFilter MagFilter,
                                   VkSamplerAddressMode AddressMode);
}
