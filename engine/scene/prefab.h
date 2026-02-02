// Prefab System - Reusable Entity Templates
// Save entities as prefabs, instantiate with optional overrides
#pragma once

#include "engine/scene/entity.h"
#include "engine/scene/scene_graph.h"
#include "engine/serialization/json.h"
#include "engine/material/material.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <fstream>

namespace luma {

// ===== Prefab Data =====
// Serializable prefab representation
struct PrefabData {
    std::string name;
    std::string path;
    uint32_t version = 1;
    
    // Root entity data (hierarchical)
    struct EntityData {
        std::string name;
        bool enabled = true;
        
        // Transform
        Vec3 position{0, 0, 0};
        Vec3 rotation{0, 0, 0};  // Euler angles in degrees
        Vec3 scale{1, 1, 1};
        
        // Model
        bool hasModel = false;
        std::string modelPath;
        
        // Material
        bool hasMaterial = false;
        std::string materialName;
        Vec3 albedo{1, 1, 1};
        float metallic = 0.0f;
        float roughness = 0.5f;
        std::string albedoTexture;
        std::string normalTexture;
        
        // Light
        bool hasLight = false;
        int lightType = 0;  // 0=Point, 1=Directional, 2=Spot
        Vec3 lightColor{1, 1, 1};
        float lightIntensity = 1.0f;
        float lightRange = 10.0f;
        
        // Children
        std::vector<EntityData> children;
    };
    
    EntityData rootEntity;
};

// ===== Prefab Instance =====
// Links an entity to its source prefab
struct PrefabInstance {
    std::string prefabPath;
    EntityID rootEntityId = INVALID_ENTITY;
    
    // Property overrides (key = property path like "transform.position")
    std::unordered_map<std::string, std::string> overrides;
    
    bool hasOverride(const std::string& property) const {
        return overrides.find(property) != overrides.end();
    }
};

// ===== Prefab Manager =====
class PrefabManager {
public:
    static PrefabManager& get() {
        static PrefabManager instance;
        return instance;
    }
    
    // ===== Save Entity as Prefab =====
    bool savePrefab(const Entity* entity, const std::string& path) {
        if (!entity) return false;
        
        PrefabData prefab;
        prefab.name = entity->name;
        prefab.path = path;
        prefab.rootEntity = serializeEntity(entity);
        
        return saveToFile(path, prefab);
    }
    
    // ===== Load Prefab from File =====
    bool loadPrefab(const std::string& path, PrefabData& outPrefab) {
        return loadFromFile(path, outPrefab);
    }
    
    // ===== Instantiate Prefab into Scene =====
    Entity* instantiate(const std::string& prefabPath, SceneGraph& scene,
                       Entity* parent = nullptr, const Vec3& position = {0,0,0}) {
        PrefabData prefab;
        if (!loadPrefab(prefabPath, prefab)) {
            return nullptr;
        }
        
        // Create entity hierarchy
        Entity* root = instantiateEntity(prefab.rootEntity, scene, nullptr);
        if (!root) return nullptr;
        
        // Set parent if specified
        if (parent) {
            scene.setParent(root, parent);
        }
        
        // Apply position offset
        root->localTransform.position = root->localTransform.position + position;
        root->updateWorldMatrix();
        
        // Track as prefab instance
        PrefabInstance instance;
        instance.prefabPath = prefabPath;
        instance.rootEntityId = root->id;
        instances_[root->id] = instance;
        
        return root;
    }
    
    // ===== Check if Entity is Prefab Instance =====
    bool isPrefabInstance(EntityID id) const {
        return instances_.find(id) != instances_.end();
    }
    
    const PrefabInstance* getPrefabInstance(EntityID id) const {
        auto it = instances_.find(id);
        return it != instances_.end() ? &it->second : nullptr;
    }
    
    // ===== Apply Prefab (reset overrides) =====
    void applyPrefab(EntityID id, SceneGraph& scene) {
        auto it = instances_.find(id);
        if (it == instances_.end()) return;
        
        Entity* entity = scene.getEntity(id);
        if (!entity) return;
        
        PrefabData prefab;
        if (!loadPrefab(it->second.prefabPath, prefab)) return;
        
        // Re-apply prefab data (reset to original)
        applyEntityData(prefab.rootEntity, entity);
        it->second.overrides.clear();
    }
    
    // ===== Revert Override =====
    void revertOverride(EntityID id, const std::string& property, SceneGraph& scene) {
        auto it = instances_.find(id);
        if (it == instances_.end()) return;
        
        Entity* entity = scene.getEntity(id);
        if (!entity) return;
        
        PrefabData prefab;
        if (!loadPrefab(it->second.prefabPath, prefab)) return;
        
        // Revert specific property
        if (property == "position") {
            entity->localTransform.position = prefab.rootEntity.position;
        } else if (property == "rotation") {
            entity->localTransform.setEulerDegrees(prefab.rootEntity.rotation);
        } else if (property == "scale") {
            entity->localTransform.scale = prefab.rootEntity.scale;
        }
        
        it->second.overrides.erase(property);
        entity->updateWorldMatrix();
    }
    
    // ===== Record Override =====
    void recordOverride(EntityID id, const std::string& property, const std::string& value) {
        auto it = instances_.find(id);
        if (it == instances_.end()) return;
        it->second.overrides[property] = value;
    }
    
    // ===== Unpack Prefab Instance (break link) =====
    void unpackInstance(EntityID id) {
        instances_.erase(id);
    }
    
    // ===== Get All Prefab Paths =====
    std::vector<std::string> getLoadedPrefabs() const {
        std::vector<std::string> paths;
        for (const auto& [path, prefab] : loadedPrefabs_) {
            paths.push_back(path);
        }
        return paths;
    }
    
    // Set model loader callback
    using ModelLoaderFunc = std::function<bool(const std::string&, RHILoadedModel&)>;
    void setModelLoader(ModelLoaderFunc loader) {
        modelLoader_ = std::move(loader);
    }
    
private:
    PrefabManager() = default;
    
    std::unordered_map<EntityID, PrefabInstance> instances_;
    std::unordered_map<std::string, PrefabData> loadedPrefabs_;
    ModelLoaderFunc modelLoader_;
    
    // ===== Serialize Entity to PrefabData =====
    PrefabData::EntityData serializeEntity(const Entity* entity) {
        PrefabData::EntityData data;
        
        data.name = entity->name;
        data.enabled = entity->enabled;
        data.position = entity->localTransform.position;
        data.rotation = entity->localTransform.getEulerDegrees();
        data.scale = entity->localTransform.scale;
        
        // Model
        data.hasModel = entity->hasModel;
        if (entity->hasModel) {
            data.modelPath = entity->model.debugName.empty() ? entity->model.name : entity->model.debugName;
        }
        
        // Material
        if (entity->material) {
            data.hasMaterial = true;
            data.materialName = entity->material->name;
            data.albedo = entity->material->baseColor;
            data.metallic = entity->material->metallic;
            data.roughness = entity->material->roughness;
            data.albedoTexture = entity->material->texturePaths[static_cast<size_t>(TextureSlot::Albedo)];
            data.normalTexture = entity->material->texturePaths[static_cast<size_t>(TextureSlot::Normal)];
        }
        
        // Light
        data.hasLight = entity->hasLight;
        if (entity->hasLight) {
            data.lightType = static_cast<int>(entity->light.type);
            data.lightColor = entity->light.color;
            data.lightIntensity = entity->light.intensity;
            data.lightRange = entity->light.range;
        }
        
        // Children
        for (const Entity* child : entity->children) {
            data.children.push_back(serializeEntity(child));
        }
        
        return data;
    }
    
    // ===== Create Entity from PrefabData =====
    Entity* instantiateEntity(const PrefabData::EntityData& data, SceneGraph& scene, Entity* parent) {
        Entity* entity = scene.createEntity(data.name);
        entity->enabled = data.enabled;
        
        entity->localTransform.position = data.position;
        entity->localTransform.setEulerDegrees(data.rotation);
        entity->localTransform.scale = data.scale;
        
        // Model
        if (data.hasModel && modelLoader_ && !data.modelPath.empty()) {
            RHILoadedModel model;
            if (modelLoader_(data.modelPath, model)) {
                entity->hasModel = true;
                entity->model = model;
            }
        }
        
        // Material
        if (data.hasMaterial) {
            entity->material = std::make_shared<Material>();
            entity->material->name = data.materialName;
            entity->material->baseColor = data.albedo;
            entity->material->metallic = data.metallic;
            entity->material->roughness = data.roughness;
            entity->material->texturePaths[static_cast<size_t>(TextureSlot::Albedo)] = data.albedoTexture;
            entity->material->texturePaths[static_cast<size_t>(TextureSlot::Normal)] = data.normalTexture;
        }
        
        // Light
        if (data.hasLight) {
            entity->hasLight = true;
            entity->light.type = static_cast<LightType>(data.lightType);
            entity->light.color = data.lightColor;
            entity->light.intensity = data.lightIntensity;
            entity->light.range = data.lightRange;
        }
        
        // Set parent
        if (parent) {
            scene.setParent(entity, parent);
        }
        
        // Children
        for (const auto& childData : data.children) {
            instantiateEntity(childData, scene, entity);
        }
        
        entity->updateWorldMatrix();
        return entity;
    }
    
    // ===== Apply EntityData to Entity =====
    void applyEntityData(const PrefabData::EntityData& data, Entity* entity) {
        entity->name = data.name;
        entity->enabled = data.enabled;
        
        entity->localTransform.position = data.position;
        entity->localTransform.setEulerDegrees(data.rotation);
        entity->localTransform.scale = data.scale;
        
        entity->updateWorldMatrix();
    }
    
    // ===== File I/O =====
    bool saveToFile(const std::string& path, const PrefabData& prefab) {
        JsonValue root = JsonValue::object();
        root["name"] = prefab.name;
        root["version"] = static_cast<int>(prefab.version);
        root["entity"] = serializeEntityToJson(prefab.rootEntity);
        
        return saveJsonFile(path, root, true);
    }
    
    bool loadFromFile(const std::string& path, PrefabData& prefab) {
        try {
            JsonValue root = loadJsonFile(path);
            
            prefab.name = root.get<std::string>("name", "Prefab");
            prefab.version = static_cast<uint32_t>(root.get<int>("version", 1));
            prefab.path = path;
            
            if (root.has("entity")) {
                prefab.rootEntity = deserializeEntityFromJson(root["entity"]);
            }
            
            loadedPrefabs_[path] = prefab;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    JsonValue serializeEntityToJson(const PrefabData::EntityData& data) {
        JsonValue obj = JsonValue::object();
        
        obj["name"] = data.name;
        obj["enabled"] = data.enabled;
        
        // Transform
        JsonValue pos = JsonValue::array();
        pos.push(data.position.x); pos.push(data.position.y); pos.push(data.position.z);
        obj["position"] = pos;
        
        JsonValue rot = JsonValue::array();
        rot.push(data.rotation.x); rot.push(data.rotation.y); rot.push(data.rotation.z);
        obj["rotation"] = rot;
        
        JsonValue scl = JsonValue::array();
        scl.push(data.scale.x); scl.push(data.scale.y); scl.push(data.scale.z);
        obj["scale"] = scl;
        
        // Model
        if (data.hasModel) {
            obj["hasModel"] = true;
            obj["modelPath"] = data.modelPath;
        }
        
        // Material
        if (data.hasMaterial) {
            JsonValue mat = JsonValue::object();
            mat["name"] = data.materialName;
            JsonValue alb = JsonValue::array();
            alb.push(data.albedo.x); alb.push(data.albedo.y); alb.push(data.albedo.z);
            mat["albedo"] = alb;
            mat["metallic"] = data.metallic;
            mat["roughness"] = data.roughness;
            if (!data.albedoTexture.empty()) mat["albedoTexture"] = data.albedoTexture;
            if (!data.normalTexture.empty()) mat["normalTexture"] = data.normalTexture;
            obj["material"] = mat;
        }
        
        // Light
        if (data.hasLight) {
            JsonValue light = JsonValue::object();
            light["type"] = data.lightType;
            JsonValue col = JsonValue::array();
            col.push(data.lightColor.x); col.push(data.lightColor.y); col.push(data.lightColor.z);
            light["color"] = col;
            light["intensity"] = data.lightIntensity;
            light["range"] = data.lightRange;
            obj["light"] = light;
        }
        
        // Children
        if (!data.children.empty()) {
            JsonValue children = JsonValue::array();
            for (const auto& child : data.children) {
                children.push(serializeEntityToJson(child));
            }
            obj["children"] = children;
        }
        
        return obj;
    }
    
    PrefabData::EntityData deserializeEntityFromJson(const JsonValue& json) {
        PrefabData::EntityData data;
        
        data.name = json.get<std::string>("name", "Entity");
        data.enabled = json.get<bool>("enabled", true);
        
        // Transform
        if (json.has("position")) {
            const auto& pos = json["position"];
            data.position = {pos[0].asFloat(), pos[1].asFloat(), pos[2].asFloat()};
        }
        if (json.has("rotation")) {
            const auto& rot = json["rotation"];
            data.rotation = {rot[0].asFloat(), rot[1].asFloat(), rot[2].asFloat()};
        }
        if (json.has("scale")) {
            const auto& scl = json["scale"];
            data.scale = {scl[0].asFloat(), scl[1].asFloat(), scl[2].asFloat()};
        }
        
        // Model
        data.hasModel = json.get<bool>("hasModel", false);
        data.modelPath = json.get<std::string>("modelPath", "");
        
        // Material
        if (json.has("material")) {
            const auto& mat = json["material"];
            data.hasMaterial = true;
            data.materialName = mat.get<std::string>("name", "Material");
            if (mat.has("albedo")) {
                const auto& alb = mat["albedo"];
                data.albedo = {alb[0].asFloat(), alb[1].asFloat(), alb[2].asFloat()};
            }
            data.metallic = mat.get<float>("metallic", 0.0f);
            data.roughness = mat.get<float>("roughness", 0.5f);
            data.albedoTexture = mat.get<std::string>("albedoTexture", "");
            data.normalTexture = mat.get<std::string>("normalTexture", "");
        }
        
        // Light
        if (json.has("light")) {
            const auto& light = json["light"];
            data.hasLight = true;
            data.lightType = light.get<int>("type", 0);
            if (light.has("color")) {
                const auto& col = light["color"];
                data.lightColor = {col[0].asFloat(), col[1].asFloat(), col[2].asFloat()};
            }
            data.lightIntensity = light.get<float>("intensity", 1.0f);
            data.lightRange = light.get<float>("range", 10.0f);
        }
        
        // Children
        if (json.has("children")) {
            const auto& children = json["children"].asArray();
            for (const auto& child : children) {
                data.children.push_back(deserializeEntityFromJson(child));
            }
        }
        
        return data;
    }
};

// ===== Global Accessor =====
inline PrefabManager& getPrefabManager() {
    return PrefabManager::get();
}

}  // namespace luma
