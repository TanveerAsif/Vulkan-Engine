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

// Forward declaration
class Texture;

class BufferAndMemory {
public:
  VkBuffer mBuffer;
  VkDeviceMemory mMemory;
  VkDeviceSize mAllocationSize;

  BufferAndMemory()
      : mBuffer(VK_NULL_HANDLE), mMemory(VK_NULL_HANDLE), mAllocationSize(0) {}

  void Destroy(VkDevice device);
  void update(VkDevice device, const void *pData, VkDeviceSize size);
};

// class VulkanTexture {
// public:
//   VkImage mImage;
//   VkDeviceMemory mImageMemory;
//   VkImageView mImageView;
//   VkSampler mSampler;
//   uint32_t mWidth;
//   uint32_t mHeight;

//   VulkanTexture()
//       : mImage(VK_NULL_HANDLE), mImageMemory(VK_NULL_HANDLE),
//         mImageView(VK_NULL_HANDLE), mSampler(VK_NULL_HANDLE), mWidth(0),
//         mHeight(0) {}

//   void Destroy(VkDevice device);
// };

class VulkanCore {
public:
  VulkanCore();
  ~VulkanCore();

  // Friend class to allow Texture to access private methods
  friend class Texture;

  void initialize(std::string appName, GLFWwindow *window, bool depthEnabled);
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

  void createTexture(std::string filePath, Texture& outTexture);
  VkFormat getDepthFormat() const
  {
      return mPhysicalDevice.getSelectedPhysicalDeviceProperties().mDepthFormat;
  }
  VkFormat getSwapchainSurfaceFormat() const
  {
      return mSwapchainSurfaceFormat.format;
  }

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

  void createTextureImageFromData(Texture& outTexture, const void* pixels, int texWidth, int texHeight,
                                  VkFormat imageFormat);
  void createImage(Texture& outTexture, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags memProperties);
  void updateTextureImage(Texture& outTexture, uint32_t width, uint32_t height, VkFormat format, const void* pixels);

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  void submitCopyCommand();
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void createDepthResources();

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

  bool mDepthEnabled;
  std::vector<Texture> mDepthImages;
};

} // namespace VulkanCore

#endif // VULKANCORE_CORE_H
