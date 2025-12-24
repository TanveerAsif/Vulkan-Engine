#ifndef VULKANCORE_CORE_H
#define VULKANCORE_CORE_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#include <string>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "PhysicalDevice.h"
#include "Queue.h"
#include <cstring>

namespace VulkanCore {

class BufferAndMemory {
public:
  VkBuffer mBuffer;
  VkDeviceMemory mMemory;
  VkDeviceSize mAllocationSize;

  BufferAndMemory()
      : mBuffer(VK_NULL_HANDLE), mMemory(VK_NULL_HANDLE), mAllocationSize(0) {}

  void Destroy(VkDevice device) {
    if (mBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device, mBuffer, nullptr);
      mBuffer = VK_NULL_HANDLE;
    }
    if (mMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device, mMemory, nullptr);
      mMemory = VK_NULL_HANDLE;
    }
    mAllocationSize = 0;
  }

  void update(VkDevice device, const void *pData, VkDeviceSize size) {
    void *mappedData = nullptr;
    if (vkMapMemory(device, mMemory, 0, size, 0, &mappedData) == VK_SUCCESS) {
      memcpy(mappedData, pData, static_cast<size_t>(size));
      vkUnmapMemory(device, mMemory);
    }
  }
};

class VulkanCore {
public:
  VulkanCore();
  ~VulkanCore();

  void initialize(std::string appName, GLFWwindow *window);
  int32_t getSwapchainImageCount() const;
  void createCommandBuffers(VkCommandBuffer *commandBuffers, int32_t count);
  void freeCommandBuffers(VkCommandBuffer *commandBuffers, int32_t count);

  VkImage getSwapchainImage(int32_t index) const;
  VulkanQueue *getGraphicsQueue() { return &mGraphicsQueue; }
  VkDevice getDevice() const { return mLogicalDevice; }

  VkRenderPass createSimpleRenderPass();
  std::vector<VkFramebuffer> createFrameBuffer(VkRenderPass renderPass);
  void destroyFramebuffers(std::vector<VkFramebuffer> &framebuffers);

  BufferAndMemory createVertexBuffer(const void *pVertices, size_t size);
  std::vector<BufferAndMemory> createUniformBuffers(size_t size);

private:
  void createInstance(std::string appName);
  void createDebugCallback();
  void createSurface(GLFWwindow *window);
  void createLogicalDevice();
  void createSwapChain();
  void createCommandBufferPool();
  BufferAndMemory createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties);
  uint32_t getMemoryTypeIndex(uint32_t typeFilter,
                              VkMemoryPropertyFlags properties);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

  VkInstance mVulkanInstance;
  VkDebugUtilsMessengerEXT mDebugMessenger;
  GLFWwindow *mWindow;
  VkSurfaceKHR mSurface;

  PhysicalDevice mPhysicalDevice;
  uint32_t mQueueFamilyIndex; // Index of the queue family on selected physical
                              // device

  VkDevice mLogicalDevice;

  // Swapchain handle which maintain the series of images for presentation,
  // format etc.,
  VkSurfaceFormatKHR mSwapchainSurfaceFormat;
  VkSwapchainKHR mSwapchain;
  std::vector<VkImage> mSwapchainImages;
  std::vector<VkImageView> mSwapchainImageViews;

  // Memory pool for command buffers
  VkCommandPool mCommandPool;

  VulkanQueue mGraphicsQueue;
  std::vector<VkFramebuffer> mFrameBuffers;

  VkCommandBuffer mCopyCmdBuffer;
};

} // namespace VulkanCore

#endif // VULKANCORE_CORE_H
