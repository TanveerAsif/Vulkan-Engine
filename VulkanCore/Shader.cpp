#include "Shader.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan_core.h>

namespace VulkanCore
{
shaderc_shader_kind getShaderKindFromExtension(const std::string& filename)
{
    if (filename.ends_with(".vert"))
        return shaderc_vertex_shader;
    else if (filename.ends_with(".frag"))
        return shaderc_fragment_shader;
    else if (filename.ends_with(".comp"))
        return shaderc_compute_shader;
    else
        throw std::runtime_error("Unsupported shader file extension: " + filename);
}

VkShaderModule CreateShaderModuleFromText(const VkDevice& device, const std::string& shaderCode)
{
    // Read shader source code from file
    std::ifstream file(shaderCode);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + shaderCode);
    }
    std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Compile GLSL to SPIR-V using shaderc
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    shaderc_shader_kind shaderKind = getShaderKindFromExtension(shaderCode);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(sourceCode, shaderKind, shaderCode.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("Shader compilation failed: " + module.GetErrorMessage());
    }

    std::vector<uint32_t> spirvCode(module.cbegin(), module.cend());

    // Create Vulkan shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module.");
    }

    // Once spriv binary code is generated, we can save it to a file for future use
    // next time, we can load the SPIR-V binary directly from the file instead of recompiling
    std::string spirvFilename = shaderCode + ".spv";
    std::ofstream spirvFile(spirvFilename, std::ios::binary);
    if (spirvFile.is_open())
    {
        spirvFile.write(reinterpret_cast<const char*>(spirvCode.data()), spirvCode.size() * sizeof(uint32_t));
        spirvFile.close();
    }
    else
    {
        throw std::runtime_error("Failed to write SPIR-V binary to file: " + spirvFilename);
    }

    return shaderModule;
}

} // namespace VulkanCore
