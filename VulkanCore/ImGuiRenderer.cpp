#include "ImGuiRenderer.h"
#include "Wrapper.h"

#include "backend/imgui_impl_glfw.h"
#include "backend/imgui_impl_vulkan.h"
#include "imgui.h"
#include "include/ImGuiRenderer.h"
#include <X11/Xlib.h>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace
{
static void CheckVKResult(VkResult err)
{
    if (err == 0)
        return;
    std::cout << "VkResult: " << err << std::endl;
    if (err < 0)
        abort();
}
} // namespace

bool isMouseControlledByImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

namespace VulkanCore
{

ImGuiRenderer::ImGuiRenderer(VulkanCore* vulkanCore)
    : mVulkanCore{vulkanCore}, mFramebufferWidth{0}, mFramebufferHeight{0}, mCommandBuffers{}, mDescriptorPool{
                                                                                                   VK_NULL_HANDLE}
{
    mVulkanCore->getFramebufferSize(mFramebufferWidth, mFramebufferHeight);
    createDescriptorPool();
    initImGui();
}

ImGuiRenderer::~ImGuiRenderer()
{
    // destroy();
}

void ImGuiRenderer::destroy()
{
    // Shutdown ImGui backends first
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Now free our manually created resources
    mVulkanCore->freeCommandBuffers(mCommandBuffers.data(), static_cast<int32_t>(mCommandBuffers.size()));
    vkDestroyDescriptorPool(mVulkanCore->getDevice(), mDescriptorPool, nullptr);
} // copy from ImGui_ImplVulkan_example
void ImGuiRenderer::createDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * std::size(poolSizes);
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(mVulkanCore->getDevice(), &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void ImGuiRenderer::initImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos; // Enable Mouse Controls
    io.DisplaySize = ImVec2(static_cast<float>(mFramebufferWidth), static_cast<float>(mFramebufferHeight));

    // Set global font scale instead of window font scale
    io.FontGlobalScale = 1.5f;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    bool installGLFWCallbacks = true;
    ImGui_ImplGlfw_InitForVulkan(mVulkanCore->getWindow(), installGLFWCallbacks);

    VkFormat colorFormat = mVulkanCore->getSwapchainSurfaceFormat();

    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion = VK_API_VERSION_1_2,
        .Instance = mVulkanCore->getVulkanInstance(),
        .PhysicalDevice = mVulkanCore->getPhysicalDevice().getSelectedPhysicalDeviceProperties().mPhysicalDevice,
        .Device = mVulkanCore->getDevice(),
        .QueueFamily = mVulkanCore->getQueueFamilyIndex(),
        .Queue = mVulkanCore->getGraphicsQueue()->getVkQueue(),
        .DescriptorPool = mDescriptorPool,
        .MinImageCount =
            mVulkanCore->getPhysicalDevice().getSelectedPhysicalDeviceProperties().mSurfaceCaps.minImageCount,
        .ImageCount = static_cast<uint32_t>(mVulkanCore->getSwapchainImageCount()),
        .PipelineCache = VK_NULL_HANDLE,
        .PipelineInfoMain =
            {
                .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
                .PipelineRenderingCreateInfo =
                    {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                        .pNext = nullptr,
                        .colorAttachmentCount = 1,
                        .pColorAttachmentFormats = &colorFormat,
                        .depthAttachmentFormat = mVulkanCore->getDepthFormat(),
                        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
                    },
            },
        .UseDynamicRendering = true,
        .Allocator = nullptr,
        .CheckVkResultFn = CheckVKResult,
    };

    ImGui_ImplVulkan_Init(&init_info);

    mCommandBuffers.resize(mVulkanCore->getSwapchainImageCount());
    mVulkanCore->createCommandBuffers(mCommandBuffers.data(), static_cast<int32_t>(mCommandBuffers.size()));
}

// Must be called after ImGUI frame was prepared on the application side
VkCommandBuffer ImGuiRenderer::prepareCommandBuffer(uint32_t imageIndex)
{
    // Record command buffer
    BeginCommandBuffer(mCommandBuffers[imageIndex], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Image should already be in COLOR_ATTACHMENT_OPTIMAL from the main scene rendering
    // Just continue rendering ImGui on top of it

    mVulkanCore->beginDynamicRendering(mCommandBuffers[imageIndex], imageIndex, NULL, NULL);

    ImDrawData* pDrawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(pDrawData, mCommandBuffers[imageIndex]);
    vkCmdEndRendering(mCommandBuffers[imageIndex]);

    // Transition image to PRESENT_SRC after all rendering is complete
    imageMemBarrier(mCommandBuffers[imageIndex], mVulkanCore->getSwapchainImage(imageIndex),
                    mVulkanCore->getSwapchainSurfaceFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    if (vkEndCommandBuffer(mCommandBuffers[imageIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record ImGui command buffer " + std::to_string(imageIndex));
    }
    return mCommandBuffers[imageIndex];
}

} // namespace VulkanCore
