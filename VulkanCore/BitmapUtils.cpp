#include "include/BitmapUtils.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace VulkanCore
{

Bitmap::Bitmap(int32_t width, int32_t height, int32_t channels, eBitmapFormat format, void* data)
    : w_(width), h_(height), comp_(channels), fmt_(format)
{
    const size_t bytesPerPixel = (format == eBitmapFormat_UnsignedByte) ? channels : (channels * sizeof(float));
    const size_t totalBytes = width * height * bytesPerPixel;

    data_.resize(totalBytes);

    if (data != nullptr)
    {
        memcpy(data_.data(), data, totalBytes);
    }
}

int32_t getBytesPerTexFormat(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            return 1;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        default:
            throw std::runtime_error("Unsupported texture format");
    }
}

// Helper function to convert spherical coordinates to Cartesian
static void sphericalToCartesian(float theta, float phi, float& x, float& y, float& z)
{
    x = sin(phi) * cos(theta);
    y = sin(phi) * sin(theta);
    z = cos(phi);
}

// Helper function to sample equirectangular texture
static void sampleEquirectangular(const Bitmap& source, float x, float y, float z, uint8_t* outColor)
{
    // Convert Cartesian to spherical coordinates
    // For equirectangular: theta rotates around Y-axis, phi is angle from Y-axis
    float theta = atan2(z, x);
    float phi = acos(y);

    // Convert to texture coordinates
    float u = (theta + M_PI) / (2.0f * M_PI);
    float v = phi / M_PI;

    // Clamp to [0, 1]
    u = fmax(0.0f, fmin(1.0f, u));
    v = fmax(0.0f, fmin(1.0f, v));

    // Convert to pixel coordinates
    int32_t px = static_cast<int32_t>(u * (source.w_ - 1));
    int32_t py = static_cast<int32_t>(v * (source.h_ - 1));

    // Clamp to image bounds
    px = fmax(0, fmin(source.w_ - 1, px));
    py = fmax(0, fmin(source.h_ - 1, py));

    // Sample the pixel
    const int32_t index = (py * source.w_ + px) * source.comp_;
    for (int32_t c = 0; c < source.comp_; ++c)
    {
        outColor[c] = source.data_[index + c];
    }
}

int32_t convertEquirectangularToCubemap(const Bitmap& source, std::vector<Bitmap>& outCubemap)
{
    // Determine cube face size (typically use source height as face size)
    const int32_t faceSize = source.h_ / 2;

    // Create 6 cube faces
    outCubemap.clear();
    outCubemap.reserve(6);

    // Vulkan cubemap face order: +X, -X, +Y, -Y, +Z, -Z
    for (int32_t face = 0; face < 6; ++face)
    {
        // Create bitmap for this face
        Bitmap faceBitmap(faceSize, faceSize, source.comp_, source.fmt_);

        // Generate each pixel for this cube face
        for (int32_t j = 0; j < faceSize; ++j)
        {
            for (int32_t i = 0; i < faceSize; ++i)
            {
                // Convert pixel coordinate to [-1, 1] range
                float u = (2.0f * static_cast<float>(i) / static_cast<float>(faceSize - 1)) - 1.0f;
                float v = (2.0f * static_cast<float>(j) / static_cast<float>(faceSize - 1)) - 1.0f;

                // Calculate 3D direction vector based on cubemap face
                // Vulkan coordinate system: +X right, +Y down, +Z forward
                float x, y, z;

                switch (face)
                {
                    case 0: // +X (right)
                        x = 1.0f;
                        y = -v;
                        z = -u;
                        break;
                    case 1: // -X (left)
                        x = -1.0f;
                        y = -v;
                        z = u;
                        break;
                    case 2: // +Y (down/bottom in Vulkan)
                        x = u;
                        y = 1.0f;
                        z = -v;
                        break;
                    case 3: // -Y (up/top in Vulkan)
                        x = u;
                        y = -1.0f;
                        z = v;
                        break;
                    case 4: // +Z (forward)
                        x = u;
                        y = -v;
                        z = 1.0f;
                        break;
                    case 5: // -Z (backward)
                        x = -u;
                        y = -v;
                        z = -1.0f;
                        break;
                }

                // Normalize direction vector
                float len = sqrt(x * x + y * y + z * z);
                x /= len;
                y /= len;
                z /= len;

                // Sample from equirectangular map
                uint8_t color[4] = {0, 0, 0, 255};
                sampleEquirectangular(source, x, y, z, color);

                // Write to face bitmap
                const int32_t pixelIndex = (j * faceSize + i) * source.comp_;
                for (int32_t c = 0; c < source.comp_; ++c)
                {
                    faceBitmap.data_[pixelIndex + c] = color[c];
                }
            }
        }

        outCubemap.push_back(std::move(faceBitmap));
    }

    return faceSize;
}

} // namespace VulkanCore
