// SceneGraph - Manages entities and their hierarchy
#pragma once

#include "entity.h"
#include <unordered_map>
#include <functional>
#include <memory>

namespace luma {

class SceneGraph {
public:
    SceneGraph();
    ~SceneGraph();
    
    // === Entity Management ===
    
    // Create a new entity
    Entity* createEntity(const std::string& name = "Entity");
    
    // Create entity with a model
    Entity* createEntityWithModel(const std::string& name, const RHILoadedModel& model);
    
    // Destroy an entity (and all its children)
    void destroyEntity(EntityID id);
    void destroyEntity(Entity* entity);
    
    // Find entity by ID
    Entity* getEntity(EntityID id);
    const Entity* getEntity(EntityID id) const;
    
    // Find entity by name (returns first match)
    Entity* findEntityByName(const std::string& name);
    
    // Get all root entities (no parent)
    const std::vector<Entity*>& getRootEntities() const { return rootEntities_; }
    
    // Get all entities
    const std::unordered_map<EntityID, std::unique_ptr<Entity>>& getAllEntities() const { 
        return entities_; 
    }
    
    // === Hierarchy ===
    
    // Set parent (nullptr for root)
    void setParent(Entity* child, Entity* parent);
    
    // === Selection ===
    
    // Single selection (legacy compatibility)
    Entity* getSelectedEntity() { 
        return selectedEntities_.empty() ? nullptr : selectedEntities_[0]; 
    }
    const Entity* getSelectedEntity() const { 
        return selectedEntities_.empty() ? nullptr : selectedEntities_[0]; 
    }
    void setSelectedEntity(Entity* entity) { 
        selectedEntities_.clear();
        if (entity) selectedEntities_.push_back(entity);
    }
    void clearSelection() { selectedEntities_.clear(); }
    
    // Multi-selection
    const std::vector<Entity*>& getSelectedEntities() const { return selectedEntities_; }
    bool isSelected(Entity* entity) const {
        return std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) 
               != selectedEntities_.end();
    }
    void addToSelection(Entity* entity) {
        if (entity && !isSelected(entity)) {
            selectedEntities_.push_back(entity);
        }
    }
    void removeFromSelection(Entity* entity) {
        auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
        if (it != selectedEntities_.end()) {
            selectedEntities_.erase(it);
        }
    }
    void toggleSelection(Entity* entity) {
        if (isSelected(entity)) {
            removeFromSelection(entity);
        } else {
            addToSelection(entity);
        }
    }
    size_t getSelectionCount() const { return selectedEntities_.size(); }
    
    // Copy/Paste clipboard
    void copySelection() {
        clipboard_.clear();
        for (Entity* entity : selectedEntities_) {
            clipboard_.push_back(entity->id);
        }
    }
    
    bool hasClipboard() const { return !clipboard_.empty(); }
    
    void pasteClipboard() {
        std::vector<Entity*> newEntities;
        for (EntityID id : clipboard_) {
            Entity* original = getEntity(id);
            if (original) {
                Entity* copy = duplicateEntity(original);
                if (copy) {
                    // Offset position slightly
                    copy->localTransform.position.x += 1.0f;
                    copy->localTransform.position.z += 1.0f;
                    copy->updateWorldMatrix();
                    newEntities.push_back(copy);
                }
            }
        }
        // Select the new entities
        selectedEntities_ = newEntities;
    }
    
    // Duplicate entity (deep copy)
    Entity* duplicateEntity(Entity* original) {
        if (!original) return nullptr;
        
        Entity* copy = createEntity(original->name + " (Copy)");
        copy->enabled = original->enabled;
        copy->localTransform = original->localTransform;
        copy->hasModel = original->hasModel;
        copy->model = original->model;
        copy->hasLight = original->hasLight;
        copy->light = original->light;
        if (original->material) {
            copy->material = std::make_shared<Material>(*original->material);
        }
        
        copy->updateWorldMatrix();
        return copy;
    }
    
    // === Traversal ===
    
    // Visit all entities (depth-first)
    void traverse(const std::function<void(Entity*)>& visitor);
    
    // Visit all enabled entities with models
    void traverseRenderables(const std::function<void(Entity*)>& visitor);
    
    // === Updates ===
    
    // Update all world matrices (call after transform changes)
    void updateAllWorldMatrices();
    
    // === Scene Info ===
    
    size_t getEntityCount() const { return entities_.size(); }
    
    // Clear all entities
    void clear();
    
private:
    void traverseEntity(Entity* entity, const std::function<void(Entity*)>& visitor);
    void destroyEntityInternal(Entity* entity);
    
    std::unordered_map<EntityID, std::unique_ptr<Entity>> entities_;
    std::vector<Entity*> rootEntities_;
    std::vector<Entity*> selectedEntities_;
    std::vector<EntityID> clipboard_;
    EntityID nextEntityId_ = 1;
};

// ===== Implementation =====

inline SceneGraph::SceneGraph() = default;
inline SceneGraph::~SceneGraph() = default;

inline Entity* SceneGraph::createEntity(const std::string& name) {
    auto entity = std::make_unique<Entity>();
    entity->id = nextEntityId_++;
    entity->name = name;
    entity->updateWorldMatrix();
    
    Entity* ptr = entity.get();
    entities_[entity->id] = std::move(entity);
    rootEntities_.push_back(ptr);
    
    return ptr;
}

inline Entity* SceneGraph::createEntityWithModel(const std::string& name, const RHILoadedModel& model) {
    Entity* entity = createEntity(name);
    entity->hasModel = true;
    entity->model = model;
    return entity;
}

inline void SceneGraph::destroyEntity(EntityID id) {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
        destroyEntityInternal(it->second.get());
    }
}

inline void SceneGraph::destroyEntity(Entity* entity) {
    if (entity) {
        destroyEntity(entity->id);
    }
}

inline void SceneGraph::destroyEntityInternal(Entity* entity) {
    if (!entity) return;
    
    // Recursively destroy children first
    while (!entity->children.empty()) {
        destroyEntityInternal(entity->children.back());
    }
    
    // Remove from parent's children list
    if (entity->parent) {
        entity->parent->removeChild(entity);
    }
    
    // Remove from root entities
    auto rootIt = std::find(rootEntities_.begin(), rootEntities_.end(), entity);
    if (rootIt != rootEntities_.end()) {
        rootEntities_.erase(rootIt);
    }
    
    // Clear selection if this was selected
    auto selIt = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
    if (selIt != selectedEntities_.end()) {
        selectedEntities_.erase(selIt);
    }
    
    // Remove from map
    entities_.erase(entity->id);
}

inline Entity* SceneGraph::getEntity(EntityID id) {
    auto it = entities_.find(id);
    return (it != entities_.end()) ? it->second.get() : nullptr;
}

inline const Entity* SceneGraph::getEntity(EntityID id) const {
    auto it = entities_.find(id);
    return (it != entities_.end()) ? it->second.get() : nullptr;
}

inline Entity* SceneGraph::findEntityByName(const std::string& name) {
    for (auto& [id, entity] : entities_) {
        if (entity->name == name) {
            return entity.get();
        }
    }
    return nullptr;
}

inline void SceneGraph::setParent(Entity* child, Entity* parent) {
    if (!child || child == parent) return;
    
    // Check for circular reference
    Entity* p = parent;
    while (p) {
        if (p == child) return;  // Would create cycle
        p = p->parent;
    }
    
    // Remove from current parent
    if (child->parent) {
        child->parent->removeChild(child);
    } else {
        // Remove from root list
        auto it = std::find(rootEntities_.begin(), rootEntities_.end(), child);
        if (it != rootEntities_.end()) {
            rootEntities_.erase(it);
        }
    }
    
    // Add to new parent
    if (parent) {
        parent->addChild(child);
    } else {
        child->parent = nullptr;
        rootEntities_.push_back(child);
    }
    
    child->updateWorldMatrix();
}

inline void SceneGraph::traverse(const std::function<void(Entity*)>& visitor) {
    for (Entity* root : rootEntities_) {
        traverseEntity(root, visitor);
    }
}

inline void SceneGraph::traverseEntity(Entity* entity, const std::function<void(Entity*)>& visitor) {
    if (!entity) return;
    visitor(entity);
    for (Entity* child : entity->children) {
        traverseEntity(child, visitor);
    }
}

inline void SceneGraph::traverseRenderables(const std::function<void(Entity*)>& visitor) {
    traverse([&](Entity* entity) {
        if (entity->enabled && entity->hasModel) {
            visitor(entity);
        }
    });
}

inline void SceneGraph::updateAllWorldMatrices() {
    for (Entity* root : rootEntities_) {
        root->updateWorldMatrix();
    }
}

inline void SceneGraph::clear() {
    entities_.clear();
    rootEntities_.clear();
    selectedEntities_.clear();
    clipboard_.clear();
    nextEntityId_ = 1;
}

}  // namespace luma
