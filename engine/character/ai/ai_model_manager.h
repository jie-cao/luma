// AI Model Manager - Downloads, validates, and manages AI models for character creation
// Part of LUMA Character Creation System
#pragma once

#include "ai_inference.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace luma {

// ============================================================================
// Model Info
// ============================================================================

struct AIModelInfo {
    std::string name;
    std::string description;
    std::string filename;
    std::string downloadUrl;  // Empty if bundled or user-provided
    size_t expectedSize = 0;  // Expected file size in bytes (0 = unknown)
    std::string sha256;       // SHA256 hash for validation (empty = skip)
    bool required = true;     // If false, pipeline can work without it
    
    enum class Status {
        NotFound,
        Downloading,
        Validating,
        Ready,
        Error
    };
    Status status = Status::NotFound;
    std::string errorMessage;
    float downloadProgress = 0.0f;
};

// ============================================================================
// Model Registry - Known models for character creation
// ============================================================================

struct ModelRegistry {
    // Face Detection Models
    static AIModelInfo getFaceDetectorModel() {
        AIModelInfo info;
        info.name = "Face Detector";
        info.description = "MediaPipe-compatible face detection model";
        info.filename = "face_detection_short_range.onnx";
        info.downloadUrl = ""; // User must provide
        info.expectedSize = 0;
        info.required = true;
        return info;
    }
    
    // Face Mesh Models
    static AIModelInfo getFaceMeshModel() {
        AIModelInfo info;
        info.name = "Face Mesh";
        info.description = "MediaPipe Face Mesh - 468 3D landmarks";
        info.filename = "face_landmark.onnx";
        info.downloadUrl = "";
        info.expectedSize = 0;
        info.required = true;
        return info;
    }
    
    // 3DMM Regression Models
    static AIModelInfo get3DMMModel() {
        AIModelInfo info;
        info.name = "3DMM Regressor";
        info.description = "DECA/EMOCA compatible 3D Morphable Model";
        info.filename = "deca_model.onnx";
        info.downloadUrl = "";
        info.expectedSize = 0;
        info.required = false;  // Can use landmarks-only fallback
        return info;
    }
    
    // Face Recognition (for identity preservation)
    static AIModelInfo getFaceRecognitionModel() {
        AIModelInfo info;
        info.name = "Face Recognition";
        info.description = "Face embedding model for identity preservation";
        info.filename = "arcface_model.onnx";
        info.downloadUrl = "";
        info.expectedSize = 0;
        info.required = false;
        return info;
    }
    
    // Get all registered models
    static std::vector<AIModelInfo> getAllModels() {
        return {
            getFaceDetectorModel(),
            getFaceMeshModel(),
            get3DMMModel(),
            getFaceRecognitionModel()
        };
    }
};

// ============================================================================
// AI Model Manager
// ============================================================================

class CharacterAIModelManager {
public:
    // Singleton access
    static CharacterAIModelManager& getInstance() {
        static CharacterAIModelManager instance;
        return instance;
    }
    
    // === Configuration ===
    
    // Set base directory for model storage
    void setModelDirectory(const std::string& path) {
        modelDirectory_ = path;
        
        // Create directory if it doesn't exist
        std::filesystem::create_directories(path);
    }
    
    const std::string& getModelDirectory() const {
        return modelDirectory_;
    }
    
    // === Model Management ===
    
    // Register a model
    void registerModel(const std::string& modelId, const AIModelInfo& info) {
        models_[modelId] = info;
        updateModelStatus(modelId);
    }
    
    // Register all default models
    void registerDefaultModels() {
        registerModel("face_detector", ModelRegistry::getFaceDetectorModel());
        registerModel("face_mesh", ModelRegistry::getFaceMeshModel());
        registerModel("3dmm", ModelRegistry::get3DMMModel());
        registerModel("face_recognition", ModelRegistry::getFaceRecognitionModel());
    }
    
    // Get model info
    const AIModelInfo* getModelInfo(const std::string& modelId) const {
        auto it = models_.find(modelId);
        return it != models_.end() ? &it->second : nullptr;
    }
    
    // Get all models
    const std::unordered_map<std::string, AIModelInfo>& getAllModels() const {
        return models_;
    }
    
    // === Status Checking ===
    
    // Check if model file exists
    bool modelExists(const std::string& modelId) const {
        auto it = models_.find(modelId);
        if (it == models_.end()) return false;
        
        std::string path = getModelPath(modelId);
        return std::filesystem::exists(path);
    }
    
    // Get full path to model file
    std::string getModelPath(const std::string& modelId) const {
        auto it = models_.find(modelId);
        if (it == models_.end()) return "";
        
        return modelDirectory_ + "/" + it->second.filename;
    }
    
    // Update status of all models
    void updateAllModelStatus() {
        for (auto& [id, info] : models_) {
            updateModelStatus(id);
        }
    }
    
    // Check if all required models are ready
    bool allRequiredModelsReady() const {
        for (const auto& [id, info] : models_) {
            if (info.required && info.status != AIModelInfo::Status::Ready) {
                return false;
            }
        }
        return true;
    }
    
    // Get list of missing required models
    std::vector<std::string> getMissingRequiredModels() const {
        std::vector<std::string> missing;
        for (const auto& [id, info] : models_) {
            if (info.required && info.status != AIModelInfo::Status::Ready) {
                missing.push_back(id);
            }
        }
        return missing;
    }
    
    // === Model Loading ===
    
    // Load model into inference session
    bool loadModel(const std::string& modelId, InferenceSession& session) {
        if (!modelExists(modelId)) {
            return false;
        }
        
        std::string path = getModelPath(modelId);
        return session.loadModel(path);
    }
    
    // === User Model Import ===
    
    // Import a model from user-specified path
    bool importModel(const std::string& modelId, const std::string& sourcePath) {
        auto it = models_.find(modelId);
        if (it == models_.end()) return false;
        
        std::string destPath = getModelPath(modelId);
        
        try {
            // Copy file
            std::filesystem::copy_file(
                sourcePath, 
                destPath,
                std::filesystem::copy_options::overwrite_existing
            );
            
            updateModelStatus(modelId);
            return models_[modelId].status == AIModelInfo::Status::Ready;
        } catch (const std::exception& e) {
            models_[modelId].errorMessage = e.what();
            models_[modelId].status = AIModelInfo::Status::Error;
            return false;
        }
    }
    
    // === Callbacks ===
    
    // Set callback for status changes
    void setStatusCallback(std::function<void(const std::string&, AIModelInfo::Status)> callback) {
        statusCallback_ = callback;
    }
    
private:
    CharacterAIModelManager() {
        // Default model directory
        modelDirectory_ = "models/ai";
    }
    
    void updateModelStatus(const std::string& modelId) {
        auto it = models_.find(modelId);
        if (it == models_.end()) return;
        
        std::string path = getModelPath(modelId);
        
        if (!std::filesystem::exists(path)) {
            it->second.status = AIModelInfo::Status::NotFound;
        } else {
            // Basic validation: check file size if specified
            if (it->second.expectedSize > 0) {
                auto fileSize = std::filesystem::file_size(path);
                if (fileSize != it->second.expectedSize) {
                    it->second.status = AIModelInfo::Status::Error;
                    it->second.errorMessage = "File size mismatch";
                    return;
                }
            }
            
            // TODO: SHA256 validation if specified
            
            it->second.status = AIModelInfo::Status::Ready;
        }
        
        if (statusCallback_) {
            statusCallback_(modelId, it->second.status);
        }
    }
    
    std::string modelDirectory_;
    std::unordered_map<std::string, AIModelInfo> models_;
    std::function<void(const std::string&, AIModelInfo::Status)> statusCallback_;
};

// ============================================================================
// AI Model Setup UI Helper
// ============================================================================

struct AIModelSetupState {
    bool showSetupWindow = false;
    std::string selectedModelId;
    char importPath[512] = "";
    std::string lastError;
    
    // Render status icon
    static const char* getStatusIcon(AIModelInfo::Status status) {
        switch (status) {
            case AIModelInfo::Status::NotFound: return "[X]";
            case AIModelInfo::Status::Downloading: return "[...]";
            case AIModelInfo::Status::Validating: return "[?]";
            case AIModelInfo::Status::Ready: return "[OK]";
            case AIModelInfo::Status::Error: return "[!]";
        }
        return "[ ]";
    }
    
    // Get status color
    static void getStatusColor(AIModelInfo::Status status, float& r, float& g, float& b) {
        switch (status) {
            case AIModelInfo::Status::NotFound:
                r = 1.0f; g = 0.5f; b = 0.0f; break;
            case AIModelInfo::Status::Downloading:
                r = 0.5f; g = 0.5f; b = 1.0f; break;
            case AIModelInfo::Status::Validating:
                r = 1.0f; g = 1.0f; b = 0.5f; break;
            case AIModelInfo::Status::Ready:
                r = 0.2f; g = 1.0f; b = 0.2f; break;
            case AIModelInfo::Status::Error:
                r = 1.0f; g = 0.2f; b = 0.2f; break;
        }
    }
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline CharacterAIModelManager& getCharacterAIModelManager() {
    return CharacterAIModelManager::getInstance();
}

} // namespace luma
