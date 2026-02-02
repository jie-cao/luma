// LUMA Plugin Examples - Sample plugin implementations
// Use these as templates for creating your own plugins
#pragma once

#include "engine/plugin/plugin_system.h"
#include "engine/plugin/plugin_manager.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include "engine/character/blend_shape.h"
#include <memory>

namespace luma {

// ============================================================================
// Example 1: Simple Clothing Pack Plugin
// ============================================================================

class ExampleClothingPlugin : public IClothingPlugin {
public:
    ExampleClothingPlugin() {
        metadata_.id = "com.example.basic-clothing";
        metadata_.name = "Basic Clothing Pack";
        metadata_.description = "A starter pack with basic clothing items";
        metadata_.author = "LUMA Team";
        metadata_.version = {1, 0, 0};
        metadata_.type = PluginType::Clothing;
        metadata_.tags = {"clothing", "basic", "starter"};
    }
    
    const PluginMetadata& getMetadata() const override { return metadata_; }
    
    bool initialize() override {
        // Register clothing items
        items_.push_back({
            "tshirt_basic",
            "Basic T-Shirt",
            "tops",
            "chest",
            {},
            "assets/meshes/tshirt.obj",
            "assets/textures/tshirt_diffuse.png",
            "assets/textures/tshirt_normal.png",
            {"male", "female"},
            false,
            true
        });
        
        items_.push_back({
            "jeans_basic",
            "Basic Jeans",
            "bottoms",
            "legs",
            {},
            "assets/meshes/jeans.obj",
            "assets/textures/jeans_diffuse.png",
            "assets/textures/jeans_normal.png",
            {"male", "female"},
            false,
            true
        });
        
        // Build assets list
        for (const auto& item : items_) {
            PluginAsset asset;
            asset.id = item.id;
            asset.name = item.name;
            asset.category = item.category;
            asset.meshPath = item.meshPath;
            asset.texturePath = item.texturePath;
            assets_.push_back(asset);
        }
        
        return true;
    }
    
    void shutdown() override {
        items_.clear();
        assets_.clear();
    }
    
    std::vector<PluginAsset> getAssets() const override { return assets_; }
    
    const PluginAsset* getAsset(const std::string& assetId) const override {
        for (const auto& asset : assets_) {
            if (asset.id == assetId) return &asset;
        }
        return nullptr;
    }
    
    std::vector<ClothingPluginItem> getClothingItems() const override { return items_; }
    
    const ClothingPluginItem* getClothingItem(const std::string& id) const override {
        for (const auto& item : items_) {
            if (item.id == id) return &item;
        }
        return nullptr;
    }
    
    std::shared_ptr<Mesh> loadClothingMesh(
        const std::string& itemId,
        float bodyHeight,
        float bodyWeight) override
    {
        // In a real implementation, load and adjust mesh based on body params
        // For now, return empty mesh
        return std::make_shared<Mesh>();
    }
    
private:
    PluginMetadata metadata_;
    std::vector<ClothingPluginItem> items_;
    std::vector<PluginAsset> assets_;
};

// ============================================================================
// Example 2: Hair Style Pack Plugin
// ============================================================================

class ExampleHairPlugin : public IHairPlugin {
public:
    ExampleHairPlugin() {
        metadata_.id = "com.example.basic-hair";
        metadata_.name = "Basic Hair Pack";
        metadata_.description = "A collection of basic hairstyles";
        metadata_.author = "LUMA Team";
        metadata_.version = {1, 0, 0};
        metadata_.type = PluginType::Hair;
        metadata_.tags = {"hair", "basic", "starter"};
    }
    
    const PluginMetadata& getMetadata() const override { return metadata_; }
    
    bool initialize() override {
        styles_.push_back({
            "short_buzz",
            "Buzz Cut",
            "short",
            "assets/meshes/hair_buzz.obj",
            "assets/textures/hair_buzz.png",
            {0.15f, 0.1f, 0.05f},
            true,
            false
        });
        
        styles_.push_back({
            "medium_wavy",
            "Medium Wavy",
            "medium",
            "assets/meshes/hair_wavy.obj",
            "assets/textures/hair_wavy.png",
            {0.3f, 0.2f, 0.1f},
            true,
            true
        });
        
        styles_.push_back({
            "long_straight",
            "Long Straight",
            "long",
            "assets/meshes/hair_long.obj",
            "assets/textures/hair_long.png",
            {0.2f, 0.15f, 0.08f},
            true,
            true
        });
        
        // Build assets
        for (const auto& style : styles_) {
            PluginAsset asset;
            asset.id = style.id;
            asset.name = style.name;
            asset.category = style.category;
            asset.meshPath = style.meshPath;
            asset.texturePath = style.texturePath;
            assets_.push_back(asset);
        }
        
        return true;
    }
    
    void shutdown() override {
        styles_.clear();
        assets_.clear();
    }
    
    std::vector<PluginAsset> getAssets() const override { return assets_; }
    
    const PluginAsset* getAsset(const std::string& assetId) const override {
        for (const auto& asset : assets_) {
            if (asset.id == assetId) return &asset;
        }
        return nullptr;
    }
    
    std::vector<HairPluginStyle> getHairStyles() const override { return styles_; }
    
    const HairPluginStyle* getHairStyle(const std::string& id) const override {
        for (const auto& style : styles_) {
            if (style.id == id) return &style;
        }
        return nullptr;
    }
    
    std::shared_ptr<Mesh> loadHairMesh(
        const std::string& styleId,
        const Vec3& color) override
    {
        // In real implementation, load mesh and apply color
        return std::make_shared<Mesh>();
    }
    
private:
    PluginMetadata metadata_;
    std::vector<HairPluginStyle> styles_;
    std::vector<PluginAsset> assets_;
};

// ============================================================================
// Example 3: Custom Character Template Plugin
// ============================================================================

class ExampleRobotTemplatePlugin : public ICharacterTemplatePlugin {
public:
    ExampleRobotTemplatePlugin() {
        metadata_.id = "com.example.robot-character";
        metadata_.name = "Robot Character Template";
        metadata_.description = "Create customizable robot characters";
        metadata_.author = "LUMA Team";
        metadata_.version = {1, 0, 0};
        metadata_.type = PluginType::CharacterTemplate;
        metadata_.tags = {"robot", "sci-fi", "mechanical"};
    }
    
    const PluginMetadata& getMetadata() const override { return metadata_; }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    
    std::vector<PluginAsset> getAssets() const override {
        PluginAsset asset;
        asset.id = "robot_template";
        asset.name = "Robot";
        asset.description = "Customizable robot character";
        asset.category = "character_template";
        return {asset};
    }
    
    const PluginAsset* getAsset(const std::string& assetId) const override {
        if (assetId == "robot_template") {
            static PluginAsset asset;
            asset.id = "robot_template";
            asset.name = "Robot";
            return &asset;
        }
        return nullptr;
    }
    
    CharacterTemplatePluginResult createCharacter(
        const CharacterTemplatePluginParams& params) override
    {
        CharacterTemplatePluginResult result;
        
        // Create robot mesh (simplified)
        result.mesh = std::make_shared<Mesh>();
        result.skeleton = std::make_shared<Skeleton>();
        
        // Build robot skeleton
        int root = result.skeleton->addBone("root", -1);
        int body = result.skeleton->addBone("body", root);
        int head = result.skeleton->addBone("head", body);
        
        // Arms
        result.skeleton->addBone("arm_L", body);
        result.skeleton->addBone("arm_R", body);
        
        // Legs  
        result.skeleton->addBone("leg_L", body);
        result.skeleton->addBone("leg_R", body);
        
        // TODO: Generate actual robot mesh based on params
        
        result.success = true;
        return result;
    }
    
    std::vector<std::string> getCustomizableParams() const override {
        return {
            "metalness",
            "rustLevel",
            "eyeGlow",
            "antennaLength",
            "armLength",
            "legLength"
        };
    }
    
    std::pair<float, float> getParamRange(const std::string& param) const override {
        if (param == "rustLevel") return {0.0f, 1.0f};
        if (param == "eyeGlow") return {0.0f, 2.0f};
        if (param == "antennaLength") return {0.0f, 0.5f};
        return {0.0f, 1.0f};
    }
    
    CharacterTemplatePluginParams getDefaultParams() const override {
        CharacterTemplatePluginParams params;
        params.height = 1.8f;
        params.primaryColor = Vec3(0.7f, 0.7f, 0.7f);  // Silver
        params.secondaryColor = Vec3(0.2f, 0.2f, 0.25f);  // Dark metal
        params.accentColor = Vec3(0.0f, 0.8f, 1.0f);  // Cyan glow
        params.customParams["metalness"] = 0.9f;
        params.customParams["rustLevel"] = 0.0f;
        params.customParams["eyeGlow"] = 1.0f;
        return params;
    }
    
private:
    PluginMetadata metadata_;
};

// ============================================================================
// Register Example Plugins (for testing)
// ============================================================================

inline void registerExamplePlugins() {
    auto& factory = PluginFactory::getInstance();
    
    factory.registerFactory("com.example.basic-clothing", 
        []() { return std::make_shared<ExampleClothingPlugin>(); });
    
    factory.registerFactory("com.example.basic-hair",
        []() { return std::make_shared<ExampleHairPlugin>(); });
    
    factory.registerFactory("com.example.robot-character",
        []() { return std::make_shared<ExampleRobotTemplatePlugin>(); });
}

}  // namespace luma
