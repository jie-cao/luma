// ONNX Runtime Inference Wrapper
// Provides real ONNX model loading and inference when LUMA_HAS_ONNXRUNTIME is defined.
// Falls back to placeholder behavior otherwise.
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <cstring>
#include <cstdio>

#ifdef LUMA_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace luma {

// ============================================================================
// OnnxRuntime - Global environment singleton
// ============================================================================

class OnnxRuntime {
public:
    static OnnxRuntime& instance() {
        static OnnxRuntime inst;
        return inst;
    }

    bool isAvailable() const {
#ifdef LUMA_HAS_ONNXRUNTIME
        return initialized_;
#else
        return false;
#endif
    }

#ifdef LUMA_HAS_ONNXRUNTIME
    Ort::Env& env() { return *env_; }
#endif

private:
    OnnxRuntime() {
#ifdef LUMA_HAS_ONNXRUNTIME
        try {
            env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "luma");
            initialized_ = true;
            printf("[OnnxRuntime] Initialized successfully\n");
        } catch (const Ort::Exception& e) {
            printf("[OnnxRuntime] Failed to initialize: %s\n", e.what());
            initialized_ = false;
        }
#endif
    }

    bool initialized_ = false;
#ifdef LUMA_HAS_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env_;
#endif
};

// ============================================================================
// OnnxSession - Wraps a single ONNX model session
// ============================================================================

class OnnxSession {
public:
    struct TensorInfo {
        std::string name;
        std::vector<int64_t> shape;
    };

    OnnxSession() = default;
    ~OnnxSession() = default;

    bool loadModel(const std::string& modelPath, int numThreads = 4) {
        modelPath_ = modelPath;
#ifdef LUMA_HAS_ONNXRUNTIME
        if (!OnnxRuntime::instance().isAvailable()) {
            lastError_ = "ONNX Runtime not initialized";
            return false;
        }

        try {
            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(numThreads);
            options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            // Convert path to wide string for Windows
#ifdef _WIN32
            std::wstring wpath(modelPath.begin(), modelPath.end());
            session_ = std::make_unique<Ort::Session>(
                OnnxRuntime::instance().env(), wpath.c_str(), options);
#else
            session_ = std::make_unique<Ort::Session>(
                OnnxRuntime::instance().env(), modelPath.c_str(), options);
#endif
            // Extract input/output info
            Ort::AllocatorWithDefaultOptions allocator;

            size_t numInputs = session_->GetInputCount();
            for (size_t i = 0; i < numInputs; i++) {
                TensorInfo info;
                auto namePtr = session_->GetInputNameAllocated(i, allocator);
                info.name = namePtr.get();
                auto typeInfo = session_->GetInputTypeInfo(i);
                auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                info.shape = tensorInfo.GetShape();
                inputInfo_.push_back(info);
            }

            size_t numOutputs = session_->GetOutputCount();
            for (size_t i = 0; i < numOutputs; i++) {
                TensorInfo info;
                auto namePtr = session_->GetOutputNameAllocated(i, allocator);
                info.name = namePtr.get();
                auto typeInfo = session_->GetOutputTypeInfo(i);
                auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                info.shape = tensorInfo.GetShape();
                outputInfo_.push_back(info);
            }

            loaded_ = true;
            printf("[OnnxSession] Loaded model: %s (%zu inputs, %zu outputs)\n",
                   modelPath.c_str(), numInputs, numOutputs);
            return true;

        } catch (const Ort::Exception& e) {
            lastError_ = e.what();
            printf("[OnnxSession] Failed to load model %s: %s\n",
                   modelPath.c_str(), e.what());
            return false;
        }
#else
        lastError_ = "ONNX Runtime not available (compile with LUMA_HAS_ONNXRUNTIME)";
        return false;
#endif
    }

    // Run inference with float input/output
    bool run(const std::vector<float>& inputData,
             const std::vector<int64_t>& inputShape,
             std::vector<float>& outputData) {
#ifdef LUMA_HAS_ONNXRUNTIME
        if (!loaded_ || !session_) {
            lastError_ = "Model not loaded";
            return false;
        }

        try {
            auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memInfo, const_cast<float*>(inputData.data()),
                inputData.size(), inputShape.data(), inputShape.size());

            std::vector<const char*> inputNames;
            for (const auto& info : inputInfo_) inputNames.push_back(info.name.c_str());

            std::vector<const char*> outputNames;
            for (const auto& info : outputInfo_) outputNames.push_back(info.name.c_str());

            auto outputs = session_->Run(
                Ort::RunOptions{nullptr},
                inputNames.data(), &inputTensor, 1,
                outputNames.data(), outputNames.size());

            if (!outputs.empty() && outputs[0].IsTensor()) {
                auto tensorInfo = outputs[0].GetTensorTypeAndShapeInfo();
                size_t totalSize = tensorInfo.GetElementCount();
                const float* rawOutput = outputs[0].GetTensorData<float>();
                outputData.assign(rawOutput, rawOutput + totalSize);
                return true;
            }

            lastError_ = "No output tensor";
            return false;

        } catch (const Ort::Exception& e) {
            lastError_ = e.what();
            return false;
        }
#else
        (void)inputData; (void)inputShape; (void)outputData;
        lastError_ = "ONNX Runtime not available";
        return false;
#endif
    }

    // Run with multiple outputs
    bool runMultiOutput(const std::vector<float>& inputData,
                        const std::vector<int64_t>& inputShape,
                        std::vector<std::vector<float>>& outputs) {
#ifdef LUMA_HAS_ONNXRUNTIME
        if (!loaded_ || !session_) return false;

        try {
            auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memInfo, const_cast<float*>(inputData.data()),
                inputData.size(), inputShape.data(), inputShape.size());

            std::vector<const char*> inputNames;
            for (const auto& info : inputInfo_) inputNames.push_back(info.name.c_str());

            std::vector<const char*> outputNames;
            for (const auto& info : outputInfo_) outputNames.push_back(info.name.c_str());

            auto ortOutputs = session_->Run(
                Ort::RunOptions{nullptr},
                inputNames.data(), &inputTensor, 1,
                outputNames.data(), outputNames.size());

            outputs.resize(ortOutputs.size());
            for (size_t i = 0; i < ortOutputs.size(); i++) {
                if (ortOutputs[i].IsTensor()) {
                    auto info = ortOutputs[i].GetTensorTypeAndShapeInfo();
                    size_t count = info.GetElementCount();
                    const float* data = ortOutputs[i].GetTensorData<float>();
                    outputs[i].assign(data, data + count);
                }
            }
            return true;

        } catch (const Ort::Exception& e) {
            lastError_ = e.what();
            return false;
        }
#else
        (void)inputData; (void)inputShape; (void)outputs;
        return false;
#endif
    }

    bool isLoaded() const { return loaded_; }
    const std::string& lastError() const { return lastError_; }
    const std::vector<TensorInfo>& inputInfo() const { return inputInfo_; }
    const std::vector<TensorInfo>& outputInfo() const { return outputInfo_; }

private:
    bool loaded_ = false;
    std::string modelPath_;
    std::string lastError_;
    std::vector<TensorInfo> inputInfo_;
    std::vector<TensorInfo> outputInfo_;

#ifdef LUMA_HAS_ONNXRUNTIME
    std::unique_ptr<Ort::Session> session_;
#endif
};

// ============================================================================
// OnnxModelManager - Manages multiple model sessions
// ============================================================================

class OnnxModelManager {
public:
    static OnnxModelManager& instance() {
        static OnnxModelManager mgr;
        return mgr;
    }

    void setModelDirectory(const std::string& dir) { modelDir_ = dir; }
    const std::string& modelDirectory() const { return modelDir_; }

    // Load a model by ID
    bool loadModel(const std::string& modelId, const std::string& filename) {
        std::string fullPath = modelDir_.empty() ? filename : modelDir_ + "/" + filename;

        auto session = std::make_shared<OnnxSession>();
        if (!session->loadModel(fullPath)) {
            printf("[ModelManager] Failed to load '%s': %s\n",
                   modelId.c_str(), session->lastError().c_str());
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[modelId] = session;
        return true;
    }

    // Get a loaded session
    std::shared_ptr<OnnxSession> getSession(const std::string& modelId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(modelId);
        return (it != sessions_.end()) ? it->second : nullptr;
    }

    // Check if a model file exists
    static bool modelFileExists(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return true; }
        return false;
    }

    // Check which models are available in the model directory
    struct ModelStatus {
        bool faceDetector = false;
        bool landmarkDetector = false;
        bool tddfaRegressor = false;
    };

    ModelStatus checkAvailableModels() const {
        ModelStatus status;
        auto check = [&](const std::string& filename) {
            std::string path = modelDir_.empty() ? filename : modelDir_ + "/" + filename;
            return modelFileExists(path);
        };
        status.faceDetector = check("det_10g.onnx");
        status.landmarkDetector = check("2d106det.onnx");
        status.tddfaRegressor = check("mb1_120x120.onnx");
        return status;
    }

private:
    OnnxModelManager() = default;
    std::string modelDir_ = "models";
    std::unordered_map<std::string, std::shared_ptr<OnnxSession>> sessions_;
    std::mutex mutex_;
};

} // namespace luma
