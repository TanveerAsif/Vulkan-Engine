#ifndef TEXTURE_H
#define TEXTURE_H

#include "Core.h"
#include <cstdint>
#include <string>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{

class Texture
{
  public:
    Texture()
        : mImage(VK_NULL_HANDLE), mImageMemory(VK_NULL_HANDLE), mImageView(VK_NULL_HANDLE), mSampler(VK_NULL_HANDLE),
          mWidth(0), mHeight(0), m_pVulkanCore(nullptr)
    {
    }

    ~Texture() = default;

    Texture(VulkanCore* pVulkanCore)
        : mImage(VK_NULL_HANDLE), mImageMemory(VK_NULL_HANDLE), mImageView(VK_NULL_HANDLE), mSampler(VK_NULL_HANDLE),
          mWidth(0), mHeight(0), m_pVulkanCore{pVulkanCore}
    {
    }

    void destroy(VkDevice device);
    void LoadFromFile(const std::string& filePath);
    void Load(uint32_t bufferSize, void* pImageData);

    VkImage mImage{VK_NULL_HANDLE};
    VkDeviceMemory mImageMemory{VK_NULL_HANDLE};
    VkImageView mImageView{VK_NULL_HANDLE};
    VkSampler mSampler{VK_NULL_HANDLE};

    uint32_t mWidth;
    uint32_t mHeight;

  private:
    VulkanCore* m_pVulkanCore{nullptr};
};

} // namespace VulkanCore

#endif // VULKANCORE_TEXTURE_H
