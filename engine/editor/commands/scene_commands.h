// Scene Commands - Undoable scene graph operations
#pragma once

#include "engine/editor/command.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include <sstream>

namespace luma {

// ===== Create Entity Command =====
class CreateEntityCommand : public Command {
public:
    CreateEntityCommand(SceneGraph* scene, const std::string& name = "Entity")
        : scene_(scene), name_(name), entityId_(INVALID_ENTITY) {}
    
    void execute() override {
        if (entityId_ == INVALID_ENTITY) {
            // First execution - create new entity
            Entity* entity = scene_->createEntity(name_);
            entityId_ = entity->id;
            createdEntity_ = entity;
        } else {
            // Re-execution after undo - restore entity
            // Note: This is complex because entity was deleted
            // For simplicity, create a new one with same name
            Entity* entity = scene_->createEntity(name_);
            entityId_ = entity->id;
            createdEntity_ = entity;
        }
    }
    
    void undo() override {
        if (entityId_ != INVALID_ENTITY) {
            scene_->destroyEntity(entityId_);
            createdEntity_ = nullptr;
        }
    }
    
    std::string getDescription() const override {
        return "Create " + name_;
    }
    
    const char* getTypeId() const override { return "CreateEntity"; }
    
    Entity* getCreatedEntity() const { return createdEntity_; }
    
private:
    SceneGraph* scene_;
    std::string name_;
    EntityID entityId_;
    Entity* createdEntity_ = nullptr;
};

// ===== Delete Entity Command =====
class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(SceneGraph* scene, Entity* entity)
        : scene_(scene)
        , entityId_(entity->id)
        , name_(entity->name)
        , transform_(entity->localTransform)
        , parentId_(entity->parent ? entity->parent->id : INVALID_ENTITY)
        , hasModel_(entity->hasModel) {
        // Store child IDs for potential restoration
        for (Entity* child : entity->children) {
            childIds_.push_back(child->id);
        }
    }
    
    void execute() override {
        scene_->destroyEntity(entityId_);
    }
    
    void undo() override {
        // Recreate entity
        Entity* entity = scene_->createEntity(name_);
        entity->localTransform = transform_;
        
        // Restore parent relationship
        if (parentId_ != INVALID_ENTITY) {
            Entity* parent = scene_->getEntity(parentId_);
            if (parent) {
                scene_->setParent(entity, parent);
            }
        }
        
        entityId_ = entity->id;
        entity->updateWorldMatrix();
    }
    
    std::string getDescription() const override {
        return "Delete " + name_;
    }
    
    const char* getTypeId() const override { return "DeleteEntity"; }
    
private:
    SceneGraph* scene_;
    EntityID entityId_;
    std::string name_;
    Transform transform_;
    EntityID parentId_;
    bool hasModel_;
    std::vector<EntityID> childIds_;
};

// ===== Rename Entity Command =====
class RenameEntityCommand : public Command {
public:
    RenameEntityCommand(Entity* entity, const std::string& newName)
        : entity_(entity)
        , oldName_(entity->name)
        , newName_(newName) {}
    
    void execute() override {
        entity_->name = newName_;
    }
    
    void undo() override {
        entity_->name = oldName_;
    }
    
    std::string getDescription() const override {
        return "Rename to " + newName_;
    }
    
    const char* getTypeId() const override { return "RenameEntity"; }
    
private:
    Entity* entity_;
    std::string oldName_;
    std::string newName_;
};

// ===== Reparent Entity Command =====
class ReparentEntityCommand : public Command {
public:
    ReparentEntityCommand(SceneGraph* scene, Entity* entity, Entity* newParent)
        : scene_(scene)
        , entity_(entity)
        , oldParent_(entity->parent)
        , newParent_(newParent) {}
    
    void execute() override {
        scene_->setParent(entity_, newParent_);
    }
    
    void undo() override {
        scene_->setParent(entity_, oldParent_);
    }
    
    std::string getDescription() const override {
        std::ostringstream ss;
        ss << "Reparent " << entity_->name;
        if (newParent_) {
            ss << " to " << newParent_->name;
        } else {
            ss << " to root";
        }
        return ss.str();
    }
    
    const char* getTypeId() const override { return "ReparentEntity"; }
    
private:
    SceneGraph* scene_;
    Entity* entity_;
    Entity* oldParent_;
    Entity* newParent_;
};

// ===== Duplicate Entity Command =====
class DuplicateEntityCommand : public Command {
public:
    DuplicateEntityCommand(SceneGraph* scene, Entity* source)
        : scene_(scene)
        , sourceId_(source->id)
        , sourceName_(source->name)
        , sourceTransform_(source->localTransform)
        , duplicateId_(INVALID_ENTITY) {}
    
    void execute() override {
        Entity* source = scene_->getEntity(sourceId_);
        if (!source) return;
        
        // Create duplicate
        std::string newName = sourceName_ + " (Copy)";
        Entity* duplicate = scene_->createEntity(newName);
        duplicate->localTransform = sourceTransform_;
        
        // Offset position slightly
        duplicate->localTransform.position.x += 1.0f;
        
        // Copy model reference if exists
        if (source->hasModel) {
            duplicate->hasModel = true;
            duplicate->model = source->model;
        }
        
        duplicate->updateWorldMatrix();
        duplicateId_ = duplicate->id;
        duplicateEntity_ = duplicate;
    }
    
    void undo() override {
        if (duplicateId_ != INVALID_ENTITY) {
            scene_->destroyEntity(duplicateId_);
            duplicateEntity_ = nullptr;
        }
    }
    
    std::string getDescription() const override {
        return "Duplicate " + sourceName_;
    }
    
    const char* getTypeId() const override { return "DuplicateEntity"; }
    
    Entity* getDuplicatedEntity() const { return duplicateEntity_; }
    
private:
    SceneGraph* scene_;
    EntityID sourceId_;
    std::string sourceName_;
    Transform sourceTransform_;
    EntityID duplicateId_;
    Entity* duplicateEntity_ = nullptr;
};

}  // namespace luma
