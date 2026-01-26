// Transform Commands - Undoable transform operations
#pragma once

#include "engine/editor/command.h"
#include "engine/scene/entity.h"
#include "engine/foundation/math_types.h"
#include <sstream>

namespace luma {

// ===== Set Position Command =====
class SetPositionCommand : public Command {
public:
    SetPositionCommand(Entity* entity, const Vec3& newPosition)
        : entity_(entity)
        , oldPosition_(entity->localTransform.position)
        , newPosition_(newPosition) {}
    
    void execute() override {
        entity_->localTransform.position = newPosition_;
        entity_->updateWorldMatrix();
    }
    
    void undo() override {
        entity_->localTransform.position = oldPosition_;
        entity_->updateWorldMatrix();
    }
    
    std::string getDescription() const override {
        std::ostringstream ss;
        ss << "Move " << entity_->name;
        return ss.str();
    }
    
    bool canMergeWith(const Command* other) const override {
        if (auto* cmd = dynamic_cast<const SetPositionCommand*>(other)) {
            return cmd->entity_ == entity_;
        }
        return false;
    }
    
    void mergeWith(Command* other) override {
        if (auto* cmd = dynamic_cast<SetPositionCommand*>(other)) {
            oldPosition_ = cmd->oldPosition_;
        }
    }
    
    const char* getTypeId() const override { return "SetPosition"; }
    
private:
    Entity* entity_;
    Vec3 oldPosition_;
    Vec3 newPosition_;
};

// ===== Set Rotation Command =====
class SetRotationCommand : public Command {
public:
    SetRotationCommand(Entity* entity, const Quat& newRotation)
        : entity_(entity)
        , oldRotation_(entity->localTransform.rotation)
        , newRotation_(newRotation) {}
    
    void execute() override {
        entity_->localTransform.rotation = newRotation_;
        entity_->updateWorldMatrix();
    }
    
    void undo() override {
        entity_->localTransform.rotation = oldRotation_;
        entity_->updateWorldMatrix();
    }
    
    std::string getDescription() const override {
        std::ostringstream ss;
        ss << "Rotate " << entity_->name;
        return ss.str();
    }
    
    bool canMergeWith(const Command* other) const override {
        if (auto* cmd = dynamic_cast<const SetRotationCommand*>(other)) {
            return cmd->entity_ == entity_;
        }
        return false;
    }
    
    void mergeWith(Command* other) override {
        if (auto* cmd = dynamic_cast<SetRotationCommand*>(other)) {
            oldRotation_ = cmd->oldRotation_;
        }
    }
    
    const char* getTypeId() const override { return "SetRotation"; }
    
private:
    Entity* entity_;
    Quat oldRotation_;
    Quat newRotation_;
};

// ===== Set Scale Command =====
class SetScaleCommand : public Command {
public:
    SetScaleCommand(Entity* entity, const Vec3& newScale)
        : entity_(entity)
        , oldScale_(entity->localTransform.scale)
        , newScale_(newScale) {}
    
    void execute() override {
        entity_->localTransform.scale = newScale_;
        entity_->updateWorldMatrix();
    }
    
    void undo() override {
        entity_->localTransform.scale = oldScale_;
        entity_->updateWorldMatrix();
    }
    
    std::string getDescription() const override {
        std::ostringstream ss;
        ss << "Scale " << entity_->name;
        return ss.str();
    }
    
    bool canMergeWith(const Command* other) const override {
        if (auto* cmd = dynamic_cast<const SetScaleCommand*>(other)) {
            return cmd->entity_ == entity_;
        }
        return false;
    }
    
    void mergeWith(Command* other) override {
        if (auto* cmd = dynamic_cast<SetScaleCommand*>(other)) {
            oldScale_ = cmd->oldScale_;
        }
    }
    
    const char* getTypeId() const override { return "SetScale"; }
    
private:
    Entity* entity_;
    Vec3 oldScale_;
    Vec3 newScale_;
};

// ===== Set Full Transform Command =====
class SetTransformCommand : public Command {
public:
    SetTransformCommand(Entity* entity, const Transform& newTransform)
        : entity_(entity)
        , oldTransform_(entity->localTransform)
        , newTransform_(newTransform) {}
    
    void execute() override {
        entity_->localTransform = newTransform_;
        entity_->updateWorldMatrix();
    }
    
    void undo() override {
        entity_->localTransform = oldTransform_;
        entity_->updateWorldMatrix();
    }
    
    std::string getDescription() const override {
        std::ostringstream ss;
        ss << "Transform " << entity_->name;
        return ss.str();
    }
    
    bool canMergeWith(const Command* other) const override {
        if (auto* cmd = dynamic_cast<const SetTransformCommand*>(other)) {
            return cmd->entity_ == entity_;
        }
        return false;
    }
    
    void mergeWith(Command* other) override {
        if (auto* cmd = dynamic_cast<SetTransformCommand*>(other)) {
            oldTransform_ = cmd->oldTransform_;
        }
    }
    
    const char* getTypeId() const override { return "SetTransform"; }
    
private:
    Entity* entity_;
    Transform oldTransform_;
    Transform newTransform_;
};

}  // namespace luma
