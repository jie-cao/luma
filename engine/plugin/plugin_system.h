// LUMA Plugin System - Extensible architecture for third-party content
// Supports: Character Templates, Clothing, Hair, Accessories, and more
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>

namespace luma {

// Forward declarations
class Mesh;
class Skeleton;
class BlendShapeMesh;

// ============================================================================
// Plugin Types
// ============================================================================

enum class PluginType {
    CharacterTemplate,      // New character type (e.g., Robot, Alien)
    Clothing,              // Clothing items
    Hair,                  // Hairstyles
    Accessory,             // Accessories (glasses, hats, etc.)
    Material,              // Custom materials/shaders
    Animation,             // Animation clips
    Expression,            // Facial expressions
    BodyPart,              // Custom body parts
    Effect,                // Visual effects
    Tool,                  // Editor tools
    Exporter,              // Export formats
    Unknown
};

inline std::string pluginTypeToString(PluginType type) {
    switch (type) {
        case PluginType::CharacterTemplate: return "character_template";
        case PluginType::Clothing: return "clothing";
        case PluginType::Hair: return "hair";
        case PluginType::Accessory: return "accessory";
        case PluginType::Material: return "material";
        case PluginType::Animation: return "animation";
        case PluginType::Expression: return "expression";
        case PluginType::BodyPart: return "body_part";
        case PluginType::Effect: return "effect";
        case PluginType::Tool: return "tool";
        case PluginType::Exporter: return "exporter";
        default: return "unknown";
    }
}

inline PluginType stringToPluginType(const std::string& str) {
    if (str == "character_template") return PluginType::CharacterTemplate;
    if (str == "clothing") return PluginType::Clothing;
    if (str == "hair") return PluginType::Hair;
    if (str == "accessory") return PluginType::Accessory;
    if (str == "material") return PluginType::Material;
    if (str == "animation") return PluginType::Animation;
    if (str == "expression") return PluginType::Expression;
    if (str == "body_part") return PluginType::BodyPart;
    if (str == "effect") return PluginType::Effect;
    if (str == "tool") return PluginType::Tool;
    if (str == "exporter") return PluginType::Exporter;
    return PluginType::Unknown;
}

// ============================================================================
// Plugin Metadata
// ============================================================================

struct PluginVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    
    std::string toString() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch);
    }
    
    bool isCompatibleWith(const PluginVersion& required) const {
        // Major version must match, minor must be >= required
        return major == required.major && 
               (minor > required.minor || (minor == required.minor && patch >= required.patch));
    }
    
    static PluginVersion parse(const std::string& str) {
        PluginVersion v;
        sscanf(str.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch);
        return v;
    }
};

struct PluginMetadata {
    std::string id;                     // Unique identifier (e.g., "com.artist.robot-template")
    std::string name;                   // Display name
    std::string description;            // Description
    std::string author;                 // Author name
    std::string website;                // Author website
    std::string license;                // License (MIT, CC-BY, etc.)
    PluginVersion version;              // Plugin version
    PluginVersion minEngineVersion;     // Minimum LUMA version required
    PluginType type = PluginType::Unknown;
    
    std::vector<std::string> tags;      // Search tags
    std::vector<std::string> dependencies;  // Required plugin IDs
    
    std::string thumbnailPath;          // Preview image path (relative to plugin)
    std::string entryPoint;             // Main file (script or shared lib)
    
    // Validation
    bool isValid() const {
        return !id.empty() && !name.empty() && type != PluginType::Unknown;
    }
};

// ============================================================================
// Plugin Asset - A single item provided by a plugin
// ============================================================================

struct PluginAsset {
    std::string id;                     // Unique ID within plugin
    std::string name;                   // Display name
    std::string category;               // Category for organization
    std::string description;
    std::string thumbnailPath;
    
    // Asset-specific paths
    std::string meshPath;               // 3D model file
    std::string texturePath;            // Main texture
    std::string materialPath;           // Material definition
    std::string configPath;             // Additional config (JSON)
    
    // Metadata
    std::unordered_map<std::string, std::string> properties;
    std::vector<std::string> tags;
    
    // Get full asset ID (plugin_id:asset_id)
    std::string getFullId(const std::string& pluginId) const {
        return pluginId + ":" + id;
    }
};

// ============================================================================
// Plugin Interface - Base class for all plugins
// ============================================================================

class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // === Required Methods ===
    
    virtual const PluginMetadata& getMetadata() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // === Asset Discovery ===
    
    virtual std::vector<PluginAsset> getAssets() const = 0;
    virtual const PluginAsset* getAsset(const std::string& assetId) const = 0;
    
    // === Optional Callbacks ===
    
    virtual void onAssetSelected(const std::string& assetId) {}
    virtual void onAssetApplied(const std::string& assetId) {}
    
    // === Helper ===
    
    PluginType getType() const { return getMetadata().type; }
    const std::string& getId() const { return getMetadata().id; }
    const std::string& getName() const { return getMetadata().name; }
};

// ============================================================================
// Character Template Plugin Interface
// ============================================================================

struct CharacterTemplatePluginParams {
    float height = 1.8f;
    Vec3 primaryColor{1, 1, 1};
    Vec3 secondaryColor{0.5f, 0.5f, 0.5f};
    Vec3 accentColor{1, 0, 0};
    std::unordered_map<std::string, float> customParams;
};

struct CharacterTemplatePluginResult {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<BlendShapeMesh> blendShapes;
    bool success = false;
    std::string errorMessage;
};

class ICharacterTemplatePlugin : public IPlugin {
public:
    // Create character from parameters
    virtual CharacterTemplatePluginResult createCharacter(
        const CharacterTemplatePluginParams& params) = 0;
    
    // Get available customization parameters
    virtual std::vector<std::string> getCustomizableParams() const = 0;
    
    // Get parameter range
    virtual std::pair<float, float> getParamRange(const std::string& param) const {
        return {0.0f, 1.0f};
    }
    
    // Get default parameters
    virtual CharacterTemplatePluginParams getDefaultParams() const = 0;
};

// ============================================================================
// Clothing Plugin Interface
// ============================================================================

struct ClothingPluginItem {
    std::string id;
    std::string name;
    std::string category;           // "tops", "bottoms", "shoes", "full_body"
    std::string slot;               // Equip slot
    std::vector<std::string> conflictingSlots;
    
    std::string meshPath;
    std::string texturePath;
    std::string normalMapPath;
    
    // Fit parameters
    std::vector<std::string> supportedBodyTypes;
    bool hasPhysics = false;
    bool hasSkinning = true;
};

class IClothingPlugin : public IPlugin {
public:
    virtual std::vector<ClothingPluginItem> getClothingItems() const = 0;
    virtual const ClothingPluginItem* getClothingItem(const std::string& id) const = 0;
    
    // Load mesh for specific body parameters
    virtual std::shared_ptr<Mesh> loadClothingMesh(
        const std::string& itemId,
        float bodyHeight,
        float bodyWeight) = 0;
};

// ============================================================================
// Hair Plugin Interface
// ============================================================================

struct HairPluginStyle {
    std::string id;
    std::string name;
    std::string category;           // "short", "medium", "long", "updo"
    
    std::string meshPath;
    std::string texturePath;
    
    Vec3 defaultColor{0.2f, 0.15f, 0.1f};
    bool supportsColorChange = true;
    bool hasPhysics = false;
};

class IHairPlugin : public IPlugin {
public:
    virtual std::vector<HairPluginStyle> getHairStyles() const = 0;
    virtual const HairPluginStyle* getHairStyle(const std::string& id) const = 0;
    
    virtual std::shared_ptr<Mesh> loadHairMesh(
        const std::string& styleId,
        const Vec3& color) = 0;
};

// ============================================================================
// Plugin Factory - Creates plugins from metadata
// ============================================================================

using PluginFactoryFunc = std::function<std::shared_ptr<IPlugin>()>;

class PluginFactory {
public:
    static PluginFactory& getInstance() {
        static PluginFactory instance;
        return instance;
    }
    
    // Register factory function for plugin type
    void registerFactory(const std::string& pluginId, PluginFactoryFunc factory) {
        factories_[pluginId] = factory;
    }
    
    // Create plugin instance
    std::shared_ptr<IPlugin> createPlugin(const std::string& pluginId) {
        auto it = factories_.find(pluginId);
        if (it != factories_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    bool hasFactory(const std::string& pluginId) const {
        return factories_.find(pluginId) != factories_.end();
    }
    
private:
    PluginFactory() = default;
    std::unordered_map<std::string, PluginFactoryFunc> factories_;
};

// ============================================================================
// Plugin Registration Macros
// ============================================================================

#define LUMA_PLUGIN_REGISTER(PluginClass, pluginId) \
    namespace { \
        struct PluginClass##_Registrar { \
            PluginClass##_Registrar() { \
                luma::PluginFactory::getInstance().registerFactory( \
                    pluginId, \
                    []() -> std::shared_ptr<luma::IPlugin> { \
                        return std::make_shared<PluginClass>(); \
                    }); \
            } \
        }; \
        static PluginClass##_Registrar pluginClass##_registrar; \
    }

// For dynamic library plugins
#define LUMA_PLUGIN_EXPORT(PluginClass) \
    extern "C" { \
        LUMA_EXPORT luma::IPlugin* luma_create_plugin() { \
            return new PluginClass(); \
        } \
        LUMA_EXPORT void luma_destroy_plugin(luma::IPlugin* plugin) { \
            delete plugin; \
        } \
        LUMA_EXPORT const char* luma_get_plugin_api_version() { \
            return "1.0.0"; \
        } \
    }

#ifdef _WIN32
    #define LUMA_EXPORT __declspec(dllexport)
#else
    #define LUMA_EXPORT __attribute__((visibility("default")))
#endif

}  // namespace luma
