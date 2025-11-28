

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include "Queue.h"

namespace VulkanApp{

class App
{
public:
    App();
    ~App();

    void init(std::string appName, GLFWwindow *window);
    void run();

private:
    void createCommandBuffers();
    void recordCommandBuffer();

    VulkanCore::VulkanCore mVulkanCore;
    VulkanCore::VulkanQueue* mGraphicsQueue;

    int32_t mNumImages;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkRenderPass mRenderPass;
    std::vector<VkFramebuffer> mFrameBuffers;
};

} // namespace VulkanApp
