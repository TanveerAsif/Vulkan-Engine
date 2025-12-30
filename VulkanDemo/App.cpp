#include "App.h"

#include "Shader.h"
#include "Texture.h"
#include "Wrapper.h"

#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanApp
{

App::App(int32_t width, int32_t height)
    : mWindow{nullptr}, mVulkanCore{}, mGraphicsQueue{nullptr},
      /*mGraphicsPipeline{nullptr},*/ mNumImages{0}, mCommandBuffers{}, mRenderPass{VK_NULL_HANDLE}, mFrameBuffers{},
      mWindowWidth{width}, mWindowHeight{height}, mCamera{nullptr}, mGraphicsPipelineV2{nullptr}, mModel{nullptr}
{
}

App::~App()
{
    // 1. Command buffers will be freed when command pool is destroyed
    mVulkanCore.freeCommandBuffers(mCommandBuffers.data(), mNumImages);

    // 2. Destroy framebuffers
    mVulkanCore.destroyFramebuffers(mFrameBuffers);

    // 3. Destroy shader modules
    vkDestroyShaderModule(mVulkanCore.getDevice(), mVSShaderModule, nullptr);
    vkDestroyShaderModule(mVulkanCore.getDevice(), mFSShaderModule, nullptr);

    // 4. Destroy graphics pipeline
    // if (mGraphicsPipeline) {
    //   delete mGraphicsPipeline;
    //   mGraphicsPipeline = nullptr;
    // }
    if (mGraphicsPipelineV2)
    {
        delete mGraphicsPipelineV2;
        mGraphicsPipelineV2 = nullptr;
    }

    // 5. Destroy render pass
    vkDestroyRenderPass(mVulkanCore.getDevice(), mRenderPass, nullptr);

    // 6. Destroy vertex buffer
    mMesh.Destroyed(mVulkanCore.getDevice());

    // 7. Destroy uniform buffers
    for (auto& uniformBuffer : mUniformBuffers)
    {
        uniformBuffer.Destroy(mVulkanCore.getDevice());
    }

    // 8. Delete camera
    if (mCamera)
    {
        delete mCamera;
        mCamera = nullptr;
    }

    // 9. Destroy model
    if (mModel)
    {
        mModel->destroy();
        delete mModel;
        mModel = nullptr;
    }

    // 10. Cleanup Vulkan core resources in destructror of VulkanCore
}

void App::init(std::string appName)
{
    mWindow = VulkanCore::glfw_vulkan_init(mWindowWidth, mWindowHeight, appName.c_str());
    mVulkanCore.initialize(appName, mWindow, true /* enable depth buffer */);
    mNumImages = mVulkanCore.getSwapchainImageCount();
    mGraphicsQueue = mVulkanCore.getGraphicsQueue();
    mRenderPass = mVulkanCore.createSimpleRenderPass();
    mFrameBuffers = mVulkanCore.createFrameBuffer(mRenderPass);
    createShaders();
    createMesh();
    createUniformBuffers();
    createPipeline();
    createCommandBuffers();
    recordCommandBuffer();
    defaultCreateCameraPers();
    VulkanCore::glfw_vulkan_set_callbacks(mWindow, this);
}

void App::renderScene()
{
    // Main application loop here
    uint32_t imageIndex = mGraphicsQueue->acquireNextImage();
    updateUniformBuffer(imageIndex);
    mGraphicsQueue->submitAsync(mCommandBuffers[imageIndex]);
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
    // Handle mouse button events
}

void App::createCommandBuffers()
{
    mCommandBuffers.resize(mNumImages);
    mVulkanCore.createCommandBuffers(mCommandBuffers.data(), mNumImages);
}

void App::recordCommandBuffer()
{

    std::array<VkClearValue, 2> clearValue = {};
    clearValue[0].color = {{0.0f, 1.0f, 0.0f, 1.0f}}; // green clear color
    clearValue[1].depthStencil = {1.0f, 0};           // max depth

    VkRenderPassBeginInfo renderPassBeginInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                 .pNext = nullptr,
                                                 .renderPass = mRenderPass,
                                                 .framebuffer = VK_NULL_HANDLE, // will be set later
                                                 .renderArea = {.offset = {0, 0},
                                                                .extent =
                                                                    {
                                                                        .width = 800,
                                                                        .height = 600,
                                                                    }},
                                                 .clearValueCount = static_cast<uint32_t>(clearValue.size()),
                                                 .pClearValues = clearValue.data()};

    mModel->createDescriptorSets(mGraphicsPipelineV2);

    for (uint32_t i = 0; i < mCommandBuffers.size(); ++i)
    {
        // Begin command buffer recording
        VulkanCore::BeginCommandBuffer(mCommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        // frame buffer specific info per command buffer
        renderPassBeginInfo.framebuffer = mFrameBuffers[i];
        vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // mGraphicsPipeline->bind(mCommandBuffers[i], i);
        mGraphicsPipelineV2->bind(mCommandBuffers[i]);
        mModel->recordCommandBuffer(mCommandBuffers[i], mGraphicsPipelineV2, i);

        // uint32_t vertexCount = 9;
        // uint32_t instanceCount = 1;
        // uint32_t firstVertex = 0;
        // uint32_t firstInstance = 0;
        // vkCmdDraw(mCommandBuffers[i], vertexCount, instanceCount, firstVertex,
        //           firstInstance);
        vkCmdEndRenderPass(mCommandBuffers[i]);

        if (vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer " + std::to_string(i));
        }
    }

    std::cout << "Recorded " << mCommandBuffers.size() << " command buffers." << std::endl;
}

void App::createShaders()
{
    mVSShaderModule =
        VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.vert");
    mFSShaderModule =
        VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.frag");
    std::cout << "Shader modules created successfully." << std::endl;
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
    mGraphicsPipelineV2 =
        new VulkanCore::GraphicsPipelineV2(mVulkanCore.getDevice(), mWindow, mRenderPass, mVSShaderModule,
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

    // static float_t time = 0.0f;
    // glm::mat4 rotate = glm::mat4(1.0f);
    // rotate = glm::rotate(rotate, glm::radians(time),
    //                      glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
    // time += 0.1f; // assuming 60 FPS for simplicity
    //
    //   glm::mat4 wvp = mCamera->getVPMatrix() * rotate;
    //   mUniformBuffers[currentImage].update(mVulkanCore.getDevice(), &wvp,
    //                                        sizeof(wvp));

    glm::mat4 wvp = mCamera->getVPMatrix();
    mModel->update(currentImage, wvp);
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

} // namespace VulkanApp
