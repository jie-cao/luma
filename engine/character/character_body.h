// Character Body System - Parametric body customization
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace luma {

// ============================================================================
// Enums
// ============================================================================

enum class Gender {
    Male,
    Female,
    Neutral      // Androgynous / non-binary
};

enum class AgeGroup {
    Child,       // 5-12
    Teen,        // 13-17
    YoungAdult,  // 18-30
    Adult,       // 31-50
    Senior       // 51+
};

enum class BodyPreset {
    // Male presets
    MaleSlim,
    MaleAverage,
    MaleMuscular,
    MaleHeavy,
    MaleElderly,
    
    // Female presets
    FemaleSlim,
    FemaleAverage,
    FemaleCurvy,
    FemaleAthletic,
    FemaleElderly,
    
    // Child presets
    ChildToddler,
    ChildYoung,
    ChildTeen,
    
    // Special
    Custom
};

// ============================================================================
// Body Measurements (in normalized units, 0.0 to 1.0)
// ============================================================================

struct BodyMeasurements {
    // Overall
    float height = 0.5f;          // Overall height (0=short, 1=tall)
    float weight = 0.5f;          // Overall mass (0=thin, 1=heavy)
    float muscularity = 0.3f;     // Muscle definition (0=soft, 1=muscular)
    float bodyFat = 0.3f;         // Body fat percentage (0=lean, 1=high)
    
    // Torso
    float shoulderWidth = 0.5f;   // Shoulder breadth
    float chestSize = 0.5f;       // Chest circumference
    float waistSize = 0.5f;       // Waist circumference
    float hipWidth = 0.5f;        // Hip width
    float torsoLength = 0.5f;     // Torso proportion
    
    // Arms
    float armLength = 0.5f;       // Arm length relative to body
    float armThickness = 0.5f;    // Upper arm thickness
    float forearmThickness = 0.5f;// Forearm thickness
    float handSize = 0.5f;        // Hand scale
    
    // Legs
    float legLength = 0.5f;       // Leg length relative to body
    float thighThickness = 0.5f;  // Thigh circumference
    float calfThickness = 0.5f;   // Calf circumference
    float footSize = 0.5f;        // Foot scale
    
    // Gender-specific
    float bustSize = 0.5f;        // Bust size (primarily female)
    float neckThickness = 0.5f;   // Neck circumference
    
    // Apply from preset
    void applyPreset(BodyPreset preset);
    
    // Interpolate between two measurements
    static BodyMeasurements lerp(const BodyMeasurements& a, 
                                  const BodyMeasurements& b, 
                                  float t) {
        BodyMeasurements result;
        result.height = a.height + (b.height - a.height) * t;
        result.weight = a.weight + (b.weight - a.weight) * t;
        result.muscularity = a.muscularity + (b.muscularity - a.muscularity) * t;
        result.bodyFat = a.bodyFat + (b.bodyFat - a.bodyFat) * t;
        result.shoulderWidth = a.shoulderWidth + (b.shoulderWidth - a.shoulderWidth) * t;
        result.chestSize = a.chestSize + (b.chestSize - a.chestSize) * t;
        result.waistSize = a.waistSize + (b.waistSize - a.waistSize) * t;
        result.hipWidth = a.hipWidth + (b.hipWidth - a.hipWidth) * t;
        result.torsoLength = a.torsoLength + (b.torsoLength - a.torsoLength) * t;
        result.armLength = a.armLength + (b.armLength - a.armLength) * t;
        result.armThickness = a.armThickness + (b.armThickness - a.armThickness) * t;
        result.forearmThickness = a.forearmThickness + (b.forearmThickness - a.forearmThickness) * t;
        result.handSize = a.handSize + (b.handSize - a.handSize) * t;
        result.legLength = a.legLength + (b.legLength - a.legLength) * t;
        result.thighThickness = a.thighThickness + (b.thighThickness - a.thighThickness) * t;
        result.calfThickness = a.calfThickness + (b.calfThickness - a.calfThickness) * t;
        result.footSize = a.footSize + (b.footSize - a.footSize) * t;
        result.bustSize = a.bustSize + (b.bustSize - a.bustSize) * t;
        result.neckThickness = a.neckThickness + (b.neckThickness - a.neckThickness) * t;
        return result;
    }
};

// Preset implementations
inline void BodyMeasurements::applyPreset(BodyPreset preset) {
    switch (preset) {
        case BodyPreset::MaleSlim:
            height = 0.55f; weight = 0.25f; muscularity = 0.3f; bodyFat = 0.15f;
            shoulderWidth = 0.5f; chestSize = 0.35f; waistSize = 0.3f; hipWidth = 0.4f;
            armThickness = 0.3f; legLength = 0.55f; thighThickness = 0.35f;
            break;
            
        case BodyPreset::MaleAverage:
            height = 0.5f; weight = 0.5f; muscularity = 0.4f; bodyFat = 0.35f;
            shoulderWidth = 0.55f; chestSize = 0.5f; waistSize = 0.45f; hipWidth = 0.45f;
            armThickness = 0.45f; legLength = 0.5f; thighThickness = 0.45f;
            break;
            
        case BodyPreset::MaleMuscular:
            height = 0.55f; weight = 0.65f; muscularity = 0.85f; bodyFat = 0.2f;
            shoulderWidth = 0.75f; chestSize = 0.75f; waistSize = 0.45f; hipWidth = 0.55f;
            armThickness = 0.75f; legLength = 0.5f; thighThickness = 0.65f;
            neckThickness = 0.7f;
            break;
            
        case BodyPreset::MaleHeavy:
            height = 0.5f; weight = 0.8f; muscularity = 0.35f; bodyFat = 0.75f;
            shoulderWidth = 0.6f; chestSize = 0.7f; waistSize = 0.8f; hipWidth = 0.65f;
            armThickness = 0.6f; legLength = 0.45f; thighThickness = 0.7f;
            break;
            
        case BodyPreset::MaleElderly:
            height = 0.45f; weight = 0.45f; muscularity = 0.2f; bodyFat = 0.4f;
            shoulderWidth = 0.45f; chestSize = 0.45f; waistSize = 0.5f; hipWidth = 0.45f;
            armThickness = 0.35f; legLength = 0.45f; thighThickness = 0.4f;
            break;
            
        case BodyPreset::FemaleSlim:
            height = 0.45f; weight = 0.2f; muscularity = 0.15f; bodyFat = 0.25f;
            shoulderWidth = 0.35f; chestSize = 0.3f; waistSize = 0.25f; hipWidth = 0.45f;
            armThickness = 0.25f; legLength = 0.55f; thighThickness = 0.35f;
            bustSize = 0.3f;
            break;
            
        case BodyPreset::FemaleAverage:
            height = 0.45f; weight = 0.45f; muscularity = 0.2f; bodyFat = 0.4f;
            shoulderWidth = 0.4f; chestSize = 0.45f; waistSize = 0.4f; hipWidth = 0.55f;
            armThickness = 0.35f; legLength = 0.5f; thighThickness = 0.5f;
            bustSize = 0.5f;
            break;
            
        case BodyPreset::FemaleCurvy:
            height = 0.45f; weight = 0.55f; muscularity = 0.15f; bodyFat = 0.5f;
            shoulderWidth = 0.4f; chestSize = 0.55f; waistSize = 0.45f; hipWidth = 0.7f;
            armThickness = 0.4f; legLength = 0.5f; thighThickness = 0.6f;
            bustSize = 0.7f;
            break;
            
        case BodyPreset::FemaleAthletic:
            height = 0.5f; weight = 0.45f; muscularity = 0.55f; bodyFat = 0.2f;
            shoulderWidth = 0.5f; chestSize = 0.45f; waistSize = 0.35f; hipWidth = 0.5f;
            armThickness = 0.45f; legLength = 0.55f; thighThickness = 0.5f;
            bustSize = 0.4f;
            break;
            
        case BodyPreset::FemaleElderly:
            height = 0.4f; weight = 0.5f; muscularity = 0.1f; bodyFat = 0.5f;
            shoulderWidth = 0.4f; chestSize = 0.5f; waistSize = 0.55f; hipWidth = 0.55f;
            armThickness = 0.35f; legLength = 0.45f; thighThickness = 0.5f;
            bustSize = 0.45f;
            break;
            
        case BodyPreset::ChildToddler:
            height = 0.15f; weight = 0.3f; muscularity = 0.05f; bodyFat = 0.45f;
            shoulderWidth = 0.35f; chestSize = 0.4f; waistSize = 0.45f; hipWidth = 0.4f;
            armThickness = 0.35f; legLength = 0.35f; thighThickness = 0.45f;
            torsoLength = 0.55f;  // Children have proportionally longer torsos
            break;
            
        case BodyPreset::ChildYoung:
            height = 0.3f; weight = 0.3f; muscularity = 0.1f; bodyFat = 0.35f;
            shoulderWidth = 0.35f; chestSize = 0.35f; waistSize = 0.35f; hipWidth = 0.35f;
            armThickness = 0.3f; legLength = 0.4f; thighThickness = 0.35f;
            break;
            
        case BodyPreset::ChildTeen:
            height = 0.4f; weight = 0.35f; muscularity = 0.2f; bodyFat = 0.3f;
            shoulderWidth = 0.4f; chestSize = 0.4f; waistSize = 0.35f; hipWidth = 0.4f;
            armThickness = 0.35f; legLength = 0.5f; thighThickness = 0.4f;
            break;
            
        default:  // Custom - keep current values
            break;
    }
}

// ============================================================================
// Body Parameters - High-level character body configuration
// ============================================================================

struct BodyParams {
    Gender gender = Gender::Male;
    AgeGroup ageGroup = AgeGroup::Adult;
    BodyPreset preset = BodyPreset::MaleAverage;
    BodyMeasurements measurements;
    
    // Skin appearance
    Vec3 skinColor{0.85f, 0.65f, 0.5f};   // Base skin tone
    float skinRoughness = 0.5f;            // Skin roughness (PBR)
    float skinSubsurface = 0.3f;           // Subsurface scattering intensity
    
    // Apply preset to measurements
    void applyPreset() {
        measurements.applyPreset(preset);
    }
    
    // Get default preset for gender/age
    static BodyPreset getDefaultPreset(Gender g, AgeGroup age) {
        if (age == AgeGroup::Child) return BodyPreset::ChildYoung;
        if (age == AgeGroup::Teen) return BodyPreset::ChildTeen;
        
        switch (g) {
            case Gender::Male:
                return (age == AgeGroup::Senior) ? BodyPreset::MaleElderly : BodyPreset::MaleAverage;
            case Gender::Female:
                return (age == AgeGroup::Senior) ? BodyPreset::FemaleElderly : BodyPreset::FemaleAverage;
            case Gender::Neutral:
                return BodyPreset::MaleAverage;  // Will be blended
        }
        return BodyPreset::MaleAverage;
    }
};

// ============================================================================
// Body BlendShape Mapping - Maps measurements to blend shape weights
// ============================================================================

struct BodyBlendShapeMapping {
    std::string channelName;           // BlendShape channel name
    
    // Which measurement affects this channel
    enum class MeasurementSource {
        Height,
        Weight,
        Muscularity,
        BodyFat,
        ShoulderWidth,
        ChestSize,
        WaistSize,
        HipWidth,
        TorsoLength,
        ArmLength,
        ArmThickness,
        ForearmThickness,
        HandSize,
        LegLength,
        ThighThickness,
        CalfThickness,
        FootSize,
        BustSize,
        NeckThickness,
        Gender,        // 0=male, 1=female
        Age            // 0=child, 1=senior
    };
    
    MeasurementSource source;
    
    // Mapping curve (piecewise linear)
    // Input: measurement value (0-1)
    // Output: blend shape weight
    std::vector<std::pair<float, float>> curve;  // (input, output) pairs
    
    // Evaluate the mapping
    float evaluate(float inputValue) const {
        if (curve.empty()) return 0.0f;
        if (curve.size() == 1) return curve[0].second;
        
        // Find segment
        for (size_t i = 0; i < curve.size() - 1; i++) {
            if (inputValue >= curve[i].first && inputValue <= curve[i + 1].first) {
                float t = (inputValue - curve[i].first) / 
                         (curve[i + 1].first - curve[i].first);
                return curve[i].second + t * (curve[i + 1].second - curve[i].second);
            }
        }
        
        // Clamp to ends
        if (inputValue < curve.front().first) return curve.front().second;
        return curve.back().second;
    }
    
    // Create linear mapping (0->0, 1->1)
    static BodyBlendShapeMapping linear(const std::string& channel, MeasurementSource src) {
        BodyBlendShapeMapping m;
        m.channelName = channel;
        m.source = src;
        m.curve = {{0.0f, 0.0f}, {1.0f, 1.0f}};
        return m;
    }
    
    // Create inverse mapping (0->1, 1->0)
    static BodyBlendShapeMapping inverse(const std::string& channel, MeasurementSource src) {
        BodyBlendShapeMapping m;
        m.channelName = channel;
        m.source = src;
        m.curve = {{0.0f, 1.0f}, {1.0f, 0.0f}};
        return m;
    }
    
    // Create centered mapping (-1 to 1 from 0 to 1 input)
    static BodyBlendShapeMapping centered(const std::string& channel, MeasurementSource src) {
        BodyBlendShapeMapping m;
        m.channelName = channel;
        m.source = src;
        m.curve = {{0.0f, -1.0f}, {0.5f, 0.0f}, {1.0f, 1.0f}};
        return m;
    }
};

// ============================================================================
// Character Body - Manages body mesh and customization
// ============================================================================

class CharacterBody {
public:
    CharacterBody() = default;
    
    // === Configuration ===
    
    void setParams(const BodyParams& params) {
        params_ = params;
        updateBlendShapeWeights();
    }
    
    const BodyParams& getParams() const { return params_; }
    BodyParams& getParams() { return params_; }
    
    // === Quick setters ===
    
    void setGender(Gender g) {
        params_.gender = g;
        params_.preset = BodyParams::getDefaultPreset(g, params_.ageGroup);
        params_.applyPreset();
        updateBlendShapeWeights();
    }
    
    void setAgeGroup(AgeGroup age) {
        params_.ageGroup = age;
        params_.preset = BodyParams::getDefaultPreset(params_.gender, age);
        params_.applyPreset();
        updateBlendShapeWeights();
    }
    
    void setPreset(BodyPreset preset) {
        params_.preset = preset;
        params_.applyPreset();
        updateBlendShapeWeights();
    }
    
    void setHeight(float h) { params_.measurements.height = h; updateBlendShapeWeights(); }
    void setWeight(float w) { params_.measurements.weight = w; updateBlendShapeWeights(); }
    void setMuscularity(float m) { params_.measurements.muscularity = m; updateBlendShapeWeights(); }
    void setBodyFat(float f) { params_.measurements.bodyFat = f; updateBlendShapeWeights(); }
    
    // === BlendShape Integration ===
    
    void setBlendShapeMesh(BlendShapeMesh* mesh) {
        blendShapeMesh_ = mesh;
        updateBlendShapeWeights();
    }
    
    BlendShapeMesh* getBlendShapeMesh() { return blendShapeMesh_; }
    
    void addMapping(const BodyBlendShapeMapping& mapping) {
        mappings_.push_back(mapping);
    }
    
    void clearMappings() {
        mappings_.clear();
    }
    
    // Setup default mappings for a standard body rig
    void setupDefaultMappings() {
        mappings_.clear();
        
        using Src = BodyBlendShapeMapping::MeasurementSource;
        
        // Height
        addMapping(BodyBlendShapeMapping::centered("body_height", Src::Height));
        addMapping(BodyBlendShapeMapping::linear("leg_length", Src::LegLength));
        addMapping(BodyBlendShapeMapping::linear("torso_length", Src::TorsoLength));
        
        // Weight / Body Fat
        addMapping(BodyBlendShapeMapping::linear("body_fat", Src::BodyFat));
        addMapping(BodyBlendShapeMapping::linear("weight_overall", Src::Weight));
        
        // Muscularity
        addMapping(BodyBlendShapeMapping::linear("muscle_arms", Src::Muscularity));
        addMapping(BodyBlendShapeMapping::linear("muscle_chest", Src::Muscularity));
        addMapping(BodyBlendShapeMapping::linear("muscle_legs", Src::Muscularity));
        addMapping(BodyBlendShapeMapping::linear("muscle_abs", Src::Muscularity));
        
        // Upper body
        addMapping(BodyBlendShapeMapping::centered("shoulder_width", Src::ShoulderWidth));
        addMapping(BodyBlendShapeMapping::linear("chest_size", Src::ChestSize));
        addMapping(BodyBlendShapeMapping::linear("waist_size", Src::WaistSize));
        addMapping(BodyBlendShapeMapping::centered("hip_width", Src::HipWidth));
        
        // Arms
        addMapping(BodyBlendShapeMapping::centered("arm_length", Src::ArmLength));
        addMapping(BodyBlendShapeMapping::linear("arm_thickness", Src::ArmThickness));
        addMapping(BodyBlendShapeMapping::linear("forearm_thickness", Src::ForearmThickness));
        addMapping(BodyBlendShapeMapping::centered("hand_size", Src::HandSize));
        
        // Legs
        addMapping(BodyBlendShapeMapping::centered("leg_length", Src::LegLength));
        addMapping(BodyBlendShapeMapping::linear("thigh_thickness", Src::ThighThickness));
        addMapping(BodyBlendShapeMapping::linear("calf_thickness", Src::CalfThickness));
        addMapping(BodyBlendShapeMapping::centered("foot_size", Src::FootSize));
        
        // Gender-specific
        addMapping(BodyBlendShapeMapping::linear("bust_size", Src::BustSize));
        addMapping(BodyBlendShapeMapping::linear("neck_thickness", Src::NeckThickness));
        
        // Gender morph (for transitioning between male/female base)
        addMapping(BodyBlendShapeMapping::linear("gender_female", Src::Gender));
        
        // Age morphs
        addMapping(BodyBlendShapeMapping::linear("age_elderly", Src::Age));
    }
    
    // Update blend shape weights from current params
    void updateBlendShapeWeights() {
        if (!blendShapeMesh_) return;
        
        for (const auto& mapping : mappings_) {
            float inputValue = getMeasurementValue(mapping.source);
            float weight = mapping.evaluate(inputValue);
            blendShapeMesh_->setWeight(mapping.channelName, weight);
        }
    }
    
    // === Serialization ===
    
    // Save body params to JSON-like structure
    void serialize(std::unordered_map<std::string, float>& outData) const {
        outData["gender"] = static_cast<float>(params_.gender);
        outData["age_group"] = static_cast<float>(params_.ageGroup);
        outData["preset"] = static_cast<float>(params_.preset);
        
        const auto& m = params_.measurements;
        outData["height"] = m.height;
        outData["weight"] = m.weight;
        outData["muscularity"] = m.muscularity;
        outData["body_fat"] = m.bodyFat;
        outData["shoulder_width"] = m.shoulderWidth;
        outData["chest_size"] = m.chestSize;
        outData["waist_size"] = m.waistSize;
        outData["hip_width"] = m.hipWidth;
        outData["torso_length"] = m.torsoLength;
        outData["arm_length"] = m.armLength;
        outData["arm_thickness"] = m.armThickness;
        outData["forearm_thickness"] = m.forearmThickness;
        outData["hand_size"] = m.handSize;
        outData["leg_length"] = m.legLength;
        outData["thigh_thickness"] = m.thighThickness;
        outData["calf_thickness"] = m.calfThickness;
        outData["foot_size"] = m.footSize;
        outData["bust_size"] = m.bustSize;
        outData["neck_thickness"] = m.neckThickness;
        
        outData["skin_r"] = params_.skinColor.x;
        outData["skin_g"] = params_.skinColor.y;
        outData["skin_b"] = params_.skinColor.z;
    }
    
    void deserialize(const std::unordered_map<std::string, float>& data) {
        auto get = [&](const std::string& key, float def) {
            auto it = data.find(key);
            return (it != data.end()) ? it->second : def;
        };
        
        params_.gender = static_cast<Gender>(static_cast<int>(get("gender", 0)));
        params_.ageGroup = static_cast<AgeGroup>(static_cast<int>(get("age_group", 2)));
        params_.preset = static_cast<BodyPreset>(static_cast<int>(get("preset", 1)));
        
        auto& m = params_.measurements;
        m.height = get("height", 0.5f);
        m.weight = get("weight", 0.5f);
        m.muscularity = get("muscularity", 0.3f);
        m.bodyFat = get("body_fat", 0.3f);
        m.shoulderWidth = get("shoulder_width", 0.5f);
        m.chestSize = get("chest_size", 0.5f);
        m.waistSize = get("waist_size", 0.5f);
        m.hipWidth = get("hip_width", 0.5f);
        m.torsoLength = get("torso_length", 0.5f);
        m.armLength = get("arm_length", 0.5f);
        m.armThickness = get("arm_thickness", 0.5f);
        m.forearmThickness = get("forearm_thickness", 0.5f);
        m.handSize = get("hand_size", 0.5f);
        m.legLength = get("leg_length", 0.5f);
        m.thighThickness = get("thigh_thickness", 0.5f);
        m.calfThickness = get("calf_thickness", 0.5f);
        m.footSize = get("foot_size", 0.5f);
        m.bustSize = get("bust_size", 0.5f);
        m.neckThickness = get("neck_thickness", 0.5f);
        
        params_.skinColor.x = get("skin_r", 0.85f);
        params_.skinColor.y = get("skin_g", 0.65f);
        params_.skinColor.z = get("skin_b", 0.5f);
        
        updateBlendShapeWeights();
    }
    
private:
    BodyParams params_;
    BlendShapeMesh* blendShapeMesh_ = nullptr;
    std::vector<BodyBlendShapeMapping> mappings_;
    
    float getMeasurementValue(BodyBlendShapeMapping::MeasurementSource src) const {
        using Src = BodyBlendShapeMapping::MeasurementSource;
        const auto& m = params_.measurements;
        
        switch (src) {
            case Src::Height: return m.height;
            case Src::Weight: return m.weight;
            case Src::Muscularity: return m.muscularity;
            case Src::BodyFat: return m.bodyFat;
            case Src::ShoulderWidth: return m.shoulderWidth;
            case Src::ChestSize: return m.chestSize;
            case Src::WaistSize: return m.waistSize;
            case Src::HipWidth: return m.hipWidth;
            case Src::TorsoLength: return m.torsoLength;
            case Src::ArmLength: return m.armLength;
            case Src::ArmThickness: return m.armThickness;
            case Src::ForearmThickness: return m.forearmThickness;
            case Src::HandSize: return m.handSize;
            case Src::LegLength: return m.legLength;
            case Src::ThighThickness: return m.thighThickness;
            case Src::CalfThickness: return m.calfThickness;
            case Src::FootSize: return m.footSize;
            case Src::BustSize: return m.bustSize;
            case Src::NeckThickness: return m.neckThickness;
            case Src::Gender: return (params_.gender == Gender::Female) ? 1.0f : 0.0f;
            case Src::Age: {
                switch (params_.ageGroup) {
                    case AgeGroup::Child: return 0.0f;
                    case AgeGroup::Teen: return 0.2f;
                    case AgeGroup::YoungAdult: return 0.4f;
                    case AgeGroup::Adult: return 0.6f;
                    case AgeGroup::Senior: return 1.0f;
                }
                return 0.5f;
            }
        }
        return 0.5f;
    }
};

// ============================================================================
// Preset Library - Collection of body presets
// ============================================================================

class BodyPresetLibrary {
public:
    struct PresetEntry {
        std::string name;
        std::string category;
        BodyParams params;
        std::string thumbnailPath;
    };
    
    void addPreset(const PresetEntry& entry) {
        presets_.push_back(entry);
        categoryIndex_[entry.category].push_back(presets_.size() - 1);
    }
    
    const std::vector<PresetEntry>& getAllPresets() const { return presets_; }
    
    std::vector<const PresetEntry*> getPresetsByCategory(const std::string& category) const {
        std::vector<const PresetEntry*> result;
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            for (size_t idx : it->second) {
                result.push_back(&presets_[idx]);
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
    
    const PresetEntry* findPreset(const std::string& name) const {
        for (const auto& p : presets_) {
            if (p.name == name) return &p;
        }
        return nullptr;
    }
    
    // Initialize with default presets
    void initializeDefaults() {
        // Male presets
        addPreset({"Slim Male", "Male", createParams(Gender::Male, BodyPreset::MaleSlim), ""});
        addPreset({"Average Male", "Male", createParams(Gender::Male, BodyPreset::MaleAverage), ""});
        addPreset({"Muscular Male", "Male", createParams(Gender::Male, BodyPreset::MaleMuscular), ""});
        addPreset({"Heavy Male", "Male", createParams(Gender::Male, BodyPreset::MaleHeavy), ""});
        addPreset({"Elderly Male", "Male", createParams(Gender::Male, BodyPreset::MaleElderly), ""});
        
        // Female presets
        addPreset({"Slim Female", "Female", createParams(Gender::Female, BodyPreset::FemaleSlim), ""});
        addPreset({"Average Female", "Female", createParams(Gender::Female, BodyPreset::FemaleAverage), ""});
        addPreset({"Curvy Female", "Female", createParams(Gender::Female, BodyPreset::FemaleCurvy), ""});
        addPreset({"Athletic Female", "Female", createParams(Gender::Female, BodyPreset::FemaleAthletic), ""});
        addPreset({"Elderly Female", "Female", createParams(Gender::Female, BodyPreset::FemaleElderly), ""});
        
        // Child presets
        addPreset({"Toddler", "Child", createParams(Gender::Neutral, BodyPreset::ChildToddler), ""});
        addPreset({"Young Child", "Child", createParams(Gender::Neutral, BodyPreset::ChildYoung), ""});
        addPreset({"Teenager", "Child", createParams(Gender::Neutral, BodyPreset::ChildTeen), ""});
    }
    
private:
    std::vector<PresetEntry> presets_;
    std::unordered_map<std::string, std::vector<size_t>> categoryIndex_;
    
    static BodyParams createParams(Gender g, BodyPreset preset) {
        BodyParams p;
        p.gender = g;
        p.preset = preset;
        p.applyPreset();
        return p;
    }
};

} // namespace luma
