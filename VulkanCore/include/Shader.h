#pragma once
#include <string>
#include <vulkan/vulkan.h>

namespace VulkanCore
{

VkShaderModule CreateShaderModuleFromText(const VkDevice& device, const std::string& shaderCode);

VkShaderModule CreateShaderModuleFromBinary(const VkDevice& device, const std::string& shaderCode);

}; // namespace VulkanCore
