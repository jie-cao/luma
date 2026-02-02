// Character System - Unified character management (face + body + clothing + animation)
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character_body.h"
#include "engine/character/character_face.h"
#include "engine/animation/skeleton.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace luma {

// ============================================================================
// Forward Declarations
// ============================================================================

class CharacterClothing;
class CharacterAnimation;

// ============================================================================
// Character Style
// ============================================================================

enum class CharacterStyle {
    Realistic,       // Photorealistic style
    Stylized,        // Stylized but still human proportions
    Anime,           // Anime/manga style
    Cartoon,         // Western cartoon style
    Chibi,           // Super-deformed cute style
    Custom
};

// ============================================================================
// Character Export Format
// ============================================================================

enum class CharacterExportFormat {
    GLTF,            // glTF 2.0 (.gltf/.glb)
    FBX,             // Autodesk FBX
    OBJ,             // Wavefront OBJ (mesh only)
    USD,             // Universal Scene Description
    VRM,             // VRM (for VTuber/avatar use)
    LUMA             // Native LUMA format
};

// ============================================================================
// Character Clothing Item
// ============================================================================

struct ClothingItem {
    std::string id;                      // Unique identifier
    std::string name;                    // Display name
    std::string category;                // "top", "bottom", "shoes", "accessory", etc.
    std::string slot;                    // Specific slot (e.g., "shirt", "jacket")
    int layer = 0;                       // Layer for multi-layer clothing
    
    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool hasSkeleton = true;             // Usually rigged to character skeleton
    std::vector<SkinnedVertex> skinnedVertices;
    
    // Material
    Vec3 baseColor{1.0f, 1.0f, 1.0f};
    std::string diffuseTexturePath;
    std::string normalTexturePath;
    float roughness = 0.5f;
    float metallic = 0.0f;
    
    // Customization
    bool colorAdjustable = true;         // Can user change color?
    bool patternAdjustable = false;      // Can user change pattern?
    
    // Body adaptation (blend shapes to fit different body types)
    BlendShapeMesh adaptationBlendShapes;
    
    // Physics (for cloth simulation)
    bool hasPhysics = false;
    float mass = 1.0f;
    float stiffness = 0.5f;
};

// ============================================================================
// Character Clothing Manager
// ============================================================================

class CharacterClothing {
public:
    // Equip/unequip
    void equipItem(const std::string& itemId) {
        equippedItems_.insert(itemId);
        updateVisibility();
    }
    
    void unequipItem(const std::string& itemId) {
        equippedItems_.erase(itemId);
        updateVisibility();
    }
    
    bool isEquipped(const std::string& itemId) const {
        return equippedItems_.find(itemId) != equippedItems_.end();
    }
    
    const std::unordered_set<std::string>& getEquippedItems() const {
        return equippedItems_;
    }
    
    // Item library
    void addItemToLibrary(const ClothingItem& item) {
        itemLibrary_[item.id] = item;
        categoryIndex_[item.category].push_back(item.id);
    }
    
    const ClothingItem* getItem(const std::string& itemId) const {
        auto it = itemLibrary_.find(itemId);
        return (it != itemLibrary_.end()) ? &it->second : nullptr;
    }
    
    std::vector<const ClothingItem*> getItemsByCategory(const std::string& category) const {
        std::vector<const ClothingItem*> result;
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            for (const auto& id : it->second) {
                auto itemIt = itemLibrary_.find(id);
                if (itemIt != itemLibrary_.end()) {
                    result.push_back(&itemIt->second);
                }
            }
        }
        return result;
    }
    
    std::vector<std::string> getCategories() const {
        std::vector<std::string> cats;
        for (const auto& [cat, _] : categoryIndex_) {
            cats.push_back(cat);
        }
        return cats;
    }
    
    // Color customization
    void setItemColor(const std::string& itemId, const Vec3& color) {
        auto it = itemLibrary_.find(itemId);
        if (it != itemLibrary_.end() && it->second.colorAdjustable) {
            it->second.baseColor = color;
            dirty_ = true;
        }
    }
    
    // State
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    
private:
    std::unordered_map<std::string, ClothingItem> itemLibrary_;
    std::unordered_map<std::string, std::vector<std::string>> categoryIndex_;
    std::unordered_set<std::string> equippedItems_;
    bool dirty_ = false;
    
    void updateVisibility() {
        // Handle layer conflicts, etc.
        dirty_ = true;
    }
};

// ============================================================================
// Character Animation State
// ============================================================================

struct CharacterAnimationState {
    std::string currentPose = "t_pose";      // Current pose name
    std::string currentAnimation = "";        // Currently playing animation
    float animationTime = 0.0f;
    float animationSpeed = 1.0f;
    bool animationLooping = true;
    
    // Blend between poses/animations
    std::string blendFromPose;
    float blendWeight = 0.0f;
    float blendDuration = 0.3f;
};

// ============================================================================
// Character - Main unified character class
// ============================================================================

class Character {
public:
    Character() = default;
    explicit Character(const std::string& name) : name_(name) {}
    
    // === Basic Info ===
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    CharacterStyle getStyle() const { return style_; }
    void setStyle(CharacterStyle style) { style_ = style; }
    
    // === Face ===
    
    CharacterFace& getFace() { return face_; }
    const CharacterFace& getFace() const { return face_; }
    
    // === Body ===
    
    CharacterBody& getBody() { return body_; }
    const CharacterBody& getBody() const { return body_; }
    
    // === Clothing ===
    
    CharacterClothing& getClothing() { return clothing_; }
    const CharacterClothing& getClothing() const { return clothing_; }
    
    // === Animation ===
    
    CharacterAnimationState& getAnimationState() { return animationState_; }
    const CharacterAnimationState& getAnimationState() const { return animationState_; }
    
    void setPose(const std::string& poseName) {
        animationState_.currentPose = poseName;
        // Apply pose to skeleton
        applyPose(poseName);
    }
    
    void playAnimation(const std::string& animName, bool loop = true) {
        animationState_.currentAnimation = animName;
        animationState_.animationTime = 0.0f;
        animationState_.animationLooping = loop;
    }
    
    void stopAnimation() {
        animationState_.currentAnimation = "";
        animationState_.animationTime = 0.0f;
    }
    
    // === Skeleton ===
    
    Skeleton& getSkeleton() { return skeleton_; }
    const Skeleton& getSkeleton() const { return skeleton_; }
    
    void initializeStandardSkeleton() {
        skeleton_ = Skeleton();
        
        // Create standard humanoid skeleton
        int root = skeleton_.addBone("Root", -1);
        int hips = skeleton_.addBone("Hips", root);
        int spine = skeleton_.addBone("Spine", hips);
        int spine1 = skeleton_.addBone("Spine1", spine);
        int spine2 = skeleton_.addBone("Spine2", spine1);
        int neck = skeleton_.addBone("Neck", spine2);
        int head = skeleton_.addBone("Head", neck);
        
        // Left arm
        int leftShoulder = skeleton_.addBone("LeftShoulder", spine2);
        int leftArm = skeleton_.addBone("LeftArm", leftShoulder);
        int leftForeArm = skeleton_.addBone("LeftForeArm", leftArm);
        int leftHand = skeleton_.addBone("LeftHand", leftForeArm);
        
        // Right arm
        int rightShoulder = skeleton_.addBone("RightShoulder", spine2);
        int rightArm = skeleton_.addBone("RightArm", rightShoulder);
        int rightForeArm = skeleton_.addBone("RightForeArm", rightArm);
        int rightHand = skeleton_.addBone("RightHand", rightForeArm);
        
        // Left leg
        int leftUpLeg = skeleton_.addBone("LeftUpLeg", hips);
        int leftLeg = skeleton_.addBone("LeftLeg", leftUpLeg);
        int leftFoot = skeleton_.addBone("LeftFoot", leftLeg);
        int leftToeBase = skeleton_.addBone("LeftToeBase", leftFoot);
        
        // Right leg
        int rightUpLeg = skeleton_.addBone("RightUpLeg", hips);
        int rightLeg = skeleton_.addBone("RightLeg", rightUpLeg);
        int rightFoot = skeleton_.addBone("RightFoot", rightLeg);
        int rightToeBase = skeleton_.addBone("RightToeBase", rightFoot);
        
        // Face bones (simplified)
        int leftEye = skeleton_.addBone("LeftEye", head);
        int rightEye = skeleton_.addBone("RightEye", head);
        int jaw = skeleton_.addBone("Jaw", head);
        
        // Fingers (simplified - just one bone per finger for now)
        for (const char* side : {"Left", "Right"}) {
            int hand = skeleton_.findBoneByName(std::string(side) + "Hand");
            for (const char* finger : {"Thumb", "Index", "Middle", "Ring", "Pinky"}) {
                skeleton_.addBone(std::string(side) + finger, hand);
            }
        }
        
        (void)leftToeBase;  // Suppress unused variable warning
        (void)rightToeBase;
        (void)leftEye;
        (void)rightEye;
        (void)jaw;
    }
    
    // === BlendShape Mesh ===
    
    BlendShapeMesh& getBlendShapeMesh() { return blendShapeMesh_; }
    const BlendShapeMesh& getBlendShapeMesh() const { return blendShapeMesh_; }
    
    // Connect face and body to the shared blend shape mesh
    void connectBlendShapes() {
        face_.setBlendShapeMesh(&blendShapeMesh_);
        body_.setBlendShapeMesh(&blendShapeMesh_);
        
        face_.setupDefaultMappings();
        body_.setupDefaultMappings();
    }
    
    // === Mesh Data ===
    
    // Base mesh (before blend shapes)
    void setBaseMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        baseVertices_ = vertices;
        indices_ = indices;
    }
    
    void setBaseSkinnedMesh(const std::vector<SkinnedVertex>& vertices, const std::vector<uint32_t>& indices) {
        baseSkinnedVertices_ = vertices;
        indices_ = indices;
        hasSkeleton_ = true;
    }
    
    const std::vector<Vertex>& getBaseVertices() const { return baseVertices_; }
    const std::vector<SkinnedVertex>& getBaseSkinnedVertices() const { return baseSkinnedVertices_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    bool hasSkeleton() const { return hasSkeleton_; }
    
    // Get deformed mesh (with blend shapes applied)
    void getDeformedVertices(std::vector<Vertex>& outVertices) const {
        blendShapeMesh_.applyToMesh(baseVertices_, outVertices);
    }
    
    // === Update ===
    
    void update(float deltaTime) {
        // Update animation
        if (!animationState_.currentAnimation.empty()) {
            animationState_.animationTime += deltaTime * animationState_.animationSpeed;
            // Apply animation to skeleton...
        }
        
        // Update blend shape weights
        face_.getShapeParams();  // Trigger weight update if needed
        body_.getParams();       // Trigger weight update if needed
    }
    
    // === Neck Integration ===
    
    // Get parameters for seamless face-body connection at the neck
    struct NeckIntegrationParams {
        int neckRingStartVertex = 0;     // First vertex of neck ring
        int neckRingVertexCount = 32;    // Number of vertices in neck ring
        float blendZoneSize = 0.05f;     // Size of blend zone in model units
    };
    
    void setNeckIntegrationParams(const NeckIntegrationParams& params) {
        neckParams_ = params;
    }
    
    const NeckIntegrationParams& getNeckIntegrationParams() const {
        return neckParams_;
    }
    
    // Ensure skin color continuity at neck
    void matchSkinColors() {
        Vec3 faceSkin = face_.getTextureParams().skinTone;
        body_.getParams().skinColor = faceSkin;
    }
    
    // === Export ===
    
    bool exportTo(const std::string& path, CharacterExportFormat format) const {
        // Implementation would handle different export formats
        // For now, just a placeholder
        switch (format) {
            case CharacterExportFormat::GLTF:
                return exportToGLTF(path);
            case CharacterExportFormat::FBX:
                return exportToFBX(path);
            case CharacterExportFormat::VRM:
                return exportToVRM(path);
            default:
                return false;
        }
    }
    
    // === Serialization ===
    
    void serialize(std::unordered_map<std::string, std::string>& outData) const {
        outData["name"] = name_;
        outData["style"] = std::to_string(static_cast<int>(style_));
        
        // Serialize face
        std::unordered_map<std::string, float> faceData;
        face_.serialize(faceData);
        for (const auto& [key, value] : faceData) {
            outData["face_" + key] = std::to_string(value);
        }
        
        // Serialize body
        std::unordered_map<std::string, float> bodyData;
        body_.serialize(bodyData);
        for (const auto& [key, value] : bodyData) {
            outData["body_" + key] = std::to_string(value);
        }
        
        // Serialize equipped clothing
        std::string clothingStr;
        for (const auto& item : clothing_.getEquippedItems()) {
            if (!clothingStr.empty()) clothingStr += ",";
            clothingStr += item;
        }
        outData["equipped_clothing"] = clothingStr;
    }
    
    void deserialize(const std::unordered_map<std::string, std::string>& data) {
        auto getString = [&](const std::string& key, const std::string& def) {
            auto it = data.find(key);
            return (it != data.end()) ? it->second : def;
        };
        
        auto getFloat = [&](const std::string& key, float def) {
            auto it = data.find(key);
            return (it != data.end()) ? std::stof(it->second) : def;
        };
        
        name_ = getString("name", "Character");
        style_ = static_cast<CharacterStyle>(static_cast<int>(getFloat("style", 0)));
        
        // Deserialize face
        std::unordered_map<std::string, float> faceData;
        for (const auto& [key, value] : data) {
            if (key.find("face_") == 0) {
                faceData[key.substr(5)] = std::stof(value);
            }
        }
        face_.deserialize(faceData);
        
        // Deserialize body
        std::unordered_map<std::string, float> bodyData;
        for (const auto& [key, value] : data) {
            if (key.find("body_") == 0) {
                bodyData[key.substr(5)] = std::stof(value);
            }
        }
        body_.deserialize(bodyData);
    }
    
    // === Presets ===
    
    void applyPreset(const std::string& presetName) {
        // Apply face preset
        // Apply body preset
        // Apply default clothing
        presetName_ = presetName;
    }
    
    // === Random Generation ===
    
    void randomize(unsigned int seed = 0) {
        if (seed == 0) seed = static_cast<unsigned int>(time(nullptr));
        srand(seed);
        
        auto randFloat = []() { return static_cast<float>(rand()) / RAND_MAX; };
        auto randRange = [&](float min, float max) { return min + randFloat() * (max - min); };
        
        // Randomize body
        auto& bodyParams = body_.getParams();
        bodyParams.gender = (rand() % 2 == 0) ? Gender::Male : Gender::Female;
        bodyParams.measurements.height = randRange(0.3f, 0.7f);
        bodyParams.measurements.weight = randRange(0.3f, 0.7f);
        bodyParams.measurements.muscularity = randRange(0.1f, 0.6f);
        
        // Randomize face
        auto& faceParams = face_.getShapeParams();
        faceParams.faceWidth = randRange(0.35f, 0.65f);
        faceParams.faceLength = randRange(0.35f, 0.65f);
        faceParams.eyeSize = randRange(0.4f, 0.6f);
        faceParams.noseLength = randRange(0.35f, 0.65f);
        faceParams.mouthWidth = randRange(0.4f, 0.6f);
        faceParams.jawWidth = randRange(0.35f, 0.65f);
        
        // Randomize skin tone (within realistic range)
        face_.getTextureParams().skinTone = Vec3(
            randRange(0.4f, 0.95f),
            randRange(0.3f, 0.75f),
            randRange(0.2f, 0.6f)
        );
        
        // Randomize eye color
        face_.getTextureParams().eyeColor = Vec3(
            randRange(0.1f, 0.6f),
            randRange(0.1f, 0.5f),
            randRange(0.1f, 0.4f)
        );
        
        body_.updateBlendShapeWeights();
    }
    
private:
    std::string name_ = "Character";
    std::string presetName_;
    CharacterStyle style_ = CharacterStyle::Realistic;
    
    CharacterFace face_;
    CharacterBody body_;
    CharacterClothing clothing_;
    CharacterAnimationState animationState_;
    
    Skeleton skeleton_;
    BlendShapeMesh blendShapeMesh_;
    
    // Mesh data
    std::vector<Vertex> baseVertices_;
    std::vector<SkinnedVertex> baseSkinnedVertices_;
    std::vector<uint32_t> indices_;
    bool hasSkeleton_ = false;
    
    NeckIntegrationParams neckParams_;
    
    void applyPose(const std::string& poseName) {
        if (poseName == "t_pose") {
            skeleton_.resetToBindPose();
        }
        // Other poses would be loaded from a pose library
    }
    
    bool exportToGLTF(const std::string& path) const {
        // TODO: Implement glTF export using tinygltf
        (void)path;
        return false;
    }
    
    bool exportToFBX(const std::string& path) const {
        // TODO: Implement FBX export
        (void)path;
        return false;
    }
    
    bool exportToVRM(const std::string& path) const {
        // TODO: Implement VRM export
        (void)path;
        return false;
    }
};

// ============================================================================
// Character Factory - Create characters from templates
// ============================================================================

class CharacterFactory {
public:
    using CharacterPtr = std::unique_ptr<Character>;
    
    // Create a blank character
    static CharacterPtr createBlank(const std::string& name = "New Character") {
        auto character = std::make_unique<Character>(name);
        character->initializeStandardSkeleton();
        character->connectBlendShapes();
        return character;
    }
    
    // Create from preset
    static CharacterPtr createFromPreset(const std::string& presetName) {
        auto character = createBlank(presetName);
        character->applyPreset(presetName);
        return character;
    }
    
    // Create random character
    static CharacterPtr createRandom(unsigned int seed = 0) {
        auto character = createBlank("Random Character");
        character->randomize(seed);
        return character;
    }
    
    // Create from photo (placeholder - requires AI pipeline)
    static CharacterPtr createFromPhoto(const std::string& photoPath) {
        auto character = createBlank("Photo Character");
        // TODO: Run AI pipeline to extract face
        // PhotoFaceResult result = AIFacePipeline::processPhoto(photoPath);
        // character->getFace().applyPhotoFaceResult(result);
        (void)photoPath;
        return character;
    }
    
    // Clone character
    static CharacterPtr clone(const Character& source) {
        auto character = std::make_unique<Character>();
        
        // Serialize and deserialize to clone
        std::unordered_map<std::string, std::string> data;
        source.serialize(data);
        character->deserialize(data);
        
        character->initializeStandardSkeleton();
        character->connectBlendShapes();
        
        return character;
    }
};

// ============================================================================
// Character Manager - Manages multiple characters
// ============================================================================

class CharacterManager {
public:
    using CharacterPtr = std::unique_ptr<Character>;
    
    // Add character
    void addCharacter(CharacterPtr character) {
        const std::string& name = character->getName();
        characters_[name] = std::move(character);
    }
    
    // Get character
    Character* getCharacter(const std::string& name) {
        auto it = characters_.find(name);
        return (it != characters_.end()) ? it->second.get() : nullptr;
    }
    
    // Remove character
    void removeCharacter(const std::string& name) {
        characters_.erase(name);
    }
    
    // Get all character names
    std::vector<std::string> getCharacterNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : characters_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Update all characters
    void update(float deltaTime) {
        for (auto& [name, character] : characters_) {
            character->update(deltaTime);
        }
    }
    
    // Save all characters
    void saveAll(const std::string& directory) {
        // TODO: Save each character to a file
        (void)directory;
    }
    
    // Load all characters from directory
    void loadAll(const std::string& directory) {
        // TODO: Load all character files from directory
        (void)directory;
    }
    
private:
    std::unordered_map<std::string, CharacterPtr> characters_;
};

} // namespace luma
