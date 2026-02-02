// LUMA Character Template System
// Abstract interface for different character types (human, cartoon, mascot, etc.)
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/character/blend_shape.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace luma {

// ============================================================================
// Character Type
// ============================================================================

enum class CharacterType {
    Human,          // Realistic human
    Anime,          // Anime/manga style human
    Cartoon,        // Western cartoon (Mickey, Bugs Bunny)
    Mascot,         // Cute mascot (Hello Kitty, Pikachu)
    Animal,         // Realistic or stylized animal
    Robot,          // Mechanical character
    Fantasy,        // Fantasy creatures (elves, orcs)
    Chibi,          // Super-deformed cute
    Custom          // User-defined
};

inline const char* getCharacterTypeName(CharacterType type) {
    switch (type) {
        case CharacterType::Human:    return "Human";
        case CharacterType::Anime:    return "Anime";
        case CharacterType::Cartoon:  return "Cartoon";
        case CharacterType::Mascot:   return "Mascot";
        case CharacterType::Animal:   return "Animal";
        case CharacterType::Robot:    return "Robot";
        case CharacterType::Fantasy:  return "Fantasy";
        case CharacterType::Chibi:    return "Chibi";
        case CharacterType::Custom:   return "Custom";
    }
    return "Unknown";
}

// ============================================================================
// Body Proportions
// ============================================================================

struct BodyProportions {
    // Head-to-body ratio (normal human = 7-8, chibi = 2-3)
    float headToBodyRatio = 7.5f;
    
    // Relative sizes (1.0 = normal)
    float headScale = 1.0f;
    float eyeScale = 1.0f;
    float bodyScale = 1.0f;
    float armScale = 1.0f;
    float legScale = 1.0f;
    
    // Position offsets
    float eyeHeight = 0.5f;      // 0-1 on head
    float eyeSpacing = 0.3f;     // Distance between eyes
    float mouthHeight = 0.2f;    // 0-1 on head
    
    // Body shape
    float shoulderWidth = 1.0f;
    float hipWidth = 1.0f;
    float limbThickness = 1.0f;
    
    // Presets
    static BodyProportions realistic() {
        BodyProportions p;
        p.headToBodyRatio = 7.5f;
        return p;
    }
    
    static BodyProportions anime() {
        BodyProportions p;
        p.headToBodyRatio = 6.0f;
        p.headScale = 1.2f;
        p.eyeScale = 1.8f;
        p.eyeHeight = 0.45f;
        p.eyeSpacing = 0.35f;
        return p;
    }
    
    static BodyProportions chibi() {
        BodyProportions p;
        p.headToBodyRatio = 2.5f;
        p.headScale = 2.0f;
        p.eyeScale = 2.5f;
        p.bodyScale = 0.5f;
        p.armScale = 0.6f;
        p.legScale = 0.5f;
        p.eyeHeight = 0.4f;
        p.eyeSpacing = 0.4f;
        p.limbThickness = 1.5f;
        return p;
    }
    
    static BodyProportions mascot() {
        BodyProportions p;
        p.headToBodyRatio = 1.5f;  // Almost all head
        p.headScale = 2.5f;
        p.eyeScale = 2.0f;
        p.bodyScale = 0.3f;
        p.armScale = 0.4f;
        p.legScale = 0.3f;
        p.eyeHeight = 0.5f;
        p.eyeSpacing = 0.45f;
        p.limbThickness = 2.0f;
        return p;
    }
    
    static BodyProportions cartoon() {
        BodyProportions p;
        p.headToBodyRatio = 4.0f;
        p.headScale = 1.5f;
        p.eyeScale = 1.5f;
        p.bodyScale = 0.8f;
        p.armScale = 1.0f;
        p.legScale = 0.9f;
        p.eyeHeight = 0.5f;
        p.limbThickness = 1.2f;
        return p;
    }
};

// ============================================================================
// Character Parameters (unified across all types)
// ============================================================================

struct CharacterParams {
    std::string name = "Character";
    CharacterType type = CharacterType::Human;
    
    // Base proportions
    BodyProportions proportions;
    
    // Overall size
    float height = 1.0f;  // Normalized, actual height varies by type
    float width = 1.0f;
    
    // Colors
    Vec3 primaryColor{1.0f, 1.0f, 1.0f};      // Main body/skin color
    Vec3 secondaryColor{0.5f, 0.5f, 0.5f};    // Secondary color
    Vec3 accentColor{1.0f, 0.0f, 0.0f};       // Accent (bow, accessories)
    
    // Features (interpretation depends on type)
    bool hasEars = true;
    bool hasTail = false;
    bool hasWings = false;
    bool hasMouth = true;  // Hello Kitty doesn't have a mouth!
    bool hasNose = true;
    
    // Style
    int eyeStyle = 0;      // Index into eye style library
    int mouthStyle = 0;    // Index into mouth style library
    int earStyle = 0;      // Human/elf/cat/dog/mouse/etc.
    int bodyStyle = 0;     // Body shape variant
};

// ============================================================================
// Template Creation Result
// ============================================================================

struct CharacterCreationResult {
    bool success = false;
    std::string errorMessage;
    
    // Generated data
    Mesh baseMesh;
    Skeleton skeleton;
    BlendShapeMesh blendShapes;
    
    // Bounds
    Vec3 boundsMin;
    Vec3 boundsMax;
    Vec3 center;
    float radius = 1.0f;
    
    // Metadata
    std::unordered_map<std::string, int> partIndices;  // Part name -> vertex start index
};

// ============================================================================
// ICharacterTemplate - Abstract Interface
// ============================================================================

class ICharacterTemplate {
public:
    virtual ~ICharacterTemplate() = default;
    
    // === Identity ===
    virtual CharacterType getType() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual std::string getDescription() const = 0;
    
    // === Capabilities ===
    virtual bool supportsFeature(const std::string& feature) const = 0;
    virtual std::vector<std::string> getSupportedFeatures() const = 0;
    
    // === Default Parameters ===
    virtual CharacterParams getDefaultParams() const = 0;
    virtual BodyProportions getDefaultProportions() const = 0;
    
    // === Creation ===
    virtual CharacterCreationResult create(const CharacterParams& params) = 0;
    
    // === Skeleton ===
    virtual Skeleton createSkeleton(const CharacterParams& params) = 0;
    virtual std::vector<std::string> getRequiredBones() const = 0;
    virtual std::vector<std::string> getOptionalBones() const = 0;
    
    // === Mesh Generation ===
    virtual Mesh createBaseMesh(const CharacterParams& params) = 0;
    
    // === BlendShapes ===
    virtual BlendShapeMesh createBlendShapes(const CharacterParams& params, 
                                             const Mesh& baseMesh) = 0;
    virtual std::vector<std::string> getAvailableExpressions() const = 0;
    
    // === Customization ===
    virtual std::vector<std::string> getCustomizableAttributes() const = 0;
    virtual void applyCustomization(CharacterCreationResult& result,
                                    const std::string& attribute,
                                    float value) = 0;
    
    // === Validation ===
    virtual bool validateParams(const CharacterParams& params,
                               std::string& outError) const = 0;
};

// ============================================================================
// Character Template Registry
// ============================================================================

class CharacterTemplateRegistry {
public:
    static CharacterTemplateRegistry& getInstance() {
        static CharacterTemplateRegistry instance;
        return instance;
    }
    
    // Register a template
    void registerTemplate(std::shared_ptr<ICharacterTemplate> tmpl) {
        if (tmpl) {
            templates_[tmpl->getType()] = tmpl;
        }
    }
    
    // Get template by type
    std::shared_ptr<ICharacterTemplate> getTemplate(CharacterType type) const {
        auto it = templates_.find(type);
        return it != templates_.end() ? it->second : nullptr;
    }
    
    // Get all registered types
    std::vector<CharacterType> getRegisteredTypes() const {
        std::vector<CharacterType> types;
        for (const auto& [type, tmpl] : templates_) {
            types.push_back(type);
        }
        return types;
    }
    
    // Check if type is registered
    bool hasTemplate(CharacterType type) const {
        return templates_.count(type) > 0;
    }
    
    // Create character from type and params
    CharacterCreationResult createCharacter(CharacterType type,
                                           const CharacterParams& params) {
        auto tmpl = getTemplate(type);
        if (!tmpl) {
            CharacterCreationResult result;
            result.success = false;
            result.errorMessage = "Template not registered: " + 
                                  std::string(getCharacterTypeName(type));
            return result;
        }
        return tmpl->create(params);
    }
    
private:
    CharacterTemplateRegistry() = default;
    std::unordered_map<CharacterType, std::shared_ptr<ICharacterTemplate>> templates_;
};

// Convenience function
inline CharacterTemplateRegistry& getTemplateRegistry() {
    return CharacterTemplateRegistry::getInstance();
}

// ============================================================================
// Base Template Implementation (common functionality)
// ============================================================================

class BaseCharacterTemplate : public ICharacterTemplate {
public:
    BaseCharacterTemplate(CharacterType type, const std::string& name, const std::string& desc)
        : type_(type), typeName_(name), description_(desc) {}
    
    CharacterType getType() const override { return type_; }
    std::string getTypeName() const override { return typeName_; }
    std::string getDescription() const override { return description_; }
    
    bool supportsFeature(const std::string& feature) const override {
        auto features = getSupportedFeatures();
        return std::find(features.begin(), features.end(), feature) != features.end();
    }
    
    CharacterCreationResult create(const CharacterParams& params) override {
        CharacterCreationResult result;
        
        // Validate
        std::string error;
        if (!validateParams(params, error)) {
            result.success = false;
            result.errorMessage = error;
            return result;
        }
        
        // Create components
        result.skeleton = createSkeleton(params);
        result.baseMesh = createBaseMesh(params);
        result.blendShapes = createBlendShapes(params, result.baseMesh);
        
        // Calculate bounds
        calculateBounds(result);
        
        result.success = true;
        return result;
    }
    
    bool validateParams(const CharacterParams& params, std::string& outError) const override {
        if (params.height <= 0 || params.height > 10.0f) {
            outError = "Invalid height value";
            return false;
        }
        return true;
    }
    
protected:
    CharacterType type_;
    std::string typeName_;
    std::string description_;
    
    void calculateBounds(CharacterCreationResult& result) const {
        if (result.baseMesh.vertices.empty()) return;
        
        result.boundsMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        result.boundsMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for (const auto& v : result.baseMesh.vertices) {
            result.boundsMin.x = std::min(result.boundsMin.x, v.position[0]);
            result.boundsMin.y = std::min(result.boundsMin.y, v.position[1]);
            result.boundsMin.z = std::min(result.boundsMin.z, v.position[2]);
            
            result.boundsMax.x = std::max(result.boundsMax.x, v.position[0]);
            result.boundsMax.y = std::max(result.boundsMax.y, v.position[1]);
            result.boundsMax.z = std::max(result.boundsMax.z, v.position[2]);
        }
        
        result.center = (result.boundsMin + result.boundsMax) * 0.5f;
        Vec3 extent = result.boundsMax - result.boundsMin;
        result.radius = extent.length() * 0.5f;
    }
};

}  // namespace luma
