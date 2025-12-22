

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include "Queue.h"
#include "GraphicsPipeline.h"

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
    void createShaders();
    void createPipeline();

    GLFWwindow* mWindow;
    VulkanCore::VulkanCore mVulkanCore;
    VulkanCore::VulkanQueue* mGraphicsQueue;
    VulkanCore::GraphicsPipeline* mGraphicsPipeline;

    int32_t mNumImages;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkRenderPass mRenderPass;
    std::vector<VkFramebuffer> mFrameBuffers;

    VkShaderModule mVSShaderModule;
    VkShaderModule mFSShaderModule;
};

} // namespace VulkanApp
