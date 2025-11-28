#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


namespace VulkanCore
{

class VulkanQueue
{
public:
    VulkanQueue();
    ~VulkanQueue();

    void init(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamilyIndex, uint32_t queueIndex);
    void destroySemaphores();

    uint32_t acquireNextImage();
    void submitSync(VkCommandBuffer commandBuffer);
    void submitAsync(VkCommandBuffer commandBuffer);
    void submitAsync(VkCommandBuffer* commandBuffer, uint32_t numOfCommandBuffers);
    void presentImage(uint32_t imageIndex);

    // Hang until queue finishes all commands buffers insided
    void waitIdle();

    VkQueue getVkQueue() const { return mQueue; }

private:
    void createSyncObjects();

    VkDevice mDevice;
    VkQueue mQueue;
    VkSwapchainKHR mSwapchain;

    std::vector<VkSemaphore> mRenderCompleteSemaphores;  // Signals when rendering is complete
    std::vector<VkSemaphore> mImageAvailableSemaphores;  // Signals when an image is available for rendering
    std::vector<VkFence>     mInFlightFences;            // Fences to ensure that command buffers have finished executing

    uint32_t mNumberOfSwapchainImages;      // Number of images in the swapchain
    uint32_t mAcquiredImageIndex;           // Index of the last acquired swapchain image
    uint32_t mFrameIndex;                   // Index of the current frame
};

} // namespace VulkanCore
