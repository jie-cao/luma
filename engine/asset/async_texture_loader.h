// Async Texture Loader - Background texture decoding and upload
// Improves loading experience by not blocking the main thread
#pragma once

#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <memory>

namespace luma {

// ===== Texture Load Request =====
struct TextureLoadRequest {
    uint32_t id;                    // Unique request ID
    std::string path;               // File path or embedded data identifier
    std::vector<uint8_t> embeddedData;  // For embedded textures
    bool isEmbedded = false;
    
    // Callback when texture is decoded (on worker thread)
    // Returns decoded TextureData
};

// ===== Texture Load Result =====
struct TextureLoadResult {
    uint32_t id;
    TextureData data;
    bool success = false;
    std::string error;
};

// ===== Async Texture Loader =====
// Thread-safe texture loading system
class AsyncTextureLoader {
public:
    AsyncTextureLoader(size_t numThreads = 2);
    ~AsyncTextureLoader();
    
    // Non-copyable
    AsyncTextureLoader(const AsyncTextureLoader&) = delete;
    AsyncTextureLoader& operator=(const AsyncTextureLoader&) = delete;
    
    // Submit a texture load request
    // Returns a unique request ID
    uint32_t loadTexture(const std::string& path);
    uint32_t loadTextureFromMemory(const std::vector<uint8_t>& data, const std::string& name);
    
    // Check for completed textures (call from main thread)
    // Returns true if there are completed results
    bool hasCompletedTextures() const;
    
    // Get all completed textures (call from main thread)
    // Clears the completed queue
    std::vector<TextureLoadResult> getCompletedTextures();
    
    // Get pending count
    size_t getPendingCount() const;
    
    // Wait for all pending loads to complete
    void waitForAll();
    
    // Shutdown the loader
    void shutdown();
    
private:
    void workerThread();
    TextureData decodeTexture(const std::string& path);
    TextureData decodeTextureFromMemory(const std::vector<uint8_t>& data);
    
    std::vector<std::thread> workers_;
    std::queue<TextureLoadRequest> pendingQueue_;
    std::vector<TextureLoadResult> completedResults_;
    
    mutable std::mutex pendingMutex_;
    mutable std::mutex completedMutex_;
    std::condition_variable workAvailable_;
    
    std::atomic<bool> running_{true};
    std::atomic<uint32_t> nextId_{1};
    std::atomic<size_t> pendingCount_{0};
};

// ===== Global Loader Instance =====
AsyncTextureLoader& getAsyncTextureLoader();

}  // namespace luma
