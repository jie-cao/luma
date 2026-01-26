// Material Editor Commands for Undo/Redo
#pragma once

#include "engine/editor/command.h"
#include "engine/material/material.h"
#include "engine/scene/entity.h"
#include <memory>

namespace luma {

// ===== Set Material Property Command =====
// Generic command for any material property change
class SetMaterialCommand : public Command {
public:
    SetMaterialCommand(Entity* entity, const Material& newMaterial)
        : entity_(entity)
        , newMaterial_(newMaterial) {
        if (entity->material) {
            oldMaterial_ = *entity->material;
        }
    }
    
    void execute() override {
        if (!entity_->material) {
            entity_->material = std::make_shared<Material>();
        }
        *entity_->material = newMaterial_;
    }
    
    void undo() override {
        if (entity_->material) {
            *entity_->material = oldMaterial_;
        }
    }
    
    std::string getDescription() const override {
        return "Change Material";
    }
    
    const char* getTypeId() const override { return "SetMaterial"; }
    
private:
    Entity* entity_;
    Material oldMaterial_;
    Material newMaterial_;
};

// ===== Set Base Color Command =====
class SetBaseColorCommand : public Command {
public:
    SetBaseColorCommand(Entity* entity, const Vec3& color, float alpha = 1.0f)
        : entity_(entity), newColor_(color), newAlpha_(alpha) {
        if (entity->material) {
            oldColor_ = entity->material->baseColor;
            oldAlpha_ = entity->material->alpha;
        }
    }
    
    void execute() override {
        if (!entity_->material) {
            entity_->material = std::make_shared<Material>();
        }
        entity_->material->baseColor = newColor_;
        entity_->material->alpha = newAlpha_;
    }
    
    void undo() override {
        if (entity_->material) {
            entity_->material->baseColor = oldColor_;
            entity_->material->alpha = oldAlpha_;
        }
    }
    
    std::string getDescription() const override {
        return "Change Base Color";
    }
    
    const char* getTypeId() const override { return "SetBaseColor"; }
    
    bool canMergeWith(const Command* other) const override {
        auto* o = dynamic_cast<const SetBaseColorCommand*>(other);
        return o && o->entity_ == entity_;
    }
    
    void mergeWith(const Command* other) override {
        auto* o = dynamic_cast<const SetBaseColorCommand*>(other);
        if (o) {
            newColor_ = o->newColor_;
            newAlpha_ = o->newAlpha_;
        }
    }
    
private:
    Entity* entity_;
    Vec3 oldColor_{1, 1, 1};
    Vec3 newColor_;
    float oldAlpha_ = 1.0f;
    float newAlpha_;
};

// ===== Set Metallic Command =====
class SetMetallicCommand : public Command {
public:
    SetMetallicCommand(Entity* entity, float metallic)
        : entity_(entity), newMetallic_(metallic) {
        if (entity->material) {
            oldMetallic_ = entity->material->metallic;
        }
    }
    
    void execute() override {
        if (!entity_->material) {
            entity_->material = std::make_shared<Material>();
        }
        entity_->material->metallic = newMetallic_;
    }
    
    void undo() override {
        if (entity_->material) {
            entity_->material->metallic = oldMetallic_;
        }
    }
    
    std::string getDescription() const override {
        return "Change Metallic";
    }
    
    const char* getTypeId() const override { return "SetMetallic"; }
    
    bool canMergeWith(const Command* other) const override {
        auto* o = dynamic_cast<const SetMetallicCommand*>(other);
        return o && o->entity_ == entity_;
    }
    
    void mergeWith(const Command* other) override {
        auto* o = dynamic_cast<const SetMetallicCommand*>(other);
        if (o) {
            newMetallic_ = o->newMetallic_;
        }
    }
    
private:
    Entity* entity_;
    float oldMetallic_ = 0.0f;
    float newMetallic_;
};

// ===== Set Roughness Command =====
class SetRoughnessCommand : public Command {
public:
    SetRoughnessCommand(Entity* entity, float roughness)
        : entity_(entity), newRoughness_(roughness) {
        if (entity->material) {
            oldRoughness_ = entity->material->roughness;
        }
    }
    
    void execute() override {
        if (!entity_->material) {
            entity_->material = std::make_shared<Material>();
        }
        entity_->material->roughness = newRoughness_;
    }
    
    void undo() override {
        if (entity_->material) {
            entity_->material->roughness = oldRoughness_;
        }
    }
    
    std::string getDescription() const override {
        return "Change Roughness";
    }
    
    const char* getTypeId() const override { return "SetRoughness"; }
    
    bool canMergeWith(const Command* other) const override {
        auto* o = dynamic_cast<const SetRoughnessCommand*>(other);
        return o && o->entity_ == entity_;
    }
    
    void mergeWith(const Command* other) override {
        auto* o = dynamic_cast<const SetRoughnessCommand*>(other);
        if (o) {
            newRoughness_ = o->newRoughness_;
        }
    }
    
private:
    Entity* entity_;
    float oldRoughness_ = 0.5f;
    float newRoughness_;
};

// ===== Apply Material Preset Command =====
class ApplyMaterialPresetCommand : public Command {
public:
    ApplyMaterialPresetCommand(Entity* entity, const std::string& presetName, const Material& preset)
        : entity_(entity), presetName_(presetName), newMaterial_(preset) {
        if (entity->material) {
            oldMaterial_ = *entity->material;
        }
    }
    
    void execute() override {
        if (!entity_->material) {
            entity_->material = std::make_shared<Material>();
        }
        *entity_->material = newMaterial_;
    }
    
    void undo() override {
        if (entity_->material) {
            *entity_->material = oldMaterial_;
        }
    }
    
    std::string getDescription() const override {
        return "Apply Preset: " + presetName_;
    }
    
    const char* getTypeId() const override { return "ApplyMaterialPreset"; }
    
private:
    Entity* entity_;
    std::string presetName_;
    Material oldMaterial_;
    Material newMaterial_;
};

}  // namespace luma
