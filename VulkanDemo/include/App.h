#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Camera.h"
#include "Core.h"
#include "GLFW.h"
#include "GraphicsPipeline.h"
#include "GraphicsPipelineV2.h"
#include "ImGuiRenderer.h"
#include "Queue.h"
#include "SimpleMesh.h"
#include "VulkanModel.h"

namespace VulkanApp
{

class App : public VulkanCore::GLFWCallbacks
{
  public:
    App(int32_t width, int32_t height);
    ~App();

    void init(std::string appName);
    void run();

    // GLFWCallbacks interface
    void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
    void onMouseMove(GLFWwindow* window, double xoffset, double yoffset) override;
    void onMouseButtonEvent(GLFWwindow* window, int button, int action, int mods) override;

  private:
    void createCommandBuffers();
    void recordCommandBuffer();
    void createShaders();
    void createPipeline();
    void createVertexBuffer();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    void defaultCreateCameraPers();
    void renderScene();
    void createMesh();
    void loadTexture();
    void recordCommandBufferInteral(bool withSecondBarrier, std::vector<VkCommandBuffer>& commandBuffers);
    void updateGUI();

    GLFWwindow* mWindow;
    VulkanCore::VulkanCore mVulkanCore;
    VulkanCore::VulkanQueue* mGraphicsQueue;
    // VulkanCore::GraphicsPipeline* mGraphicsPipeline;
    VulkanCore::SimpleMesh mMesh;

    int32_t mNumImages;
    struct
    {
        std::vector<VkCommandBuffer> withGUI;
        std::vector<VkCommandBuffer> withoutGUI;
    } mCommandBuffers;

    VkShaderModule mVSShaderModule;
    VkShaderModule mFSShaderModule;

    std::vector<VulkanCore::BufferAndMemory> mUniformBuffers;
    int32_t mWindowWidth, mWindowHeight;
    VulkanCore::Camera* mCamera;

    VulkanCore::GraphicsPipelineV2* mGraphicsPipelineV2;
    VulkanCore::VulkanModel* mModel;
    VulkanCore::ImGuiRenderer* mImGuiRenderer;
    int32_t mImGuiWidth, mImGuiHeight;

    bool mShowImGui;
    glm::vec3 mClearColor;
    glm::vec3 mPosition;
    glm::vec3 mRotation;
    float_t mScale;
};

} // namespace VulkanApp
