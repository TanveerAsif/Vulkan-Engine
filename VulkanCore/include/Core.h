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

namespace VulkanCore
{

// Forward declaration
class Texture;

class BufferAndMemory
{
  public:
    VkBuffer mBuffer;
    VkDeviceMemory mMemory;
    VkDeviceSize mAllocationSize;

    BufferAndMemory() : mBuffer(VK_NULL_HANDLE), mMemory(VK_NULL_HANDLE), mAllocationSize(0) {}

    void Destroy(VkDevice device);
    void update(VkDevice device, const void* pData, VkDeviceSize size);
};

class VulkanCore
{
  public:
    VulkanCore();
    ~VulkanCore();

    // Friend class to allow Texture to access private methods
    friend class Texture;

    void initialize(std::string appName, GLFWwindow* window, bool depthEnabled);
    int32_t getSwapchainImageCount() const;
    void createCommandBuffers(VkCommandBuffer* commandBuffers, int32_t count);
    void freeCommandBuffers(VkCommandBuffer* commandBuffers, int32_t count);

    VkImage getSwapchainImage(int32_t index) const;
    VulkanQueue* getGraphicsQueue()
    {
        return &mGraphicsQueue;
    }
    VkDevice getDevice() const
    {
        return mLogicalDevice;
    }

    VkRenderPass createSimpleRenderPass();
    std::vector<VkFramebuffer> createFrameBuffer(VkRenderPass renderPass);
    void destroyFramebuffers(std::vector<VkFramebuffer>& framebuffers);

    BufferAndMemory createVertexBuffer(const void* pVertices, size_t size);
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

    const VkPhysicalDeviceLimits& getPhysicalDeviceLimits() const
    {
        return mPhysicalDevice.getSelectedPhysicalDeviceProperties().mDeviceProperties.limits;
    }

    VkImageView getSwapchainImageView(uint32_t index) const;
    VkImageView getDepthImageView(uint32_t index) const;

    VkInstance getVulkanInstance() const
    {
        return mVulkanInstance;
    }

    uint32_t getQueueFamilyIndex() const
    {
        return mQueueFamilyIndex;
    }

    PhysicalDevice getPhysicalDevice() const
    {
        return mPhysicalDevice;
    }

    void beginDynamicRendering(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkClearValue* clearColor,
                               VkClearValue* clearDepth);

    GLFWwindow* getWindow() const
    {
        return mWindow;
    }

    void getFramebufferSize(int& width, int& height) const
    {
        glfwGetFramebufferSize(mWindow, &width, &height);
    }

  private:
    void createInstance(std::string appName);
    void createDebugCallback();
    void createSurface(GLFWwindow* window);
    void createLogicalDevice();
    void createSwapChain();
    void createCommandBufferPool();
    BufferAndMemory createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    uint32_t getMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createTextureImageFromData(Texture& outTexture, const void* pixels, int texWidth, int texHeight,
                                    VkFormat imageFormat, bool isCubemap);
    void createCubemapTexture(std::string filePath, Texture& outTexture);
    void createTextureFromData(const void* pixels, uint32_t width, uint32_t height, VkFormat format, bool isCubemap,
                               Texture& outTexture);
    void createImage(Texture& outTexture, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags memPropertiesj, bool isCubemap);
    void updateTextureImage(Texture& outTexture, uint32_t width, uint32_t height, VkFormat format, int32_t layerCount,
                            const void* pixels, bool isCubemap);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               int32_t layerCount);
    void submitCopyCommand();
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDeviceSize layerSize,
                           int32_t layerCount);
    void createDepthResources();

    void getInstanceVersion();

    VkInstance mVulkanInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    GLFWwindow* mWindow;
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

    struct InstanceVersion
    {
        uint32_t major{0};
        uint32_t minor{0};
        uint32_t patch{0};
    } mInstanceVersion;
};

} // namespace VulkanCore

#endif // VULKANCORE_CORE_H
