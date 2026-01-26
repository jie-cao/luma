// Asset Manager - Resource caching with reference counting
// Handles loading, caching, and lifecycle management of assets
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>

namespace luma {

// ===== Asset Handle =====
// Type-safe handle with automatic reference counting

template<typename T>
class AssetHandle {
    friend class AssetManager;
    
    struct ControlBlock {
        std::shared_ptr<T> asset;
        std::atomic<uint32_t> refCount{0};
        std::string path;
        std::chrono::steady_clock::time_point lastAccess;
        bool persistent = false;  // If true, never unload
    };
    
    ControlBlock* block_ = nullptr;
    
    explicit AssetHandle(ControlBlock* block) : block_(block) {
        if (block_) {
            block_->refCount.fetch_add(1);
            block_->lastAccess = std::chrono::steady_clock::now();
        }
    }
    
public:
    AssetHandle() = default;
    
    AssetHandle(const AssetHandle& other) : block_(other.block_) {
        if (block_) {
            block_->refCount.fetch_add(1);
        }
    }
    
    AssetHandle(AssetHandle&& other) noexcept : block_(other.block_) {
        other.block_ = nullptr;
    }
    
    AssetHandle& operator=(const AssetHandle& other) {
        if (this != &other) {
            release();
            block_ = other.block_;
            if (block_) {
                block_->refCount.fetch_add(1);
            }
        }
        return *this;
    }
    
    AssetHandle& operator=(AssetHandle&& other) noexcept {
        if (this != &other) {
            release();
            block_ = other.block_;
            other.block_ = nullptr;
        }
        return *this;
    }
    
    ~AssetHandle() {
        release();
    }
    
    void release() {
        if (block_) {
            block_->refCount.fetch_sub(1);
            block_ = nullptr;
        }
    }
    
    // Access operators
    T* get() const { 
        if (block_) {
            block_->lastAccess = std::chrono::steady_clock::now();
            return block_->asset.get();
        }
        return nullptr;
    }
    
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    
    explicit operator bool() const { return block_ && block_->asset; }
    bool isValid() const { return block_ && block_->asset; }
    
    // Get reference count
    uint32_t refCount() const { return block_ ? block_->refCount.load() : 0; }
    
    // Get path
    const std::string& path() const {
        static std::string empty;
        return block_ ? block_->path : empty;
    }
};

// ===== Asset Types =====
enum class AssetType {
    Unknown,
    Model,
    Texture,
    Shader,
    Material,
    Audio,
    Scene
};

// Asset metadata
struct AssetMetadata {
    std::string path;
    AssetType type = AssetType::Unknown;
    size_t sizeBytes = 0;
    std::chrono::steady_clock::time_point loadTime;
    bool isLoaded = false;
};

// ===== Asset Manager =====
class AssetManager {
public:
    using TextureData = std::vector<uint8_t>;
    
    // Loader callbacks (user provides implementation)
    using ModelLoader = std::function<std::shared_ptr<void>(const std::string& path)>;
    using TextureLoader = std::function<std::shared_ptr<void>(const std::string& path)>;
    using ShaderLoader = std::function<std::shared_ptr<void>(const std::string& path)>;
    
private:
    // Internal storage for different asset types
    template<typename T>
    struct AssetStorage {
        std::unordered_map<std::string, typename AssetHandle<T>::ControlBlock*> assets;
        std::mutex mutex;
    };
    
    // Generic asset storage (type-erased)
    struct GenericAsset {
        std::shared_ptr<void> data;
        std::atomic<uint32_t> refCount{0};
        AssetType type = AssetType::Unknown;
        size_t sizeBytes = 0;
        std::chrono::steady_clock::time_point lastAccess;
        bool persistent = false;
    };
    
    std::unordered_map<std::string, std::unique_ptr<GenericAsset>> assets_;
    std::mutex mutex_;
    
    // Loaders
    ModelLoader modelLoader_;
    TextureLoader textureLoader_;
    ShaderLoader shaderLoader_;
    
    // Cache settings
    size_t maxCacheSizeBytes_ = 512 * 1024 * 1024;  // 512 MB default
    size_t currentCacheSizeBytes_ = 0;
    std::chrono::seconds unusedAssetTimeout_{300};  // 5 minutes
    
    // Statistics
    std::atomic<uint64_t> totalLoads_{0};
    std::atomic<uint64_t> cacheHits_{0};
    std::atomic<uint64_t> cacheMisses_{0};
    
public:
    AssetManager() = default;
    ~AssetManager() { clear(); }
    
    // Non-copyable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    
    // ===== Loader Registration =====
    
    void setModelLoader(ModelLoader loader) { modelLoader_ = std::move(loader); }
    void setTextureLoader(TextureLoader loader) { textureLoader_ = std::move(loader); }
    void setShaderLoader(ShaderLoader loader) { shaderLoader_ = std::move(loader); }
    
    // ===== Asset Loading =====
    
    // Load asset (returns cached if already loaded)
    template<typename T>
    std::shared_ptr<T> load(const std::string& path, AssetType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        totalLoads_++;
        
        // Check cache
        auto it = assets_.find(path);
        if (it != assets_.end() && it->second->data) {
            cacheHits_++;
            it->second->lastAccess = std::chrono::steady_clock::now();
            it->second->refCount++;
            return std::static_pointer_cast<T>(it->second->data);
        }
        
        cacheMisses_++;
        
        // Load asset
        std::shared_ptr<void> data;
        switch (type) {
            case AssetType::Model:
                if (modelLoader_) data = modelLoader_(path);
                break;
            case AssetType::Texture:
                if (textureLoader_) data = textureLoader_(path);
                break;
            case AssetType::Shader:
                if (shaderLoader_) data = shaderLoader_(path);
                break;
            default:
                break;
        }
        
        if (!data) return nullptr;
        
        // Store in cache
        auto asset = std::make_unique<GenericAsset>();
        asset->data = data;
        asset->type = type;
        asset->refCount = 1;
        asset->lastAccess = std::chrono::steady_clock::now();
        assets_[path] = std::move(asset);
        
        return std::static_pointer_cast<T>(data);
    }
    
    // Convenience loaders
    template<typename ModelT>
    std::shared_ptr<ModelT> loadModel(const std::string& path) {
        return load<ModelT>(path, AssetType::Model);
    }
    
    template<typename TextureT>
    std::shared_ptr<TextureT> loadTexture(const std::string& path) {
        return load<TextureT>(path, AssetType::Texture);
    }
    
    // ===== Manual Asset Registration =====
    
    // Register an externally loaded asset
    template<typename T>
    void registerAsset(const std::string& path, std::shared_ptr<T> asset, 
                       AssetType type, size_t sizeBytes = 0, bool persistent = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto entry = std::make_unique<GenericAsset>();
        entry->data = std::static_pointer_cast<void>(asset);
        entry->type = type;
        entry->sizeBytes = sizeBytes;
        entry->refCount = 1;
        entry->lastAccess = std::chrono::steady_clock::now();
        entry->persistent = persistent;
        
        currentCacheSizeBytes_ += sizeBytes;
        assets_[path] = std::move(entry);
    }
    
    // ===== Asset Query =====
    
    bool isLoaded(const std::string& path) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
        auto it = assets_.find(path);
        return it != assets_.end() && it->second->data;
    }
    
    template<typename T>
    std::shared_ptr<T> get(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = assets_.find(path);
        if (it != assets_.end() && it->second->data) {
            it->second->lastAccess = std::chrono::steady_clock::now();
            return std::static_pointer_cast<T>(it->second->data);
        }
        return nullptr;
    }
    
    // ===== Reference Counting =====
    
    void addRef(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = assets_.find(path);
        if (it != assets_.end()) {
            it->second->refCount++;
        }
    }
    
    void release(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = assets_.find(path);
        if (it != assets_.end()) {
            if (it->second->refCount > 0) {
                it->second->refCount--;
            }
        }
    }
    
    uint32_t getRefCount(const std::string& path) const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
        auto it = assets_.find(path);
        if (it != assets_.end()) {
            return it->second->refCount.load();
        }
        return 0;
    }
    
    // ===== Cache Management =====
    
    void setMaxCacheSize(size_t bytes) { maxCacheSizeBytes_ = bytes; }
    size_t getMaxCacheSize() const { return maxCacheSizeBytes_; }
    size_t getCurrentCacheSize() const { return currentCacheSizeBytes_; }
    
    void setUnusedTimeout(std::chrono::seconds timeout) { unusedAssetTimeout_ = timeout; }
    
    // Unload unused assets (refCount == 0 and expired)
    size_t collectGarbage() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> toRemove;
        
        for (auto& [path, asset] : assets_) {
            if (asset->persistent) continue;
            if (asset->refCount > 0) continue;
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - asset->lastAccess);
            if (elapsed >= unusedAssetTimeout_) {
                toRemove.push_back(path);
            }
        }
        
        for (const auto& path : toRemove) {
            auto it = assets_.find(path);
            if (it != assets_.end()) {
                currentCacheSizeBytes_ -= it->second->sizeBytes;
                assets_.erase(it);
            }
        }
        
        return toRemove.size();
    }
    
    // Force unload specific asset
    bool unload(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = assets_.find(path);
        if (it != assets_.end()) {
            if (it->second->refCount > 0) {
                return false;  // Still in use
            }
            currentCacheSizeBytes_ -= it->second->sizeBytes;
            assets_.erase(it);
            return true;
        }
        return false;
    }
    
    // Clear all assets (forced)
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        assets_.clear();
        currentCacheSizeBytes_ = 0;
    }
    
    // ===== Statistics =====
    
    struct Statistics {
        uint64_t totalLoads;
        uint64_t cacheHits;
        uint64_t cacheMisses;
        float hitRate;
        size_t cachedAssets;
        size_t cacheSizeBytes;
    };
    
    Statistics getStatistics() const {
        Statistics stats;
        stats.totalLoads = totalLoads_.load();
        stats.cacheHits = cacheHits_.load();
        stats.cacheMisses = cacheMisses_.load();
        stats.hitRate = stats.totalLoads > 0 
            ? static_cast<float>(stats.cacheHits) / stats.totalLoads 
            : 0.0f;
        
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
        stats.cachedAssets = assets_.size();
        stats.cacheSizeBytes = currentCacheSizeBytes_;
        
        return stats;
    }
    
    // ===== Asset Enumeration =====
    
    std::vector<AssetMetadata> getLoadedAssets() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
        
        std::vector<AssetMetadata> result;
        result.reserve(assets_.size());
        
        for (const auto& [path, asset] : assets_) {
            AssetMetadata meta;
            meta.path = path;
            meta.type = asset->type;
            meta.sizeBytes = asset->sizeBytes;
            meta.loadTime = asset->lastAccess;
            meta.isLoaded = asset->data != nullptr;
            result.push_back(meta);
        }
        
        return result;
    }
    
    // Mark asset as persistent (won't be garbage collected)
    void setPersistent(const std::string& path, bool persistent) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = assets_.find(path);
        if (it != assets_.end()) {
            it->second->persistent = persistent;
        }
    }
};

// ===== Global Asset Manager Instance =====
inline AssetManager& getAssetManager() {
    static AssetManager instance;
    return instance;
}

}  // namespace luma
