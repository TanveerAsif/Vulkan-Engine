

#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "PhysicalDevice.h"

namespace {
VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
  std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT,
                                        VK_FORMAT_D32_SFLOAT_S8_UINT,
                                        VK_FORMAT_D24_UNORM_S8_UINT};

  // Check for each format if it is supported as a depth-stencil attachment
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features =
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  for (VkFormat format : depthFormats) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("Failed to find a supported depth format.");
}

} // namespace

namespace VulkanCore {
 

    PhysicalDevice::PhysicalDevice()
        : mSelectedPhysicalDeviceIndex(-1) // No physical device selected
    {
    }

    PhysicalDevice::~PhysicalDevice()
    {
        //ToDo : Cleanup if needed
    }

    void PhysicalDevice::init(const VkInstance& instance, const VkSurfaceKHR& surface)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support.");
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        mDevices.resize(deviceCount);

        for (uint32_t i = 0; i < deviceCount; ++i) {
            VkPhysicalDevice PhysDev = physicalDevices[i];
            mDevices[i].mPhysicalDevice = PhysDev;

            vkGetPhysicalDeviceProperties(PhysDev, &mDevices[i].mDeviceProperties);

            // Get queue family properties : like graphics, compute, transfer etc.,
            // each family can have multiple queues
            // we need to find at least one family that supports graphics and one that supports presentation
            // they can be the same family or different
            // so we query all families and check their capabilities
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &queueFamilyCount, nullptr);
            mDevices[i].mQueueFamilyProperties.resize(queueFamilyCount);
            mDevices[i].mQueueSupportPresent.resize(queueFamilyCount);

            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &queueFamilyCount, mDevices[i].mQueueFamilyProperties.data());

            // Check which queue families support presentation to the given surface
            // we need this information to create a swapchain later
            // for each queue family, check if it supports presentation
            for (uint32_t j = 0; j < queueFamilyCount; ++j) {
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(PhysDev, j, surface, &presentSupport);
                mDevices[i].mQueueSupportPresent[j] = presentSupport;
            }

            // Get supported surface formats : like color space, pixel format etc.,
            // we need this information to create a swapchain later
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, surface, &formatCount, nullptr);
            if (formatCount != 0) {
                mDevices[i].mSurfaceFormats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, surface, &formatCount, mDevices[i].mSurfaceFormats.data());
            }

            // Get supported present modes : like in opengl we have double buffering, vsync etc.,
            // here we get more supported present modes like FIFO, MAILBOX etc.,
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, surface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                mDevices[i].mPresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, surface, &presentModeCount, mDevices[i].mPresentModes.data());
            }

            // Get surface capabilities : 1 - max images in swapchain, 2 - image size, 3- transforms supported
            // 4 - usage flags : how the images are used (color attachment, texture, Depth  or stencil attachment etc., )
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDev, surface, &mDevices[i].mSurfaceCaps);

            // Get memory properties : 1- memory heaps, 2- memory types : memory that is local to the device, memory that is visible to the host CPU etc.,
            vkGetPhysicalDeviceMemoryProperties(PhysDev, &mDevices[i].mMemoryProperties);

            // Get supported features : like geometry shader, tessellation shader, wide lines etc.,
            vkGetPhysicalDeviceFeatures(PhysDev, &mDevices[i].mFeatures);

            // Find a suitable depth format
            mDevices[i].mDepthFormat = findDepthFormat(PhysDev);
        }

        printPhysicalDeviceInfo();
    }

    uint32_t PhysicalDevice::selectPhysicalDevice(VkQueueFlags requiredQueueFlags, bool requirePresentSupport)
    {
        // VkQueueFlags : bitmask specifying capabilities of queues
        // e.g., VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT etc.,
        // we need to find a physical device that has at least one queue family that supports the required flags
        for (uint32_t devIdx{0}; devIdx < mDevices.size(); ++devIdx) {

            for (uint32_t qFamilyProp = 0; qFamilyProp < mDevices[devIdx].mQueueFamilyProperties.size(); ++qFamilyProp) {
                const auto& queueFamily = mDevices[devIdx].mQueueFamilyProperties[qFamilyProp];
                if((queueFamily.queueFlags & requiredQueueFlags) && (mDevices[devIdx].mQueueSupportPresent[qFamilyProp] == requirePresentSupport))
                {
                    mSelectedPhysicalDeviceIndex = static_cast<int>(devIdx);
                    std::cout<< "Selected Physical Device: " <<    mDevices[devIdx].mDeviceProperties.deviceName << std::endl;
                    return devIdx;
                }
            }
        }

        throw std::runtime_error("Failed to find a suitable GPU.");
    }

    const PhysicalDeviceProperties& PhysicalDevice::getSelectedPhysicalDeviceProperties() const
    {
        if (mSelectedPhysicalDeviceIndex < 0 || mSelectedPhysicalDeviceIndex >= static_cast<int>(mDevices.size())) {
            throw std::runtime_error("No physical device selected.");
        }
        return mDevices[mSelectedPhysicalDeviceIndex];
    }


    void PhysicalDevice::printPhysicalDeviceInfo()
    {
        for (const auto& device : mDevices) {
            const auto& deviceProperties = device.mDeviceProperties;
            std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
            //std::cout << "Device Type: " << deviceProperties.deviceType << std::endl;
            // std::cout << "API Version: " 
            //           << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
            //           << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
            //           << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
        }
    }
}
