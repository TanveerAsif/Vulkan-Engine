#include "Queue.h"
#include <cassert>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include "Wrapper.h"

namespace VulkanCore
{

VulkanQueue::VulkanQueue()
    : mDevice{VK_NULL_HANDLE}
    , mQueue{VK_NULL_HANDLE}
    , mSwapchain{VK_NULL_HANDLE}
    , mRenderCompleteSemaphores{}
    , mImageAvailableSemaphores{}
    , mInFlightFences{}
    , mNumberOfSwapchainImages{0}
    , mAcquiredImageIndex{0}
    , mFrameIndex{0}
{
}

VulkanQueue::~VulkanQueue()
{
    destroySemaphores();
    mDevice = VK_NULL_HANDLE;
    mQueue = VK_NULL_HANDLE;
    mSwapchain = VK_NULL_HANDLE;
}

void VulkanQueue::init(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamilyIndex, uint32_t queueIndex)
{
    mDevice = device;
    mSwapchain = swapchain;

    // Queue doesn't need to be created, just fetch from device
    vkGetDeviceQueue(mDevice, queueFamilyIndex, queueIndex, &mQueue);
    std::cout<<"VulkanQueue::init - Queue initialized successfully."<<std::endl;

    if(vkGetSwapchainImagesKHR(mDevice, mSwapchain, &mNumberOfSwapchainImages, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get number of swapchain images.");
    }

    createSyncObjects();
}


void VulkanQueue::createSyncObjects()
{
    mRenderCompleteSemaphores.resize(mNumberOfSwapchainImages);
    mImageAvailableSemaphores.resize(mNumberOfSwapchainImages);
    mInFlightFences.resize(mNumberOfSwapchainImages);

    for(VkSemaphore& Sem : mImageAvailableSemaphores)
    {
        Sem = CreateSemaphore(mDevice);
    }

    for(VkSemaphore& Sem : mRenderCompleteSemaphores)
    {
        Sem = CreateSemaphore(mDevice);
    }

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT  // Start signaled so that the first frame can be submitted without waiting
    };

    for(VkFence& Fence : mInFlightFences)
    {
        if (vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &Fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fence.");
        }
    }
}

void VulkanQueue::destroySemaphores()
{
    for (VkSemaphore& Sem : mImageAvailableSemaphores) {
        waitIdle();
		vkDestroySemaphore(mDevice, Sem, nullptr);
	}

	for (VkSemaphore& Sem : mRenderCompleteSemaphores) {
        waitIdle();
		vkDestroySemaphore(mDevice, Sem, nullptr);
	}

    for(VkFence& Fence : mInFlightFences) {
        vkWaitForFences(mDevice, 1, &Fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(mDevice, Fence, nullptr);
    }

    mImageAvailableSemaphores.clear();
    mRenderCompleteSemaphores.clear();
    mInFlightFences.clear();
}

uint32_t VulkanQueue::acquireNextImage()
{
    vkWaitForFences(mDevice, 1, &mInFlightFences[mFrameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(mDevice, 1, &mInFlightFences[mFrameIndex]);

    VkResult result = vkAcquireNextImageKHR(
        mDevice,
        mSwapchain,
        UINT64_MAX,  // timeout, waiting indefinitely
        mImageAvailableSemaphores[mFrameIndex],
        VK_NULL_HANDLE,
        &mAcquiredImageIndex
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to acquire next image from swapchain.");
    }

    return mAcquiredImageIndex;
}


void VulkanQueue::waitIdle()
{
    vkQueueWaitIdle(mQueue);
}


void VulkanQueue::submitSync(VkCommandBuffer commandBuffer)
{
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,  // No wait semaphores
        .pWaitDstStageMask = nullptr, // No wait stages
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr   // No signal semaphores
    };

    if (vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit command buffer to queue.");
    }

    waitIdle();
}


void VulkanQueue::submitAsync(VkCommandBuffer commandBuffer)
{
    submitAsync(&commandBuffer, 1);
}

void VulkanQueue::submitAsync(VkCommandBuffer* commandBuffer, uint32_t numOfCommandBuffers)
{
    // Specify the pipeline stage at which the semaphore wait will occur
    // This ensures that the command buffer waits until the color attachment output stage is ready
    // before it begins execution.
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mImageAvailableSemaphores[mFrameIndex], // Wait until image is available
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = numOfCommandBuffers,
        .pCommandBuffers = commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mRenderCompleteSemaphores[mAcquiredImageIndex]
    };

    if (vkQueueSubmit(mQueue, 1, &submitInfo, mInFlightFences[mFrameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit command buffer to queue.");
    }
}


void VulkanQueue::presentImage(uint32_t imageIndex)
{
    assert(imageIndex == mAcquiredImageIndex); // Ensure the image index matches the acquired image index

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mRenderCompleteSemaphores[mAcquiredImageIndex], // Wait until rendering is complete
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    VkResult result = vkQueuePresentKHR(mQueue, &presentInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present image to swapchain.");
    }

    mFrameIndex = (mFrameIndex + 1) % mNumberOfSwapchainImages;
}

} // namespace VulkanCore
