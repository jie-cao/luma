#include "scene_manager.h"
#include "engine/asset/model_loader.h"
#include <algorithm>
#include <cmath>

namespace luma {

SceneManager::SceneManager() : nextAssetId_(1) {
}

bool SceneManager::loadModel(const std::string& path, PBRRenderer& renderer, AssetID& outAssetId) {
    if (path.empty()) {
        return false;  // Empty path not allowed
    }
    
    // Check if already loaded (by path)
    for (const auto& [id, model] : models_) {
        if (model.name == path) {
            outAssetId = id;
            return true;
        }
    }
    
    // Load new model
    LoadedModel model;
    if (!renderer.loadModel(path, model)) {
        return false;
    }
    
    AssetID assetId = generateAssetId(path);
    models_[assetId] = std::move(model);
    outAssetId = assetId;
    return true;
}

bool SceneManager::hasModel(const AssetID& assetId) const {
    return models_.find(assetId) != models_.end();
}

const LoadedModel* SceneManager::getModel(const AssetID& assetId) const {
    auto it = models_.find(assetId);
    return (it != models_.end()) ? &it->second : nullptr;
}

LoadedModel* SceneManager::getModel(const AssetID& assetId) {
    auto it = models_.find(assetId);
    return (it != models_.end()) ? &it->second : nullptr;
}

void SceneManager::unloadModel(const AssetID& assetId) {
    models_.erase(assetId);
}

uint32_t SceneManager::createNodeFromModel(const AssetID& modelAssetId, const std::string& name) {
    const LoadedModel* model = getModel(modelAssetId);
    if (!model) return 0;
    
    SceneNode node;
    node.name = name.empty() ? model->name : name;
    node.renderable = modelAssetId;
    
    // Position at model's center
    node.transform.position = Vec3(model->center[0], model->center[1], model->center[2]);
    
    return scene_.addNode(node);
}

uint32_t SceneManager::createNodeFromModel(const std::string& modelPath, PBRRenderer& renderer, const std::string& name) {
    AssetID assetId;
    if (!loadModel(modelPath, renderer, assetId)) {
        return 0;
    }
    
    return createNodeFromModel(assetId, name);
}

void SceneManager::computeSceneBounds(float* outCenter, float& outRadius) const {
    if (scene_.getAllNodes().empty()) {
        outCenter[0] = outCenter[1] = outCenter[2] = 0.0f;
        outRadius = 1.0f;
        return;
    }
    
    float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
    float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
    bool hasBounds = false;
    
    for (const auto& [nodeId, node] : scene_.getAllNodes()) {
        if (node.renderable.empty()) continue;
        
        const LoadedModel* model = getModel(node.renderable);
        if (!model) continue;
        
        // Transform model bounds by node's world matrix
        float modelMin[3] = {
            model->center[0] - model->radius,
            model->center[1] - model->radius,
            model->center[2] - model->radius
        };
        float modelMax[3] = {
            model->center[0] + model->radius,
            model->center[1] + model->radius,
            model->center[2] + model->radius
        };
        
        // Transform corners by world matrix
        for (int i = 0; i < 8; i++) {
            float x = (i & 1) ? modelMax[0] : modelMin[0];
            float y = (i & 2) ? modelMax[1] : modelMin[1];
            float z = (i & 4) ? modelMax[2] : modelMin[2];
            
            // Transform by world matrix
            float wx = node.worldMatrix[0]*x + node.worldMatrix[4]*y + node.worldMatrix[8]*z + node.worldMatrix[12];
            float wy = node.worldMatrix[1]*x + node.worldMatrix[5]*y + node.worldMatrix[9]*z + node.worldMatrix[13];
            float wz = node.worldMatrix[2]*x + node.worldMatrix[6]*y + node.worldMatrix[10]*z + node.worldMatrix[14];
            
            if (!hasBounds) {
                minX = maxX = wx;
                minY = maxY = wy;
                minZ = maxZ = wz;
                hasBounds = true;
            } else {
                minX = std::min(minX, wx);
                minY = std::min(minY, wy);
                minZ = std::min(minZ, wz);
                maxX = std::max(maxX, wx);
                maxY = std::max(maxY, wy);
                maxZ = std::max(maxZ, wz);
            }
        }
    }
    
    if (!hasBounds) {
        outCenter[0] = outCenter[1] = outCenter[2] = 0.0f;
        outRadius = 1.0f;
        return;
    }
    
    outCenter[0] = (minX + maxX) * 0.5f;
    outCenter[1] = (minY + maxY) * 0.5f;
    outCenter[2] = (minZ + maxZ) * 0.5f;
    
    float dx = maxX - minX;
    float dy = maxY - minY;
    float dz = maxZ - minZ;
    outRadius = std::sqrt(dx*dx + dy*dy + dz*dz) * 0.5f;
    outRadius = std::max(outRadius, 0.1f);  // Minimum radius
}

void SceneManager::clear() {
    scene_ = Scene();
    models_.clear();
    nextAssetId_ = 1;
}

AssetID SceneManager::generateAssetId(const std::string& path) {
    // Simple asset ID generation (could use hash in the future)
    return "asset_" + std::to_string(nextAssetId_++);
}

}  // namespace luma
