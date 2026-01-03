#include "Texture.h"
#include "stb_image.h"

namespace VulkanCore
{

void Texture::LoadFromFile(const std::string& filePath)
{
    m_pVulkanCore->createTexture(filePath.c_str(), *this);
}

void Texture::Load(uint32_t bufferSize, void* pImageData)
{
    int Width = 0;
    int Height = 0;
    int BPP = 0;

    void* pLoadedImageData = stbi_load_from_memory((const stbi_uc*)pImageData, bufferSize, &Width, &Height, &BPP, 0);

    m_pVulkanCore->createTextureImageFromData(*this, pLoadedImageData, Width, Height, VK_FORMAT_R8G8B8A8_SRGB, false);
    stbi_image_free(pLoadedImageData);
}

void Texture::destroy(VkDevice device)
{
    if (mSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, mSampler, nullptr);
        mSampler = VK_NULL_HANDLE;
    }

    if (mImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, mImageView, nullptr);
        mImageView = VK_NULL_HANDLE;
    }

    if (mImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, mImage, nullptr);
        mImage = VK_NULL_HANDLE;
    }

    if (mImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, mImageMemory, nullptr);
        mImageMemory = VK_NULL_HANDLE;
    }

    mWidth = 0;
    mHeight = 0;
}

void Texture::loadEctCubemap(const std::string& fileName)
{
    m_pVulkanCore->createCubemapTexture(fileName, *this);
}

} // namespace VulkanCore
