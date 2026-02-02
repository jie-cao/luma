// Scene Layout System - Multi-object scene composition
// Place, arrange, and manage multiple objects in a scene
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace luma {

// ============================================================================
// Scene Object Types
// ============================================================================

enum class SceneObjectType {
    Character,      // 角色
    Prop,           // 道具
    Light,          // 灯光
    Camera,         // 相机
    Environment,    // 环境物体
    Ground,         // 地面
    Background,     // 背景
    Effect,         // 特效
    Group           // 分组
};

inline std::string sceneObjectTypeToString(SceneObjectType type) {
    switch (type) {
        case SceneObjectType::Character: return "Character";
        case SceneObjectType::Prop: return "Prop";
        case SceneObjectType::Light: return "Light";
        case SceneObjectType::Camera: return "Camera";
        case SceneObjectType::Environment: return "Environment";
        case SceneObjectType::Ground: return "Ground";
        case SceneObjectType::Background: return "Background";
        case SceneObjectType::Effect: return "Effect";
        case SceneObjectType::Group: return "Group";
        default: return "Unknown";
    }
}

// ============================================================================
// Scene Object
// ============================================================================

struct SceneObject {
    // Identity
    std::string id;
    std::string name;
    std::string nameCN;
    SceneObjectType type = SceneObjectType::Prop;
    
    // Transform
    Vec3 position{0, 0, 0};
    Quat rotation = Quat::identity();
    Vec3 scale{1, 1, 1};
    
    // Hierarchy
    std::string parentId;
    std::vector<std::string> childIds;
    
    // Visibility
    bool visible = true;
    bool locked = false;
    bool selected = false;
    
    // Layer
    int layer = 0;
    std::string layerName = "Default";
    
    // Custom data
    std::string assetPath;       // Path to model/texture
    std::string presetId;        // For presets
    std::unordered_map<std::string, std::string> metadata;
    
    // Rendering
    bool castShadow = true;
    bool receiveShadow = true;
    float opacity = 1.0f;
    
    // Physics
    bool isStatic = true;
    bool hasCollision = true;
    
    // Get world transform (considering parent)
    Mat4 getLocalMatrix() const {
        Mat4 t = Mat4::translation(position);
        Mat4 r = rotation.toMatrix();
        Mat4 s = Mat4::scale(scale);
        return t * r * s;
    }
};

// ============================================================================
// Scene Layer
// ============================================================================

struct SceneLayer {
    std::string name;
    std::string nameCN;
    int order = 0;
    bool visible = true;
    bool locked = false;
    Vec3 color{0.5f, 0.5f, 0.5f};  // Layer color for UI
};

// ============================================================================
// Scene Preset - Pre-built scene configurations
// ============================================================================

struct ScenePreset {
    std::string id;
    std::string name;
    std::string nameCN;
    std::string description;
    std::string category;        // "Studio", "Outdoor", "Fantasy", etc.
    std::string thumbnailPath;
    
    // Objects to create
    struct PresetObject {
        std::string name;
        SceneObjectType type;
        Vec3 position;
        Vec3 rotation;  // Euler angles
        Vec3 scale;
        std::string assetPath;
    };
    std::vector<PresetObject> objects;
    
    // Environment settings
    Vec3 ambientColor{0.3f, 0.3f, 0.35f};
    Vec3 backgroundColor{0.2f, 0.2f, 0.22f};
    bool useHDRI = false;
    std::string hdriPath;
};

// ============================================================================
// Scene Layout Manager
// ============================================================================

class SceneLayoutManager {
public:
    static SceneLayoutManager& getInstance() {
        static SceneLayoutManager instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // Create default layers
        addLayer({"Default", "默认", 0, true, false, {0.5f, 0.5f, 0.5f}});
        addLayer({"Characters", "角色", 1, true, false, {0.3f, 0.6f, 0.9f}});
        addLayer({"Props", "道具", 2, true, false, {0.9f, 0.6f, 0.3f}});
        addLayer({"Environment", "环境", 3, true, false, {0.3f, 0.9f, 0.4f}});
        addLayer({"Lights", "灯光", 4, true, false, {0.9f, 0.9f, 0.3f}});
        addLayer({"Effects", "特效", 5, true, false, {0.9f, 0.3f, 0.9f}});
        
        // Register built-in presets
        registerBuiltInPresets();
        
        initialized_ = true;
    }
    
    // === Object Management ===
    
    std::string addObject(const SceneObject& obj) {
        SceneObject newObj = obj;
        if (newObj.id.empty()) {
            newObj.id = generateUniqueId();
        }
        objects_[newObj.id] = newObj;
        
        // Update parent's children list
        if (!newObj.parentId.empty()) {
            if (objects_.find(newObj.parentId) != objects_.end()) {
                objects_[newObj.parentId].childIds.push_back(newObj.id);
            }
        }
        
        notifyChange();
        return newObj.id;
    }
    
    void removeObject(const std::string& id) {
        auto it = objects_.find(id);
        if (it == objects_.end()) return;
        
        // Remove from parent's children
        if (!it->second.parentId.empty()) {
            auto& parent = objects_[it->second.parentId];
            parent.childIds.erase(
                std::remove(parent.childIds.begin(), parent.childIds.end(), id),
                parent.childIds.end());
        }
        
        // Remove children (recursively)
        for (const auto& childId : it->second.childIds) {
            removeObject(childId);
        }
        
        objects_.erase(it);
        notifyChange();
    }
    
    SceneObject* getObject(const std::string& id) {
        auto it = objects_.find(id);
        return (it != objects_.end()) ? &it->second : nullptr;
    }
    
    const SceneObject* getObject(const std::string& id) const {
        auto it = objects_.find(id);
        return (it != objects_.end()) ? &it->second : nullptr;
    }
    
    std::vector<SceneObject*> getAllObjects() {
        std::vector<SceneObject*> result;
        for (auto& [id, obj] : objects_) {
            result.push_back(&obj);
        }
        return result;
    }
    
    std::vector<SceneObject*> getObjectsByType(SceneObjectType type) {
        std::vector<SceneObject*> result;
        for (auto& [id, obj] : objects_) {
            if (obj.type == type) {
                result.push_back(&obj);
            }
        }
        return result;
    }
    
    std::vector<SceneObject*> getObjectsByLayer(const std::string& layerName) {
        std::vector<SceneObject*> result;
        for (auto& [id, obj] : objects_) {
            if (obj.layerName == layerName) {
                result.push_back(&obj);
            }
        }
        return result;
    }
    
    std::vector<SceneObject*> getRootObjects() {
        std::vector<SceneObject*> result;
        for (auto& [id, obj] : objects_) {
            if (obj.parentId.empty()) {
                result.push_back(&obj);
            }
        }
        return result;
    }
    
    // === Transform Operations ===
    
    void setPosition(const std::string& id, const Vec3& position) {
        if (auto* obj = getObject(id)) {
            obj->position = position;
            notifyChange();
        }
    }
    
    void setRotation(const std::string& id, const Quat& rotation) {
        if (auto* obj = getObject(id)) {
            obj->rotation = rotation;
            notifyChange();
        }
    }
    
    void setScale(const std::string& id, const Vec3& scale) {
        if (auto* obj = getObject(id)) {
            obj->scale = scale;
            notifyChange();
        }
    }
    
    void translate(const std::string& id, const Vec3& delta) {
        if (auto* obj = getObject(id)) {
            obj->position = obj->position + delta;
            notifyChange();
        }
    }
    
    void rotate(const std::string& id, const Quat& delta) {
        if (auto* obj = getObject(id)) {
            obj->rotation = (delta * obj->rotation).normalized();
            notifyChange();
        }
    }
    
    Mat4 getWorldMatrix(const std::string& id) const {
        const SceneObject* obj = getObject(id);
        if (!obj) return Mat4::identity();
        
        Mat4 local = obj->getLocalMatrix();
        
        if (!obj->parentId.empty()) {
            Mat4 parent = getWorldMatrix(obj->parentId);
            return parent * local;
        }
        
        return local;
    }
    
    // === Hierarchy Operations ===
    
    void setParent(const std::string& childId, const std::string& parentId) {
        SceneObject* child = getObject(childId);
        if (!child) return;
        
        // Remove from old parent
        if (!child->parentId.empty()) {
            if (SceneObject* oldParent = getObject(child->parentId)) {
                auto& children = oldParent->childIds;
                children.erase(std::remove(children.begin(), children.end(), childId), children.end());
            }
        }
        
        // Add to new parent
        child->parentId = parentId;
        if (!parentId.empty()) {
            if (SceneObject* newParent = getObject(parentId)) {
                newParent->childIds.push_back(childId);
            }
        }
        
        notifyChange();
    }
    
    void unparent(const std::string& id) {
        setParent(id, "");
    }
    
    // === Selection ===
    
    void select(const std::string& id, bool additive = false) {
        if (!additive) {
            clearSelection();
        }
        
        if (SceneObject* obj = getObject(id)) {
            obj->selected = true;
            selectedIds_.push_back(id);
        }
    }
    
    void deselect(const std::string& id) {
        if (SceneObject* obj = getObject(id)) {
            obj->selected = false;
            selectedIds_.erase(
                std::remove(selectedIds_.begin(), selectedIds_.end(), id),
                selectedIds_.end());
        }
    }
    
    void clearSelection() {
        for (const auto& id : selectedIds_) {
            if (SceneObject* obj = getObject(id)) {
                obj->selected = false;
            }
        }
        selectedIds_.clear();
    }
    
    const std::vector<std::string>& getSelectedIds() const {
        return selectedIds_;
    }
    
    std::vector<SceneObject*> getSelectedObjects() {
        std::vector<SceneObject*> result;
        for (const auto& id : selectedIds_) {
            if (SceneObject* obj = getObject(id)) {
                result.push_back(obj);
            }
        }
        return result;
    }
    
    // === Layer Management ===
    
    void addLayer(const SceneLayer& layer) {
        layers_[layer.name] = layer;
    }
    
    void removeLayer(const std::string& name) {
        if (name == "Default") return;  // Can't remove default layer
        
        // Move objects to default layer
        for (auto& [id, obj] : objects_) {
            if (obj.layerName == name) {
                obj.layerName = "Default";
            }
        }
        
        layers_.erase(name);
    }
    
    SceneLayer* getLayer(const std::string& name) {
        auto it = layers_.find(name);
        return (it != layers_.end()) ? &it->second : nullptr;
    }
    
    std::vector<SceneLayer*> getAllLayers() {
        std::vector<SceneLayer*> result;
        for (auto& [name, layer] : layers_) {
            result.push_back(&layer);
        }
        // Sort by order
        std::sort(result.begin(), result.end(), [](const SceneLayer* a, const SceneLayer* b) {
            return a->order < b->order;
        });
        return result;
    }
    
    void setLayerVisibility(const std::string& name, bool visible) {
        if (SceneLayer* layer = getLayer(name)) {
            layer->visible = visible;
            notifyChange();
        }
    }
    
    // === Presets ===
    
    void applyPreset(const std::string& presetId) {
        auto it = presets_.find(presetId);
        if (it == presets_.end()) return;
        
        const ScenePreset& preset = it->second;
        
        for (const auto& po : preset.objects) {
            SceneObject obj;
            obj.name = po.name;
            obj.type = po.type;
            obj.position = po.position;
            obj.rotation = Quat::fromEuler(po.rotation.x, po.rotation.y, po.rotation.z);
            obj.scale = po.scale;
            obj.assetPath = po.assetPath;
            
            // Set layer based on type
            switch (obj.type) {
                case SceneObjectType::Character:
                    obj.layerName = "Characters";
                    break;
                case SceneObjectType::Prop:
                    obj.layerName = "Props";
                    break;
                case SceneObjectType::Light:
                    obj.layerName = "Lights";
                    break;
                case SceneObjectType::Environment:
                case SceneObjectType::Ground:
                case SceneObjectType::Background:
                    obj.layerName = "Environment";
                    break;
                case SceneObjectType::Effect:
                    obj.layerName = "Effects";
                    break;
                default:
                    obj.layerName = "Default";
                    break;
            }
            
            addObject(obj);
        }
        
        // Apply environment settings
        ambientColor_ = preset.ambientColor;
        backgroundColor_ = preset.backgroundColor;
        
        if (onPresetApplied_) onPresetApplied_(presetId);
    }
    
    const std::unordered_map<std::string, ScenePreset>& getPresets() const {
        return presets_;
    }
    
    std::vector<const ScenePreset*> getPresetsByCategory(const std::string& category) const {
        std::vector<const ScenePreset*> result;
        for (const auto& [id, preset] : presets_) {
            if (preset.category == category) {
                result.push_back(&preset);
            }
        }
        return result;
    }
    
    // === Scene Operations ===
    
    void clear() {
        objects_.clear();
        selectedIds_.clear();
        notifyChange();
    }
    
    void duplicateSelected() {
        std::vector<std::string> newIds;
        
        for (const auto& id : selectedIds_) {
            if (const SceneObject* obj = getObject(id)) {
                SceneObject copy = *obj;
                copy.id = "";  // Generate new ID
                copy.name = copy.name + " Copy";
                copy.selected = false;
                copy.position = copy.position + Vec3(0.5f, 0, 0.5f);  // Offset
                
                newIds.push_back(addObject(copy));
            }
        }
        
        // Select duplicates
        clearSelection();
        for (const auto& id : newIds) {
            select(id, true);
        }
    }
    
    void groupSelected(const std::string& groupName = "Group") {
        if (selectedIds_.size() < 2) return;
        
        // Create group object
        SceneObject group;
        group.name = groupName;
        group.type = SceneObjectType::Group;
        
        // Calculate center of selection
        Vec3 center(0, 0, 0);
        for (const auto& id : selectedIds_) {
            if (const SceneObject* obj = getObject(id)) {
                center = center + obj->position;
            }
        }
        center = center * (1.0f / selectedIds_.size());
        group.position = center;
        
        std::string groupId = addObject(group);
        
        // Parent selected objects to group
        for (const auto& id : selectedIds_) {
            setParent(id, groupId);
            // Adjust local position
            if (SceneObject* obj = getObject(id)) {
                obj->position = obj->position - center;
            }
        }
        
        // Select group
        clearSelection();
        select(groupId);
    }
    
    void ungroupSelected() {
        std::vector<std::string> toRemove;
        
        for (const auto& id : selectedIds_) {
            SceneObject* obj = getObject(id);
            if (!obj || obj->type != SceneObjectType::Group) continue;
            
            // Unparent children
            for (const auto& childId : obj->childIds) {
                if (SceneObject* child = getObject(childId)) {
                    // Convert to world position
                    Mat4 worldMat = getWorldMatrix(childId);
                    child->position = Vec3(worldMat.m[12], worldMat.m[13], worldMat.m[14]);
                }
                unparent(childId);
            }
            
            toRemove.push_back(id);
        }
        
        // Remove group objects
        for (const auto& id : toRemove) {
            removeObject(id);
        }
    }
    
    // === Alignment ===
    
    void alignSelectedX() { alignSelected(0); }
    void alignSelectedY() { alignSelected(1); }
    void alignSelectedZ() { alignSelected(2); }
    
    void distributeSelectedX() { distributeSelected(0); }
    void distributeSelectedY() { distributeSelected(1); }
    void distributeSelectedZ() { distributeSelected(2); }
    
    // === Environment ===
    
    Vec3& getAmbientColor() { return ambientColor_; }
    Vec3& getBackgroundColor() { return backgroundColor_; }
    
    // === Callbacks ===
    
    void setOnChange(std::function<void()> callback) {
        onChange_ = callback;
    }
    
    void setOnPresetApplied(std::function<void(const std::string&)> callback) {
        onPresetApplied_ = callback;
    }
    
private:
    SceneLayoutManager() = default;
    
    std::string generateUniqueId() {
        return "obj_" + std::to_string(nextId_++);
    }
    
    void notifyChange() {
        if (onChange_) onChange_();
    }
    
    void alignSelected(int axis) {
        if (selectedIds_.size() < 2) return;
        
        // Use first selected as reference
        const SceneObject* ref = getObject(selectedIds_[0]);
        if (!ref) return;
        
        float alignValue = (axis == 0) ? ref->position.x : 
                          (axis == 1) ? ref->position.y : ref->position.z;
        
        for (size_t i = 1; i < selectedIds_.size(); i++) {
            if (SceneObject* obj = getObject(selectedIds_[i])) {
                if (axis == 0) obj->position.x = alignValue;
                else if (axis == 1) obj->position.y = alignValue;
                else obj->position.z = alignValue;
            }
        }
        
        notifyChange();
    }
    
    void distributeSelected(int axis) {
        if (selectedIds_.size() < 3) return;
        
        // Sort by position on axis
        std::vector<std::pair<float, std::string>> sorted;
        for (const auto& id : selectedIds_) {
            if (const SceneObject* obj = getObject(id)) {
                float val = (axis == 0) ? obj->position.x :
                           (axis == 1) ? obj->position.y : obj->position.z;
                sorted.push_back({val, id});
            }
        }
        std::sort(sorted.begin(), sorted.end());
        
        // Distribute evenly
        float start = sorted.front().first;
        float end = sorted.back().first;
        float step = (end - start) / (sorted.size() - 1);
        
        for (size_t i = 1; i < sorted.size() - 1; i++) {
            if (SceneObject* obj = getObject(sorted[i].second)) {
                float newVal = start + step * i;
                if (axis == 0) obj->position.x = newVal;
                else if (axis == 1) obj->position.y = newVal;
                else obj->position.z = newVal;
            }
        }
        
        notifyChange();
    }
    
    void registerBuiltInPresets() {
        // Studio presets
        {
            ScenePreset p;
            p.id = "studio_simple";
            p.name = "Simple Studio";
            p.nameCN = "简约工作室";
            p.category = "Studio";
            p.description = "Clean studio setup with three-point lighting";
            p.backgroundColor = Vec3(0.15f, 0.15f, 0.18f);
            
            p.objects = {
                {"Key Light", SceneObjectType::Light, {3, 4, 2}, {-30, 45, 0}, {1, 1, 1}, ""},
                {"Fill Light", SceneObjectType::Light, {-3, 3, 2}, {-20, -45, 0}, {1, 1, 1}, ""},
                {"Rim Light", SceneObjectType::Light, {0, 3, -3}, {-30, 180, 0}, {1, 1, 1}, ""},
                {"Ground Plane", SceneObjectType::Ground, {0, 0, 0}, {0, 0, 0}, {10, 1, 10}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        {
            ScenePreset p;
            p.id = "studio_photo";
            p.name = "Photo Studio";
            p.nameCN = "摄影棚";
            p.category = "Studio";
            p.description = "Professional photo studio with cyclorama";
            p.backgroundColor = Vec3(0.9f, 0.9f, 0.92f);
            
            p.objects = {
                {"Main Light", SceneObjectType::Light, {2, 5, 3}, {-45, 30, 0}, {1, 1, 1}, ""},
                {"Soft Fill", SceneObjectType::Light, {-2, 3, 2}, {-30, -30, 0}, {1, 1, 1}, ""},
                {"Background Light", SceneObjectType::Light, {0, 2, -4}, {0, 180, 0}, {1, 1, 1}, ""},
                {"Cyclorama", SceneObjectType::Background, {0, 0, -3}, {0, 0, 0}, {8, 6, 1}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        // Outdoor presets
        {
            ScenePreset p;
            p.id = "outdoor_park";
            p.name = "Park";
            p.nameCN = "公园";
            p.category = "Outdoor";
            p.description = "Outdoor park setting with natural lighting";
            p.backgroundColor = Vec3(0.5f, 0.7f, 0.9f);
            p.ambientColor = Vec3(0.4f, 0.45f, 0.5f);
            
            p.objects = {
                {"Sun", SceneObjectType::Light, {10, 15, 5}, {-50, 30, 0}, {1, 1, 1}, ""},
                {"Grass Ground", SceneObjectType::Ground, {0, 0, 0}, {0, 0, 0}, {20, 1, 20}, ""},
                {"Tree 1", SceneObjectType::Environment, {5, 0, 3}, {0, 0, 0}, {1, 1, 1}, ""},
                {"Tree 2", SceneObjectType::Environment, {-4, 0, -2}, {0, 45, 0}, {1.2f, 1.2f, 1.2f}, ""},
                {"Bench", SceneObjectType::Prop, {2, 0, 0}, {0, -15, 0}, {1, 1, 1}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        {
            ScenePreset p;
            p.id = "outdoor_street";
            p.name = "City Street";
            p.nameCN = "城市街道";
            p.category = "Outdoor";
            p.description = "Urban street scene";
            p.backgroundColor = Vec3(0.6f, 0.65f, 0.7f);
            
            p.objects = {
                {"Sun", SceneObjectType::Light, {8, 12, 4}, {-45, 60, 0}, {1, 1, 1}, ""},
                {"Street Ground", SceneObjectType::Ground, {0, 0, 0}, {0, 0, 0}, {30, 1, 10}, ""},
                {"Building 1", SceneObjectType::Environment, {-8, 0, -5}, {0, 0, 0}, {5, 15, 5}, ""},
                {"Building 2", SceneObjectType::Environment, {8, 0, -5}, {0, 0, 0}, {6, 12, 5}, ""},
                {"Street Lamp", SceneObjectType::Prop, {3, 0, 1}, {0, 0, 0}, {1, 1, 1}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        // Fantasy presets
        {
            ScenePreset p;
            p.id = "fantasy_castle";
            p.name = "Castle Hall";
            p.nameCN = "城堡大厅";
            p.category = "Fantasy";
            p.description = "Medieval castle interior";
            p.backgroundColor = Vec3(0.2f, 0.18f, 0.15f);
            p.ambientColor = Vec3(0.15f, 0.12f, 0.1f);
            
            p.objects = {
                {"Chandelier", SceneObjectType::Light, {0, 6, 0}, {0, 0, 0}, {2, 2, 2}, ""},
                {"Torch 1", SceneObjectType::Light, {-4, 2.5f, -3}, {0, 0, 0}, {0.5f, 0.5f, 0.5f}, ""},
                {"Torch 2", SceneObjectType::Light, {4, 2.5f, -3}, {0, 0, 0}, {0.5f, 0.5f, 0.5f}, ""},
                {"Stone Floor", SceneObjectType::Ground, {0, 0, 0}, {0, 0, 0}, {15, 1, 15}, ""},
                {"Throne", SceneObjectType::Prop, {0, 0, -5}, {0, 0, 0}, {1.5f, 1.5f, 1.5f}, ""},
                {"Banner Left", SceneObjectType::Prop, {-5, 3, -4}, {0, 0, 0}, {1, 3, 0.1f}, ""},
                {"Banner Right", SceneObjectType::Prop, {5, 3, -4}, {0, 0, 0}, {1, 3, 0.1f}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        // Sci-Fi presets
        {
            ScenePreset p;
            p.id = "scifi_spaceship";
            p.name = "Spaceship Interior";
            p.nameCN = "飞船内部";
            p.category = "Sci-Fi";
            p.description = "Futuristic spaceship bridge";
            p.backgroundColor = Vec3(0.05f, 0.08f, 0.12f);
            p.ambientColor = Vec3(0.1f, 0.15f, 0.2f);
            
            p.objects = {
                {"Ceiling Light", SceneObjectType::Light, {0, 4, 0}, {-90, 0, 0}, {1, 1, 1}, ""},
                {"Console Glow", SceneObjectType::Light, {0, 1, 2}, {0, 0, 0}, {0.5f, 0.5f, 0.5f}, ""},
                {"Metal Floor", SceneObjectType::Ground, {0, 0, 0}, {0, 0, 0}, {10, 1, 10}, ""},
                {"Control Panel", SceneObjectType::Prop, {0, 0.8f, 2}, {0, 0, 0}, {3, 1, 0.5f}, ""},
                {"Captain Chair", SceneObjectType::Prop, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, ""},
                {"Side Console L", SceneObjectType::Prop, {-3, 0.6f, 1}, {0, 30, 0}, {1.5f, 0.8f, 0.4f}, ""},
                {"Side Console R", SceneObjectType::Prop, {3, 0.6f, 1}, {0, -30, 0}, {1.5f, 0.8f, 0.4f}, ""}
            };
            
            presets_[p.id] = p;
        }
        
        // Empty preset
        {
            ScenePreset p;
            p.id = "empty";
            p.name = "Empty Scene";
            p.nameCN = "空场景";
            p.category = "Basic";
            p.description = "Start with an empty scene";
            p.backgroundColor = Vec3(0.2f, 0.2f, 0.22f);
            p.ambientColor = Vec3(0.3f, 0.3f, 0.35f);
            
            p.objects = {
                {"Default Light", SceneObjectType::Light, {3, 5, 3}, {-45, 45, 0}, {1, 1, 1}, ""}
            };
            
            presets_[p.id] = p;
        }
    }
    
    std::unordered_map<std::string, SceneObject> objects_;
    std::unordered_map<std::string, SceneLayer> layers_;
    std::unordered_map<std::string, ScenePreset> presets_;
    
    std::vector<std::string> selectedIds_;
    
    Vec3 ambientColor_{0.3f, 0.3f, 0.35f};
    Vec3 backgroundColor_{0.2f, 0.2f, 0.22f};
    
    int nextId_ = 1;
    bool initialized_ = false;
    
    std::function<void()> onChange_;
    std::function<void(const std::string&)> onPresetApplied_;
};

// ============================================================================
// Convenience
// ============================================================================

inline SceneLayoutManager& getSceneLayout() {
    return SceneLayoutManager::getInstance();
}

}  // namespace luma
