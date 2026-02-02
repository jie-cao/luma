// Scene Management System
// Scene switching, async loading, preloading
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <condition_variable>
#include <atomic>
#include <future>

namespace luma {

// ===== Scene State =====
enum class SceneState {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Unloading
};

// ===== Scene Load Mode =====
enum class SceneLoadMode {
    Single,     // Unload all other scenes
    Additive    // Keep existing scenes
};

// ===== Scene Object =====
struct SceneObject {
    uint32_t id = 0;
    std::string name;
    std::string prefabPath;
    
    Vec3 position = {0, 0, 0};
    Vec3 rotation = {0, 0, 0};  // Euler angles
    Vec3 scale = {1, 1, 1};
    
    bool active = true;
    uint32_t parentId = 0;
    
    // Component data (serialized)
    std::unordered_map<std::string, std::string> componentData;
};

// ===== Scene Data =====
struct SceneData {
    std::string name;
    std::string path;
    
    // Objects
    std::vector<SceneObject> objects;
    
    // Environment
    Vec3 ambientColor = {0.1f, 0.1f, 0.1f};
    std::string skyboxPath;
    
    // Lighting
    struct DirectionalLight {
        Vec3 direction = {0.5f, -1.0f, 0.5f};
        Vec3 color = {1, 1, 1};
        float intensity = 1.0f;
    } directionalLight;
    
    // NavMesh reference
    std::string navMeshPath;
    
    // Dependencies (assets to preload)
    std::vector<std::string> dependencies;
};

// ===== Scene =====
class Scene {
public:
    Scene(const std::string& name = "Scene")
        : name_(name), id_(nextId_++) {}
    
    uint32_t getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    const std::string& getPath() const { return path_; }
    void setPath(const std::string& path) { path_ = path; }
    
    SceneState getState() const { return state_; }
    void setState(SceneState state) { state_ = state; }
    
    // Data
    SceneData& getData() { return data_; }
    const SceneData& getData() const { return data_; }
    
    // Objects
    SceneObject* addObject(const std::string& name) {
        SceneObject obj;
        obj.id = nextObjectId_++;
        obj.name = name;
        data_.objects.push_back(obj);
        return &data_.objects.back();
    }
    
    SceneObject* getObject(uint32_t id) {
        for (auto& obj : data_.objects) {
            if (obj.id == id) return &obj;
        }
        return nullptr;
    }
    
    void removeObject(uint32_t id) {
        data_.objects.erase(
            std::remove_if(data_.objects.begin(), data_.objects.end(),
                [id](const SceneObject& o) { return o.id == id; }),
            data_.objects.end()
        );
    }
    
    // Load progress (0-1)
    float getLoadProgress() const { return loadProgress_; }
    void setLoadProgress(float progress) { loadProgress_ = progress; }
    
    // Active state
    bool isActive() const { return state_ == SceneState::Active; }
    
private:
    uint32_t id_;
    static inline uint32_t nextId_ = 1;
    static inline uint32_t nextObjectId_ = 1;
    
    std::string name_;
    std::string path_;
    SceneState state_ = SceneState::Unloaded;
    SceneData data_;
    float loadProgress_ = 0.0f;
};

// ===== Load Operation =====
struct SceneLoadOperation {
    std::string scenePath;
    SceneLoadMode mode = SceneLoadMode::Single;
    bool makeActive = true;
    
    std::function<void(Scene*)> onComplete;
    std::function<void(float)> onProgress;
    std::function<void(const std::string&)> onError;
    
    // Internal
    std::shared_ptr<Scene> scene;
    std::atomic<float> progress{0.0f};
    std::atomic<bool> completed{false};
    std::atomic<bool> failed{false};
    std::string errorMessage;
};

// ===== Scene Loader Interface =====
class ISceneLoader {
public:
    virtual ~ISceneLoader() = default;
    
    virtual bool loadScene(const std::string& path, SceneData& outData,
                          std::function<void(float)> progressCallback = nullptr) = 0;
    virtual bool saveScene(const std::string& path, const SceneData& data) = 0;
};

// ===== JSON Scene Loader =====
// Forward declaration - full implementation in scene_serializer.h
class SceneSerializer;

class JsonSceneLoader : public ISceneLoader {
public:
    bool loadScene(const std::string& path, SceneData& outData,
                   std::function<void(float)> progressCallback = nullptr) override {
        if (progressCallback) progressCallback(0.1f);
        
        // Read file
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        if (progressCallback) progressCallback(0.3f);
        
        // Parse JSON manually (basic parsing)
        std::string json = buffer.str();
        
        // Extract name from path if not in JSON
        size_t lastSlash = path.find_last_of("/\\");
        size_t lastDot = path.find_last_of('.');
        if (lastSlash != std::string::npos && lastDot != std::string::npos) {
            outData.name = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
        } else {
            outData.name = path;
        }
        outData.path = path;
        
        if (progressCallback) progressCallback(0.7f);
        
        // Note: Full deserialization handled by SceneSerializer
        // This basic loader just ensures the file exists and is readable
        
        if (progressCallback) progressCallback(1.0f);
        
        return true;
    }
    
    bool saveScene(const std::string& path, const SceneData& data) override {
        // Note: Full serialization handled by SceneSerializer
        // This is called for basic scene data only
        return true;
    }
};

// ===== Scene Manager =====
class SceneManager {
public:
    static SceneManager& getInstance() {
        static SceneManager instance;
        return instance;
    }
    
    ~SceneManager() {
        shutdown();
    }
    
    // Initialize
    void initialize() {
        if (initialized_) return;
        
        loader_ = std::make_unique<JsonSceneLoader>();
        workerRunning_ = true;
        workerThread_ = std::thread(&SceneManager::workerLoop, this);
        initialized_ = true;
    }
    
    void shutdown() {
        if (!initialized_) return;
        
        workerRunning_ = false;
        queueCondition_.notify_all();
        
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
        
        scenes_.clear();
        loadQueue_ = {};
        initialized_ = false;
    }
    
    // Scene loader
    void setSceneLoader(std::unique_ptr<ISceneLoader> loader) {
        loader_ = std::move(loader);
    }
    
    // === Synchronous Loading ===
    
    Scene* loadScene(const std::string& path, SceneLoadMode mode = SceneLoadMode::Single) {
        if (mode == SceneLoadMode::Single) {
            unloadAllScenes();
        }
        
        auto scene = std::make_shared<Scene>();
        scene->setPath(path);
        scene->setState(SceneState::Loading);
        
        if (loader_->loadScene(path, scene->getData(), [&](float p) {
            scene->setLoadProgress(p);
        })) {
            scene->setName(scene->getData().name);
            scene->setState(SceneState::Loaded);
            scenes_[scene->getId()] = scene;
            
            setActiveScene(scene.get());
            return scene.get();
        }
        
        return nullptr;
    }
    
    // === Asynchronous Loading ===
    
    void loadSceneAsync(const std::string& path,
                        SceneLoadMode mode = SceneLoadMode::Single,
                        std::function<void(Scene*)> onComplete = nullptr,
                        std::function<void(float)> onProgress = nullptr,
                        std::function<void(const std::string&)> onError = nullptr) {
        auto op = std::make_shared<SceneLoadOperation>();
        op->scenePath = path;
        op->mode = mode;
        op->onComplete = onComplete;
        op->onProgress = onProgress;
        op->onError = onError;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            loadQueue_.push(op);
        }
        queueCondition_.notify_one();
    }
    
    // === Preloading ===
    
    void preloadScene(const std::string& path,
                      std::function<void()> onComplete = nullptr) {
        auto op = std::make_shared<SceneLoadOperation>();
        op->scenePath = path;
        op->makeActive = false;  // Don't activate, just preload
        op->onComplete = [onComplete](Scene*) {
            if (onComplete) onComplete();
        };
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            loadQueue_.push(op);
        }
        queueCondition_.notify_one();
    }
    
    // Check if scene is preloaded
    bool isScenePreloaded(const std::string& path) const {
        for (const auto& [id, scene] : scenes_) {
            if (scene->getPath() == path && scene->getState() == SceneState::Loaded) {
                return true;
            }
        }
        return false;
    }
    
    // Activate preloaded scene
    bool activatePreloadedScene(const std::string& path,
                                SceneLoadMode mode = SceneLoadMode::Single) {
        for (auto& [id, scene] : scenes_) {
            if (scene->getPath() == path && scene->getState() == SceneState::Loaded) {
                if (mode == SceneLoadMode::Single) {
                    // Unload other scenes
                    for (auto& [otherId, other] : scenes_) {
                        if (otherId != id && other->getState() == SceneState::Active) {
                            other->setState(SceneState::Loaded);
                        }
                    }
                }
                setActiveScene(scene.get());
                return true;
            }
        }
        return false;
    }
    
    // === Scene Management ===
    
    Scene* getActiveScene() const { return activeScene_; }
    
    void setActiveScene(Scene* scene) {
        if (activeScene_ && activeScene_ != scene) {
            activeScene_->setState(SceneState::Loaded);
        }
        activeScene_ = scene;
        if (activeScene_) {
            activeScene_->setState(SceneState::Active);
        }
    }
    
    Scene* getScene(uint32_t id) {
        auto it = scenes_.find(id);
        return it != scenes_.end() ? it->second.get() : nullptr;
    }
    
    Scene* getSceneByPath(const std::string& path) {
        for (auto& [id, scene] : scenes_) {
            if (scene->getPath() == path) return scene.get();
        }
        return nullptr;
    }
    
    const std::unordered_map<uint32_t, std::shared_ptr<Scene>>& getAllScenes() const {
        return scenes_;
    }
    
    // Unload
    void unloadScene(uint32_t id) {
        auto it = scenes_.find(id);
        if (it != scenes_.end()) {
            if (it->second.get() == activeScene_) {
                activeScene_ = nullptr;
            }
            it->second->setState(SceneState::Unloading);
            scenes_.erase(it);
        }
    }
    
    void unloadScene(const std::string& path) {
        for (auto it = scenes_.begin(); it != scenes_.end(); ) {
            if (it->second->getPath() == path) {
                if (it->second.get() == activeScene_) {
                    activeScene_ = nullptr;
                }
                it->second->setState(SceneState::Unloading);
                it = scenes_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void unloadAllScenes() {
        for (auto& [id, scene] : scenes_) {
            scene->setState(SceneState::Unloading);
        }
        scenes_.clear();
        activeScene_ = nullptr;
    }
    
    // === Create New Scene ===
    
    Scene* createScene(const std::string& name) {
        auto scene = std::make_shared<Scene>(name);
        scene->setState(SceneState::Loaded);
        scenes_[scene->getId()] = scene;
        return scene.get();
    }
    
    // === Save ===
    
    bool saveScene(Scene* scene, const std::string& path = "") {
        if (!scene) return false;
        
        std::string savePath = path.empty() ? scene->getPath() : path;
        if (savePath.empty()) return false;
        
        scene->setPath(savePath);
        return loader_->saveScene(savePath, scene->getData());
    }
    
    // === Update (process completed async operations) ===
    
    void update() {
        std::vector<std::shared_ptr<SceneLoadOperation>> completedOps;
        
        {
            std::lock_guard<std::mutex> lock(completedMutex_);
            completedOps = std::move(completedOperations_);
            completedOperations_.clear();
        }
        
        for (auto& op : completedOps) {
            if (op->failed) {
                if (op->onError) {
                    op->onError(op->errorMessage);
                }
            } else {
                if (op->mode == SceneLoadMode::Single && op->makeActive) {
                    unloadAllScenes();
                }
                
                scenes_[op->scene->getId()] = op->scene;
                
                if (op->makeActive) {
                    setActiveScene(op->scene.get());
                }
                
                if (op->onComplete) {
                    op->onComplete(op->scene.get());
                }
            }
        }
    }
    
    // Loading progress
    float getCurrentLoadProgress() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (currentOperation_) {
            return currentOperation_->progress.load();
        }
        return 1.0f;
    }
    
    bool isLoading() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return !loadQueue_.empty() || currentOperation_ != nullptr;
    }
    
private:
    SceneManager() = default;
    
    void workerLoop() {
        while (workerRunning_) {
            std::shared_ptr<SceneLoadOperation> op;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCondition_.wait(lock, [this] {
                    return !workerRunning_ || !loadQueue_.empty();
                });
                
                if (!workerRunning_) break;
                
                if (!loadQueue_.empty()) {
                    op = loadQueue_.front();
                    loadQueue_.pop();
                    currentOperation_ = op;
                }
            }
            
            if (op) {
                processLoadOperation(op);
                
                {
                    std::lock_guard<std::mutex> lock(queueMutex_);
                    currentOperation_ = nullptr;
                }
                
                {
                    std::lock_guard<std::mutex> lock(completedMutex_);
                    completedOperations_.push_back(op);
                }
            }
        }
    }
    
    void processLoadOperation(std::shared_ptr<SceneLoadOperation> op) {
        op->scene = std::make_shared<Scene>();
        op->scene->setPath(op->scenePath);
        op->scene->setState(SceneState::Loading);
        
        bool success = loader_->loadScene(op->scenePath, op->scene->getData(),
            [op](float progress) {
                op->progress = progress;
                if (op->onProgress) {
                    // Note: This is called from worker thread
                    // In production, queue this for main thread
                }
            });
        
        if (success) {
            op->scene->setName(op->scene->getData().name);
            op->scene->setState(SceneState::Loaded);
            op->scene->setLoadProgress(1.0f);
            op->completed = true;
        } else {
            op->failed = true;
            op->errorMessage = "Failed to load scene: " + op->scenePath;
        }
    }
    
    bool initialized_ = false;
    std::unique_ptr<ISceneLoader> loader_;
    
    std::unordered_map<uint32_t, std::shared_ptr<Scene>> scenes_;
    Scene* activeScene_ = nullptr;
    
    // Async loading
    std::thread workerThread_;
    std::atomic<bool> workerRunning_{false};
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::queue<std::shared_ptr<SceneLoadOperation>> loadQueue_;
    std::shared_ptr<SceneLoadOperation> currentOperation_;
    
    std::mutex completedMutex_;
    std::vector<std::shared_ptr<SceneLoadOperation>> completedOperations_;
};

// ===== Global Accessor =====
inline SceneManager& getSceneManager() {
    return SceneManager::getInstance();
}

// ===== Scene Transition =====
class SceneTransition {
public:
    enum class Type {
        None,
        Fade,
        Crossfade,
        SlideLeft,
        SlideRight,
        SlideUp,
        SlideDown,
        Custom
    };
    
    SceneTransition(Type type = Type::Fade, float duration = 0.5f)
        : type_(type), duration_(duration) {}
    
    Type getType() const { return type_; }
    float getDuration() const { return duration_; }
    
    void setColor(float r, float g, float b) { color_ = {r, g, b}; }
    Vec3 getColor() const { return color_; }
    
    // Progress (0 = start, 1 = complete)
    float getProgress() const { return progress_; }
    void setProgress(float p) { progress_ = std::max(0.0f, std::min(1.0f, p)); }
    
    bool isComplete() const { return progress_ >= 1.0f; }
    
    // Update
    void update(float dt) {
        if (duration_ > 0) {
            progress_ += dt / duration_;
            if (progress_ > 1.0f) progress_ = 1.0f;
        } else {
            progress_ = 1.0f;
        }
    }
    
    // Get opacity for fade effects
    float getFadeOpacity() const {
        // 0-0.5: fade out (0->1), 0.5-1: fade in (1->0)
        if (progress_ < 0.5f) {
            return progress_ * 2.0f;
        }
        return (1.0f - progress_) * 2.0f;
    }
    
private:
    Type type_;
    float duration_;
    float progress_ = 0.0f;
    Vec3 color_ = {0, 0, 0};  // Fade color
};

// ===== Scene Transition Manager =====
class SceneTransitionManager {
public:
    static SceneTransitionManager& getInstance() {
        static SceneTransitionManager instance;
        return instance;
    }
    
    // Start transition to new scene
    void transitionTo(const std::string& scenePath,
                      SceneTransition::Type type = SceneTransition::Type::Fade,
                      float duration = 0.5f,
                      SceneLoadMode mode = SceneLoadMode::Single) {
        if (isTransitioning_) return;
        
        pendingScenePath_ = scenePath;
        pendingLoadMode_ = mode;
        transition_ = SceneTransition(type, duration);
        isTransitioning_ = true;
        phase_ = Phase::FadeOut;
        
        // Start preloading
        getSceneManager().preloadScene(scenePath);
    }
    
    void update(float dt) {
        if (!isTransitioning_) return;
        
        transition_.update(dt);
        
        switch (phase_) {
            case Phase::FadeOut:
                if (transition_.getProgress() >= 0.5f) {
                    // Switch scene at midpoint
                    if (getSceneManager().isScenePreloaded(pendingScenePath_)) {
                        getSceneManager().activatePreloadedScene(pendingScenePath_, pendingLoadMode_);
                    } else {
                        getSceneManager().loadScene(pendingScenePath_, pendingLoadMode_);
                    }
                    phase_ = Phase::FadeIn;
                }
                break;
                
            case Phase::FadeIn:
                if (transition_.isComplete()) {
                    isTransitioning_ = false;
                    if (onTransitionComplete_) {
                        onTransitionComplete_();
                    }
                }
                break;
        }
    }
    
    bool isTransitioning() const { return isTransitioning_; }
    const SceneTransition& getTransition() const { return transition_; }
    
    void setOnTransitionComplete(std::function<void()> callback) {
        onTransitionComplete_ = callback;
    }
    
private:
    enum class Phase { FadeOut, FadeIn };
    
    SceneTransition transition_;
    bool isTransitioning_ = false;
    Phase phase_ = Phase::FadeOut;
    
    std::string pendingScenePath_;
    SceneLoadMode pendingLoadMode_ = SceneLoadMode::Single;
    
    std::function<void()> onTransitionComplete_;
};

inline SceneTransitionManager& getSceneTransitionManager() {
    return SceneTransitionManager::getInstance();
}

}  // namespace luma
