// Async Texture Loader Implementation
#include "async_texture_loader.h"

// Use stb_image functions from model_loader.cpp
extern "C" {
    extern unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    extern unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
    extern void stbi_image_free(void *retval_from_stbi_load);
    extern void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
}

#include <iostream>
#include <fstream>
#include <algorithm>

namespace luma {

// ===== AsyncTextureLoader Implementation =====

AsyncTextureLoader::AsyncTextureLoader(size_t numThreads) {
    // Start worker threads
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&AsyncTextureLoader::workerThread, this);
    }
    std::cout << "[async_loader] Started with " << numThreads << " worker threads" << std::endl;
}

AsyncTextureLoader::~AsyncTextureLoader() {
    shutdown();
}

void AsyncTextureLoader::shutdown() {
    if (!running_) return;
    
    running_ = false;
    workAvailable_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    std::cout << "[async_loader] Shutdown complete" << std::endl;
}

uint32_t AsyncTextureLoader::loadTexture(const std::string& path) {
    TextureLoadRequest request;
    request.id = nextId_++;
    request.path = path;
    request.isEmbedded = false;
    
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingQueue_.push(std::move(request));
        pendingCount_++;
    }
    workAvailable_.notify_one();
    
    return request.id;
}

uint32_t AsyncTextureLoader::loadTextureFromMemory(const std::vector<uint8_t>& data, const std::string& name) {
    TextureLoadRequest request;
    request.id = nextId_++;
    request.path = name;
    request.embeddedData = data;
    request.isEmbedded = true;
    
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingQueue_.push(std::move(request));
        pendingCount_++;
    }
    workAvailable_.notify_one();
    
    return request.id;
}

bool AsyncTextureLoader::hasCompletedTextures() const {
    std::lock_guard<std::mutex> lock(completedMutex_);
    return !completedResults_.empty();
}

std::vector<TextureLoadResult> AsyncTextureLoader::getCompletedTextures() {
    std::lock_guard<std::mutex> lock(completedMutex_);
    std::vector<TextureLoadResult> results = std::move(completedResults_);
    completedResults_.clear();
    return results;
}

size_t AsyncTextureLoader::getPendingCount() const {
    return pendingCount_.load();
}

void AsyncTextureLoader::waitForAll() {
    while (pendingCount_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AsyncTextureLoader::workerThread() {
    while (running_) {
        TextureLoadRequest request;
        
        {
            std::unique_lock<std::mutex> lock(pendingMutex_);
            workAvailable_.wait(lock, [this] {
                return !pendingQueue_.empty() || !running_;
            });
            
            if (!running_ && pendingQueue_.empty()) break;
            if (pendingQueue_.empty()) continue;
            
            request = std::move(pendingQueue_.front());
            pendingQueue_.pop();
        }
        
        // Decode texture (this is the slow part we want off the main thread)
        TextureLoadResult result;
        result.id = request.id;
        
        try {
            if (request.isEmbedded) {
                result.data = decodeTextureFromMemory(request.embeddedData);
            } else {
                result.data = decodeTexture(request.path);
            }
            
            if (!result.data.pixels.empty()) {
                result.success = true;
                result.data.path = request.path;
            } else {
                result.success = false;
                result.error = "Failed to decode texture: " + request.path;
            }
        } catch (const std::exception& e) {
            result.success = false;
            result.error = std::string("Exception: ") + e.what();
        }
        
        // Add to completed queue
        {
            std::lock_guard<std::mutex> lock(completedMutex_);
            completedResults_.push_back(std::move(result));
        }
        
        pendingCount_--;
    }
}

TextureData AsyncTextureLoader::decodeTexture(const std::string& path) {
    TextureData tex;
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (data) {
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.assign(data, data + width * height * 4);
        stbi_image_free(data);
        
        std::cout << "[async_loader] Decoded: " << path 
                  << " (" << width << "x" << height << ")" << std::endl;
    }
    
    return tex;
}

TextureData AsyncTextureLoader::decodeTextureFromMemory(const std::vector<uint8_t>& data) {
    TextureData tex;
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    
    unsigned char* decoded = stbi_load_from_memory(
        data.data(), static_cast<int>(data.size()),
        &width, &height, &channels, 4);
    
    if (decoded) {
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.assign(decoded, decoded + width * height * 4);
        stbi_image_free(decoded);
    }
    
    return tex;
}

// ===== Global Loader Instance =====
static std::unique_ptr<AsyncTextureLoader> g_asyncLoader;
static std::once_flag g_asyncLoaderInit;

AsyncTextureLoader& getAsyncTextureLoader() {
    std::call_once(g_asyncLoaderInit, [] {
        g_asyncLoader = std::make_unique<AsyncTextureLoader>(2);
    });
    return *g_asyncLoader;
}

}  // namespace luma
