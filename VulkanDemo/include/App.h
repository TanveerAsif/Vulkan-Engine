#include <GLFW/glfw3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Core.h"
#include "GraphicsPipeline.h"
#include "Queue.h"
#include "SimpleMesh.h"

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
    void createVertexBuffer();

    GLFWwindow* mWindow;
    VulkanCore::VulkanCore mVulkanCore;
    VulkanCore::VulkanQueue* mGraphicsQueue;
    VulkanCore::GraphicsPipeline* mGraphicsPipeline;
    VulkanCore::SimpleMesh mMesh;

    int32_t mNumImages;
    std::vector<VkCommandBuffer> mCommandBuffers;

    VkRenderPass mRenderPass;
    std::vector<VkFramebuffer> mFrameBuffers;

    VkShaderModule mVSShaderModule;
    VkShaderModule mFSShaderModule;
};

} // namespace VulkanApp
