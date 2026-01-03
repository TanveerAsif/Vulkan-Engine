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
    float theta = atan2(y, x);
    float phi = acos(z);

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
    // This gives reasonable results for most equirectangular images
    const int32_t faceSize = source.h_ / 2;

    // Create 6 cube faces
    outCubemap.clear();
    outCubemap.reserve(6);

    // Cube face orientations (right, left, top, bottom, front, back)
    // Each face uses different coordinate transformations
    const int32_t directions[6][3] = {
        {1, 0, 2}, // +X (right)
        {1, 0, 2}, // -X (left)
        {0, 2, 1}, // +Y (top)
        {0, 2, 1}, // -Y (bottom)
        {0, 1, 2}, // +Z (front)
        {0, 1, 2}  // -Z (back)
    };

    const int32_t signs[6][3] = {
        {1, -1, -1}, // +X
        {-1, -1, 1}, // -X
        {1, 1, 1},   // +Y
        {1, -1, -1}, // -Y
        {1, -1, 1},  // +Z
        {-1, -1, -1} // -Z
    };

    for (int32_t face = 0; face < 6; ++face)
    {
        // Create bitmap for this face
        Bitmap faceBitmap(faceSize, faceSize, source.comp_, source.fmt_);

        // Generate each pixel for this cube face
        for (int32_t y = 0; y < faceSize; ++y)
        {
            for (int32_t x = 0; x < faceSize; ++x)
            {
                // Convert pixel coordinate to [-1, 1] range
                float u = (2.0f * static_cast<float>(x) / static_cast<float>(faceSize - 1)) - 1.0f;
                float v = (2.0f * static_cast<float>(y) / static_cast<float>(faceSize - 1)) - 1.0f;

                // Calculate 3D direction vector for this cube face pixel
                float dir[3] = {0.0f, 0.0f, 0.0f};
                dir[directions[face][0]] = u * signs[face][0];
                dir[directions[face][1]] = v * signs[face][1];
                dir[directions[face][2]] = 1.0f * signs[face][2];

                // Normalize direction vector
                float len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
                dir[0] /= len;
                dir[1] /= len;
                dir[2] /= len;

                // Sample from equirectangular map
                uint8_t color[4] = {0, 0, 0, 255};
                sampleEquirectangular(source, dir[0], dir[1], dir[2], color);

                // Write to face bitmap
                const int32_t pixelIndex = (y * faceSize + x) * source.comp_;
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
