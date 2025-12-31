#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{

struct PhysicalDeviceProperties
{
    VkPhysicalDevice mPhysicalDevice; // Physical device : external GPU ,
                                      // internal GPU , graphics accelerator,
    VkPhysicalDeviceProperties mDeviceProperties;
    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    std::vector<VkBool32> mQueueSupportPresent;
    std::vector<VkSurfaceFormatKHR> mSurfaceFormats;
    std::vector<VkPresentModeKHR> mPresentModes;
    VkSurfaceCapabilitiesKHR mSurfaceCaps;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    VkPhysicalDeviceFeatures mFeatures;
    VkFormat mDepthFormat;
    struct
    {
        uint32_t variant{0};
        uint32_t major{0};
        uint32_t minor{0};
        uint32_t patch{0};
    } mInstanceVersion;
    std::vector<VkExtensionProperties> mSupportedExtensions;

    bool isExtensionSupported(const char* extensionName) const
    {
        std::string requestedExt(extensionName);
        for (const auto& extension : mSupportedExtensions)
        {
            std::string extName(extension.extensionName);
            if (extName == requestedExt)
            {
                return true;
            }
        }
        return false;
    }
};

class PhysicalDevice
{
  public:
    PhysicalDevice();
    ~PhysicalDevice();

    void init(const VkInstance& instance, const VkSurfaceKHR& surface);
    uint32_t selectPhysicalDevice(VkQueueFlags requiredQueueFlags, bool requirePresentSupport);
    const PhysicalDeviceProperties& getSelectedPhysicalDeviceProperties() const;

  private:
    void printPhysicalDeviceInfo();
    void getExtension(uint32_t deviceIndex);
    void getDeviceAPIVersion(uint32_t deviceIndex);

    std::vector<PhysicalDeviceProperties> mDevices;
    int mSelectedPhysicalDeviceIndex;
};

} // namespace VulkanCore
