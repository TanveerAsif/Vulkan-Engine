#ifndef SKYBOX_H
#define SKYBOX_H

#include "Core.h"
#include "GraphicsPipelineV2.h"
#include "Texture.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{

class SkyBox
{
  public:
    SkyBox(VulkanCore* vulkanCore, std::string fileName);
    ~SkyBox();

    void destroy();

    // record the skybox rendering into the command buffer
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void update(int32_t imageIndex, const glm::mat4& transformation);

  private:
    void createDescriptorSets();

    VulkanCore* mVulkanCore;

    int32_t mNumImages;
    Texture* mCubemapTexture;

    std::vector<BufferAndMemory> mUniformBuffers;
    std::vector<std::vector<VkDescriptorSet>> mDescriptorSets; // vp matrix for skybox
    VkShaderModule mVertexShaderModule;
    VkShaderModule mFragmentShaderModule;

    GraphicsPipelineV2* mGraphicsPipeline;
};

} // namespace VulkanCore

#endif // SKYBOX_H
