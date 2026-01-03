#ifndef VULKANCORE_BITMAP_UTILS_H
#define VULKANCORE_BITMAP_UTILS_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{

enum eBitmapFormat
{
    eBitmapFormat_UnsignedByte,
    eBitmapFormat_Float
};

struct Bitmap
{
    Bitmap(int32_t width, int32_t height, int32_t channels, eBitmapFormat format, void* data = nullptr);
    ~Bitmap() = default;

    int32_t w_{0};
    int32_t h_{0};
    int32_t comp_{0};
    eBitmapFormat fmt_{eBitmapFormat_UnsignedByte};
    std::vector<uint8_t> data_;
};

// Convert equirectangular HDR image to cubemap faces
// Returns the size of each cube face
int32_t convertEquirectangularToCubemap(const Bitmap& source, std::vector<Bitmap>& outCubemap);

// Get bytes per pixel for a Vulkan format
int32_t getBytesPerTexFormat(VkFormat format);

} // namespace VulkanCore

#endif // VULKANCORE_BITMAP_UTILS_H
