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
      mWindowHeight{height}, mCamera{nullptr}, mGraphicsPipelineV2{nullptr}, mModel{nullptr},
      mImGuiRenderer{nullptr}, mSkybox{nullptr}, mImGuiWidth{100}, mImGuiHeight{500}, mShowImGui{true},
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

    // 9. Destroy skybox
    if (mSkybox)
    {
        mSkybox->destroy();
        delete mSkybox;
        mSkybox = nullptr;
    }

    // 10. Cleanup Vulkan core resources in destructror of VulkanCore
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
    mSkybox = new VulkanCore::SkyBox(&mVulkanCore, "VulkanDemo/assets/skybox/piazza_bologni_1k.hdr");
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

    uint32_t frameCount = 0;
    float_t fpsTime{0.0F};
    while (!glfwWindowShouldClose(mWindow))
    {
        float_t newTime = glfwGetTime();
        float_t deltaTime = newTime - currentTime;

        mCamera->setTick(deltaTime);
        mCamera->process();

        renderScene();
        glfwPollEvents();

        currentTime = newTime;

        frameCount++;
        fpsTime += deltaTime;
        if (fpsTime >= 1.0F)
        {
            glfwSetWindowTitle(mWindow, const_cast<char*>(("Vulkan FPS: " + std::to_string(frameCount)).c_str()));
            frameCount = 0;
            fpsTime -= 1.0F; // Subtract 1 second instead of resetting to preserve accuracy
        }
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

    float_t _fTick = 1.0F; // mCamera->getTick();
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

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), mPosition);

    glm::mat4 modelMatrix = translation * rotation * scale;
    glm::mat4 vp = mCamera->getVPMatrix();
    mModel->update(currentImage, vp * modelMatrix);

    // For skybox: remove translation from view matrix (keep only rotation)
    glm::mat4 viewMatrix = mCamera->getCameraMatrix();
    // Extract rotation and construct 4x4 matrix manually
    // glm::mat4 viewNoTranslation = glm::mat4(glm::vec4(viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], 0.0f),
    //                                         glm::vec4(viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], 0.0f),
    //                                         glm::vec4(viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], 0.0f),
    //                                         glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(viewMatrix));

    glm::mat4 skyboxVP = mCamera->getProjectionMatrix() * viewNoTranslation;
    mSkybox->update(currentImage, skyboxVP);
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
                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

        mVulkanCore.beginDynamicRendering(commandBuffers[i], i, &clearColor, &clearDepth);
        mGraphicsPipelineV2->bind(commandBuffers[i]);
        mModel->recordCommandBuffer(commandBuffers[i], mGraphicsPipelineV2, i);
        mSkybox->recordCommandBuffer(commandBuffers[i], i);

        vkCmdEndRendering(commandBuffers[i]);

        if (!withSecondBarrier)
        {
            // For standalone rendering (no ImGui), do final transition to present
            VulkanCore::imageMemBarrier(commandBuffers[i], mVulkanCore.getSwapchainImage(i),
                                        mVulkanCore.getSwapchainSurfaceFormat(),
                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
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
    ImGui::SetNextWindowSize(ImVec2(mImGuiWidth, mImGuiHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    // Add window flags for better appearance
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
    // windowFlags |= ImGuiWindowFlags_MenuBar;  // Uncomment to add menu bar

    ImGui::Begin("🎮 Control Panel", NULL, windowFlags);

    // Add FPS display with color coding
    ImGuiIO& io = ImGui::GetIO();
    float fps = io.Framerate;
    ImVec4 fpsColor = fps > 60.0f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :     // Green
                          fps > 30.0f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : // Yellow
                              ImVec4(1.0f, 0.0f, 0.0f, 1.0f);            // Red
    ImGui::TextColored(fpsColor, "FPS: %.1f (%.2f ms)", fps, 1000.0f / fps);

    ImGui::Separator();
    ImGui::Spacing();

    // Transform Controls with icons and better layout
    if (ImGui::CollapsingHeader("📍 Position", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushItemWidth(-100); // Make sliders wider

        // Use DragFloat for finer control
        ImGui::Text("X:");
        ImGui::SameLine(50);
        ImGui::DragFloat("##PosX", &mPosition.x, 0.1f, -100.0f, 100.0f, "%.2f");

        ImGui::Text("Y:");
        ImGui::SameLine(50);
        ImGui::DragFloat("##PosY", &mPosition.y, 0.1f, -100.0f, 100.0f, "%.2f");

        ImGui::Text("Z:");
        ImGui::SameLine(50);
        ImGui::DragFloat("##PosZ", &mPosition.z, 0.1f, -100.0f, 100.0f, "%.2f");

        ImGui::PopItemWidth();

        // Color-coded reset button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 0.8f));
        if (ImGui::Button("🔄 Reset Position", ImVec2(-1, 0)))
        {
            mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        ImGui::PopStyleColor(2);
    }

    if (ImGui::CollapsingHeader("🔄 Rotation"))
    {
        ImGui::PushItemWidth(-100);

        ImGui::Text("X:");
        ImGui::SameLine(50);
        if (ImGui::SliderAngle("##RotX", &mRotation.x, 0.0f, 360.0f))
            mRotation.x = glm::radians(glm::mod(glm::degrees(mRotation.x), 360.0f));

        ImGui::Text("Y:");
        ImGui::SameLine(50);
        if (ImGui::SliderAngle("##RotY", &mRotation.y, 0.0f, 360.0f))
            mRotation.y = glm::radians(glm::mod(glm::degrees(mRotation.y), 360.0f));

        ImGui::Text("Z:");
        ImGui::SameLine(50);
        if (ImGui::SliderAngle("##RotZ", &mRotation.z, 0.0f, 360.0f))
            mRotation.z = glm::radians(glm::mod(glm::degrees(mRotation.z), 360.0f));

        ImGui::PopItemWidth();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.8f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 1.0f, 0.8f));
        if (ImGui::Button("🏠 Reset Rotation", ImVec2(-1, 0)))
        {
            mRotation = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        ImGui::PopStyleColor(2);
    }

    if (ImGui::CollapsingHeader("📏 Scale"))
    {
        ImGui::PushItemWidth(-1);

        // Single unified scale slider with visual feedback
        ImGui::SliderFloat("##Scale", &mScale, 0.1f, 10.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);

        // Visual scale indicator
        ImGui::ProgressBar((mScale - 0.1f) / 9.9f, ImVec2(-1, 0), "");

        ImGui::PopItemWidth();

        // Quick scale presets
        ImGui::Text("Presets:");
        if (ImGui::Button("0.5x", ImVec2(60, 0)))
            mScale = 0.5f;
        ImGui::SameLine();
        if (ImGui::Button("1.0x", ImVec2(60, 0)))
            mScale = 1.0f;
        ImGui::SameLine();
        if (ImGui::Button("2.0x", ImVec2(60, 0)))
            mScale = 2.0f;
        ImGui::SameLine();
        if (ImGui::Button("5.0x", ImVec2(60, 0)))
            mScale = 5.0f;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.8f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 1.0f, 0.4f, 0.8f));
        if (ImGui::Button("🔄 Reset Scale", ImVec2(-1, 0)))
        {
            mScale = 1.0f;
        }
        ImGui::PopStyleColor(2);
    }

    if (ImGui::CollapsingHeader("📷 Camera"))
    {
        float_t cameraSpeed = mCamera->getSpeed();
        ImGui::PushItemWidth(-1);

        ImGui::Text("Speed:");
        ImGui::SliderFloat("##CameraSpeed", &cameraSpeed, 1.0F, 100.0F, "%.1f");
        mCamera->setSpeed(cameraSpeed);

        ImGui::Spacing();

        // Camera info display
        glm::vec3 camPos = mCamera->getPosition();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y,
                           camPos.z);

        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.3f, 0.8f));
        if (ImGui::Button("🏠 Reset Camera", ImVec2(-1, 0)))
        {
            mCamera->setPosition(glm::vec3(0.0f, 0.0f, -50.0f));
            mCamera->setYaw(0.0f);
            mCamera->setPitch(0.0f);
            mCamera->setRoll(0.0f);
            mCamera->setSpeed(10.0f);
            mCamera->process();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Help section
    if (ImGui::CollapsingHeader("❓ Help"))
    {
        ImGui::BulletText("WASD/Arrows: Move camera");
        ImGui::BulletText("Mouse: Look around");
        ImGui::BulletText("Page Up/Down: Move up/down");
        ImGui::BulletText("Space: Toggle UI");
        ImGui::BulletText("ESC: Exit");
    }

    ImGui::End();

    ImGui::Render();
}

} // namespace VulkanApp
