#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#include <string>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "PhysicalDevice.h"
#include "Queue.h"

namespace VulkanCore {

    class VulkanCore
    {
        public:
            VulkanCore();
            ~VulkanCore();

            void initialize(std::string appName, GLFWwindow *window);
            int32_t getSwapchainImageCount() const;
            void createCommandBuffers(VkCommandBuffer* commandBuffers, int32_t count);
            void freeCommandBuffers(VkCommandBuffer* commandBuffers, int32_t count);

            VkImage getSwapchainImage(int32_t index) const;
            VulkanQueue* getGraphicsQueue() { return &mGraphicsQueue; }
            VkDevice getDevice() const { return mLogicalDevice; }

            VkRenderPass createSimpleRenderPass();
            std::vector<VkFramebuffer> createFrameBuffer(VkRenderPass renderPass);
            void destroyFramebuffers(std::vector<VkFramebuffer>& framebuffers);

        private:
            void createInstance(std::string appName);
            void createDebugCallback();
            void createSurface(GLFWwindow *window);
            void createLogicalDevice();
            void createSwapChain();
            void createCommandBufferPool();

            VkInstance mVulkanInstance;
            VkDebugUtilsMessengerEXT mDebugMessenger;
            GLFWwindow* mWindow;
            VkSurfaceKHR mSurface;

            PhysicalDevice mPhysicalDevice;
            uint32_t mQueueFamilyIndex; // Index of the queue family on selected physical device

            VkDevice mLogicalDevice;

            // Swapchain handle which maintain the series of images for presentation, format etc.,
            VkSurfaceFormatKHR mSwapchainSurfaceFormat;
            VkSwapchainKHR mSwapchain;
            std::vector<VkImage> mSwapchainImages;
            std::vector<VkImageView> mSwapchainImageViews;

            // Memory pool for command buffers
            VkCommandPool mCommandPool;

            VulkanQueue   mGraphicsQueue;
            std::vector<VkFramebuffer> mFrameBuffers;
    };

}
