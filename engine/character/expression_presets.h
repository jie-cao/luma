// Expression Presets - Extended expression library
// Additional facial expressions for various scenarios
#pragma once

#include "engine/character/facial_rig.h"
#include <string>
#include <vector>

namespace luma {

// ============================================================================
// Expression Categories
// ============================================================================

enum class ExpressionCategory {
    Basic,          // 基本表情
    Emotion,        // 情感表情
    Communication,  // 交流表情
    Action,         // 动作表情
    GameStyle,      // 游戏风格
    AnimeStyle,     // 动漫风格
    Custom          // 自定义
};

inline std::string expressionCategoryToString(ExpressionCategory cat) {
    switch (cat) {
        case ExpressionCategory::Basic: return "Basic";
        case ExpressionCategory::Emotion: return "Emotion";
        case ExpressionCategory::Communication: return "Communication";
        case ExpressionCategory::Action: return "Action";
        case ExpressionCategory::GameStyle: return "GameStyle";
        case ExpressionCategory::AnimeStyle: return "AnimeStyle";
        case ExpressionCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Extended Expression Info
// ============================================================================

struct ExtendedExpressionInfo {
    std::string id;
    std::string name;
    std::string nameCN;
    std::string description;
    ExpressionCategory category = ExpressionCategory::Basic;
    ExpressionPreset preset;
    std::vector<std::string> tags;
    
    // For animation
    bool isTransient = false;  // Quick expression that returns to neutral
    float holdTime = 0.5f;     // How long to hold if transient
};

// ============================================================================
// Extended Expression Library
// ============================================================================

class ExtendedExpressionLibrary {
public:
    static ExtendedExpressionLibrary& getInstance() {
        static ExtendedExpressionLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // Register all expressions with the base library
        auto& baseLib = ExpressionLibrary::getInstance();
        
        // === Basic Emotions (already in base, but we add metadata) ===
        registerExpression("happy", "Happy", "开心", "Basic happy expression",
            ExpressionCategory::Basic, {"basic", "positive"});
        registerExpression("sad", "Sad", "悲伤", "Basic sad expression",
            ExpressionCategory::Basic, {"basic", "negative"});
        registerExpression("angry", "Angry", "生气", "Basic angry expression",
            ExpressionCategory::Basic, {"basic", "negative"});
        registerExpression("surprised", "Surprised", "惊讶", "Basic surprised expression",
            ExpressionCategory::Basic, {"basic"});
        registerExpression("fear", "Fear", "恐惧", "Basic fear expression",
            ExpressionCategory::Basic, {"basic", "negative"});
        registerExpression("disgust", "Disgust", "厌恶", "Basic disgust expression",
            ExpressionCategory::Basic, {"basic", "negative"});
        
        // === Extended Emotions ===
        addSmirk();
        addPout();
        addCrying();
        addLaughing();
        addThinking();
        addSleepy();
        addDetermined();
        addEmbarrassed();
        addConfused();
        addProud();
        
        // === Communication ===
        addTalking();
        addWhispering();
        addShouting();
        addKissing();
        addWhistling();
        
        // === Actions ===
        addSneezing();
        addYawning();
        addEating();
        addDrinking();
        addBitingLip();
        
        // === Game/Combat Style ===
        addBattleCry();
        addVictory();
        addDefeat();
        addConcentration();
        addPain();
        addEvil();
        
        // === Anime Style ===
        addAnimeShock();
        addAnimeCute();
        addAnimeSmug();
        addAnimeDead();
        addAnimeSparkling();
        
        initialized_ = true;
    }
    
    const ExtendedExpressionInfo* getExpression(const std::string& id) const {
        auto it = expressions_.find(id);
        return (it != expressions_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getExpressionIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : expressions_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::vector<const ExtendedExpressionInfo*> getExpressionsByCategory(ExpressionCategory cat) const {
        std::vector<const ExtendedExpressionInfo*> result;
        for (const auto& [id, info] : expressions_) {
            if (info.category == cat) {
                result.push_back(&info);
            }
        }
        return result;
    }
    
    std::vector<ExpressionCategory> getCategories() const {
        return {
            ExpressionCategory::Basic,
            ExpressionCategory::Emotion,
            ExpressionCategory::Communication,
            ExpressionCategory::Action,
            ExpressionCategory::GameStyle,
            ExpressionCategory::AnimeStyle
        };
    }
    
private:
    ExtendedExpressionLibrary() { initialize(); }
    
    void registerExpression(const std::string& id, const std::string& name, 
                           const std::string& nameCN, const std::string& desc,
                           ExpressionCategory cat, const std::vector<std::string>& tags) {
        ExtendedExpressionInfo info;
        info.id = id;
        info.name = name;
        info.nameCN = nameCN;
        info.description = desc;
        info.category = cat;
        info.tags = tags;
        // Preset is already in base library
        expressions_[id] = info;
    }
    
    void addExpressionWithPreset(const ExtendedExpressionInfo& info) {
        expressions_[info.id] = info;
        ExpressionLibrary::getInstance().addPreset(info.preset);
    }
    
    // === Extended Emotion Implementations ===
    
    void addSmirk() {
        ExtendedExpressionInfo info;
        info.id = "smirk";
        info.name = "Smirk";
        info.nameCN = "得意";
        info.description = "Confident one-sided smile";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "confident", "asymmetric"};
        
        info.preset.name = "smirk";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addPout() {
        ExtendedExpressionInfo info;
        info.id = "pout";
        info.name = "Pout";
        info.nameCN = "嘟嘴";
        info.description = "Cute pouting expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "cute", "negative"};
        
        info.preset.name = "pout";
        info.preset.data.setWeight(ARKitBlendShapes::mouthPucker, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addCrying() {
        ExtendedExpressionInfo info;
        info.id = "crying";
        info.name = "Crying";
        info.nameCN = "哭泣";
        info.description = "Intense crying expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "sad", "intense"};
        
        info.preset.name = "crying";
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownLeft, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownRight, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addLaughing() {
        ExtendedExpressionInfo info;
        info.id = "laughing";
        info.name = "Laughing";
        info.nameCN = "大笑";
        info.description = "Intense laughing expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "happy", "intense"};
        
        info.preset.name = "laughing";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.5f);
        
        addExpressionWithPreset(info);
    }
    
    void addThinking() {
        ExtendedExpressionInfo info;
        info.id = "thinking";
        info.name = "Thinking";
        info.nameCN = "思考";
        info.description = "Contemplative thinking expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "neutral", "contemplative"};
        
        info.preset.name = "thinking";
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookUpLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookUpRight, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressRight, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addSleepy() {
        ExtendedExpressionInfo info;
        info.id = "sleepy";
        info.name = "Sleepy";
        info.nameCN = "困倦";
        info.description = "Tired and sleepy expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "tired"};
        
        info.preset.name = "sleepy";
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.1f);
        
        addExpressionWithPreset(info);
    }
    
    void addDetermined() {
        ExtendedExpressionInfo info;
        info.id = "determined";
        info.name = "Determined";
        info.nameCN = "坚定";
        info.description = "Resolute determination expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "positive", "intense"};
        
        info.preset.name = "determined";
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::jawForward, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addEmbarrassed() {
        ExtendedExpressionInfo info;
        info.id = "embarrassed";
        info.name = "Embarrassed";
        info.nameCN = "害羞";
        info.description = "Shy embarrassed expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "shy", "cute"};
        
        info.preset.name = "embarrassed";
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookDownLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookDownRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekPuff, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addConfused() {
        ExtendedExpressionInfo info;
        info.id = "confused";
        info.name = "Confused";
        info.nameCN = "困惑";
        info.description = "Puzzled confused expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "questioning"};
        
        info.preset.name = "confused";
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownRight, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addProud() {
        ExtendedExpressionInfo info;
        info.id = "proud";
        info.name = "Proud";
        info.nameCN = "骄傲";
        info.description = "Self-satisfied proud expression";
        info.category = ExpressionCategory::Emotion;
        info.tags = {"emotion", "positive", "confident"};
        
        info.preset.name = "proud";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    // === Communication ===
    
    void addTalking() {
        ExtendedExpressionInfo info;
        info.id = "talking";
        info.name = "Talking";
        info.nameCN = "说话";
        info.description = "Mouth open for talking";
        info.category = ExpressionCategory::Communication;
        info.tags = {"communication", "mouth"};
        
        info.preset.name = "talking";
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthOpen, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addWhispering() {
        ExtendedExpressionInfo info;
        info.id = "whispering";
        info.name = "Whispering";
        info.nameCN = "悄悄话";
        info.description = "Whispering expression";
        info.category = ExpressionCategory::Communication;
        info.tags = {"communication", "quiet"};
        
        info.preset.name = "whispering";
        info.preset.data.setWeight(ARKitBlendShapes::mouthFunnel, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addShouting() {
        ExtendedExpressionInfo info;
        info.id = "shouting";
        info.name = "Shouting";
        info.nameCN = "呐喊";
        info.description = "Loud shouting expression";
        info.category = ExpressionCategory::Communication;
        info.tags = {"communication", "loud", "intense"};
        
        info.preset.name = "shouting";
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addKissing() {
        ExtendedExpressionInfo info;
        info.id = "kissing";
        info.name = "Kissing";
        info.nameCN = "亲吻";
        info.description = "Puckered lips for kissing";
        info.category = ExpressionCategory::Communication;
        info.tags = {"communication", "romantic"};
        
        info.preset.name = "kissing";
        info.preset.data.setWeight(ARKitBlendShapes::mouthPucker, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addWhistling() {
        ExtendedExpressionInfo info;
        info.id = "whistling";
        info.name = "Whistling";
        info.nameCN = "吹口哨";
        info.description = "Lips shaped for whistling";
        info.category = ExpressionCategory::Communication;
        info.tags = {"communication", "casual"};
        
        info.preset.name = "whistling";
        info.preset.data.setWeight(ARKitBlendShapes::mouthFunnel, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPucker, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    // === Actions ===
    
    void addSneezing() {
        ExtendedExpressionInfo info;
        info.id = "sneezing";
        info.name = "Sneezing";
        info.nameCN = "打喷嚏";
        info.description = "Pre-sneeze expression";
        info.category = ExpressionCategory::Action;
        info.tags = {"action", "transient"};
        info.isTransient = true;
        info.holdTime = 0.3f;
        
        info.preset.name = "sneezing";
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.5f);
        info.preset.transitionTime = 0.1f;
        
        addExpressionWithPreset(info);
    }
    
    void addYawning() {
        ExtendedExpressionInfo info;
        info.id = "yawning";
        info.name = "Yawning";
        info.nameCN = "打哈欠";
        info.description = "Wide yawn expression";
        info.category = ExpressionCategory::Action;
        info.tags = {"action", "tired"};
        
        info.preset.name = "yawning";
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addEating() {
        ExtendedExpressionInfo info;
        info.id = "eating";
        info.name = "Eating";
        info.nameCN = "吃东西";
        info.description = "Chewing expression";
        info.category = ExpressionCategory::Action;
        info.tags = {"action", "mouth"};
        
        info.preset.name = "eating";
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthClose, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekPuff, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addDrinking() {
        ExtendedExpressionInfo info;
        info.id = "drinking";
        info.name = "Drinking";
        info.nameCN = "喝水";
        info.description = "Sipping expression";
        info.category = ExpressionCategory::Action;
        info.tags = {"action", "mouth"};
        
        info.preset.name = "drinking";
        info.preset.data.setWeight(ARKitBlendShapes::mouthFunnel, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPucker, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addBitingLip() {
        ExtendedExpressionInfo info;
        info.id = "biting_lip";
        info.name = "Biting Lip";
        info.nameCN = "咬唇";
        info.description = "Nervous lip biting";
        info.category = ExpressionCategory::Action;
        info.tags = {"action", "nervous"};
        
        info.preset.name = "biting_lip";
        info.preset.data.setWeight(ARKitBlendShapes::mouthRollLower, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::jawForward, 0.2f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    // === Game/Combat Style ===
    
    void addBattleCry() {
        ExtendedExpressionInfo info;
        info.id = "battle_cry";
        info.name = "Battle Cry";
        info.nameCN = "战吼";
        info.description = "Fierce battle cry expression";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "combat", "intense"};
        
        info.preset.name = "battle_cry";
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerRight, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthUpperUpLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthUpperUpRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addVictory() {
        ExtendedExpressionInfo info;
        info.id = "victory";
        info.name = "Victory";
        info.nameCN = "胜利";
        info.description = "Triumphant victory expression";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "positive", "victory"};
        
        info.preset.name = "victory";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addDefeat() {
        ExtendedExpressionInfo info;
        info.id = "defeat";
        info.name = "Defeat";
        info.nameCN = "失败";
        info.description = "Disappointed defeat expression";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "negative", "defeat"};
        
        info.preset.name = "defeat";
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownLeft, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthFrownRight, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookDownLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeLookDownRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addConcentration() {
        ExtendedExpressionInfo info;
        info.id = "concentration";
        info.name = "Concentration";
        info.nameCN = "专注";
        info.description = "Intense focus expression";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "focus"};
        
        info.preset.name = "concentration";
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthPressRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addPain() {
        ExtendedExpressionInfo info;
        info.id = "pain";
        info.name = "Pain";
        info.nameCN = "痛苦";
        info.description = "Expression of pain";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "combat", "negative"};
        
        info.preset.name = "pain";
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthStretchRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::noseSneerRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    void addEvil() {
        ExtendedExpressionInfo info;
        info.id = "evil";
        info.name = "Evil";
        info.nameCN = "邪恶";
        info.description = "Villainous evil expression";
        info.category = ExpressionCategory::GameStyle;
        info.tags = {"game", "villain", "negative"};
        
        info.preset.name = "evil";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::browDownRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    // === Anime Style ===
    
    void addAnimeShock() {
        ExtendedExpressionInfo info;
        info.id = "anime_shock";
        info.name = "Anime Shock";
        info.nameCN = "动漫震惊";
        info.description = "Exaggerated anime shock";
        info.category = ExpressionCategory::AnimeStyle;
        info.tags = {"anime", "exaggerated"};
        
        info.preset.name = "anime_shock";
        info.preset.data.setWeight(ARKitBlendShapes::eyeWideLeft, 1.0f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeWideRight, 1.0f);
        info.preset.data.setWeight(ARKitBlendShapes::browInnerUp, 0.9f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpLeft, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.6f);
        
        addExpressionWithPreset(info);
    }
    
    void addAnimeCute() {
        ExtendedExpressionInfo info;
        info.id = "anime_cute";
        info.name = "Anime Cute";
        info.nameCN = "动漫卖萌";
        info.description = "Kawaii cute expression";
        info.category = ExpressionCategory::AnimeStyle;
        info.tags = {"anime", "cute", "kawaii"};
        
        info.preset.name = "anime_cute";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.5f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addAnimeSmug() {
        ExtendedExpressionInfo info;
        info.id = "anime_smug";
        info.name = "Anime Smug";
        info.nameCN = "动漫得意";
        info.description = "Smug anime face";
        info.category = ExpressionCategory::AnimeStyle;
        info.tags = {"anime", "confident"};
        
        info.preset.name = "anime_smug";
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.4f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.4f);
        
        addExpressionWithPreset(info);
    }
    
    void addAnimeDead() {
        ExtendedExpressionInfo info;
        info.id = "anime_dead";
        info.name = "Anime Dead";
        info.nameCN = "动漫死亡";
        info.description = "Comedic dead/exhausted anime face";
        info.category = ExpressionCategory::AnimeStyle;
        info.tags = {"anime", "comedy", "exhausted"};
        
        info.preset.name = "anime_dead";
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 0.7f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthOpen, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::jawOpen, 0.2f);
        
        addExpressionWithPreset(info);
    }
    
    void addAnimeSparkling() {
        ExtendedExpressionInfo info;
        info.id = "anime_sparkling";
        info.name = "Anime Sparkling";
        info.nameCN = "动漫闪亮";
        info.description = "Sparkling eyes anime expression";
        info.category = ExpressionCategory::AnimeStyle;
        info.tags = {"anime", "excited", "positive"};
        
        info.preset.name = "anime_sparkling";
        info.preset.data.setWeight(ARKitBlendShapes::eyeWideLeft, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::eyeWideRight, 0.6f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.8f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintLeft, 0.3f);
        info.preset.data.setWeight(ARKitBlendShapes::cheekSquintRight, 0.3f);
        
        addExpressionWithPreset(info);
    }
    
    std::unordered_map<std::string, ExtendedExpressionInfo> expressions_;
    bool initialized_ = false;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline ExtendedExpressionLibrary& getExtendedExpressionLibrary() {
    return ExtendedExpressionLibrary::getInstance();
}

}  // namespace luma
