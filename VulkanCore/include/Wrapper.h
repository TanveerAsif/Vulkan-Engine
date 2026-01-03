#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{

void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);

VkSemaphore CreateSemaphore(VkDevice Device);

void imageMemBarrier(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format, VkImageLayout OldLayout,
                     VkImageLayout NewLayout, int32_t layerCount);

VkImageView createImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags,
                            bool isCubemap);

VkSampler createTextureSampler(VkDevice Device, VkFilter MinFilter, VkFilter MagFilter,
                               VkSamplerAddressMode AddressMode);
} // namespace VulkanCore
