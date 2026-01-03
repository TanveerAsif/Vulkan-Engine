#ifndef GRAPHICS_PIPELINE_V2_H
#define GRAPHICS_PIPELINE_V2_H

#include <GLFW/glfw3.h>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "ModelDesc.h"

namespace VulkanCore
{

enum V2_Binding
{
    V2_BindingVB = 0,
    V2_BindingIB = 1,
    V2_BindingUniform = 2,
    V2_BindingTexture2D = 3,
    V2_BindingTextureCube = 4,
    V2_Binding_Count = 5
};

struct PipelineDesc
{
    VkDevice mDevice = VK_NULL_HANDLE;
    GLFWwindow* mWindow = nullptr;
    VkShaderModule mVertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule mFragmentShaderModule = VK_NULL_HANDLE;
    int32_t mNumSwapchainImages = 0;
    VkFormat mColorFormat = VK_FORMAT_UNDEFINED;
    VkFormat mDepthFormat = VK_FORMAT_UNDEFINED;
    VkCompareOp mDepthCompareOp = VK_COMPARE_OP_LESS;
    bool mIsVB = false;
    bool mIsIB = false;
    bool mIsUniform = false;
    bool mIsTex2D = false;
    bool mIsCubemap = false;
};

class GraphicsPipelineV2
{
  public:
    GraphicsPipelineV2(VkDevice device, GLFWwindow* window, VkRenderPass renderPass, VkShaderModule vsModule,
                       VkShaderModule fsModule, int32_t numImages, VkFormat colorFormat, VkFormat depthFormat);

    GraphicsPipelineV2(const PipelineDesc& pd);
    ~GraphicsPipelineV2();

    void bind(VkCommandBuffer commandBuffer);
    void allocateDescriptorSets(int32_t numSubmeshes, std::vector<std::vector<VkDescriptorSet>>& descriptorSets);
    void updateDescriptorSets(const ModelDesc& modelDesc, std::vector<std::vector<VkDescriptorSet>>& descriptorSets);

    VkPipelineLayout getPipelineLayout() const
    {
        return mPipelineLayout;
    }

  private:
    void initCommon(GLFWwindow* window, VkRenderPass renderPass, VkShaderModule vsModule, VkShaderModule fsModule,
                    int32_t numImages, VkFormat colorFormat, VkFormat depthFormat, VkCompareOp depthCompareOp);

    void allocateDescriptorSetsInternal(int32_t numSubmeshes,
                                        std::vector<std::vector<VkDescriptorSet>>& descriptorSets);
    void createDescriptorSetLayout(bool isVB, bool isIB, bool isUniform, bool isTex2D, bool isCubemap);
    void createDescriptorPool(int32_t maxSets);

    VkDevice mDevice;
    VkPipeline mGraphicsPipeline;
    VkPipelineLayout mPipelineLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;

    int32_t mNumImages;
};

} // namespace VulkanCore

#endif // GRAPHICS_PIPELINE_V2_H
