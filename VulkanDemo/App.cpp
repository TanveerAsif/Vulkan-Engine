#include "App.h"

#include "Shader.h"
#include "Texture.h"
#include "Wrapper.h"

#include <GLFW/glfw3.h>
#include <X11/extensions/randr.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <vulkan/vulkan_core.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace VulkanApp
{

App::App(int32_t width, int32_t height)
    : mWindow{nullptr}, mVulkanCore{}, mGraphicsQueue{nullptr}, mNumImages{0}, mCommandBuffers{},
      mVSShaderModule{VK_NULL_HANDLE}, mFSShaderModule{VK_NULL_HANDLE}, mWindowWidth{width},
      mWindowHeight{height}, mCamera{nullptr}, mGraphicsPipelineV2{nullptr}, mModel{nullptr}, mImGuiRenderer{nullptr},
      mImGuiWidth{100}, mImGuiHeight{500}, mShowImGui{true},
      mClearColor{0.0f, 1.0f, 0.0f}, mPosition{0.0f, 0.0f, 0.0f}, mRotation{0.0f, 0.0f, 0.0f}, mScale{1.0f}
{
}

App::~App()
{
    // 1. Command buffers will be freed when command pool is destroyed
    mVulkanCore.freeCommandBuffers(mCommandBuffers.withGUI.data(), mCommandBuffers.withGUI.size());
    mVulkanCore.freeCommandBuffers(mCommandBuffers.withoutGUI.data(), mCommandBuffers.withoutGUI.size());

    // 2. Destroy shader modules
    vkDestroyShaderModule(mVulkanCore.getDevice(), mVSShaderModule, nullptr);
    vkDestroyShaderModule(mVulkanCore.getDevice(), mFSShaderModule, nullptr);

    // 3. Destroy graphics pipeline
    // if (mGraphicsPipeline) {
    //   delete mGraphicsPipeline;
    //   mGraphicsPipeline = nullptr;
    // }
    if (mGraphicsPipelineV2)
    {
        delete mGraphicsPipelineV2;
        mGraphicsPipelineV2 = nullptr;
    }

    // 4. Destroy vertex buffer
    mMesh.Destroyed(mVulkanCore.getDevice());

    // 5. Destroy uniform buffers
    for (auto& uniformBuffer : mUniformBuffers)
    {
        uniformBuffer.Destroy(mVulkanCore.getDevice());
    }

    // 6. Delete camera
    if (mCamera)
    {
        delete mCamera;
        mCamera = nullptr;
    }

    // 7. Destroy model
    if (mModel)
    {
        mModel->destroy();
        delete mModel;
        mModel = nullptr;
    }

    // 8 . Destroy ImGui renderer
    if (mImGuiRenderer)
    {
        mImGuiRenderer->destroy();
        delete mImGuiRenderer;
        mImGuiRenderer = nullptr;
    }

    // 9. Cleanup Vulkan core resources in destructror of VulkanCore
}

void App::init(std::string appName)
{
    mWindow = VulkanCore::glfw_vulkan_init(mWindowWidth, mWindowHeight, appName.c_str());

    // Set window icon
    VulkanCore::glfw_set_window_icon(mWindow, "VulkanDemo/assets/appIcon.png");

    mVulkanCore.initialize(appName, mWindow, true /* enable depth buffer */);
    mNumImages = mVulkanCore.getSwapchainImageCount();
    mGraphicsQueue = mVulkanCore.getGraphicsQueue();
    createShaders();
    createMesh();
    createUniformBuffers();
    createPipeline();
    createCommandBuffers();
    recordCommandBuffer();
    defaultCreateCameraPers();
    VulkanCore::glfw_vulkan_set_callbacks(mWindow, this);
    mImGuiRenderer = new VulkanCore::ImGuiRenderer(&mVulkanCore, mImGuiWidth, mImGuiHeight);
}

void App::renderScene()
{
    // Main application loop here
    uint32_t imageIndex = mGraphicsQueue->acquireNextImage();
    updateUniformBuffer(imageIndex);
    if (mShowImGui)
    {
        updateGUI();

        VkCommandBuffer imguiCmdBuf = mImGuiRenderer->prepareCommandBuffer(imageIndex);
        VkCommandBuffer commandBuffers[] = {mCommandBuffers.withGUI[imageIndex], imguiCmdBuf};
        mGraphicsQueue->submitAsync(&commandBuffers[0], 2);
    }
    else
    {
        mGraphicsQueue->submitAsync(mCommandBuffers.withoutGUI[imageIndex]);
    }
    mGraphicsQueue->presentImage(imageIndex);
}

void App::run()
{
    float_t currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(mWindow))
    {
        float_t newTime = glfwGetTime();
        float_t deltaTime = newTime - currentTime;
        currentTime = newTime;
        mCamera->setTick(deltaTime);
        mCamera->process();

        renderScene();
        glfwPollEvents();
    }

    glfwTerminate();
}

void App::onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Handle keyboard input
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        mShowImGui = !mShowImGui;
    }

    if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS))
    {
        mCamera->setSpeed(100.0f);
    }

    float_t _fTick = mCamera->getTick();
    float_t _fSpeed = mCamera->getSpeed();
    float_t _fRotSpeed = mCamera->getRotSpeed();
    // Move Up
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
    {
        // m_vPos += m_vUp * (float)_fTick * m_fSpeed;
        mCamera->setPosition(mCamera->getPosition() + mCamera->getUp() * _fSpeed * _fTick);
    }

    // Move Down
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
    {
        mCamera->setPosition(mCamera->getPosition() - mCamera->getUp() * _fSpeed * _fTick);
    }

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        mCamera->setPosition(mCamera->getPosition() + mCamera->getDir() * _fSpeed * _fTick);
    }

    // Move backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        mCamera->setPosition(mCamera->getPosition() - mCamera->getDir() * _fSpeed * _fTick);
    }

    // Strafe right
    if ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) &&
        ((glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) != GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) != GLFW_PRESS)))
    {
        glm::vec3 right = mCamera->GetRight();
        mCamera->setPosition(mCamera->getPosition() - right * _fTick * _fSpeed);
    }

    // Strafe left
    if ((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) &&
        ((glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) != GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) != GLFW_PRESS)))
    {
        glm::vec3 right = mCamera->GetRight();
        mCamera->setPosition(mCamera->getPosition() + right * _fTick * _fSpeed);
    }

    // CTRL + LEFT : CLOCKWISE ROTATION
    if (((glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)) &&
        (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS))
    {
        mCamera->setYaw(mCamera->getYaw() + 1.0F * _fRotSpeed);
    }

    // CTRL + RIGHT : ANTI-CLOCKWISE ROTATION
    if (((glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)) &&
        (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS))
    {
        mCamera->setYaw(mCamera->getYaw() - 1.0F * _fRotSpeed);
    }

    // ALT + LEFT : CLOCKWISE ROTATION
    if (((glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) &&
        (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS))
    {
        mCamera->setPitch(mCamera->getPitch() + 1.0F * _fRotSpeed);
    }

    // ALT + RIGHT : ANTI-CLOCKWISE ROTATION
    if (((glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) ||
         (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)) &&
        (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS))
    {
        mCamera->setPitch(mCamera->getPitch() - 1.0F * _fRotSpeed);
    }
}

void App::onMouseMove(GLFWwindow* window, double xoffset, double yoffset)
{
    // Handle mouse movement
}

void App::onMouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
{
    if (!isMouseControlledByImGui())
    {
        mCamera->handleMouseButton(button, action, mods);
    }
}

void App::createCommandBuffers()
{
    mCommandBuffers.withGUI.resize(mNumImages);
    mVulkanCore.createCommandBuffers(mCommandBuffers.withGUI.data(), mNumImages);

    mCommandBuffers.withoutGUI.resize(mNumImages);
    mVulkanCore.createCommandBuffers(mCommandBuffers.withoutGUI.data(), mNumImages);
}

void App::recordCommandBuffer()
{
    mModel->createDescriptorSets(mGraphicsPipelineV2);
    recordCommandBufferInteral(true, mCommandBuffers.withGUI);
    recordCommandBufferInteral(false, mCommandBuffers.withoutGUI);
}

void App::createShaders()
{
    mVSShaderModule =
        VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.vert");
    mFSShaderModule =
        VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.frag");
    // std::cout << "Shader modules created successfully." << std::endl;
}

struct UniformData
{
    glm::mat4 wvp;
};
void App::createPipeline()
{
    // mGraphicsPipeline = new VulkanCore::GraphicsPipeline(
    //     mVulkanCore.getDevice(), mWindow, mRenderPass, mVSShaderModule,
    //     mFSShaderModule, &mMesh, mNumImages, mUniformBuffers, sizeof(UniformData),
    //     true /* enable depth buffer */);

    VkFormat depthFormat = mVulkanCore.getDepthFormat();
    VkFormat colorFormat = mVulkanCore.getSwapchainSurfaceFormat();
    mGraphicsPipelineV2 = new VulkanCore::GraphicsPipelineV2(mVulkanCore.getDevice(), mWindow, nullptr, mVSShaderModule,
                                                             mFSShaderModule, mNumImages, colorFormat, depthFormat);
}

void App::createVertexBuffer()
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    std::vector<Vertex> vertices = {
        // pos are in NDC
        {{-1.0F, -1.0F, 0.0F}, {0.0F, 0.0F}}, // bottom left
        {{-1.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},  // top left
        {{1.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},   // top right

        {{-1.0F, -1.0F, 0.0F}, {0.0F, 0.0F}}, // bottom left
        {{1.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},   // top right
        {{1.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},  // bottom right

        {{-1.0F, -1.0F, 5.0F}, {0.0F, 0.0F}}, // bottom left
        {{-1.0F, 1.0F, 5.0F}, {0.0F, 1.0F}},  // top left
        {{1.0F, 1.0F, 5.0F}, {1.0F, 1.0F}},   // top right
    };

    mMesh.mVertexBufferSize = sizeof(Vertex) * vertices.size();
    mMesh.mVertexBuffer = mVulkanCore.createVertexBuffer(vertices.data(), mMesh.mVertexBufferSize);
}

void App::createUniformBuffers()
{
    mUniformBuffers = mVulkanCore.createUniformBuffers(sizeof(UniformData));
}

void App::updateUniformBuffer(uint32_t currentImage)
{
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(mScale));

    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), mRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), mRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), mRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 rotation = rotationZ * rotationY * rotationX;
    glm::mat4 rotation2 =
        glm::rotate(rotation, glm::radians(180.0F), glm::normalize(glm::vec3(1.0F, 0.0F, 0.0F))); // Hack

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), mPosition);

    glm::mat4 modelMatrix = translation * rotation2 * scale; // No additional transformations
    glm::mat4 vp = mCamera->getVPMatrix();
    mModel->update(currentImage, vp * modelMatrix);
}

void App::defaultCreateCameraPers()
{
    mCamera =
        new VulkanCore::Camera(glm::vec3(0.0f, 0.0f, -50.0f), // position - moved further back to see larger models
                               glm::vec3(0.0f, 0.0f, 1.0f),   // lookAt
                               glm::vec3(0.0f, 1.0f, 0.0f),   // up
                               45.0f,                         // fov
                               static_cast<float>(mWindowWidth) / static_cast<float>(mWindowHeight), // aspect ratio
                               0.1f,                                                                 // near plane
                               1000.0f                                                               // far plane
        );
}

void App::createMesh()
{
    // createVertexBuffer();
    // loadTexture();

    mModel = new VulkanCore::VulkanModel("VulkanDemo/assets/Spider/spider.obj", &mVulkanCore);
}

void App::loadTexture()
{
    mMesh.mTexture = new VulkanCore::Texture();
    mVulkanCore.createTexture("VulkanDemo/assets/wall.jpg", *(mMesh.mTexture));
}

void App::recordCommandBufferInteral(bool withSecondBarrier, std::vector<VkCommandBuffer>& commandBuffers)
{
    VkClearValue clearColor = {.color = {{mClearColor.r, mClearColor.g, mClearColor.b, 1.0F}}};
    VkClearValue clearDepth = {.depthStencil = {1.0F, 0}};
    for (uint32_t i = 0; i < commandBuffers.size(); ++i)
    {
        // Begin command buffer recording
        VulkanCore::BeginCommandBuffer(commandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        // Transition from UNDEFINED (works for both first frame and subsequent frames)
        // On first frame: actually UNDEFINED
        // On subsequent frames: coming from PRESENT_SRC_KHR, but UNDEFINED transition is safe
        VulkanCore::imageMemBarrier(commandBuffers[i], mVulkanCore.getSwapchainImage(i),
                                    mVulkanCore.getSwapchainSurfaceFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        mVulkanCore.beginDynamicRendering(commandBuffers[i], i, &clearColor, &clearDepth);
        mGraphicsPipelineV2->bind(commandBuffers[i]);
        mModel->recordCommandBuffer(commandBuffers[i], mGraphicsPipelineV2, i);

        vkCmdEndRendering(commandBuffers[i]);

        if (!withSecondBarrier)
        {
            // For standalone rendering (no ImGui), do final transition to present
            VulkanCore::imageMemBarrier(commandBuffers[i], mVulkanCore.getSwapchainImage(i),
                                        mVulkanCore.getSwapchainSurfaceFormat(),
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer " + std::to_string(i));
        }
    }

    std::cout << "Recorded " << commandBuffers.size() << " command buffers." << std::endl;
}

void App::updateGUI()
{
    // ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplVulkan_NewFrame(); // Setup ImGui frame for Vulkan
    ImGui_ImplGlfw_NewFrame();   // Setup ImGui frame for GLFW
    ImGui::NewFrame();           // Start new ImGui frame

    // Settings Window - Set size and position
    ImGui::SetNextWindowSize(ImVec2(mImGuiWidth, mImGuiHeight), ImGuiCond_FirstUseEver); // Set window size on first use
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver); // Set window position on first use
    ImGui::Begin("Settings", NULL);                                  // Create a window called "Settings"
    ImGui::Text("This is some useful text.");

    ImGui::Separator();

    ImGui::BeginGroup();
    {
        // ImGui::Begin("Transform");
        if (ImGui::CollapsingHeader("Position"))
        {
            ImGui::SliderFloat("PosX", &mPosition.x, 0.0F, 100.0F);
            ImGui::SliderFloat("PosY", &mPosition.y, 0.0F, 100.0F);
            ImGui::SliderFloat("PosZ", &mPosition.z, 0.0F, 100.0F);
            if (ImGui::Button("Reset Position"))
            {
                mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
            }
        }

        if (ImGui::CollapsingHeader("Rotation"))
        {
            ImGui::SliderFloat("RotX", &mRotation.x, 0.0F, 3.14F);
            ImGui::SliderFloat("RotY", &mRotation.y, 0.0F, 3.14F);
            ImGui::SliderFloat("RotZ", &mRotation.z, 0.0F, 3.14F);
            if (ImGui::Button("Reset Rotation"))
            {
                mRotation = glm::vec3(0.0f, 0.0f, 0.0f);
            }
        }
        if (ImGui::CollapsingHeader("Scaling"))
        {
            ImGui::SliderFloat("Down", &mScale, 0.1f, 1.0f);
            ImGui::SliderFloat("Up", &mScale, 1.0f, 10.0f);
            if (ImGui::Button("Reset Scale"))
            {
                mScale = 1.0f;
            }
        }
    }
    ImGui::EndGroup();

    // imGuiIZMO : 3D axis and gizmo manipulation can be added here if needed

    ImGui::End(); // End Transform window

    ImGui::Render(); // Finalize the ImGui frame
}

} // namespace VulkanApp
