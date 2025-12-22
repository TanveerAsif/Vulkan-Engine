#pragma once
#include <vulkan/vulkan.h>
#include <string>

namespace VulkanCore
{

    VkShaderModule CreateShaderModuleFromText(const VkDevice& device, const std::string& shaderCode);

    VkShaderModule CreateShaderModuleFromBinary(const VkDevice& device, const std::string& shaderCode);

};
