#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanCore {

    struct PhysicalDeviceProperties
    {
        VkPhysicalDevice mPhysicalDevice; // Physical device : external GPU , internal GPU , graphics accelerator, 
        VkPhysicalDeviceProperties mDeviceProperties;
        std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
        std::vector<VkBool32> mQueueSupportPresent;
        std::vector<VkSurfaceFormatKHR> mSurfaceFormats;
        std::vector<VkPresentModeKHR> mPresentModes;
        VkSurfaceCapabilitiesKHR mSurfaceCaps;
        VkPhysicalDeviceMemoryProperties mMemoryProperties;
        VkPhysicalDeviceFeatures mFeatures;
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
            std::vector<PhysicalDeviceProperties> mDevices;
            int mSelectedPhysicalDeviceIndex;
    };

}
