#include "Core.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb.h>

#include "Wrapper.h"
#include "include/Core.h"
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace VulkanCore {

VulkanCore::VulkanCore()
    : mVulkanInstance(VK_NULL_HANDLE), mDebugMessenger(VK_NULL_HANDLE),
      mWindow(nullptr),
      mSurface(VK_NULL_HANDLE), mPhysicalDevice{}, mQueueFamilyIndex{0},
      mLogicalDevice(VK_NULL_HANDLE), mSwapchainSurfaceFormat{},
      mSwapchain(VK_NULL_HANDLE), mSwapchainImages{}, mSwapchainImageViews{},
      mCommandPool(VK_NULL_HANDLE), mGraphicsQueue{}, mFrameBuffers{},
      mCopyCmdBuffer(VK_NULL_HANDLE) {}

VulkanCore::~VulkanCore() {
  std::cout << "........................................." << std::endl;

  vkFreeCommandBuffers(mLogicalDevice, mCommandPool, 1, &mCopyCmdBuffer);
  std::cout << "Copy command buffer freed." << std::endl;

  mGraphicsQueue.destroySemaphores();
  std::cout << "Graphics queue semaphores destroyed." << std::endl;

  if (mCommandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);
    std::cout << "Command pool destroyed." << std::endl;
    mCommandPool = VK_NULL_HANDLE;
  }

  // Destroy depth resources
  for (auto &depthImage : mDepthImages) {
    depthImage.Destroy(mLogicalDevice);
  }
  mDepthImages.clear();
  std::cout << "Depth resources destroyed." << std::endl;

  // Destroy all image views associated with the swapchain and swapchain itself
  for (int32_t i = 0; i < static_cast<int32_t>(mSwapchainImageViews.size());
       ++i) {
    vkDestroyImageView(mLogicalDevice, mSwapchainImageViews[i], nullptr);
  }
  mSwapchainImageViews.clear();
  mSwapchainImages.clear();
  if (mSwapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(mLogicalDevice, mSwapchain, nullptr);
    std::cout << "Swapchain destroyed." << std::endl;
  }
  mSwapchain = VK_NULL_HANDLE;

  // Destroy logical device
  if (mLogicalDevice != VK_NULL_HANDLE) {
    vkDestroyDevice(mLogicalDevice, nullptr);
    std::cout << "Logical device destroyed." << std::endl;
  }

  // Destroy surface
  if (mSurface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(mVulkanInstance, mSurface, nullptr);
    std::cout << "Surface destroyed." << std::endl;
  }

  if (mDebugMessenger != VK_NULL_HANDLE) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        mVulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(mVulkanInstance, mDebugMessenger, nullptr);
      std::cout << "Debug messenger destroyed." << std::endl;
    }
  }

  if (mVulkanInstance != VK_NULL_HANDLE) {
    vkDestroyInstance(mVulkanInstance, nullptr);
    std::cout << "Vulkan instance destroyed." << std::endl;
  }
}

void VulkanCore::initialize(std::string appName, GLFWwindow *window,
                            bool depthEnabled) {

  mWindow = window;
  mDepthEnabled = depthEnabled;
  createInstance(appName);
  createDebugCallback();
  createSurface(mWindow);
  mPhysicalDevice.init(mVulkanInstance, mSurface);
  mQueueFamilyIndex =
      mPhysicalDevice.selectPhysicalDevice(VK_QUEUE_GRAPHICS_BIT, true);
  createLogicalDevice();
  createSwapChain();
  createCommandBufferPool();

  // Initialize graphics queue
  mGraphicsQueue.init(mLogicalDevice, mSwapchain, mQueueFamilyIndex, 0);

  createCommandBuffers(&mCopyCmdBuffer, 1);

  if (mDepthEnabled) {
    createDepthResources();
  }
}

void VulkanCore::createInstance(std::string appName) {
  std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

  std::vector<const char *> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(__APPLE__)
      VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
  };

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName.c_str();
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkResult result = vkCreateInstance(&createInfo, nullptr, &mVulkanInstance);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance");
  }
  std::cout << "Vulkan instance created successfully." << std::endl;
}

void VulkanCore::createDebugCallback() {

  VkDebugUtilsMessengerCreateInfoEXT messangerCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback =
          [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
             VkDebugUtilsMessageTypeFlagsEXT messageType,
             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
             void *pUserData) -> VkBool32 {
        // Enable the following line to print all validation layer messages
        std::cout << "Validation Layer: " << pCallbackData->pMessage
                  << std::endl;

        std::cout << "Debug Callback executed." << pCallbackData->pMessage
                  << std::endl;
        std::cout << "Message Severity: " << messageSeverity
                  << ", MessageType: " << messageType << std::endl;
        std::cout << "objectType: ";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
          std::cout << "%llx " << pCallbackData->pObjects[i].objectType
                    << ", handle : " << pCallbackData->pObjects[i].objectHandle
                    << std::endl;
        }

        return VK_FALSE;
      },
      .pUserData = nullptr};

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      mVulkanInstance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    if (func(mVulkanInstance, &messangerCreateInfo, nullptr,
             &mDebugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("Failed to set up debug messenger!");
    }
  } else {
    throw std::runtime_error("Could not load vkCreateDebugUtilsMessengerEXT");
  }

  std::cout << "Debug messenger created successfully." << std::endl;
}

void VulkanCore::createSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(mVulkanInstance, window, nullptr, &mSurface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface!");
  }
  std::cout << "Window surface created successfully." << std::endl;
}

void VulkanCore::createLogicalDevice() {
  const auto &physicalDeviceProps =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties();

  // set up queue properties whom logical device will manage
  float queuePriority = 1.0F;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueFamilyIndex = mQueueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};

  // Contains all capabilties of the physical device
  // enable the one which are required for our application
  VkPhysicalDeviceFeatures deviceFeatures = {.geometryShader = VK_TRUE,
                                             .tessellationShader = VK_TRUE,
                                             .samplerAnisotropy = VK_TRUE};

  // also, check choosen physical device support geometry and tessellation
  // shaders
  if (physicalDeviceProps.mFeatures.geometryShader == VK_FALSE) {
    throw std::runtime_error(
        "Selected physical device does not support geometry shaders.");
  }
  if (physicalDeviceProps.mFeatures.tessellationShader == VK_FALSE) {
    throw std::runtime_error(
        "Selected physical device does not support tessellation shaders.");
  }
  if (physicalDeviceProps.mFeatures.samplerAnisotropy == VK_FALSE) {
    throw std::runtime_error(
        "Selected physical device does not support sampler anisotropy.");
  }

  std::vector<const char *> deviceExtensions = {
      // swapchain extension is required for any windowed application
      // like presenting rendered images to the screen
      // or creating a series of images that are presented to the screen in a
      // cycle
      // or double buffering, triple buffering etc.,
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,

      // provide access to built-in glsl variables like gl_DrawID,
      // gl_BaseInstance etc.,
      // BaseVertex, BaseInstance are useful when doing instanced rendering
      // Indexed drawing with base vertex offsets
      VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCreateInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &deviceFeatures};

  if (vkCreateDevice(physicalDeviceProps.mPhysicalDevice, &deviceCreateInfo,
                     nullptr, &mLogicalDevice) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device!");
  }

  std::cout << "Logical device created successfully." << std::endl;
}

void VulkanCore::createSwapChain() {

  // Get surface capabilities from the selected physical device
  const auto &surfaceCaps =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mSurfaceCaps;

  // Determine number of images in the swapchain
  const uint32_t numImages = [&surfaceCaps]() -> uint32_t {
    uint32_t requiredImageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 &&
        requiredImageCount > surfaceCaps.maxImageCount)
      requiredImageCount = surfaceCaps.maxImageCount;
    else
      return surfaceCaps.minImageCount;

    return requiredImageCount;
  }();

  const auto &presentModes =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mPresentModes;
  const auto selectedPresentMode = [&presentModes]() -> VkPresentModeKHR {
    // Select present mode for the swapchain
    // VK_PRESENT_MODE_IMMEDIATE_KHR : images submitted by the application are
    // transferred to the screen right away, which may result in tearing
    // VK_PRESENT_MODE_FIFO_KHR : the presentation engine waits for the vertical
    // blanking period to update the current image, this is similar to vertical
    // sync (vsync) in modern games and applications. This mode is guaranteed to
    // be available. VK_PRESENT_MODE_FIFO_RELAXED_KHR : if the application is
    // late and misses the vertical blanking period, the image is transferred
    // right away, which may result in tearing VK_PRESENT_MODE_MAILBOX_KHR : the
    // presentation engine waits for the vertical blanking period to update the
    // current image. If there is already an image queued for presentation when
    // a new image is submitted, the new image replaces the existing one. This
    // mode is useful for avoiding tearing while maintaining low latency.

    for (const auto &mode : presentModes) {
      if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return mode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }();

  // Select surface format for the swapchain
  const auto &surfaceFormats =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mSurfaceFormats;
  mSwapchainSurfaceFormat = [&surfaceFormats]() -> VkSurfaceFormatKHR {
    for (const auto &surfFormat : surfaceFormats) {
      // std::cout << "Supported Surface Format: " << surfFormat.format << ",
      // Color Space: " << surfFormat.colorSpace << std::endl;
      if (surfFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          surfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return surfFormat;
      }
    }
    return surfaceFormats[0]; // if preferred format not found, return the
                              // firstone
  }();

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = mSurface,
      .minImageCount = numImages, // 2 : double buffering
      .imageFormat = mSwapchainSurfaceFormat.format,
      .imageColorSpace = mSwapchainSurfaceFormat.colorSpace,
      .imageExtent =
          surfaceCaps.currentExtent, // width and height of the swapchain images
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &mQueueFamilyIndex,
      .preTransform =
          surfaceCaps.currentTransform, // if need to be apply any transform
                                        // like 90 degree rotation, flip etc.,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = selectedPresentMode, // vsync
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE};

  if (vkCreateSwapchainKHR(mLogicalDevice, &swapchainCreateInfo, nullptr,
                           &mSwapchain) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain!");
  };
  std::cout << "Swapchain created successfully." << std::endl;

  uint32_t actualImageCount = 0;
  vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &actualImageCount,
                          nullptr);
  if (actualImageCount != numImages) {
    std::cerr
        << "Mismatch in requested and actual number of images in the swapchain."
        << std::endl;
  }

  mSwapchainImages.resize(actualImageCount);
  mSwapchainImageViews.resize(actualImageCount);
  vkGetSwapchainImagesKHR(
      mLogicalDevice, mSwapchain, &actualImageCount,
      mSwapchainImages.data()); // Images updated in swapchainImages vector

  for (uint32_t i{0}; i < actualImageCount; ++i) {
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = mSwapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = mSwapchainSurfaceFormat.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    VkResult result = vkCreateImageView(mLogicalDevice, &viewInfo, nullptr,
                                        &mSwapchainImageViews[i]);
    if (result != VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create image view for swapchain image " +
          std::to_string(i));
    }
  }
}

int32_t VulkanCore::getSwapchainImageCount() const {
  return static_cast<int32_t>(mSwapchainImages.size());
}

void VulkanCore::createCommandBufferPool() {
  VkCommandPoolCreateInfo poolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = mQueueFamilyIndex};

  if (vkCreateCommandPool(mLogicalDevice, &poolCreateInfo, nullptr,
                          &mCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }

  std::cout << "Command pool created successfully." << std::endl;
}

void VulkanCore::createCommandBuffers(VkCommandBuffer *commandBuffers,
                                      int32_t count) {
  VkCommandBufferAllocateInfo cmdBufAllocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = mCommandPool,
      .level =
          VK_COMMAND_BUFFER_LEVEL_PRIMARY, // primary command buffer can be
                                           // submitted to a queue for
                                           // execution, but cannot be called
                                           // from other command buffers
                                           // secondary command buffer can be
                                           // called from primary command
                                           // buffers, cannot be submitted
                                           // directly to a queue.
                                           // As of now, we are only using
                                           // primary command buffers
      .commandBufferCount = static_cast<uint32_t>(count)};

  if (vkAllocateCommandBuffers(mLogicalDevice, &cmdBufAllocInfo,
                               commandBuffers) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }

  std::cout << "Allocated " << count << " command buffers successfully."
            << std::endl;
}

void VulkanCore::freeCommandBuffers(VkCommandBuffer *commandBuffers,
                                    int32_t count) {
  vkQueueWaitIdle(mGraphicsQueue.getVkQueue()); // Wait for all work to finish
  vkFreeCommandBuffers(mLogicalDevice, mCommandPool,
                       static_cast<uint32_t>(count), commandBuffers);
}

VkImage VulkanCore::getSwapchainImage(int32_t index) const {
  if (index < 0 || index >= static_cast<int32_t>(mSwapchainImages.size())) {
    throw std::out_of_range("Swapchain image index out of range: " +
                            std::to_string(index));
  }
  return mSwapchainImages[index];
}

VkRenderPass VulkanCore::createSimpleRenderPass() {
  // Implementation of simple render pass creation
  // This function would typically set up a VkRenderPassCreateInfo structure
  // and call vkCreateRenderPass to create a render pass suitable for basic
  // rendering.

  VkAttachmentDescription colorAttachDesc = {
      .flags = 0,
      .format = mSwapchainSurfaceFormat.format,
      .samples = VK_SAMPLE_COUNT_1_BIT,        // no multisampling
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,   // clear the attachment at the
                                               // start of the render pass :
                                               // Vulkan optimise the buffers
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // store into memory
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // swapchain image layout
                                                  // before the render pass
      .finalLayout =
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // swapchain image layout to
                                          // transition to after the render pass
  };

  VkAttachmentReference colorAttachRef = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkFormat depthFormat =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mDepthFormat;

  VkAttachmentDescription depthAttachmentDesc = {
      .flags = 0,
      .format = depthFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkAttachmentReference depthAttachRef = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpassDesc = {
      .flags = 0,
      .pipelineBindPoint =
          VK_PIPELINE_BIND_POINT_GRAPHICS, // cpu or gpu pipeline
      .inputAttachmentCount = 0,           // shader read only attachments
      .pInputAttachments = nullptr,        //
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachRef,
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = mDepthEnabled ? &depthAttachRef : nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr};

  std::vector<VkAttachmentDescription> attachments;
  attachments.push_back(colorAttachDesc);

  if (mDepthEnabled) {
    attachments.push_back(depthAttachmentDesc);
  }

  VkRenderPassCreateInfo renderPassCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(), // pointer to attachment descriptions
      .subpassCount = 1,                  // only one subpass
      .pSubpasses = &subpassDesc,         // pointer to subpass description
      .dependencyCount = 0,               // no subpass dependencies
      .pDependencies = nullptr            // no subpass dependencies
  };

  VkRenderPass renderPass;
  ;
  if (vkCreateRenderPass(mLogicalDevice, &renderPassCreateInfo, nullptr,
                         &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create simple render pass!");
  }
  std::cout << "Simple render pass created successfully." << std::endl;

  return renderPass;
}

std::vector<VkFramebuffer>
VulkanCore::createFrameBuffer(VkRenderPass renderPass) {
  mFrameBuffers.resize(mSwapchainImages.size());

  int widnowWidth, windowHeight;
  glfwGetFramebufferSize(mWindow, &widnowWidth, &windowHeight);

  for (uint32_t i{0}; i < mSwapchainImages.size(); ++i) {
    std::vector<VkImageView> attachments;
    attachments.push_back(mSwapchainImageViews[i]);
    if (mDepthEnabled) {
      attachments.push_back(mDepthImages[i].mImageView);
    }

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = static_cast<uint32_t>(widnowWidth),
        .height = static_cast<uint32_t>(windowHeight),
        .layers = 1};

    if (vkCreateFramebuffer(mLogicalDevice, &framebufferCreateInfo, nullptr,
                            &mFrameBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer " +
                               std::to_string(i));
    }
  }

  std::cout << "Created " << mFrameBuffers.size()
            << " framebuffers successfully." << std::endl;
  return mFrameBuffers;
}

void VulkanCore::destroyFramebuffers(std::vector<VkFramebuffer> &framebuffers) {
  for (VkFramebuffer framebuffer : framebuffers) {
    vkDestroyFramebuffer(mLogicalDevice, framebuffer, nullptr);
  }
  framebuffers.clear();
  std::cout << "Framebuffers destroyed." << std::endl;
}

BufferAndMemory VulkanCore::createVertexBuffer(const void *pVertices,
                                               size_t size) {
  // Step1 : create staging buffer
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  BufferAndMemory stagingVB = createBuffer(size, usage, memProperties);

  // Step2 : map the memory of the stage buffer
  void *pData;
  VkDeviceSize offset = 0;
  VkMemoryMapFlags flags = 0;
  if (vkMapMemory(mLogicalDevice, stagingVB.mMemory, offset,
                  stagingVB.mAllocationSize, flags, &pData) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map vertex buffer memory!");
  }

  // Step 3 : copy the vertices to the staging buffer
  memcpy(pData, pVertices, size);

  // step4 : unmao/release the mapped meory
  vkUnmapMemory(mLogicalDevice, stagingVB.mMemory);

  // Step 5 : create the final vertex buffer with device local memory propertys
  // usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
  // VK_BUFFER_USAGE_TRANSFER_DST_BIT; // In General, vertex buffer usage flag
  // is required since we  are going to follow  Programmable vertex pulling
  // (PVP) approach so we don't need VK_BUFFER_USAGE_VERTEX_BUFFER_BIT in the
  // usage flags but STORAGE_BUFFER_BIT is required to use the buffer in the
  // descriptor set as storage buffer
  usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  memProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  BufferAndMemory vertexBuffer = createBuffer(size, usage, memProperties);

  // Step 6 : copy data from staging buffer to vertex buffer
  copyBuffer(stagingVB.mBuffer, vertexBuffer.mBuffer, size);

  // Step 7 : destroy staging buffer and free its memory
  stagingVB.Destroy(mLogicalDevice);

  return vertexBuffer;
}

BufferAndMemory
VulkanCore::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags reqMemPropFlags) {
  BufferAndMemory bufferAndMemory;

  // Create buffer
  VkBufferCreateInfo vbCreateInfo = {.sType =
                                         VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                     .pNext = nullptr,
                                     .flags = 0,
                                     .size = size,
                                     .usage = usage,
                                     .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                     .queueFamilyIndexCount = 0,
                                     .pQueueFamilyIndices = nullptr};

  // Step 1. create buffer
  if (vkCreateBuffer(mLogicalDevice, &vbCreateInfo, nullptr,
                     &bufferAndMemory.mBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer!");
  }

  // Step 2. get buffer memory requirements
  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(mLogicalDevice, bufferAndMemory.mBuffer,
                                &memRequirements);
  std::cout << "Buffer memory requirements: size = " << memRequirements.size
            << ", alignment = " << memRequirements.alignment
            << ", memoryTypeBits = " << memRequirements.memoryTypeBits
            << std::endl;
  // bufferAndMemory.mAllocationSize = memRequirements.size;

  // Step 3. get the memory type index
  uint32_t memoryTypeIndex =
      getMemoryTypeIndex(memRequirements.memoryTypeBits, reqMemPropFlags);
  std::cout << "Selected memory type index: " << memoryTypeIndex << std::endl;

  VkMemoryAllocateInfo allocInfo = {.sType =
                                        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .pNext = nullptr,
                                    .allocationSize = memRequirements.size,
                                    .memoryTypeIndex = memoryTypeIndex};

  // Step 4. allocate memory
  if (vkAllocateMemory(mLogicalDevice, &allocInfo, nullptr,
                       &bufferAndMemory.mMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  // Step 5. Bind buffer and memory
  vkBindBufferMemory(mLogicalDevice, bufferAndMemory.mBuffer,
                     bufferAndMemory.mMemory, 0);

  bufferAndMemory.mAllocationSize = memRequirements.size;

  return bufferAndMemory;
}

uint32_t VulkanCore::getMemoryTypeIndex(uint32_t typeFilter,
                                        VkMemoryPropertyFlags reqMemPropFlags) {
  const VkPhysicalDeviceMemoryProperties &memProperties =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mMemoryProperties;

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    const VkMemoryType &memType = memProperties.memoryTypes[i];
    uint32_t curBitMask = (1 << i);
    bool isRequiredType = ((typeFilter & curBitMask) != 0);
    bool hasRequiredProperties =
        ((memType.propertyFlags & reqMemPropFlags) == reqMemPropFlags);

    if (isRequiredType && hasRequiredProperties) {
      return i;
    }
  }

  std::cout << "Can't find suitable memory type! typeFilter: " << typeFilter
            << ", properties: " << reqMemPropFlags << std::endl;
  return -1;
}

void VulkanCore::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                            VkDeviceSize size) {
  BeginCommandBuffer(mCopyCmdBuffer,
                     VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VkBufferCopy copyRegion = {.srcOffset = 0, .dstOffset = 0, .size = size};

  vkCmdCopyBuffer(mCopyCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  vkEndCommandBuffer(mCopyCmdBuffer);
  mGraphicsQueue.submitSync(mCopyCmdBuffer);
  mGraphicsQueue.waitIdle(); // flush the command buffer
}

std::vector<BufferAndMemory> VulkanCore::createUniformBuffers(size_t size) {
  std::vector<BufferAndMemory> uniformBuffers(mSwapchainImages.size());
  for (int32_t i{0}; i < static_cast<int32_t>(mSwapchainImages.size()); ++i) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags memProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uniformBuffers[i] = createBuffer(size, usage, memProperties);
  }
  return uniformBuffers;
}

void BufferAndMemory::Destroy(VkDevice device) {
  if (mBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, mBuffer, nullptr);
    mBuffer = VK_NULL_HANDLE;
  }
  if (mMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, mMemory, nullptr);
    mMemory = VK_NULL_HANDLE;
  }
  mAllocationSize = 0;
}

void BufferAndMemory::update(VkDevice device, const void *pData,
                             VkDeviceSize size) {
  void *mappedData = nullptr;
  if (vkMapMemory(device, mMemory, 0, size, 0, &mappedData) == VK_SUCCESS) {
    memcpy(mappedData, pData, static_cast<size_t>(size));
    vkUnmapMemory(device, mMemory);
  }
}

void VulkanTexture::Destroy(VkDevice device) {
  if (mSampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, mSampler, nullptr);
    mSampler = VK_NULL_HANDLE;
  }
  if (mImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, mImageView, nullptr);
    mImageView = VK_NULL_HANDLE;
  }
  if (mImage != VK_NULL_HANDLE) {
    vkDestroyImage(device, mImage, nullptr);
    mImage = VK_NULL_HANDLE;
  }
  if (mImageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, mImageMemory, nullptr);
    mImageMemory = VK_NULL_HANDLE;
  }
  mWidth = 0;
  mHeight = 0;
}

void VulkanCore::createTexture(std::string filePath,
                               VulkanTexture &outTexture) {
  // Step1 : Load image using stb_image
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  if (!pixels) {
    throw std::runtime_error("Failed to load texture image: " + filePath);
  }

  std::cout << "Loaded texture: " << filePath << " (" << texWidth << "x"
            << texHeight << ", original channels: " << texChannels << ")"
            << std::endl;

  // Store dimensions
  outTexture.mWidth = static_cast<uint32_t>(texWidth);
  outTexture.mHeight = static_cast<uint32_t>(texHeight);

  // Step2 : create the image object and allocate memory
  // Note: STBI_rgb_alpha ensures pixels are always RGBA format regardless of
  // original channels
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
  createTextureImageFromData(outTexture, pixels, texWidth, texHeight,
                             imageFormat);

  // Step3 : Free pixel data loaded by stb_image
  stbi_image_free(pixels);

  // Step4 : create Vulkan image view
  VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
  outTexture.mImageView = createImageView(mLogicalDevice, outTexture.mImage,
                                          imageFormat, aspectFlags);

  // Step5 : create texture sampler
  VkFilter minFilter = VK_FILTER_LINEAR;
  VkFilter magFilter = VK_FILTER_LINEAR;
  VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  outTexture.mSampler =
      createTextureSampler(mLogicalDevice, minFilter, magFilter, addressMode);

  std::cout << "Texture created successfully from file: " << filePath
            << std::endl;
}

void VulkanCore::createTextureImageFromData(VulkanTexture &outTexture,
                                            const void *pixels, int texWidth,
                                            int texHeight,
                                            VkFormat imageFormat) {
  VkImageUsageFlags usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  VkMemoryPropertyFlags memProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  createImage(outTexture, texWidth, texHeight, imageFormat, usage,
              memProperties);
  updateTextureImage(outTexture, texWidth, texHeight, imageFormat, pixels);
}

void VulkanCore::createImage(VulkanTexture &outTexture, uint32_t width,
                             uint32_t height, VkFormat format,
                             VkImageUsageFlags usage,
                             VkMemoryPropertyFlags reqMemPropFlags) {
  // Step 1: Create image
  VkImageCreateInfo imageCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {.width = static_cast<uint32_t>(width),
                 .height = static_cast<uint32_t>(height),
                 .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  // Step1 : create image object
  if (vkCreateImage(mLogicalDevice, &imageCreateInfo, nullptr,
                    &outTexture.mImage) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image!");
  }

  // Step2 : get memory requirements
  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(mLogicalDevice, outTexture.mImage,
                               &memRequirements);

  // Step3 : get memory type index
  uint32_t memoryTypeIndex =
      getMemoryTypeIndex(memRequirements.memoryTypeBits, reqMemPropFlags);
  std::cout << "Selected memory type index for image: " << memoryTypeIndex
            << std::endl;

  // Step4 : allocate memory
  VkMemoryAllocateInfo allocInfo = {.sType =
                                        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .pNext = nullptr,
                                    .allocationSize = memRequirements.size,
                                    .memoryTypeIndex = memoryTypeIndex};
  if (vkAllocateMemory(mLogicalDevice, &allocInfo, nullptr,
                       &outTexture.mImageMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate image memory!");
  }

  // Step5 : bind image and memory
  if (vkBindImageMemory(mLogicalDevice, outTexture.mImage,
                        outTexture.mImageMemory, 0) != VK_SUCCESS) {
    throw std::runtime_error("Failed to bind image and memory!");
  }
}

void VulkanCore::updateTextureImage(VulkanTexture &outTexture, uint32_t width,
                                    uint32_t height, VkFormat format,
                                    const void *pixels) {

  int32_t bytesPerPixel = 4; // Assuming 4 bytes per pixel (RGBA)
  VkDeviceSize layerSize = width * height * bytesPerPixel;
  VkDeviceSize imageSize = layerSize; // For single layer image

  // Create staging buffer
  BufferAndMemory stagingTexture =
      createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  stagingTexture.update(mLogicalDevice, pixels, imageSize);

  // Transition image layout to TRANSFER_DST_OPTIMAL
  transitionImageLayout(outTexture.mImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Copy buffer to image
  copyBufferToImage(stagingTexture.mBuffer, outTexture.mImage,
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height));

  // Transition image layout to SHADER_READ_ONLY_OPTIMAL
  transitionImageLayout(outTexture.mImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Clean up staging buffer
  stagingTexture.Destroy(mLogicalDevice);
}

void VulkanCore::transitionImageLayout(VkImage image, VkFormat format,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout) {
  BeginCommandBuffer(mCopyCmdBuffer,
                     VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  imageMemBarrier(mCopyCmdBuffer, image, format, oldLayout, newLayout);
  submitCopyCommand();
}

void VulkanCore::submitCopyCommand() {
  vkEndCommandBuffer(mCopyCmdBuffer);
  mGraphicsQueue.submitSync(mCopyCmdBuffer);
  mGraphicsQueue.waitIdle(); // flush the command buffer
}

void VulkanCore::copyBufferToImage(VkBuffer buffer, VkImage image,
                                   uint32_t width, uint32_t height) {
  BeginCommandBuffer(mCopyCmdBuffer,
                     VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VkBufferImageCopy bufferImageCopy = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .mipLevel = 0,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1}};

  vkCmdCopyBufferToImage(mCopyCmdBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferImageCopy);

  submitCopyCommand();
}

void VulkanCore::createDepthResources() {
  const int32_t numSwapChainImages =
      static_cast<int32_t>(mSwapchainImages.size());
  mDepthImages.resize(numSwapChainImages);

  const auto &surfaceCaps =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mSurfaceCaps;
  VkFormat depthFormat =
      mPhysicalDevice.getSelectedPhysicalDeviceProperties().mDepthFormat;
  for (int32_t i = 0; i < numSwapChainImages; ++i) {
    // Create depth image
    VkImageUsageFlagBits usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkMemoryPropertyFlagBits memProperties =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createImage(mDepthImages[i], surfaceCaps.currentExtent.width,
                surfaceCaps.currentExtent.height, depthFormat, usage,
                memProperties);

    // Transition depth image layout
    transitionImageLayout(mDepthImages[i].mImage, depthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Create depth image view
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    mDepthImages[i].mImageView = createImageView(
        mLogicalDevice, mDepthImages[i].mImage, depthFormat, aspectFlags);
  }
}

} // namespace VulkanCore
