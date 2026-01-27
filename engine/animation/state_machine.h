// Animation State Machine
// Finite State Machine for animation control
#pragma once

#include "animation_clip.h"
#include "blend_tree.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <variant>

namespace luma {

// Forward declarations
class AnimationStateMachine;
class StateMachineState;

// ===== Parameter Types =====
enum class ParameterType {
    Float,
    Int,
    Bool,
    Trigger  // Auto-resets after consumption
};

struct AnimationParameter {
    std::string name;
    ParameterType type = ParameterType::Float;
    
    // Value storage
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    bool triggerValue = false;
    
    void setFloat(float v) { floatValue = v; }
    void setInt(int v) { intValue = v; }
    void setBool(bool v) { boolValue = v; }
    void setTrigger() { triggerValue = true; }
    void resetTrigger() { triggerValue = false; }
};

// ===== Transition Condition =====
enum class ConditionMode {
    If,          // parameter == value
    IfNot,       // parameter != value
    Greater,     // parameter > value
    Less,        // parameter < value
    GreaterEqual,
    LessEqual
};

struct TransitionCondition {
    std::string parameterName;
    ConditionMode mode = ConditionMode::If;
    float threshold = 0.0f;
    
    bool evaluate(const AnimationParameter& param) const {
        switch (param.type) {
            case ParameterType::Float:
                return evaluateFloat(param.floatValue);
            case ParameterType::Int:
                return evaluateFloat(static_cast<float>(param.intValue));
            case ParameterType::Bool:
                return evaluateBool(param.boolValue);
            case ParameterType::Trigger:
                return param.triggerValue;
        }
        return false;
    }
    
private:
    bool evaluateFloat(float value) const {
        switch (mode) {
            case ConditionMode::If: return std::abs(value - threshold) < 0.0001f;
            case ConditionMode::IfNot: return std::abs(value - threshold) >= 0.0001f;
            case ConditionMode::Greater: return value > threshold;
            case ConditionMode::Less: return value < threshold;
            case ConditionMode::GreaterEqual: return value >= threshold;
            case ConditionMode::LessEqual: return value <= threshold;
        }
        return false;
    }
    
    bool evaluateBool(bool value) const {
        bool thresholdBool = threshold > 0.5f;
        switch (mode) {
            case ConditionMode::If: return value == thresholdBool;
            case ConditionMode::IfNot: return value != thresholdBool;
            default: return false;
        }
    }
};

// ===== State Transition =====
struct StateTransition {
    std::string targetState;
    std::vector<TransitionCondition> conditions;  // All must be true (AND)
    
    float duration = 0.2f;      // Transition/crossfade duration
    float exitTime = 0.0f;      // Normalized time (0-1) when transition can occur
    bool hasExitTime = false;   // If true, wait for exitTime before transitioning
    bool interruptible = true;  // Can be interrupted by other transitions
    
    int priority = 0;           // Higher priority transitions checked first
    
    // Check if transition should occur
    bool shouldTransition(const std::unordered_map<std::string, AnimationParameter>& params,
                         float normalizedTime) const {
        // Check exit time
        if (hasExitTime && normalizedTime < exitTime) {
            return false;
        }
        
        // Check all conditions
        for (const auto& condition : conditions) {
            auto it = params.find(condition.parameterName);
            if (it == params.end()) return false;
            if (!condition.evaluate(it->second)) return false;
        }
        
        return true;
    }
};

// ===== State Machine State =====
class StateMachineState {
public:
    std::string name;
    
    // Motion (either clip or blend tree)
    AnimationClip* clip = nullptr;
    std::unique_ptr<BlendTreeNode> blendTree;
    
    // State settings
    float speed = 1.0f;
    bool loop = true;
    
    // Transitions from this state
    std::vector<StateTransition> transitions;
    
    // State callbacks
    std::function<void()> onEnter;
    std::function<void()> onExit;
    std::function<void(float normalizedTime)> onUpdate;
    
    // Runtime
    float time = 0.0f;
    
    void enter() {
        time = 0.0f;
        if (onEnter) onEnter();
    }
    
    void exit() {
        if (onExit) onExit();
    }
    
    void update(float deltaTime, const std::unordered_map<std::string, AnimationParameter>& params) {
        time += deltaTime * speed;
        
        // Update blend tree parameters
        if (blendTree) {
            for (const auto& [name, param] : params) {
                blendTree->setParameter(name, param.floatValue);
            }
        }
        
        // Callback
        if (onUpdate) {
            onUpdate(getNormalizedTime());
        }
    }
    
    float getDuration() const {
        if (blendTree) return blendTree->getDuration();
        if (clip) return clip->duration;
        return 1.0f;
    }
    
    float getNormalizedTime() const {
        float duration = getDuration();
        if (duration <= 0.0f) return 0.0f;
        
        if (loop) {
            return std::fmod(time, duration) / duration;
        } else {
            return std::min(time / duration, 1.0f);
        }
    }
    
    // Sample state animation
    void sample(Vec3* positions, Quat* rotations, Vec3* scales, int boneCount) const {
        float sampleTime = loop ? std::fmod(time, getDuration()) : time;
        
        if (blendTree) {
            // Blend tree handles its own sampling
            // Note: This needs deltaTime, so we'd need to restructure
            // For now, just use the clip if available
            if (clip) {
                clip->sample(sampleTime, positions, rotations, scales, boneCount);
            }
        } else if (clip) {
            clip->sample(sampleTime, positions, rotations, scales, boneCount);
        }
    }
    
    // Add transition
    StateTransition& addTransition(const std::string& targetState) {
        transitions.emplace_back();
        transitions.back().targetState = targetState;
        return transitions.back();
    }
};

// ===== Animation State Machine =====
class AnimationStateMachine {
public:
    AnimationStateMachine() = default;
    
    // === State Management ===
    
    StateMachineState* createState(const std::string& name) {
        auto state = std::make_unique<StateMachineState>();
        state->name = name;
        states_[name] = std::move(state);
        
        // First state becomes default
        if (defaultState_.empty()) {
            defaultState_ = name;
        }
        
        return states_[name].get();
    }
    
    StateMachineState* getState(const std::string& name) {
        auto it = states_.find(name);
        return it != states_.end() ? it->second.get() : nullptr;
    }
    
    void setDefaultState(const std::string& name) {
        defaultState_ = name;
    }
    
    // === Parameters ===
    
    void addParameter(const std::string& name, ParameterType type) {
        AnimationParameter param;
        param.name = name;
        param.type = type;
        parameters_[name] = param;
    }
    
    void setFloat(const std::string& name, float value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.setFloat(value);
        }
    }
    
    void setInt(const std::string& name, int value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.setInt(value);
        }
    }
    
    void setBool(const std::string& name, bool value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.setBool(value);
        }
    }
    
    void setTrigger(const std::string& name) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.setTrigger();
        }
    }
    
    float getFloat(const std::string& name) const {
        auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second.floatValue : 0.0f;
    }
    
    bool getBool(const std::string& name) const {
        auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second.boolValue : false;
    }
    
    int getInt(const std::string& name) const {
        auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second.intValue : 0;
    }
    
    // Get parameter names/types for UI
    std::vector<std::string> getParameterNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : parameters_) {
            names.push_back(name);
        }
        return names;
    }
    
    ParameterType getParameterType(const std::string& name) const {
        auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second.type : ParameterType::Float;
    }
    
    // Get state names for UI
    std::vector<std::string> getStateNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : states_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Force state transition (for debugging)
    void forceState(const std::string& stateName) {
        if (states_.find(stateName) != states_.end()) {
            currentState_ = stateName;
        }
    }
    
    // === Any State Transitions ===
    
    StateTransition& addAnyStateTransition(const std::string& targetState) {
        anyStateTransitions_.emplace_back();
        anyStateTransitions_.back().targetState = targetState;
        return anyStateTransitions_.back();
    }
    
    // === Update ===
    
    void start() {
        if (!started_) {
            started_ = true;
            currentState_ = defaultState_;
            
            if (auto* state = getState(currentState_)) {
                state->enter();
            }
        }
    }
    
    void update(float deltaTime) {
        if (!started_) {
            start();
        }
        
        // Update transition
        if (isTransitioning_) {
            updateTransition(deltaTime);
            return;
        }
        
        StateMachineState* current = getState(currentState_);
        if (!current) return;
        
        // Update current state
        current->update(deltaTime, parameters_);
        
        // Check any-state transitions first (highest priority)
        for (const auto& transition : anyStateTransitions_) {
            if (transition.targetState != currentState_ &&
                transition.shouldTransition(parameters_, current->getNormalizedTime())) {
                startTransition(transition);
                resetConsumedTriggers(transition);
                return;
            }
        }
        
        // Check state transitions (sorted by priority)
        std::vector<const StateTransition*> sortedTransitions;
        for (const auto& t : current->transitions) {
            sortedTransitions.push_back(&t);
        }
        std::sort(sortedTransitions.begin(), sortedTransitions.end(),
            [](const StateTransition* a, const StateTransition* b) {
                return a->priority > b->priority;
            });
        
        for (const auto* transition : sortedTransitions) {
            if (transition->shouldTransition(parameters_, current->getNormalizedTime())) {
                startTransition(*transition);
                resetConsumedTriggers(*transition);
                return;
            }
        }
    }
    
    // === Output ===
    
    void sample(Vec3* positions, Quat* rotations, Vec3* scales, int boneCount) const {
        if (isTransitioning_) {
            // Blend between states
            std::vector<Vec3> fromPos(boneCount), toPos(boneCount);
            std::vector<Quat> fromRot(boneCount), toRot(boneCount);
            std::vector<Vec3> fromScl(boneCount), toScl(boneCount);
            
            if (auto* from = getStateConst(previousState_)) {
                from->sample(fromPos.data(), fromRot.data(), fromScl.data(), boneCount);
            }
            if (auto* to = getStateConst(currentState_)) {
                to->sample(toPos.data(), toRot.data(), toScl.data(), boneCount);
            }
            
            // Blend
            for (int i = 0; i < boneCount; i++) {
                positions[i] = anim::lerp(fromPos[i], toPos[i], transitionProgress_);
                rotations[i] = anim::slerp(fromRot[i], toRot[i], transitionProgress_);
                scales[i] = anim::lerp(fromScl[i], toScl[i], transitionProgress_);
            }
        } else {
            // Current state only
            if (auto* current = getStateConst(currentState_)) {
                current->sample(positions, rotations, scales, boneCount);
            }
        }
    }
    
    // === Queries ===
    
    std::string getCurrentStateName() const { return currentState_; }
    bool isInState(const std::string& name) const { return currentState_ == name; }
    bool isTransitioning() const { return isTransitioning_; }
    float getTransitionProgress() const { return transitionProgress_; }
    
private:
    const StateMachineState* getStateConst(const std::string& name) const {
        auto it = states_.find(name);
        return it != states_.end() ? it->second.get() : nullptr;
    }
    
    void startTransition(const StateTransition& transition) {
        previousState_ = currentState_;
        currentState_ = transition.targetState;
        transitionDuration_ = transition.duration;
        transitionProgress_ = 0.0f;
        isTransitioning_ = true;
        
        if (auto* from = getState(previousState_)) {
            from->exit();
        }
        if (auto* to = getState(currentState_)) {
            to->enter();
        }
    }
    
    void updateTransition(float deltaTime) {
        if (transitionDuration_ > 0.0f) {
            transitionProgress_ += deltaTime / transitionDuration_;
        } else {
            transitionProgress_ = 1.0f;
        }
        
        // Update both states during transition
        if (auto* from = getState(previousState_)) {
            from->update(deltaTime, parameters_);
        }
        if (auto* to = getState(currentState_)) {
            to->update(deltaTime, parameters_);
        }
        
        if (transitionProgress_ >= 1.0f) {
            transitionProgress_ = 1.0f;
            isTransitioning_ = false;
        }
    }
    
    void resetConsumedTriggers(const StateTransition& transition) {
        for (const auto& condition : transition.conditions) {
            auto it = parameters_.find(condition.parameterName);
            if (it != parameters_.end() && it->second.type == ParameterType::Trigger) {
                it->second.resetTrigger();
            }
        }
    }
    
    std::unordered_map<std::string, std::unique_ptr<StateMachineState>> states_;
    std::unordered_map<std::string, AnimationParameter> parameters_;
    std::vector<StateTransition> anyStateTransitions_;
    
    std::string defaultState_;
    std::string currentState_;
    std::string previousState_;
    
    bool started_ = false;
    bool isTransitioning_ = false;
    float transitionDuration_ = 0.2f;
    float transitionProgress_ = 0.0f;
};

// ===== State Machine Presets =====
namespace StateMachinePresets {

// Create a simple locomotion state machine
inline std::unique_ptr<AnimationStateMachine> createLocomotionSM(
    AnimationClip* idle,
    AnimationClip* walk,
    AnimationClip* run)
{
    auto sm = std::make_unique<AnimationStateMachine>();
    
    // Parameters
    sm->addParameter("Speed", ParameterType::Float);
    sm->addParameter("IsMoving", ParameterType::Bool);
    
    // States
    auto* idleState = sm->createState("Idle");
    idleState->clip = idle;
    idleState->loop = true;
    
    auto* walkState = sm->createState("Walk");
    walkState->clip = walk;
    walkState->loop = true;
    
    auto* runState = sm->createState("Run");
    runState->clip = run;
    runState->loop = true;
    
    // Transitions: Idle -> Walk
    auto& idleToWalk = idleState->addTransition("Walk");
    idleToWalk.conditions.push_back({"IsMoving", ConditionMode::If, 1.0f});
    idleToWalk.conditions.push_back({"Speed", ConditionMode::Less, 0.5f});
    idleToWalk.duration = 0.2f;
    
    // Transitions: Idle -> Run
    auto& idleToRun = idleState->addTransition("Run");
    idleToRun.conditions.push_back({"IsMoving", ConditionMode::If, 1.0f});
    idleToRun.conditions.push_back({"Speed", ConditionMode::GreaterEqual, 0.5f});
    idleToRun.duration = 0.2f;
    
    // Transitions: Walk -> Idle
    auto& walkToIdle = walkState->addTransition("Idle");
    walkToIdle.conditions.push_back({"IsMoving", ConditionMode::IfNot, 1.0f});
    walkToIdle.duration = 0.3f;
    
    // Transitions: Walk -> Run
    auto& walkToRun = walkState->addTransition("Run");
    walkToRun.conditions.push_back({"Speed", ConditionMode::GreaterEqual, 0.5f});
    walkToRun.duration = 0.15f;
    
    // Transitions: Run -> Walk
    auto& runToWalk = runState->addTransition("Walk");
    runToWalk.conditions.push_back({"Speed", ConditionMode::Less, 0.5f});
    runToWalk.duration = 0.15f;
    
    // Transitions: Run -> Idle
    auto& runToIdle = runState->addTransition("Idle");
    runToIdle.conditions.push_back({"IsMoving", ConditionMode::IfNot, 1.0f});
    runToIdle.duration = 0.3f;
    
    sm->setDefaultState("Idle");
    
    return sm;
}

// Create a combat state machine
inline std::unique_ptr<AnimationStateMachine> createCombatSM(
    AnimationClip* idle,
    AnimationClip* attack1,
    AnimationClip* attack2,
    AnimationClip* block,
    AnimationClip* hit)
{
    auto sm = std::make_unique<AnimationStateMachine>();
    
    // Parameters
    sm->addParameter("Attack", ParameterType::Trigger);
    sm->addParameter("Block", ParameterType::Bool);
    sm->addParameter("Hit", ParameterType::Trigger);
    sm->addParameter("ComboCount", ParameterType::Int);
    
    // States
    auto* idleState = sm->createState("Idle");
    idleState->clip = idle;
    idleState->loop = true;
    
    auto* attack1State = sm->createState("Attack1");
    attack1State->clip = attack1;
    attack1State->loop = false;
    
    auto* attack2State = sm->createState("Attack2");
    attack2State->clip = attack2;
    attack2State->loop = false;
    
    auto* blockState = sm->createState("Block");
    blockState->clip = block;
    blockState->loop = true;
    
    auto* hitState = sm->createState("Hit");
    hitState->clip = hit;
    hitState->loop = false;
    
    // Transitions from Idle
    auto& idleToAttack1 = idleState->addTransition("Attack1");
    idleToAttack1.conditions.push_back({"Attack", ConditionMode::If, 1.0f});
    
    auto& idleToBlock = idleState->addTransition("Block");
    idleToBlock.conditions.push_back({"Block", ConditionMode::If, 1.0f});
    
    // Attack1 -> Attack2 (combo)
    auto& attack1ToAttack2 = attack1State->addTransition("Attack2");
    attack1ToAttack2.conditions.push_back({"Attack", ConditionMode::If, 1.0f});
    attack1ToAttack2.hasExitTime = true;
    attack1ToAttack2.exitTime = 0.5f;
    
    // Attack1 -> Idle (end)
    auto& attack1ToIdle = attack1State->addTransition("Idle");
    attack1ToIdle.hasExitTime = true;
    attack1ToIdle.exitTime = 0.9f;
    
    // Attack2 -> Idle
    auto& attack2ToIdle = attack2State->addTransition("Idle");
    attack2ToIdle.hasExitTime = true;
    attack2ToIdle.exitTime = 0.9f;
    
    // Block -> Idle
    auto& blockToIdle = blockState->addTransition("Idle");
    blockToIdle.conditions.push_back({"Block", ConditionMode::IfNot, 1.0f});
    
    // Any State -> Hit (interrupt)
    auto& anyToHit = sm->addAnyStateTransition("Hit");
    anyToHit.conditions.push_back({"Hit", ConditionMode::If, 1.0f});
    anyToHit.duration = 0.1f;
    
    // Hit -> Idle
    auto& hitToIdle = hitState->addTransition("Idle");
    hitToIdle.hasExitTime = true;
    hitToIdle.exitTime = 0.9f;
    
    sm->setDefaultState("Idle");
    
    return sm;
}

}  // namespace StateMachinePresets

}  // namespace luma
