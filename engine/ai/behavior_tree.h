// Behavior Tree System
// Hierarchical task execution for AI decision making
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <any>

namespace luma {

// ===== Node Status =====
enum class BTStatus {
    Invalid,
    Success,
    Failure,
    Running
};

// ===== Node Type =====
enum class BTNodeType {
    // Composites
    Sequence,
    Selector,
    Parallel,
    RandomSelector,
    
    // Decorators
    Inverter,
    Succeeder,
    Failer,
    Repeater,
    RepeatUntilFail,
    Limiter,
    
    // Leaves
    Action,
    Condition,
    Wait,
    Log,
    
    // Custom
    Custom
};

// Forward declaration
class BTNode;
class BehaviorTree;

// ===== Blackboard (shared data) =====
class Blackboard {
public:
    template<typename T>
    void set(const std::string& key, const T& value) {
        data_[key] = value;
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }
    
    void remove(const std::string& key) {
        data_.erase(key);
    }
    
    void clear() {
        data_.clear();
    }
    
private:
    std::unordered_map<std::string, std::any> data_;
};

// ===== BT Context =====
struct BTContext {
    Blackboard* blackboard = nullptr;
    float deltaTime = 0.0f;
    void* owner = nullptr;  // Entity/Agent that owns this BT
    
    // Navigation integration
    Vec3 ownerPosition;
    float ownerRotation = 0.0f;
};

// ===== BT Node =====
class BTNode {
public:
    BTNode(BTNodeType type, const std::string& name = "")
        : type_(type), name_(name) {}
    virtual ~BTNode() = default;
    
    BTNodeType getType() const { return type_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    BTStatus getStatus() const { return status_; }
    
    // Lifecycle
    virtual void initialize(BTContext& context) { status_ = BTStatus::Running; }
    virtual BTStatus update(BTContext& context) = 0;
    virtual void terminate(BTContext& context, BTStatus status) { status_ = status; }
    
    // Tick (calls initialize/update/terminate as needed)
    BTStatus tick(BTContext& context) {
        if (status_ != BTStatus::Running) {
            initialize(context);
        }
        
        status_ = update(context);
        
        if (status_ != BTStatus::Running) {
            terminate(context, status_);
        }
        
        return status_;
    }
    
    // Reset
    virtual void reset() { status_ = BTStatus::Invalid; }
    
    // Children (for composites/decorators)
    void addChild(std::unique_ptr<BTNode> child) {
        children_.push_back(std::move(child));
    }
    
    const std::vector<std::unique_ptr<BTNode>>& getChildren() const { return children_; }
    size_t getChildCount() const { return children_.size(); }
    
protected:
    BTNodeType type_;
    std::string name_;
    BTStatus status_ = BTStatus::Invalid;
    std::vector<std::unique_ptr<BTNode>> children_;
};

// ===== Composite Nodes =====

// Sequence: Execute children in order, fail on first failure
class BTSequence : public BTNode {
public:
    BTSequence(const std::string& name = "Sequence")
        : BTNode(BTNodeType::Sequence, name) {}
    
    void initialize(BTContext& context) override {
        currentChild_ = 0;
        BTNode::initialize(context);
    }
    
    BTStatus update(BTContext& context) override {
        while (currentChild_ < children_.size()) {
            BTStatus status = children_[currentChild_]->tick(context);
            
            if (status != BTStatus::Success) {
                return status;  // Running or Failure
            }
            
            currentChild_++;
        }
        
        return BTStatus::Success;
    }
    
    void reset() override {
        BTNode::reset();
        currentChild_ = 0;
        for (auto& child : children_) {
            child->reset();
        }
    }
    
private:
    size_t currentChild_ = 0;
};

// Selector: Execute children until one succeeds
class BTSelector : public BTNode {
public:
    BTSelector(const std::string& name = "Selector")
        : BTNode(BTNodeType::Selector, name) {}
    
    void initialize(BTContext& context) override {
        currentChild_ = 0;
        BTNode::initialize(context);
    }
    
    BTStatus update(BTContext& context) override {
        while (currentChild_ < children_.size()) {
            BTStatus status = children_[currentChild_]->tick(context);
            
            if (status != BTStatus::Failure) {
                return status;  // Running or Success
            }
            
            currentChild_++;
        }
        
        return BTStatus::Failure;
    }
    
    void reset() override {
        BTNode::reset();
        currentChild_ = 0;
        for (auto& child : children_) {
            child->reset();
        }
    }
    
private:
    size_t currentChild_ = 0;
};

// Parallel: Execute all children simultaneously
class BTParallel : public BTNode {
public:
    enum class Policy {
        RequireOne,   // Succeed if one succeeds
        RequireAll    // Succeed only if all succeed
    };
    
    BTParallel(Policy successPolicy = Policy::RequireAll,
               Policy failurePolicy = Policy::RequireOne,
               const std::string& name = "Parallel")
        : BTNode(BTNodeType::Parallel, name),
          successPolicy_(successPolicy), failurePolicy_(failurePolicy) {}
    
    BTStatus update(BTContext& context) override {
        int successCount = 0;
        int failureCount = 0;
        
        for (auto& child : children_) {
            BTStatus status = child->tick(context);
            
            if (status == BTStatus::Success) successCount++;
            else if (status == BTStatus::Failure) failureCount++;
        }
        
        if (failurePolicy_ == Policy::RequireOne && failureCount > 0) {
            return BTStatus::Failure;
        }
        if (failurePolicy_ == Policy::RequireAll && failureCount == (int)children_.size()) {
            return BTStatus::Failure;
        }
        
        if (successPolicy_ == Policy::RequireOne && successCount > 0) {
            return BTStatus::Success;
        }
        if (successPolicy_ == Policy::RequireAll && successCount == (int)children_.size()) {
            return BTStatus::Success;
        }
        
        return BTStatus::Running;
    }
    
private:
    Policy successPolicy_;
    Policy failurePolicy_;
};

// RandomSelector: Randomly select a child to execute
class BTRandomSelector : public BTNode {
public:
    BTRandomSelector(const std::string& name = "RandomSelector")
        : BTNode(BTNodeType::RandomSelector, name) {}
    
    void initialize(BTContext& context) override {
        if (!children_.empty()) {
            selectedChild_ = rand() % children_.size();
        }
        BTNode::initialize(context);
    }
    
    BTStatus update(BTContext& context) override {
        if (selectedChild_ < children_.size()) {
            return children_[selectedChild_]->tick(context);
        }
        return BTStatus::Failure;
    }
    
private:
    size_t selectedChild_ = 0;
};

// ===== Decorator Nodes =====

// Inverter: Invert child's result
class BTInverter : public BTNode {
public:
    BTInverter(const std::string& name = "Inverter")
        : BTNode(BTNodeType::Inverter, name) {}
    
    BTStatus update(BTContext& context) override {
        if (children_.empty()) return BTStatus::Failure;
        
        BTStatus status = children_[0]->tick(context);
        
        if (status == BTStatus::Success) return BTStatus::Failure;
        if (status == BTStatus::Failure) return BTStatus::Success;
        return status;
    }
};

// Succeeder: Always return success
class BTSucceeder : public BTNode {
public:
    BTSucceeder(const std::string& name = "Succeeder")
        : BTNode(BTNodeType::Succeeder, name) {}
    
    BTStatus update(BTContext& context) override {
        if (!children_.empty()) {
            children_[0]->tick(context);
        }
        return BTStatus::Success;
    }
};

// Repeater: Repeat child N times (or forever if count = -1)
class BTRepeater : public BTNode {
public:
    BTRepeater(int count = -1, const std::string& name = "Repeater")
        : BTNode(BTNodeType::Repeater, name), targetCount_(count) {}
    
    void initialize(BTContext& context) override {
        currentCount_ = 0;
        BTNode::initialize(context);
    }
    
    BTStatus update(BTContext& context) override {
        if (children_.empty()) return BTStatus::Success;
        
        while (targetCount_ < 0 || currentCount_ < targetCount_) {
            BTStatus status = children_[0]->tick(context);
            
            if (status == BTStatus::Running) {
                return BTStatus::Running;
            }
            
            if (status == BTStatus::Failure) {
                return BTStatus::Failure;
            }
            
            currentCount_++;
            children_[0]->reset();
        }
        
        return BTStatus::Success;
    }
    
private:
    int targetCount_;
    int currentCount_ = 0;
};

// Limiter: Only allow child to run N times total
class BTLimiter : public BTNode {
public:
    BTLimiter(int limit, const std::string& name = "Limiter")
        : BTNode(BTNodeType::Limiter, name), limit_(limit) {}
    
    BTStatus update(BTContext& context) override {
        if (runCount_ >= limit_) return BTStatus::Failure;
        if (children_.empty()) return BTStatus::Success;
        
        BTStatus status = children_[0]->tick(context);
        
        if (status != BTStatus::Running) {
            runCount_++;
        }
        
        return status;
    }
    
    void reset() override {
        BTNode::reset();
        runCount_ = 0;
    }
    
private:
    int limit_;
    int runCount_ = 0;
};

// ===== Leaf Nodes =====

// Action: Execute a custom function
class BTAction : public BTNode {
public:
    using ActionFunc = std::function<BTStatus(BTContext&)>;
    
    BTAction(ActionFunc func, const std::string& name = "Action")
        : BTNode(BTNodeType::Action, name), action_(func) {}
    
    BTStatus update(BTContext& context) override {
        if (action_) {
            return action_(context);
        }
        return BTStatus::Failure;
    }
    
private:
    ActionFunc action_;
};

// Condition: Check a condition
class BTCondition : public BTNode {
public:
    using ConditionFunc = std::function<bool(BTContext&)>;
    
    BTCondition(ConditionFunc func, const std::string& name = "Condition")
        : BTNode(BTNodeType::Condition, name), condition_(func) {}
    
    BTStatus update(BTContext& context) override {
        if (condition_) {
            return condition_(context) ? BTStatus::Success : BTStatus::Failure;
        }
        return BTStatus::Failure;
    }
    
private:
    ConditionFunc condition_;
};

// Wait: Wait for a duration
class BTWait : public BTNode {
public:
    BTWait(float duration, const std::string& name = "Wait")
        : BTNode(BTNodeType::Wait, name), duration_(duration) {}
    
    void initialize(BTContext& context) override {
        elapsed_ = 0.0f;
        BTNode::initialize(context);
    }
    
    BTStatus update(BTContext& context) override {
        elapsed_ += context.deltaTime;
        
        if (elapsed_ >= duration_) {
            return BTStatus::Success;
        }
        
        return BTStatus::Running;
    }
    
private:
    float duration_;
    float elapsed_ = 0.0f;
};

// Log: Print a message (for debugging)
class BTLog : public BTNode {
public:
    BTLog(const std::string& message, const std::string& name = "Log")
        : BTNode(BTNodeType::Log, name), message_(message) {}
    
    BTStatus update(BTContext& context) override {
        // Would print to console/log
        return BTStatus::Success;
    }
    
private:
    std::string message_;
};

// ===== Behavior Tree =====
class BehaviorTree {
public:
    BehaviorTree(const std::string& name = "BehaviorTree")
        : name_(name) {}
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    void setRoot(std::unique_ptr<BTNode> root) { root_ = std::move(root); }
    BTNode* getRoot() { return root_.get(); }
    
    Blackboard& getBlackboard() { return blackboard_; }
    
    // Tick the tree
    BTStatus tick(float dt, void* owner = nullptr) {
        if (!root_) return BTStatus::Invalid;
        
        context_.blackboard = &blackboard_;
        context_.deltaTime = dt;
        context_.owner = owner;
        
        return root_->tick(context_);
    }
    
    // Reset
    void reset() {
        if (root_) root_->reset();
    }
    
    // Is running
    bool isRunning() const {
        return root_ && root_->getStatus() == BTStatus::Running;
    }
    
private:
    std::string name_;
    std::unique_ptr<BTNode> root_;
    Blackboard blackboard_;
    BTContext context_;
};

// ===== BT Builder (Fluent API) =====
class BTBuilder {
public:
    BTBuilder& sequence(const std::string& name = "Sequence") {
        push(std::make_unique<BTSequence>(name));
        return *this;
    }
    
    BTBuilder& selector(const std::string& name = "Selector") {
        push(std::make_unique<BTSelector>(name));
        return *this;
    }
    
    BTBuilder& parallel(BTParallel::Policy success = BTParallel::Policy::RequireAll,
                        BTParallel::Policy failure = BTParallel::Policy::RequireOne,
                        const std::string& name = "Parallel") {
        push(std::make_unique<BTParallel>(success, failure, name));
        return *this;
    }
    
    BTBuilder& inverter(const std::string& name = "Inverter") {
        push(std::make_unique<BTInverter>(name));
        return *this;
    }
    
    BTBuilder& succeeder(const std::string& name = "Succeeder") {
        push(std::make_unique<BTSucceeder>(name));
        return *this;
    }
    
    BTBuilder& repeater(int count = -1, const std::string& name = "Repeater") {
        push(std::make_unique<BTRepeater>(count, name));
        return *this;
    }
    
    BTBuilder& action(BTAction::ActionFunc func, const std::string& name = "Action") {
        addLeaf(std::make_unique<BTAction>(func, name));
        return *this;
    }
    
    BTBuilder& condition(BTCondition::ConditionFunc func, const std::string& name = "Condition") {
        addLeaf(std::make_unique<BTCondition>(func, name));
        return *this;
    }
    
    BTBuilder& wait(float duration, const std::string& name = "Wait") {
        addLeaf(std::make_unique<BTWait>(duration, name));
        return *this;
    }
    
    BTBuilder& log(const std::string& message, const std::string& name = "Log") {
        addLeaf(std::make_unique<BTLog>(message, name));
        return *this;
    }
    
    BTBuilder& end() {
        if (stack_.size() > 1) {
            auto node = std::move(stack_.back());
            stack_.pop_back();
            stack_.back()->addChild(std::move(node));
        }
        return *this;
    }
    
    std::unique_ptr<BTNode> build() {
        while (stack_.size() > 1) {
            end();
        }
        
        if (!stack_.empty()) {
            return std::move(stack_[0]);
        }
        return nullptr;
    }
    
private:
    void push(std::unique_ptr<BTNode> node) {
        stack_.push_back(std::move(node));
    }
    
    void addLeaf(std::unique_ptr<BTNode> node) {
        if (!stack_.empty()) {
            stack_.back()->addChild(std::move(node));
        } else {
            stack_.push_back(std::move(node));
        }
    }
    
    std::vector<std::unique_ptr<BTNode>> stack_;
};

// ===== Common AI Actions =====
namespace BTActions {
    // Move to position stored in blackboard
    inline BTAction::ActionFunc moveTo(const std::string& targetKey) {
        return [targetKey](BTContext& ctx) -> BTStatus {
            if (!ctx.blackboard->has(targetKey)) return BTStatus::Failure;
            
            Vec3 target = ctx.blackboard->get<Vec3>(targetKey);
            Vec3 pos = ctx.ownerPosition;
            
            float dist = (target - pos).length();
            if (dist < 0.5f) return BTStatus::Success;
            
            // Would move agent towards target
            return BTStatus::Running;
        };
    }
    
    // Check if target is in range
    inline BTCondition::ConditionFunc inRange(const std::string& targetKey, float range) {
        return [targetKey, range](BTContext& ctx) -> bool {
            if (!ctx.blackboard->has(targetKey)) return false;
            
            Vec3 target = ctx.blackboard->get<Vec3>(targetKey);
            Vec3 pos = ctx.ownerPosition;
            
            return (target - pos).length() <= range;
        };
    }
    
    // Check blackboard value
    inline BTCondition::ConditionFunc checkBool(const std::string& key, bool expected = true) {
        return [key, expected](BTContext& ctx) -> bool {
            return ctx.blackboard->get<bool>(key, !expected) == expected;
        };
    }
    
    // Set blackboard value
    template<typename T>
    inline BTAction::ActionFunc setValue(const std::string& key, const T& value) {
        return [key, value](BTContext& ctx) -> BTStatus {
            ctx.blackboard->set(key, value);
            return BTStatus::Success;
        };
    }
}

}  // namespace luma
