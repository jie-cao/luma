// Character Preset System - Quick-start character templates
// Provides ready-to-use character configurations
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/project/project_file.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <ctime>

namespace luma {

// ============================================================================
// Preset Categories
// ============================================================================

enum class PresetCategory {
    Fantasy,        // 西方奇幻
    Wuxia,          // 武侠风
    Gufeng,         // 中国古风
    Anime,          // 动漫风格
    Cartoon,        // 卡通风格
    SciFi,          // 科幻角色
    Realistic,      // 写实风格
    Custom          // 用户自定义
};

inline std::string presetCategoryToString(PresetCategory cat) {
    switch (cat) {
        case PresetCategory::Fantasy: return "Fantasy";
        case PresetCategory::Wuxia: return "Wuxia";
        case PresetCategory::Gufeng: return "Gufeng";
        case PresetCategory::Anime: return "Anime";
        case PresetCategory::Cartoon: return "Cartoon";
        case PresetCategory::SciFi: return "Sci-Fi";
        case PresetCategory::Realistic: return "Realistic";
        case PresetCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

inline std::string presetCategoryToDisplayName(PresetCategory cat) {
    switch (cat) {
        case PresetCategory::Fantasy: return "西幻 Fantasy";
        case PresetCategory::Wuxia: return "武侠 Wuxia";
        case PresetCategory::Gufeng: return "古风 Gufeng";
        case PresetCategory::Anime: return "动漫 Anime";
        case PresetCategory::Cartoon: return "卡通 Cartoon";
        case PresetCategory::SciFi: return "科幻 Sci-Fi";
        case PresetCategory::Realistic: return "写实 Realistic";
        case PresetCategory::Custom: return "自定义 Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Character Preset
// ============================================================================

struct CharacterPreset {
    // Metadata
    std::string id;
    std::string name;
    std::string nameCN;             // Chinese name
    std::string description;
    std::string descriptionCN;
    PresetCategory category = PresetCategory::Realistic;
    std::vector<std::string> tags;
    std::string thumbnailPath;
    bool isBuiltIn = true;
    
    // Character data
    CharacterProjectData data;
    
    // Preview colors for UI thumbnail generation
    Vec3 previewSkinColor{0.85f, 0.65f, 0.5f};
    Vec3 previewHairColor{0.2f, 0.15f, 0.1f};
    Vec3 previewEyeColor{0.3f, 0.4f, 0.2f};
};

// ============================================================================
// Built-in Presets Generator
// ============================================================================

class BuiltInPresets {
public:
    // === Realistic Presets ===
    
    static CharacterPreset createBusinessMan() {
        CharacterPreset preset;
        preset.id = "realistic_business_man";
        preset.name = "Business Man";
        preset.nameCN = "商务男士";
        preset.description = "Professional male in business attire";
        preset.descriptionCN = "穿着商务装的专业男性";
        preset.category = PresetCategory::Realistic;
        preset.tags = {"male", "adult", "professional", "realistic"};
        
        auto& d = preset.data;
        d.name = "Business Man";
        d.characterType = 0;  // Human
        
        // Body
        d.body.gender = 0;  // Male
        d.body.ageGroup = 3;  // Adult
        d.body.height = 0.6f;
        d.body.weight = 0.5f;
        d.body.muscularity = 0.4f;
        d.body.bodyFat = 0.35f;
        d.body.shoulderWidth = 0.55f;
        d.body.chestSize = 0.5f;
        d.body.skinColor = Vec3(0.85f, 0.7f, 0.6f);
        
        // Face
        d.face.faceWidth = 0.5f;
        d.face.faceLength = 0.55f;
        d.face.jawWidth = 0.55f;
        d.face.eyeColor = Vec3(0.35f, 0.25f, 0.15f);  // Brown eyes
        
        // Hair
        d.hair.styleId = "short_business";
        d.hair.colorPreset = 2;  // Dark brown
        d.hair.customColor = Vec3(0.15f, 0.1f, 0.05f);
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createBusinessWoman() {
        CharacterPreset preset;
        preset.id = "realistic_business_woman";
        preset.name = "Business Woman";
        preset.nameCN = "商务女士";
        preset.description = "Professional female in business attire";
        preset.descriptionCN = "穿着商务装的专业女性";
        preset.category = PresetCategory::Realistic;
        preset.tags = {"female", "adult", "professional", "realistic"};
        
        auto& d = preset.data;
        d.name = "Business Woman";
        d.characterType = 0;
        
        d.body.gender = 1;  // Female
        d.body.ageGroup = 3;
        d.body.height = 0.45f;
        d.body.weight = 0.4f;
        d.body.muscularity = 0.2f;
        d.body.shoulderWidth = 0.4f;
        d.body.hipWidth = 0.55f;
        d.body.bustSize = 0.5f;
        d.body.skinColor = Vec3(0.9f, 0.75f, 0.65f);
        
        d.face.faceWidth = 0.45f;
        d.face.faceRoundness = 0.55f;
        d.face.eyeSize = 0.55f;
        d.face.eyeColor = Vec3(0.3f, 0.4f, 0.25f);  // Hazel
        
        d.hair.styleId = "medium_professional";
        d.hair.colorPreset = 2;
        d.hair.customColor = Vec3(0.2f, 0.12f, 0.08f);
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createAthlete() {
        CharacterPreset preset;
        preset.id = "realistic_athlete";
        preset.name = "Athlete";
        preset.nameCN = "运动员";
        preset.description = "Fit and athletic build";
        preset.descriptionCN = "健美运动员体型";
        preset.category = PresetCategory::Realistic;
        preset.tags = {"male", "adult", "athletic", "muscular"};
        
        auto& d = preset.data;
        d.name = "Athlete";
        
        d.body.gender = 0;
        d.body.ageGroup = 2;  // Young adult
        d.body.height = 0.65f;
        d.body.weight = 0.55f;
        d.body.muscularity = 0.75f;
        d.body.bodyFat = 0.15f;
        d.body.shoulderWidth = 0.7f;
        d.body.chestSize = 0.65f;
        d.body.armThickness = 0.6f;
        d.body.thighThickness = 0.6f;
        d.body.skinColor = Vec3(0.75f, 0.55f, 0.4f);
        
        d.face.jawWidth = 0.6f;
        d.face.jawLine = 0.6f;
        
        d.hair.styleId = "short_sporty";
        d.hair.colorPreset = 0;  // Black
        
        preset.previewSkinColor = d.body.skinColor;
        
        return preset;
    }
    
    static CharacterPreset createElderly() {
        CharacterPreset preset;
        preset.id = "realistic_elderly";
        preset.name = "Elderly";
        preset.nameCN = "老年人";
        preset.description = "Senior citizen with aged features";
        preset.descriptionCN = "带有年龄特征的老年人";
        preset.category = PresetCategory::Realistic;
        preset.tags = {"male", "senior", "elderly"};
        
        auto& d = preset.data;
        d.name = "Elderly";
        
        d.body.gender = 0;
        d.body.ageGroup = 4;  // Senior
        d.body.height = 0.45f;
        d.body.weight = 0.5f;
        d.body.muscularity = 0.2f;
        d.body.bodyFat = 0.45f;
        d.body.skinColor = Vec3(0.88f, 0.72f, 0.62f);
        
        d.face.faceLength = 0.6f;
        d.face.eyeSize = 0.45f;
        d.face.jawLine = 0.4f;
        
        d.hair.styleId = "short_thin";
        d.hair.colorPreset = 4;  // Gray
        d.hair.customColor = Vec3(0.7f, 0.7f, 0.7f);
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        
        return preset;
    }
    
    static CharacterPreset createChild() {
        CharacterPreset preset;
        preset.id = "realistic_child";
        preset.name = "Child";
        preset.nameCN = "儿童";
        preset.description = "Young child with youthful proportions";
        preset.descriptionCN = "拥有年轻比例的儿童";
        preset.category = PresetCategory::Realistic;
        preset.tags = {"child", "young"};
        
        auto& d = preset.data;
        d.name = "Child";
        
        d.body.gender = 2;  // Neutral
        d.body.ageGroup = 0;  // Child
        d.body.height = 0.25f;
        d.body.weight = 0.3f;
        d.body.skinColor = Vec3(0.92f, 0.78f, 0.68f);
        
        d.face.faceWidth = 0.6f;
        d.face.faceRoundness = 0.7f;
        d.face.eyeSize = 0.65f;
        d.face.noseLength = 0.35f;
        
        d.hair.styleId = "short_cute";
        d.hair.colorPreset = 1;  // Brown
        
        preset.previewSkinColor = d.body.skinColor;
        
        return preset;
    }
    
    // === Anime Presets ===
    
    static CharacterPreset createAnimeGirl() {
        CharacterPreset preset;
        preset.id = "anime_girl";
        preset.name = "Anime Girl";
        preset.nameCN = "动漫少女";
        preset.description = "Classic anime-style female character";
        preset.descriptionCN = "经典动漫风格女性角色";
        preset.category = PresetCategory::Anime;
        preset.tags = {"female", "anime", "cute"};
        
        auto& d = preset.data;
        d.name = "Anime Girl";
        d.characterType = 1;  // Cartoon/Anime
        
        d.body.gender = 1;
        d.body.ageGroup = 1;  // Teen
        d.body.height = 0.4f;
        d.body.weight = 0.35f;
        d.body.shoulderWidth = 0.35f;
        d.body.hipWidth = 0.45f;
        d.body.legLength = 0.6f;  // Long legs anime style
        d.body.skinColor = Vec3(0.98f, 0.92f, 0.88f);  // Very light
        
        d.face.faceWidth = 0.55f;
        d.face.faceRoundness = 0.65f;
        d.face.eyeSize = 0.8f;  // Big anime eyes
        d.face.eyeSpacing = 0.55f;
        d.face.eyeHeight = 0.45f;
        d.face.noseLength = 0.25f;  // Small nose
        d.face.noseWidth = 0.3f;
        d.face.mouthWidth = 0.4f;
        d.face.chinLength = 0.35f;  // Pointy chin
        d.face.eyeColor = Vec3(0.2f, 0.5f, 0.8f);  // Blue
        
        d.hair.styleId = "anime_long_twin";
        d.hair.colorPreset = 6;  // Pink
        d.hair.customColor = Vec3(1.0f, 0.6f, 0.7f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createAnimeBoy() {
        CharacterPreset preset;
        preset.id = "anime_boy";
        preset.name = "Anime Boy";
        preset.nameCN = "动漫少年";
        preset.description = "Classic anime-style male character";
        preset.descriptionCN = "经典动漫风格男性角色";
        preset.category = PresetCategory::Anime;
        preset.tags = {"male", "anime", "cool"};
        
        auto& d = preset.data;
        d.name = "Anime Boy";
        d.characterType = 1;
        
        d.body.gender = 0;
        d.body.ageGroup = 1;
        d.body.height = 0.55f;
        d.body.weight = 0.4f;
        d.body.shoulderWidth = 0.5f;
        d.body.legLength = 0.58f;
        d.body.skinColor = Vec3(0.95f, 0.88f, 0.82f);
        
        d.face.faceWidth = 0.48f;
        d.face.faceLength = 0.55f;
        d.face.eyeSize = 0.65f;
        d.face.eyeAngle = 0.55f;  // Sharper eyes
        d.face.noseLength = 0.35f;
        d.face.jawWidth = 0.5f;
        d.face.chinLength = 0.45f;
        d.face.eyeColor = Vec3(0.15f, 0.15f, 0.15f);  // Dark
        
        d.hair.styleId = "anime_spiky";
        d.hair.colorPreset = 0;  // Black
        d.hair.customColor = Vec3(0.05f, 0.05f, 0.1f);
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createChibi() {
        CharacterPreset preset;
        preset.id = "anime_chibi";
        preset.name = "Chibi";
        preset.nameCN = "Q版角色";
        preset.description = "Super-deformed cute character";
        preset.descriptionCN = "超可爱 Q 版变形角色";
        preset.category = PresetCategory::Anime;
        preset.tags = {"chibi", "cute", "sd"};
        
        auto& d = preset.data;
        d.name = "Chibi";
        d.characterType = 2;  // Mascot
        
        d.body.gender = 2;
        d.body.ageGroup = 0;
        d.body.height = 0.15f;  // Very short
        d.body.weight = 0.5f;
        d.body.shoulderWidth = 0.45f;
        d.body.armLength = 0.35f;
        d.body.legLength = 0.25f;  // Short legs
        d.body.skinColor = Vec3(1.0f, 0.95f, 0.9f);
        
        d.face.faceWidth = 0.75f;  // Wide face
        d.face.faceRoundness = 0.9f;  // Very round
        d.face.eyeSize = 0.95f;  // Huge eyes
        d.face.eyeSpacing = 0.6f;
        d.face.noseLength = 0.1f;  // Tiny nose
        d.face.mouthWidth = 0.3f;
        d.face.eyeColor = Vec3(0.4f, 0.7f, 0.3f);  // Green
        
        d.hair.styleId = "chibi_fluffy";
        d.hair.customColor = Vec3(0.9f, 0.7f, 0.3f);  // Blonde
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    // === Cartoon Presets ===
    
    static CharacterPreset createWesternCartoon() {
        CharacterPreset preset;
        preset.id = "cartoon_western";
        preset.name = "Western Cartoon";
        preset.nameCN = "西方卡通";
        preset.description = "American cartoon style character";
        preset.descriptionCN = "美式卡通风格角色";
        preset.category = PresetCategory::Cartoon;
        preset.tags = {"cartoon", "western", "fun"};
        
        auto& d = preset.data;
        d.name = "Cartoon Character";
        d.characterType = 1;
        
        d.body.gender = 0;
        d.body.height = 0.5f;
        d.body.weight = 0.55f;
        d.body.shoulderWidth = 0.6f;
        d.body.chestSize = 0.55f;
        d.body.armThickness = 0.55f;
        d.body.skinColor = Vec3(0.95f, 0.85f, 0.7f);
        
        d.face.faceWidth = 0.6f;
        d.face.faceRoundness = 0.6f;
        d.face.eyeSize = 0.7f;
        d.face.noseLength = 0.5f;
        d.face.noseWidth = 0.55f;
        d.face.mouthWidth = 0.6f;
        d.face.eyeColor = Vec3(0.2f, 0.3f, 0.5f);
        
        d.hair.styleId = "cartoon_simple";
        d.hair.customColor = Vec3(0.1f, 0.08f, 0.05f);
        
        preset.previewSkinColor = d.body.skinColor;
        
        return preset;
    }
    
    static CharacterPreset createPixarStyle() {
        CharacterPreset preset;
        preset.id = "cartoon_pixar";
        preset.name = "Pixar Style";
        preset.nameCN = "皮克斯风格";
        preset.description = "3D animation studio style";
        preset.descriptionCN = "3D 动画工作室风格";
        preset.category = PresetCategory::Cartoon;
        preset.tags = {"cartoon", "3d", "pixar"};
        
        auto& d = preset.data;
        d.name = "Pixar Character";
        d.characterType = 1;
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.5f;
        d.body.weight = 0.5f;
        d.body.shoulderWidth = 0.55f;
        d.body.skinColor = Vec3(0.92f, 0.78f, 0.65f);
        
        d.face.faceWidth = 0.55f;
        d.face.faceRoundness = 0.55f;
        d.face.eyeSize = 0.6f;
        d.face.eyeSpacing = 0.5f;
        d.face.noseLength = 0.45f;
        d.face.noseWidth = 0.5f;
        d.face.eyeColor = Vec3(0.35f, 0.45f, 0.3f);
        
        d.hair.styleId = "short_neat";
        d.hair.colorPreset = 1;
        
        preset.previewSkinColor = d.body.skinColor;
        
        return preset;
    }
    
    // === Fantasy Presets (西幻) ===
    
    static CharacterPreset createElf() {
        CharacterPreset preset;
        preset.id = "fantasy_elf";
        preset.name = "Elf";
        preset.nameCN = "精灵";
        preset.description = "Elegant fantasy elf with pointed ears";
        preset.descriptionCN = "尖耳优雅的奇幻精灵";
        preset.category = PresetCategory::Fantasy;
        preset.tags = {"fantasy", "elf", "elegant", "magic"};
        
        auto& d = preset.data;
        d.name = "Elf";
        d.characterType = 0;
        
        d.body.gender = 2;
        d.body.ageGroup = 2;
        d.body.height = 0.55f;
        d.body.weight = 0.35f;
        d.body.muscularity = 0.25f;
        d.body.shoulderWidth = 0.4f;
        d.body.legLength = 0.6f;
        d.body.skinColor = Vec3(0.98f, 0.95f, 0.92f);
        
        d.face.faceWidth = 0.42f;
        d.face.faceLength = 0.6f;
        d.face.faceRoundness = 0.35f;
        d.face.eyeSize = 0.6f;
        d.face.eyeAngle = 0.6f;
        d.face.noseLength = 0.5f;
        d.face.noseWidth = 0.35f;
        d.face.chinLength = 0.55f;
        d.face.chinWidth = 0.4f;
        d.face.eyeColor = Vec3(0.5f, 0.8f, 0.6f);
        
        d.hair.styleId = "long_flowing";
        d.hair.customColor = Vec3(0.95f, 0.92f, 0.85f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createPaladin() {
        CharacterPreset preset;
        preset.id = "fantasy_paladin";
        preset.name = "Paladin";
        preset.nameCN = "圣骑士";
        preset.description = "Holy warrior of light";
        preset.descriptionCN = "光明圣骑士";
        preset.category = PresetCategory::Fantasy;
        preset.tags = {"fantasy", "warrior", "holy", "knight"};
        
        auto& d = preset.data;
        d.name = "Paladin";
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.65f;
        d.body.weight = 0.6f;
        d.body.muscularity = 0.65f;
        d.body.shoulderWidth = 0.65f;
        d.body.chestSize = 0.6f;
        d.body.skinColor = Vec3(0.88f, 0.75f, 0.65f);
        
        d.face.faceWidth = 0.52f;
        d.face.jawWidth = 0.58f;
        d.face.jawLine = 0.55f;
        d.face.eyeColor = Vec3(0.3f, 0.5f, 0.7f);  // Blue
        
        d.hair.styleId = "short_neat";
        d.hair.customColor = Vec3(0.8f, 0.65f, 0.4f);  // Blonde
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createDarkMage() {
        CharacterPreset preset;
        preset.id = "fantasy_dark_mage";
        preset.name = "Dark Mage";
        preset.nameCN = "暗黑法师";
        preset.description = "Mysterious dark magic user";
        preset.descriptionCN = "神秘的黑暗魔法师";
        preset.category = PresetCategory::Fantasy;
        preset.tags = {"fantasy", "mage", "dark", "magic"};
        
        auto& d = preset.data;
        d.name = "Dark Mage";
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.55f;
        d.body.weight = 0.4f;
        d.body.muscularity = 0.2f;
        d.body.skinColor = Vec3(0.85f, 0.82f, 0.8f);  // Pale
        
        d.face.faceWidth = 0.48f;
        d.face.faceLength = 0.58f;
        d.face.eyeSize = 0.55f;
        d.face.eyeColor = Vec3(0.6f, 0.2f, 0.6f);  // Purple
        
        d.hair.styleId = "long_dark";
        d.hair.customColor = Vec3(0.08f, 0.05f, 0.12f);  // Dark purple-black
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createOrc() {
        CharacterPreset preset;
        preset.id = "fantasy_orc";
        preset.name = "Orc Warrior";
        preset.nameCN = "兽人战士";
        preset.description = "Fierce orc warrior";
        preset.descriptionCN = "凶猛的兽人战士";
        preset.category = PresetCategory::Fantasy;
        preset.tags = {"fantasy", "orc", "warrior"};
        
        auto& d = preset.data;
        d.name = "Orc";
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.75f;
        d.body.weight = 0.75f;
        d.body.muscularity = 0.85f;
        d.body.bodyFat = 0.3f;
        d.body.shoulderWidth = 0.8f;
        d.body.chestSize = 0.75f;
        d.body.armThickness = 0.7f;
        d.body.skinColor = Vec3(0.4f, 0.55f, 0.35f);
        
        d.face.faceWidth = 0.7f;
        d.face.faceLength = 0.55f;
        d.face.faceRoundness = 0.4f;
        d.face.eyeSize = 0.4f;
        d.face.noseLength = 0.55f;
        d.face.noseWidth = 0.7f;
        d.face.jawWidth = 0.75f;
        d.face.jawLine = 0.7f;
        d.face.eyeColor = Vec3(0.6f, 0.2f, 0.1f);
        
        d.hair.styleId = "mohawk";
        d.hair.customColor = Vec3(0.1f, 0.1f, 0.1f);
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    // === Wuxia Presets (武侠) ===
    
    static CharacterPreset createSwordsman() {
        CharacterPreset preset;
        preset.id = "wuxia_swordsman";
        preset.name = "Swordsman";
        preset.nameCN = "剑客";
        preset.description = "Wandering martial arts swordsman";
        preset.descriptionCN = "仗剑江湖的剑客侠士";
        preset.category = PresetCategory::Wuxia;
        preset.tags = {"wuxia", "martial", "sword", "hero"};
        
        auto& d = preset.data;
        d.name = "Swordsman";
        
        d.body.gender = 0;
        d.body.ageGroup = 2;
        d.body.height = 0.6f;
        d.body.weight = 0.45f;
        d.body.muscularity = 0.5f;
        d.body.bodyFat = 0.2f;
        d.body.shoulderWidth = 0.52f;
        d.body.skinColor = Vec3(0.9f, 0.78f, 0.65f);
        
        d.face.faceWidth = 0.48f;
        d.face.faceLength = 0.55f;
        d.face.eyeSize = 0.52f;
        d.face.eyeAngle = 0.55f;
        d.face.jawWidth = 0.52f;
        d.face.jawLine = 0.55f;
        d.face.eyeColor = Vec3(0.2f, 0.15f, 0.1f);  // Dark brown
        
        d.hair.styleId = "long_tied";
        d.hair.customColor = Vec3(0.08f, 0.06f, 0.04f);  // Black
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createFemaleKnight() {
        CharacterPreset preset;
        preset.id = "wuxia_female_knight";
        preset.name = "Female Knight";
        preset.nameCN = "女侠";
        preset.description = "Graceful female martial artist";
        preset.descriptionCN = "飒爽英姿的江湖女侠";
        preset.category = PresetCategory::Wuxia;
        preset.tags = {"wuxia", "martial", "female", "hero"};
        
        auto& d = preset.data;
        d.name = "Female Knight";
        
        d.body.gender = 1;
        d.body.ageGroup = 2;
        d.body.height = 0.5f;
        d.body.weight = 0.4f;
        d.body.muscularity = 0.35f;
        d.body.shoulderWidth = 0.42f;
        d.body.hipWidth = 0.5f;
        d.body.legLength = 0.55f;
        d.body.skinColor = Vec3(0.95f, 0.85f, 0.75f);
        
        d.face.faceWidth = 0.45f;
        d.face.faceRoundness = 0.5f;
        d.face.eyeSize = 0.58f;
        d.face.eyeAngle = 0.55f;
        d.face.noseLength = 0.45f;
        d.face.mouthWidth = 0.45f;
        d.face.eyeColor = Vec3(0.18f, 0.12f, 0.08f);
        
        d.hair.styleId = "long_ponytail";
        d.hair.customColor = Vec3(0.05f, 0.03f, 0.02f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createMonk() {
        CharacterPreset preset;
        preset.id = "wuxia_monk";
        preset.name = "Martial Monk";
        preset.nameCN = "武僧";
        preset.description = "Shaolin-style martial monk";
        preset.descriptionCN = "少林武僧";
        preset.category = PresetCategory::Wuxia;
        preset.tags = {"wuxia", "martial", "monk", "shaolin"};
        
        auto& d = preset.data;
        d.name = "Monk";
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.55f;
        d.body.weight = 0.5f;
        d.body.muscularity = 0.6f;
        d.body.bodyFat = 0.2f;
        d.body.shoulderWidth = 0.55f;
        d.body.chestSize = 0.55f;
        d.body.skinColor = Vec3(0.85f, 0.7f, 0.55f);
        
        d.face.faceWidth = 0.55f;
        d.face.faceRoundness = 0.5f;
        d.face.eyeSize = 0.48f;
        d.face.jawWidth = 0.55f;
        d.face.eyeColor = Vec3(0.2f, 0.15f, 0.1f);
        
        d.hair.styleId = "bald";
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = Vec3(0.5f, 0.5f, 0.5f);
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    // === Gufeng Presets (古风) ===
    
    static CharacterPreset createXianxiaHero() {
        CharacterPreset preset;
        preset.id = "gufeng_xianxia_hero";
        preset.name = "Xianxia Hero";
        preset.nameCN = "仙侠少年";
        preset.description = "Young cultivation hero";
        preset.descriptionCN = "修仙少年英雄";
        preset.category = PresetCategory::Gufeng;
        preset.tags = {"gufeng", "xianxia", "cultivation", "hero"};
        
        auto& d = preset.data;
        d.name = "Xianxia Hero";
        
        d.body.gender = 0;
        d.body.ageGroup = 1;  // Teen/Young
        d.body.height = 0.55f;
        d.body.weight = 0.4f;
        d.body.muscularity = 0.4f;
        d.body.shoulderWidth = 0.48f;
        d.body.skinColor = Vec3(0.95f, 0.88f, 0.8f);
        
        d.face.faceWidth = 0.46f;
        d.face.faceLength = 0.55f;
        d.face.faceRoundness = 0.45f;
        d.face.eyeSize = 0.55f;
        d.face.eyeAngle = 0.55f;
        d.face.noseLength = 0.48f;
        d.face.mouthWidth = 0.45f;
        d.face.jawWidth = 0.48f;
        d.face.eyeColor = Vec3(0.15f, 0.12f, 0.08f);
        
        d.hair.styleId = "long_flowing";
        d.hair.customColor = Vec3(0.05f, 0.03f, 0.02f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createFairyMaiden() {
        CharacterPreset preset;
        preset.id = "gufeng_fairy";
        preset.name = "Fairy Maiden";
        preset.nameCN = "仙子";
        preset.description = "Ethereal celestial maiden";
        preset.descriptionCN = "飘逸出尘的仙子";
        preset.category = PresetCategory::Gufeng;
        preset.tags = {"gufeng", "fairy", "celestial", "beauty"};
        
        auto& d = preset.data;
        d.name = "Fairy";
        
        d.body.gender = 1;
        d.body.ageGroup = 2;
        d.body.height = 0.5f;
        d.body.weight = 0.35f;
        d.body.muscularity = 0.15f;
        d.body.shoulderWidth = 0.38f;
        d.body.hipWidth = 0.48f;
        d.body.legLength = 0.58f;
        d.body.skinColor = Vec3(0.98f, 0.95f, 0.92f);  // Very pale
        
        d.face.faceWidth = 0.44f;
        d.face.faceLength = 0.52f;
        d.face.faceRoundness = 0.55f;
        d.face.eyeSize = 0.62f;
        d.face.eyeSpacing = 0.52f;
        d.face.noseLength = 0.42f;
        d.face.noseWidth = 0.38f;
        d.face.mouthWidth = 0.42f;
        d.face.chinLength = 0.45f;
        d.face.eyeColor = Vec3(0.4f, 0.55f, 0.65f);  // Soft blue
        
        d.hair.styleId = "long_flowing";
        d.hair.customColor = Vec3(0.1f, 0.08f, 0.05f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createEmperor() {
        CharacterPreset preset;
        preset.id = "gufeng_emperor";
        preset.name = "Emperor";
        preset.nameCN = "帝王";
        preset.description = "Majestic ancient emperor";
        preset.descriptionCN = "威严的古代帝王";
        preset.category = PresetCategory::Gufeng;
        preset.tags = {"gufeng", "emperor", "royal", "noble"};
        
        auto& d = preset.data;
        d.name = "Emperor";
        
        d.body.gender = 0;
        d.body.ageGroup = 3;
        d.body.height = 0.6f;
        d.body.weight = 0.55f;
        d.body.muscularity = 0.45f;
        d.body.shoulderWidth = 0.55f;
        d.body.skinColor = Vec3(0.92f, 0.82f, 0.72f);
        
        d.face.faceWidth = 0.52f;
        d.face.faceLength = 0.55f;
        d.face.eyeSize = 0.5f;
        d.face.eyeAngle = 0.55f;
        d.face.noseLength = 0.52f;
        d.face.jawWidth = 0.55f;
        d.face.jawLine = 0.55f;
        d.face.eyeColor = Vec3(0.18f, 0.12f, 0.08f);
        
        d.hair.styleId = "emperor_bun";
        d.hair.customColor = Vec3(0.05f, 0.03f, 0.02f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createPrincess() {
        CharacterPreset preset;
        preset.id = "gufeng_princess";
        preset.name = "Princess";
        preset.nameCN = "公主";
        preset.description = "Elegant ancient princess";
        preset.descriptionCN = "端庄典雅的公主";
        preset.category = PresetCategory::Gufeng;
        preset.tags = {"gufeng", "princess", "royal", "noble"};
        
        auto& d = preset.data;
        d.name = "Princess";
        
        d.body.gender = 1;
        d.body.ageGroup = 2;
        d.body.height = 0.48f;
        d.body.weight = 0.4f;
        d.body.muscularity = 0.15f;
        d.body.shoulderWidth = 0.4f;
        d.body.hipWidth = 0.5f;
        d.body.skinColor = Vec3(0.96f, 0.9f, 0.85f);
        
        d.face.faceWidth = 0.46f;
        d.face.faceRoundness = 0.55f;
        d.face.eyeSize = 0.58f;
        d.face.noseLength = 0.44f;
        d.face.mouthWidth = 0.44f;
        d.face.eyeColor = Vec3(0.2f, 0.15f, 0.1f);
        
        d.hair.styleId = "hanfu_updo";
        d.hair.customColor = Vec3(0.05f, 0.03f, 0.02f);
        d.hair.useCustomColor = true;
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewHairColor = d.hair.customColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    // === Sci-Fi Presets ===
    
    static CharacterPreset createCyborg() {
        CharacterPreset preset;
        preset.id = "scifi_cyborg";
        preset.name = "Cyborg";
        preset.nameCN = "赛博格";
        preset.description = "Human-machine hybrid";
        preset.descriptionCN = "人机混合体";
        preset.category = PresetCategory::SciFi;
        preset.tags = {"scifi", "cyborg", "tech"};
        
        auto& d = preset.data;
        d.name = "Cyborg";
        d.characterType = 0;
        
        d.body.gender = 0;
        d.body.ageGroup = 2;
        d.body.height = 0.6f;
        d.body.weight = 0.55f;
        d.body.muscularity = 0.6f;
        d.body.skinColor = Vec3(0.75f, 0.72f, 0.7f);  // Grayish
        
        d.face.faceWidth = 0.5f;
        d.face.eyeSize = 0.5f;
        d.face.jawWidth = 0.55f;
        d.face.eyeColor = Vec3(0.3f, 0.8f, 1.0f);  // Cyan glow
        
        d.hair.styleId = "bald";
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
    
    static CharacterPreset createAlien() {
        CharacterPreset preset;
        preset.id = "scifi_alien";
        preset.name = "Alien";
        preset.nameCN = "外星人";
        preset.description = "Extraterrestrial humanoid";
        preset.descriptionCN = "外星类人生物";
        preset.category = PresetCategory::SciFi;
        preset.tags = {"scifi", "alien", "extraterrestrial"};
        
        auto& d = preset.data;
        d.name = "Alien";
        d.characterType = 2;  // Mascot/Special
        
        d.body.gender = 2;
        d.body.height = 0.5f;
        d.body.weight = 0.3f;
        d.body.shoulderWidth = 0.4f;
        d.body.armLength = 0.6f;  // Long arms
        d.body.legLength = 0.55f;
        d.body.skinColor = Vec3(0.6f, 0.7f, 0.8f);  // Blue-gray
        
        d.face.faceWidth = 0.6f;
        d.face.faceLength = 0.7f;  // Elongated
        d.face.faceRoundness = 0.5f;
        d.face.eyeSize = 0.9f;  // Huge eyes
        d.face.eyeSpacing = 0.65f;  // Wide apart
        d.face.noseLength = 0.15f;  // Almost no nose
        d.face.noseWidth = 0.2f;
        d.face.mouthWidth = 0.3f;
        d.face.chinLength = 0.6f;
        d.face.eyeColor = Vec3(0.1f, 0.1f, 0.1f);  // Black
        
        d.hair.styleId = "bald";
        
        preset.previewSkinColor = d.body.skinColor;
        preset.previewEyeColor = d.face.eyeColor;
        
        return preset;
    }
};

// ============================================================================
// Preset Library
// ============================================================================

class PresetLibrary {
public:
    static PresetLibrary& getInstance() {
        static PresetLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // === 西幻 Fantasy (优先显示) ===
        addPreset(BuiltInPresets::createElf());
        addPreset(BuiltInPresets::createPaladin());
        addPreset(BuiltInPresets::createDarkMage());
        addPreset(BuiltInPresets::createOrc());
        
        // === 武侠 Wuxia ===
        addPreset(BuiltInPresets::createSwordsman());
        addPreset(BuiltInPresets::createFemaleKnight());
        addPreset(BuiltInPresets::createMonk());
        
        // === 古风 Gufeng ===
        addPreset(BuiltInPresets::createXianxiaHero());
        addPreset(BuiltInPresets::createFairyMaiden());
        addPreset(BuiltInPresets::createEmperor());
        addPreset(BuiltInPresets::createPrincess());
        
        // === 动漫 Anime ===
        addPreset(BuiltInPresets::createAnimeGirl());
        addPreset(BuiltInPresets::createAnimeBoy());
        addPreset(BuiltInPresets::createChibi());
        
        // === 卡通 Cartoon ===
        addPreset(BuiltInPresets::createWesternCartoon());
        addPreset(BuiltInPresets::createPixarStyle());
        
        // === 科幻 Sci-Fi ===
        addPreset(BuiltInPresets::createCyborg());
        addPreset(BuiltInPresets::createAlien());
        
        // === 写实 Realistic (放最后) ===
        addPreset(BuiltInPresets::createAthlete());
        addPreset(BuiltInPresets::createElderly());
        addPreset(BuiltInPresets::createChild());
        addPreset(BuiltInPresets::createBusinessMan());
        addPreset(BuiltInPresets::createBusinessWoman());
        
        initialized_ = true;
    }
    
    // Get preset by ID
    const CharacterPreset* getPreset(const std::string& id) const {
        auto it = presets_.find(id);
        return (it != presets_.end()) ? &it->second : nullptr;
    }
    
    // Get all preset IDs
    std::vector<std::string> getPresetIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : presets_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    // Get presets by category
    std::vector<const CharacterPreset*> getPresetsByCategory(PresetCategory category) const {
        std::vector<const CharacterPreset*> result;
        for (const auto& [id, preset] : presets_) {
            if (preset.category == category) {
                result.push_back(&preset);
            }
        }
        return result;
    }
    
    // Get all categories that have presets
    std::vector<PresetCategory> getCategories() const {
        std::vector<PresetCategory> cats;
        for (const auto& [id, preset] : presets_) {
            if (std::find(cats.begin(), cats.end(), preset.category) == cats.end()) {
                cats.push_back(preset.category);
            }
        }
        return cats;
    }
    
    // Add custom preset
    void addPreset(const CharacterPreset& preset) {
        presets_[preset.id] = preset;
    }
    
    // Remove preset (only custom)
    bool removePreset(const std::string& id) {
        auto it = presets_.find(id);
        if (it != presets_.end() && !it->second.isBuiltIn) {
            presets_.erase(it);
            return true;
        }
        return false;
    }
    
    // Search presets by tag
    std::vector<const CharacterPreset*> searchByTag(const std::string& tag) const {
        std::vector<const CharacterPreset*> result;
        for (const auto& [id, preset] : presets_) {
            for (const auto& t : preset.tags) {
                if (t == tag) {
                    result.push_back(&preset);
                    break;
                }
            }
        }
        return result;
    }
    
private:
    PresetLibrary() { initialize(); }
    
    std::unordered_map<std::string, CharacterPreset> presets_;
    bool initialized_ = false;
};

// ============================================================================
// Character Randomizer
// ============================================================================

class CharacterRandomizer {
public:
    CharacterRandomizer() : rng_(static_cast<unsigned>(std::time(nullptr))) {}
    
    // Generate completely random character
    CharacterProjectData generateRandom() {
        CharacterProjectData data;
        
        // Random gender
        data.body.gender = randomInt(0, 2);
        
        // Random age
        data.body.ageGroup = randomInt(0, 4);
        
        // Random body proportions
        data.body.height = randomFloat(0.2f, 0.8f);
        data.body.weight = randomFloat(0.2f, 0.8f);
        data.body.muscularity = randomFloat(0.1f, 0.7f);
        data.body.bodyFat = randomFloat(0.1f, 0.6f);
        data.body.shoulderWidth = randomFloat(0.3f, 0.7f);
        data.body.chestSize = randomFloat(0.3f, 0.7f);
        data.body.waistSize = randomFloat(0.3f, 0.7f);
        data.body.hipWidth = randomFloat(0.3f, 0.7f);
        data.body.armLength = randomFloat(0.4f, 0.6f);
        data.body.legLength = randomFloat(0.4f, 0.6f);
        
        if (data.body.gender == 1) {
            data.body.bustSize = randomFloat(0.3f, 0.7f);
        }
        
        // Random skin tone
        int skinTone = randomInt(0, 5);
        data.body.skinColor = getSkinTone(skinTone);
        
        // Random face
        data.face.faceWidth = randomFloat(0.35f, 0.65f);
        data.face.faceLength = randomFloat(0.4f, 0.6f);
        data.face.faceRoundness = randomFloat(0.3f, 0.7f);
        data.face.eyeSize = randomFloat(0.4f, 0.7f);
        data.face.eyeSpacing = randomFloat(0.4f, 0.6f);
        data.face.eyeHeight = randomFloat(0.4f, 0.6f);
        data.face.noseLength = randomFloat(0.35f, 0.65f);
        data.face.noseWidth = randomFloat(0.35f, 0.65f);
        data.face.mouthWidth = randomFloat(0.35f, 0.65f);
        data.face.jawWidth = randomFloat(0.4f, 0.6f);
        
        // Random eye color
        int eyeColor = randomInt(0, 5);
        data.face.eyeColor = getEyeColor(eyeColor);
        
        // Random hair
        data.hair.colorPreset = randomInt(0, 6);
        data.hair.customColor = getHairColor(data.hair.colorPreset);
        
        return data;
    }
    
    // Generate random within a category style
    CharacterProjectData generateRandomInStyle(PresetCategory category) {
        auto& lib = PresetLibrary::getInstance();
        auto presets = lib.getPresetsByCategory(category);
        
        if (presets.empty()) {
            return generateRandom();
        }
        
        // Pick random preset as base
        int idx = randomInt(0, static_cast<int>(presets.size()) - 1);
        CharacterProjectData data = presets[idx]->data;
        
        // Add random variations
        float variation = 0.15f;
        
        data.body.height += randomFloat(-variation, variation);
        data.body.weight += randomFloat(-variation, variation);
        data.body.muscularity += randomFloat(-variation, variation);
        
        data.face.faceWidth += randomFloat(-variation, variation);
        data.face.eyeSize += randomFloat(-variation, variation);
        data.face.noseLength += randomFloat(-variation, variation);
        
        // Clamp values
        clampBodyData(data);
        
        return data;
    }
    
    // Randomize specific aspects
    void randomizeBody(CharacterProjectData& data) {
        data.body.height = randomFloat(0.2f, 0.8f);
        data.body.weight = randomFloat(0.2f, 0.8f);
        data.body.muscularity = randomFloat(0.1f, 0.7f);
        data.body.shoulderWidth = randomFloat(0.3f, 0.7f);
        clampBodyData(data);
    }
    
    void randomizeFace(CharacterProjectData& data) {
        data.face.faceWidth = randomFloat(0.35f, 0.65f);
        data.face.faceLength = randomFloat(0.4f, 0.6f);
        data.face.eyeSize = randomFloat(0.4f, 0.7f);
        data.face.noseLength = randomFloat(0.35f, 0.65f);
        data.face.mouthWidth = randomFloat(0.35f, 0.65f);
        clampBodyData(data);
    }
    
    void randomizeColors(CharacterProjectData& data) {
        data.body.skinColor = getSkinTone(randomInt(0, 5));
        data.face.eyeColor = getEyeColor(randomInt(0, 5));
        data.hair.colorPreset = randomInt(0, 6);
        data.hair.customColor = getHairColor(data.hair.colorPreset);
    }
    
private:
    std::mt19937 rng_;
    
    int randomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_);
    }
    
    float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }
    
    Vec3 getSkinTone(int index) {
        const Vec3 tones[] = {
            {0.98f, 0.92f, 0.85f},  // Very light
            {0.92f, 0.78f, 0.65f},  // Light
            {0.85f, 0.68f, 0.55f},  // Medium light
            {0.72f, 0.52f, 0.38f},  // Medium
            {0.55f, 0.38f, 0.28f},  // Medium dark
            {0.38f, 0.25f, 0.18f}   // Dark
        };
        return tones[std::clamp(index, 0, 5)];
    }
    
    Vec3 getEyeColor(int index) {
        const Vec3 colors[] = {
            {0.35f, 0.25f, 0.15f},  // Brown
            {0.2f, 0.4f, 0.6f},     // Blue
            {0.3f, 0.5f, 0.25f},    // Green
            {0.4f, 0.35f, 0.25f},   // Hazel
            {0.5f, 0.5f, 0.5f},     // Gray
            {0.15f, 0.15f, 0.15f}   // Dark brown
        };
        return colors[std::clamp(index, 0, 5)];
    }
    
    Vec3 getHairColor(int index) {
        const Vec3 colors[] = {
            {0.05f, 0.05f, 0.05f},  // Black
            {0.35f, 0.22f, 0.12f},  // Brown
            {0.15f, 0.1f, 0.05f},   // Dark brown
            {0.85f, 0.7f, 0.45f},   // Blonde
            {0.7f, 0.7f, 0.7f},     // Gray
            {0.6f, 0.25f, 0.1f},    // Red
            {1.0f, 0.6f, 0.7f}      // Fantasy pink
        };
        return colors[std::clamp(index, 0, 6)];
    }
    
    void clampBodyData(CharacterProjectData& data) {
        auto clamp = [](float& v) { v = std::clamp(v, 0.0f, 1.0f); };
        
        clamp(data.body.height);
        clamp(data.body.weight);
        clamp(data.body.muscularity);
        clamp(data.body.bodyFat);
        clamp(data.body.shoulderWidth);
        clamp(data.body.chestSize);
        clamp(data.body.waistSize);
        clamp(data.body.hipWidth);
        clamp(data.body.armLength);
        clamp(data.body.legLength);
        clamp(data.body.bustSize);
        
        clamp(data.face.faceWidth);
        clamp(data.face.faceLength);
        clamp(data.face.faceRoundness);
        clamp(data.face.eyeSize);
        clamp(data.face.eyeSpacing);
        clamp(data.face.eyeHeight);
        clamp(data.face.noseLength);
        clamp(data.face.noseWidth);
        clamp(data.face.mouthWidth);
        clamp(data.face.jawWidth);
    }
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline PresetLibrary& getPresetLibrary() {
    return PresetLibrary::getInstance();
}

}  // namespace luma
