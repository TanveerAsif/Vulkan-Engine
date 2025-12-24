#include "App.h"
#include "Shader.h"
#include "Wrapper.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanApp{


App::App()
    : mWindow{nullptr}
    , mVulkanCore{}
    , mGraphicsQueue{nullptr}
    , mGraphicsPipeline{nullptr}
    , mNumImages{0}
    , mCommandBuffers{}
    , mRenderPass{VK_NULL_HANDLE}
    , mFrameBuffers{}
{

}

App::~App()
{
    //1. Command buffers will be freed when command pool is destroyed
    mVulkanCore.freeCommandBuffers(mCommandBuffers.data(), mNumImages);

    //2. Destroy framebuffers
    mVulkanCore.destroyFramebuffers(mFrameBuffers);

    //3. Destroy shader modules
    vkDestroyShaderModule(mVulkanCore.getDevice(), mVSShaderModule, nullptr);
    vkDestroyShaderModule(mVulkanCore.getDevice(), mFSShaderModule, nullptr);

    //4. Destroy graphics pipeline
    if(mGraphicsPipeline)
    {
        delete mGraphicsPipeline;
        mGraphicsPipeline = nullptr;
    }

    //5. Destroy render pass
    vkDestroyRenderPass(mVulkanCore.getDevice(), mRenderPass, nullptr);

    // 6. Destroy vertex buffer
    mMesh.Destroyed(mVulkanCore.getDevice());

    // 7. Cleanup Vulkan core resources in destructror of VulkanCore
}


void App::init(std::string appName, GLFWwindow *window)
{
    mWindow = window;
    mVulkanCore.initialize(appName, window);
    mNumImages = mVulkanCore.getSwapchainImageCount();
    mGraphicsQueue = mVulkanCore.getGraphicsQueue();
    mRenderPass = mVulkanCore.createSimpleRenderPass();
    mFrameBuffers = mVulkanCore.createFrameBuffer(mRenderPass);
    createShaders();
    createVertexBuffer();
    createPipeline();
    createCommandBuffers();
    recordCommandBuffer();
}


void App::run()
{
    // Main application loop here
    uint32_t imageIndex = mGraphicsQueue->acquireNextImage();
    mGraphicsQueue->submitAsync(mCommandBuffers[imageIndex]);
    mGraphicsQueue->presentImage(imageIndex);
}


void App::createCommandBuffers()
{
   mCommandBuffers.resize(mNumImages);
   mVulkanCore.createCommandBuffers(mCommandBuffers.data(), mNumImages);
}

void App::recordCommandBuffer()
{
    VkClearColorValue clearColor = { {0.0F, 1.0F, 0.0F, 1.0F} };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = mRenderPass,
        .framebuffer = VK_NULL_HANDLE, // will be set later
        .renderArea = {
            .offset = {0, 0},
            .extent = {
                .width = 800,
                .height = 600,
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };



    for(uint32_t i = 0; i < mCommandBuffers.size(); ++i)
    {
        // Begin command buffer recording
        VulkanCore::BeginCommandBuffer(mCommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        // frame buffer specific info per command buffer
        renderPassBeginInfo.framebuffer = mFrameBuffers[i];
        vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        mGraphicsPipeline->bind(mCommandBuffers[i]);

        uint32_t vertexCount = 3;
        uint32_t instanceCount = 1;
        uint32_t firstVertex = 0;
        uint32_t firstInstance = 0;
        vkCmdDraw(mCommandBuffers[i], vertexCount, instanceCount, firstVertex, firstInstance);
        vkCmdEndRenderPass(mCommandBuffers[i]);

        if(vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer " + std::to_string(i));
        }
    }

    std::cout<< "Recorded " << mCommandBuffers.size() << " command buffers." << std::endl;
}

void App::createShaders()
{
    mVSShaderModule = VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.vert");
    mFSShaderModule = VulkanCore::CreateShaderModuleFromText(mVulkanCore.getDevice(), "VulkanDemo/shaders/triangle.frag");
    std::cout << "Shader modules created successfully." << std::endl;

}


void App::createPipeline()
{
  mGraphicsPipeline = new VulkanCore::GraphicsPipeline(
      mVulkanCore.getDevice(), mWindow, mRenderPass, mVSShaderModule,
      mFSShaderModule, &mMesh, mNumImages);
}

void App::createVertexBuffer() {
  struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
  };

  std::vector<Vertex> vertices = {
      // pos are in NDC
      {{-1.0F, -1.0F, 0.0F}, {0.0F, 0.0F}}, // top-left
      {{1.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},  // top-right
      {{0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}}    // bottom-center
  };

  mMesh.mVertexBufferSize = sizeof(Vertex) * vertices.size();
  mMesh.mVertexBuffer =
      mVulkanCore.createVertexBuffer(vertices.data(), mMesh.mVertexBufferSize);
}

}// namespace VulkanApp
