// LUMA Viewer Android - Vulkan Renderer Implementation

#include "vulkan_renderer.h"
#include <android/log.h>
#include <chrono>
#include <cstring>
#include <sstream>
#include <array>

#define LOG_TAG "LumaVulkan"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace luma {

// Uniform buffer object
struct UniformBufferObject {
    float model[16];
    float view[16];
    float proj[16];
    float color[4];
};

// SPIR-V shader code (simplified vertex/fragment shaders)
// These would normally be compiled from GLSL, but we include pre-compiled SPIR-V for simplicity

// Vertex shader SPIR-V (simple transform + color pass-through)
static const uint32_t vertShaderCode[] = {
    0x07230203, 0x00010000, 0x0008000a, 0x0000002e, 0x00000000, 0x00020011,
    0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000a000f, 0x00000000,
    0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000012, 0x00000024,
    0x00000028, 0x0000002c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x0000000b, 0x505f6c67,
    0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b, 0x00000000,
    0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000000d, 0x00000000,
    0x00050005, 0x00000011, 0x736f5061, 0x6f697469, 0x0000006e, 0x00030005,
    0x00000012, 0x00000000, 0x00040005, 0x00000016, 0x4f425500, 0x00000000,
    0x00050006, 0x00000016, 0x00000000, 0x65646f6d, 0x0000006c, 0x00050006,
    0x00000016, 0x00000001, 0x77656976, 0x00000000, 0x00050006, 0x00000016,
    0x00000002, 0x6a6f7270, 0x00000000, 0x00050006, 0x00000016, 0x00000003,
    0x6f6c6f63, 0x00000072, 0x00030005, 0x00000018, 0x006f6275, 0x00040005,
    0x00000024, 0x6f6c6f66, 0x00007461, 0x00040005, 0x00000028, 0x6f6c6f43,
    0x00000072, 0x00040005, 0x0000002c, 0x726f4e61, 0x006c616d, 0x00050048,
    0x0000000b, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x0000000b,
    0x00000002, 0x00040047, 0x00000012, 0x0000001e, 0x00000000, 0x00040048,
    0x00000016, 0x00000000, 0x00000005, 0x00050048, 0x00000016, 0x00000000,
    0x00000023, 0x00000000, 0x00050048, 0x00000016, 0x00000000, 0x00000007,
    0x00000010, 0x00040048, 0x00000016, 0x00000001, 0x00000005, 0x00050048,
    0x00000016, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x00000016,
    0x00000001, 0x00000007, 0x00000010, 0x00040048, 0x00000016, 0x00000002,
    0x00000005, 0x00050048, 0x00000016, 0x00000002, 0x00000023, 0x00000080,
    0x00050048, 0x00000016, 0x00000002, 0x00000007, 0x00000010, 0x00050048,
    0x00000016, 0x00000003, 0x00000023, 0x000000c0, 0x00030047, 0x00000016,
    0x00000002, 0x00040047, 0x00000018, 0x00000022, 0x00000000, 0x00040047,
    0x00000018, 0x00000021, 0x00000000, 0x00040047, 0x00000024, 0x0000001e,
    0x00000000, 0x00040047, 0x00000028, 0x0000001e, 0x00000001, 0x00040047,
    0x0000002c, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021,
    0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000004, 0x0003001e, 0x0000000b, 0x00000007,
    0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b, 0x0000000c,
    0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001,
    0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040017, 0x00000010,
    0x00000006, 0x00000003, 0x00040020, 0x00000011, 0x00000001, 0x00000010,
    0x0004003b, 0x00000011, 0x00000012, 0x00000001, 0x0004002b, 0x00000006,
    0x00000014, 0x3f800000, 0x00040018, 0x00000015, 0x00000007, 0x00000004,
    0x0006001e, 0x00000016, 0x00000015, 0x00000015, 0x00000015, 0x00000007,
    0x00040020, 0x00000017, 0x00000002, 0x00000016, 0x0004003b, 0x00000017,
    0x00000018, 0x00000002, 0x0004002b, 0x0000000e, 0x00000019, 0x00000002,
    0x00040020, 0x0000001a, 0x00000002, 0x00000015, 0x0004002b, 0x0000000e,
    0x0000001d, 0x00000001, 0x00040020, 0x00000023, 0x00000003, 0x00000007,
    0x0004003b, 0x00000023, 0x00000024, 0x00000003, 0x0004002b, 0x0000000e,
    0x00000025, 0x00000003, 0x00040020, 0x00000026, 0x00000002, 0x00000007,
    0x0004003b, 0x00000011, 0x00000028, 0x00000001, 0x00040020, 0x0000002b,
    0x00000003, 0x00000010, 0x0004003b, 0x0000002b, 0x0000002c, 0x00000003,
    0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x0004003d, 0x00000010, 0x00000013, 0x00000012, 0x00050051,
    0x00000006, 0x00000020, 0x00000013, 0x00000000, 0x00050051, 0x00000006,
    0x00000021, 0x00000013, 0x00000001, 0x00050051, 0x00000006, 0x00000022,
    0x00000013, 0x00000002, 0x00070050, 0x00000007, 0x0000001c, 0x00000020,
    0x00000021, 0x00000022, 0x00000014, 0x00050041, 0x0000001a, 0x0000001b,
    0x00000018, 0x00000019, 0x0004003d, 0x00000015, 0x0000001e, 0x0000001b,
    0x00050041, 0x0000001a, 0x0000001f, 0x00000018, 0x0000001d, 0x0004003d,
    0x00000015, 0x0000002d, 0x0000001f, 0x00050041, 0x0000001a, 0x00000029,
    0x00000018, 0x0000000f, 0x0004003d, 0x00000015, 0x0000002a, 0x00000029,
    0x00050091, 0x00000007, 0x00000030, 0x0000002a, 0x0000001c, 0x00050091,
    0x00000007, 0x00000031, 0x0000002d, 0x00000030, 0x00050091, 0x00000007,
    0x00000032, 0x0000001e, 0x00000031, 0x00050041, 0x00000023, 0x00000033,
    0x0000000d, 0x0000000f, 0x0003003e, 0x00000033, 0x00000032, 0x00050041,
    0x00000026, 0x00000027, 0x00000018, 0x00000025, 0x0004003d, 0x00000007,
    0x00000034, 0x00000027, 0x0003003e, 0x00000024, 0x00000034, 0x0004003d,
    0x00000010, 0x00000035, 0x00000028, 0x0003003e, 0x0000002c, 0x00000035,
    0x000100fd, 0x00010038
};

// Fragment shader SPIR-V (simple color output)
static const uint32_t fragShaderCode[] = {
    0x07230203, 0x00010000, 0x0008000a, 0x00000013, 0x00000000, 0x00020011,
    0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004,
    0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f,
    0x726f6c6f, 0x00000000, 0x00040005, 0x0000000c, 0x6f6c6f66, 0x00007461,
    0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000c,
    0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
    0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007,
    0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040020, 0x0000000b,
    0x00000001, 0x00000007, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000001,
    0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x0004003d, 0x00000007, 0x0000000d, 0x0000000c, 0x0003003e,
    0x00000009, 0x0000000d, 0x000100fd, 0x00010038
};

VulkanRenderer::VulkanRenderer() {
}

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::initialize(ANativeWindow* window, int width, int height) {
    LOGI("Initializing Vulkan renderer %dx%d", width, height);
    
    window_ = window;
    width_ = width;
    height_ = height;
    
    if (!createInstance()) return false;
    if (!createSurface(window)) return false;
    if (!selectPhysicalDevice()) return false;
    if (!createLogicalDevice()) return false;
    if (!createSwapchain()) return false;
    if (!createRenderPass()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPool()) return false;
    if (!createDescriptorSetLayout()) return false;
    if (!createPipeline()) return false;
    if (!createUniformBuffers()) return false;
    if (!createDescriptorPool()) return false;
    if (!createDescriptorSets()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;
    
    // Create meshes
    createGridMesh();
    createCubeMesh();
    
    LOGI("Vulkan initialized successfully");
    return true;
}

void VulkanRenderer::shutdown() {
    if (device_ == VK_NULL_HANDLE) return;
    
    vkDeviceWaitIdle(device_);
    
    // Cleanup meshes
    if (gridMesh_.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, gridMesh_.vertexBuffer, nullptr);
        vkFreeMemory(device_, gridMesh_.vertexMemory, nullptr);
    }
    if (gridMesh_.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, gridMesh_.indexBuffer, nullptr);
        vkFreeMemory(device_, gridMesh_.indexMemory, nullptr);
    }
    if (cubeMesh_.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, cubeMesh_.vertexBuffer, nullptr);
        vkFreeMemory(device_, cubeMesh_.vertexMemory, nullptr);
    }
    if (cubeMesh_.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, cubeMesh_.indexBuffer, nullptr);
        vkFreeMemory(device_, cubeMesh_.indexMemory, nullptr);
    }
    
    cleanupSwapchain();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
        vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
    }
    
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
        vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
        vkDestroyFence(device_, inFlightFences_[i], nullptr);
    }
    
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
    
    device_ = VK_NULL_HANDLE;
    instance_ = VK_NULL_HANDLE;
}

void VulkanRenderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    recreateSwapchain();
}

bool VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "LUMA Viewer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "LUMA Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;
    
    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::selectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        LOGE("No Vulkan devices found");
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
    
    physicalDevice_ = devices[0];  // Use first device
    
    // Find queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());
    
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily_ = i;
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &presentSupport);
            if (presentSupport) {
                presentFamily_ = i;
                break;
            }
        }
    }
    
    return true;
}

bool VulkanRenderer::createLogicalDevice() {
    float queuePriority = 1.0f;
    
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily_;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    
    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        LOGE("Failed to create logical device");
        return false;
    }
    
    vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentFamily_, 0, &presentQueue_);
    
    return true;
}

bool VulkanRenderer::createSurface(ANativeWindow* window) {
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;
    
    if (vkCreateAndroidSurfaceKHR(instance_, &createInfo, nullptr, &surface_) != VK_SUCCESS) {
        LOGE("Failed to create Android surface");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);
    
    swapchainExtent_ = capabilities.currentExtent;
    if (swapchainExtent_.width == 0xFFFFFFFF) {
        swapchainExtent_.width = width_;
        swapchainExtent_.height = height_;
    }
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = swapchainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    swapchainFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
    
    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        LOGE("Failed to create swapchain");
        return false;
    }
    
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());
    
    // Create image views
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat_;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_[i]) != VK_SUCCESS) {
            LOGE("Failed to create image views");
            return false;
        }
    }
    
    // Create depth buffer
    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.extent.width = swapchainExtent_.width;
    depthImageInfo.extent.height = swapchainExtent_.height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkCreateImage(device_, &depthImageInfo, nullptr, &depthImage_);
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, depthImage_, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    vkAllocateMemory(device_, &allocInfo, nullptr, &depthMemory_);
    vkBindImageMemory(device_, depthImage_, depthMemory_, 0);
    
    VkImageViewCreateInfo depthViewInfo{};
    depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewInfo.image = depthImage_;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;
    
    vkCreateImageView(device_, &depthViewInfo, nullptr, &depthImageView_);
    
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        LOGE("Failed to create render pass");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    framebuffers_.resize(swapchainImageViews_.size());
    
    for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews_[i],
            depthImageView_
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            LOGE("Failed to create framebuffer");
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;
    
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        LOGE("Failed to create command pool");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
        LOGE("Failed to create descriptor set layout");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createPipeline() {
    // Create shader modules
    std::vector<uint32_t> vertCode(vertShaderCode, vertShaderCode + sizeof(vertShaderCode) / sizeof(uint32_t));
    std::vector<uint32_t> fragCode(fragShaderCode, fragShaderCode + sizeof(fragShaderCode) / sizeof(uint32_t));
    
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input (pos + normal + color)
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 9;  // pos(3) + normal(3) + color(3)
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float) * 3;
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = sizeof(float) * 6;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent_.width;
    viewport.height = (float)swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout");
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;
    
    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline");
        return false;
    }
    
    vkDestroyShaderModule(device_, fragModule, nullptr);
    vkDestroyShaderModule(device_, vertModule, nullptr);
    
    return true;
}

bool VulkanRenderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers_.size();
    
    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        LOGE("Failed to allocate command buffers");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            LOGE("Failed to create sync objects");
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers_[i], uniformBuffersMemory_[i]);
        
        vkMapMemory(device_, uniformBuffersMemory_[i], 0, bufferSize, 0, &uniformBuffersMapped_[i]);
    }
    
    return true;
}

bool VulkanRenderer::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        LOGE("Failed to create descriptor pool");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    
    descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
        LOGE("Failed to allocate descriptor sets");
        return false;
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets_[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
    }
    
    return true;
}

void VulkanRenderer::cleanupSwapchain() {
    vkDestroyImageView(device_, depthImageView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthMemory_, nullptr);
    
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    
    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyRenderPass(device_, renderPass_, nullptr);
    
    for (auto imageView : swapchainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
    
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
}

void VulkanRenderer::recreateSwapchain() {
    vkDeviceWaitIdle(device_);
    cleanupSwapchain();
    createSwapchain();
    createRenderPass();
    createPipeline();
    createFramebuffers();
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return 0;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                   VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer);
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    vkAllocateMemory(device_, &allocInfo, nullptr, &memory);
    vkBindBufferMemory(device_, buffer, memory, 0);
}

void VulkanRenderer::createGridMesh() {
    // Create a simple grid
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    const int gridSize = 10;
    const float gridSpacing = 1.0f;
    uint32_t idx = 0;
    
    for (int i = -gridSize; i <= gridSize; i++) {
        // Horizontal line
        vertices.insert(vertices.end(), {(float)i * gridSpacing, 0.0f, -gridSize * gridSpacing,  0, 1, 0,  0.3f, 0.3f, 0.3f});
        vertices.insert(vertices.end(), {(float)i * gridSpacing, 0.0f, gridSize * gridSpacing,   0, 1, 0,  0.3f, 0.3f, 0.3f});
        indices.push_back(idx++);
        indices.push_back(idx++);
        
        // Vertical line
        vertices.insert(vertices.end(), {-gridSize * gridSpacing, 0.0f, (float)i * gridSpacing,  0, 1, 0,  0.3f, 0.3f, 0.3f});
        vertices.insert(vertices.end(), {gridSize * gridSpacing, 0.0f, (float)i * gridSpacing,   0, 1, 0,  0.3f, 0.3f, 0.3f});
        indices.push_back(idx++);
        indices.push_back(idx++);
    }
    
    gridMesh_.indexCount = indices.size();
    
    // Create buffers
    VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 gridMesh_.vertexBuffer, gridMesh_.vertexMemory);
    
    void* data;
    vkMapMemory(device_, gridMesh_.vertexMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(device_, gridMesh_.vertexMemory);
    
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 gridMesh_.indexBuffer, gridMesh_.indexMemory);
    
    vkMapMemory(device_, gridMesh_.indexMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(device_, gridMesh_.indexMemory);
}

void VulkanRenderer::createCubeMesh() {
    // Simple cube vertices (pos + normal + color)
    std::vector<float> vertices = {
        // Front face (red)
        -0.5f, -0.5f,  0.5f,  0, 0, 1,  0.8f, 0.2f, 0.2f,
         0.5f, -0.5f,  0.5f,  0, 0, 1,  0.8f, 0.2f, 0.2f,
         0.5f,  0.5f,  0.5f,  0, 0, 1,  0.8f, 0.2f, 0.2f,
        -0.5f,  0.5f,  0.5f,  0, 0, 1,  0.8f, 0.2f, 0.2f,
        // Back face (green)
        -0.5f, -0.5f, -0.5f,  0, 0,-1,  0.2f, 0.8f, 0.2f,
        -0.5f,  0.5f, -0.5f,  0, 0,-1,  0.2f, 0.8f, 0.2f,
         0.5f,  0.5f, -0.5f,  0, 0,-1,  0.2f, 0.8f, 0.2f,
         0.5f, -0.5f, -0.5f,  0, 0,-1,  0.2f, 0.8f, 0.2f,
        // Top face (blue)
        -0.5f,  0.5f, -0.5f,  0, 1, 0,  0.2f, 0.2f, 0.8f,
        -0.5f,  0.5f,  0.5f,  0, 1, 0,  0.2f, 0.2f, 0.8f,
         0.5f,  0.5f,  0.5f,  0, 1, 0,  0.2f, 0.2f, 0.8f,
         0.5f,  0.5f, -0.5f,  0, 1, 0,  0.2f, 0.2f, 0.8f,
        // Bottom face (yellow)
        -0.5f, -0.5f, -0.5f,  0,-1, 0,  0.8f, 0.8f, 0.2f,
         0.5f, -0.5f, -0.5f,  0,-1, 0,  0.8f, 0.8f, 0.2f,
         0.5f, -0.5f,  0.5f,  0,-1, 0,  0.8f, 0.8f, 0.2f,
        -0.5f, -0.5f,  0.5f,  0,-1, 0,  0.8f, 0.8f, 0.2f,
        // Right face (cyan)
         0.5f, -0.5f, -0.5f,  1, 0, 0,  0.2f, 0.8f, 0.8f,
         0.5f,  0.5f, -0.5f,  1, 0, 0,  0.2f, 0.8f, 0.8f,
         0.5f,  0.5f,  0.5f,  1, 0, 0,  0.2f, 0.8f, 0.8f,
         0.5f, -0.5f,  0.5f,  1, 0, 0,  0.2f, 0.8f, 0.8f,
        // Left face (magenta)
        -0.5f, -0.5f, -0.5f, -1, 0, 0,  0.8f, 0.2f, 0.8f,
        -0.5f, -0.5f,  0.5f, -1, 0, 0,  0.8f, 0.2f, 0.8f,
        -0.5f,  0.5f,  0.5f, -1, 0, 0,  0.8f, 0.2f, 0.8f,
        -0.5f,  0.5f, -0.5f, -1, 0, 0,  0.8f, 0.2f, 0.8f,
    };
    
    std::vector<uint32_t> indices = {
        0,1,2, 0,2,3,     // Front
        4,5,6, 4,6,7,     // Back
        8,9,10, 8,10,11,  // Top
        12,13,14, 12,14,15, // Bottom
        16,17,18, 16,18,19, // Right
        20,21,22, 20,22,23  // Left
    };
    
    cubeMesh_.indexCount = indices.size();
    
    // Create buffers
    VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 cubeMesh_.vertexBuffer, cubeMesh_.vertexMemory);
    
    void* data;
    vkMapMemory(device_, cubeMesh_.vertexMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(device_, cubeMesh_.vertexMemory);
    
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 cubeMesh_.indexBuffer, cubeMesh_.indexMemory);
    
    vkMapMemory(device_, cubeMesh_.indexMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(device_, cubeMesh_.indexMemory);
}

void VulkanRenderer::buildViewMatrix(float* mat) {
    // Calculate camera position from spherical coordinates
    float cosPitch = cosf(cameraPitch_);
    float sinPitch = sinf(cameraPitch_);
    float cosYaw = cosf(cameraYaw_);
    float sinYaw = sinf(cameraYaw_);
    
    float camX = cameraTargetX_ + cameraDistance_ * cosPitch * sinYaw;
    float camY = cameraTargetY_ + cameraDistance_ * sinPitch;
    float camZ = cameraTargetZ_ + cameraDistance_ * cosPitch * cosYaw;
    
    // Build look-at matrix
    float fx = cameraTargetX_ - camX;
    float fy = cameraTargetY_ - camY;
    float fz = cameraTargetZ_ - camZ;
    float len = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= len; fy /= len; fz /= len;
    
    float ux = 0, uy = 1, uz = 0;
    
    float sx = fy * uz - fz * uy;
    float sy = fz * ux - fx * uz;
    float sz = fx * uy - fy * ux;
    len = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= len; sy /= len; sz /= len;
    
    ux = sy * fz - sz * fy;
    uy = sz * fx - sx * fz;
    uz = sx * fy - sy * fx;
    
    mat[0] = sx; mat[1] = ux; mat[2] = -fx; mat[3] = 0;
    mat[4] = sy; mat[5] = uy; mat[6] = -fy; mat[7] = 0;
    mat[8] = sz; mat[9] = uz; mat[10] = -fz; mat[11] = 0;
    mat[12] = -(sx*camX + sy*camY + sz*camZ);
    mat[13] = -(ux*camX + uy*camY + uz*camZ);
    mat[14] = (fx*camX + fy*camY + fz*camZ);
    mat[15] = 1;
}

void VulkanRenderer::buildProjectionMatrix(float* mat) {
    float aspect = (float)swapchainExtent_.width / (float)swapchainExtent_.height;
    float fov = 0.785f;  // 45 degrees
    float nearZ = 0.01f;
    float farZ = 1000.0f;
    
    float tanHalfFov = tanf(fov / 2.0f);
    
    memset(mat, 0, sizeof(float) * 16);
    mat[0] = 1.0f / (aspect * tanHalfFov);
    mat[5] = -1.0f / tanHalfFov;  // Vulkan Y is flipped
    mat[10] = farZ / (nearZ - farZ);
    mat[11] = -1.0f;
    mat[14] = (nearZ * farZ) / (nearZ - farZ);
}

void VulkanRenderer::updateUniformBuffer() {
    UniformBufferObject ubo{};
    
    // Identity model matrix
    memset(ubo.model, 0, sizeof(ubo.model));
    ubo.model[0] = ubo.model[5] = ubo.model[10] = ubo.model[15] = 1.0f;
    
    buildViewMatrix(ubo.view);
    buildProjectionMatrix(ubo.proj);
    
    ubo.color[0] = 1.0f;
    ubo.color[1] = 1.0f;
    ubo.color[2] = 1.0f;
    ubo.color[3] = 1.0f;
    
    memcpy(uniformBuffersMapped_[currentFrame_], &ubo, sizeof(ubo));
}

void VulkanRenderer::render() {
    // Auto rotate
    if (autoRotate_) {
        cameraYaw_ += 0.01f;
    }
    
    // Wait for previous frame
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    
    // Acquire image
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                             imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);
    
    // Update uniform buffer
    updateUniformBuffer();
    
    // Record command buffer
    VkCommandBuffer cmd = commandBuffers_[currentFrame_];
    vkResetCommandBuffer(cmd, 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = framebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;
    
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.15f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1,
                            &descriptorSets_[currentFrame_], 0, nullptr);
    
    // Draw cube
    VkBuffer vertexBuffers[] = {cubeMesh_.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, cubeMesh_.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, cubeMesh_.indexCount, 1, 0, 0, 0);
    
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
    
    // Submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]);
    
    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapchains[] = {swapchain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    
    vkQueuePresentKHR(presentQueue_, &presentInfo);
    
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    frameCount_++;
}

void VulkanRenderer::orbit(float deltaYaw, float deltaPitch) {
    cameraYaw_ += deltaYaw;
    cameraPitch_ += deltaPitch;
    cameraPitch_ = std::max(-1.5f, std::min(1.5f, cameraPitch_));
}

void VulkanRenderer::pan(float deltaX, float deltaY) {
    float cosYaw = cosf(cameraYaw_);
    float sinYaw = sinf(cameraYaw_);
    
    cameraTargetX_ -= deltaX * cameraDistance_ * cosYaw;
    cameraTargetZ_ -= deltaX * cameraDistance_ * sinYaw;
    cameraTargetY_ += deltaY * cameraDistance_;
}

void VulkanRenderer::zoom(float scaleFactor) {
    cameraDistance_ /= scaleFactor;
    cameraDistance_ = std::max(0.1f, std::min(100.0f, cameraDistance_));
}

void VulkanRenderer::resetCamera() {
    cameraYaw_ = 0.78f;
    cameraPitch_ = 0.4f;
    cameraDistance_ = 3.0f;
    cameraTargetX_ = cameraTargetY_ = cameraTargetZ_ = 0.0f;
}

bool VulkanRenderer::loadModel(const std::string& path) {
    // TODO: Implement model loading
    LOGI("Load model: %s", path.c_str());
    return false;
}

std::string VulkanRenderer::getInfo() const {
    std::ostringstream ss;
    ss << "LUMA Viewer (Vulkan)\n"
       << "Frame: " << frameCount_ << "\n"
       << "Size: " << swapchainExtent_.width << "x" << swapchainExtent_.height;
    return ss.str();
}

} // namespace luma
