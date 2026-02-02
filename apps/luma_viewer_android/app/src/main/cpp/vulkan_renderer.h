// LUMA Viewer Android - Vulkan Renderer Header
// Cross-platform Vulkan rendering for 3D model viewing

#pragma once

#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace luma {

// Camera parameters
struct CameraParams {
    float eyeX = 0.0f, eyeY = 0.5f, eyeZ = 3.0f;
    float atX = 0.0f, atY = 0.0f, atZ = 0.0f;
    float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
    float fovY = 0.785f;  // 45 degrees
    float aspectRatio = 1.0f;
    float nearZ = 0.01f;
    float farZ = 1000.0f;
};

// Mesh data
struct SimpleMesh {
    std::vector<float> vertices;  // pos(3) + normal(3) + uv(2)
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    uint32_t indexCount = 0;
};

// Vulkan renderer for Android
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // Initialization
    bool initialize(ANativeWindow* window, int width, int height);
    void shutdown();
    
    // Resize
    void resize(int width, int height);
    
    // Rendering
    void render();
    
    // Camera control
    void orbit(float deltaYaw, float deltaPitch);
    void pan(float deltaX, float deltaY);
    void zoom(float scaleFactor);
    void resetCamera();
    
    // Model loading
    bool loadModel(const std::string& path);
    
    // Settings
    void toggleGrid() { showGrid_ = !showGrid_; }
    void toggleAutoRotate() { autoRotate_ = !autoRotate_; }
    
    // Info
    std::string getInfo() const;

private:
    // Vulkan initialization
    bool createInstance();
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSurface(ANativeWindow* window);
    bool createSwapchain();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    bool createPipeline();
    bool createDescriptorSetLayout();
    bool createUniformBuffers();
    bool createDescriptorPool();
    bool createDescriptorSets();
    
    // Cleanup
    void cleanupSwapchain();
    void recreateSwapchain();
    
    // Helper functions
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                      VkDeviceMemory& memory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    // Mesh creation
    void createGridMesh();
    void createCubeMesh();
    void updateUniformBuffer();
    
    // Matrix math
    void buildViewMatrix(float* mat);
    void buildProjectionMatrix(float* mat);

private:
    // Window
    ANativeWindow* window_ = nullptr;
    int width_ = 800;
    int height_ = 600;
    
    // Vulkan core
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsFamily_ = 0;
    uint32_t presentFamily_ = 0;
    
    // Surface and swapchain
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent_ = {800, 600};
    
    // Depth buffer
    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    
    // Render pass and framebuffers
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
    
    // Pipeline
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkPipeline gridPipeline_ = VK_NULL_HANDLE;
    
    // Command buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    
    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    uint32_t currentFrame_ = 0;
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    
    // Descriptors
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets_;
    
    // Uniform buffers
    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceMemory> uniformBuffersMemory_;
    std::vector<void*> uniformBuffersMapped_;
    
    // Meshes
    SimpleMesh gridMesh_;
    SimpleMesh cubeMesh_;
    
    // Camera state
    CameraParams camera_;
    float cameraYaw_ = 0.78f;
    float cameraPitch_ = 0.4f;
    float cameraDistance_ = 3.0f;
    float cameraTargetX_ = 0.0f;
    float cameraTargetY_ = 0.0f;
    float cameraTargetZ_ = 0.0f;
    
    // Settings
    bool showGrid_ = true;
    bool autoRotate_ = false;
    float totalTime_ = 0.0f;
    
    // Stats
    uint32_t frameCount_ = 0;
    float fps_ = 60.0f;
};

} // namespace luma
