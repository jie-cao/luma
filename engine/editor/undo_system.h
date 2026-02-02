// Undo/Redo System - Command Pattern based history management
// Supports unlimited undo/redo with memory limit
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <deque>
#include <unordered_map>

namespace luma {

// ============================================================================
// Command Interface
// ============================================================================

class ICommand {
public:
    virtual ~ICommand() = default;
    
    // Execute the command
    virtual void execute() = 0;
    
    // Undo the command
    virtual void undo() = 0;
    
    // Get description for UI
    virtual std::string getDescription() const = 0;
    
    // Get command type for grouping
    virtual std::string getType() const = 0;
    
    // Can this command be merged with another?
    virtual bool canMergeWith(const ICommand* other) const { return false; }
    
    // Merge with another command (for continuous changes like dragging)
    virtual void mergeWith(const ICommand* other) {}
    
    // Get memory size estimate
    virtual size_t getMemorySize() const { return sizeof(*this); }
    
    // Timestamp
    float timestamp = 0;
};

using CommandPtr = std::shared_ptr<ICommand>;

// ============================================================================
// Common Command Types
// ============================================================================

// === Value Change Command (template for any value type) ===
template<typename T>
class ValueChangeCommand : public ICommand {
public:
    ValueChangeCommand(const std::string& name, T* target, const T& oldValue, const T& newValue)
        : name_(name), target_(target), oldValue_(oldValue), newValue_(newValue) {}
    
    void execute() override {
        *target_ = newValue_;
    }
    
    void undo() override {
        *target_ = oldValue_;
    }
    
    std::string getDescription() const override {
        return "Change " + name_;
    }
    
    std::string getType() const override {
        return "ValueChange:" + name_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const ValueChangeCommand<T>*>(other);
        return cmd && cmd->target_ == target_ && cmd->name_ == name_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const ValueChangeCommand<T>*>(other);
        if (cmd) {
            newValue_ = cmd->newValue_;
        }
    }
    
private:
    std::string name_;
    T* target_;
    T oldValue_;
    T newValue_;
};

// === Float Slider Command ===
class FloatSliderCommand : public ICommand {
public:
    FloatSliderCommand(const std::string& name, float* target, float oldValue, float newValue,
                       std::function<void()> onChange = nullptr)
        : name_(name), target_(target), oldValue_(oldValue), newValue_(newValue), 
          onChange_(onChange) {}
    
    void execute() override {
        *target_ = newValue_;
        if (onChange_) onChange_();
    }
    
    void undo() override {
        *target_ = oldValue_;
        if (onChange_) onChange_();
    }
    
    std::string getDescription() const override {
        return "Adjust " + name_;
    }
    
    std::string getType() const override {
        return "Slider:" + name_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const FloatSliderCommand*>(other);
        return cmd && cmd->target_ == target_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const FloatSliderCommand*>(other);
        if (cmd) {
            newValue_ = cmd->newValue_;
        }
    }
    
private:
    std::string name_;
    float* target_;
    float oldValue_;
    float newValue_;
    std::function<void()> onChange_;
};

// === Color Change Command ===
class ColorChangeCommand : public ICommand {
public:
    ColorChangeCommand(const std::string& name, float* rgb, 
                       const Vec3& oldColor, const Vec3& newColor,
                       std::function<void()> onChange = nullptr)
        : name_(name), rgb_(rgb), oldColor_(oldColor), newColor_(newColor),
          onChange_(onChange) {}
    
    void execute() override {
        rgb_[0] = newColor_.x;
        rgb_[1] = newColor_.y;
        rgb_[2] = newColor_.z;
        if (onChange_) onChange_();
    }
    
    void undo() override {
        rgb_[0] = oldColor_.x;
        rgb_[1] = oldColor_.y;
        rgb_[2] = oldColor_.z;
        if (onChange_) onChange_();
    }
    
    std::string getDescription() const override {
        return "Change " + name_ + " Color";
    }
    
    std::string getType() const override {
        return "Color:" + name_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const ColorChangeCommand*>(other);
        return cmd && cmd->rgb_ == rgb_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const ColorChangeCommand*>(other);
        if (cmd) {
            newColor_ = cmd->newColor_;
        }
    }
    
private:
    std::string name_;
    float* rgb_;
    Vec3 oldColor_;
    Vec3 newColor_;
    std::function<void()> onChange_;
};

// === Transform Command ===
class TransformCommand : public ICommand {
public:
    TransformCommand(const std::string& objectName,
                     const Vec3& oldPos, const Quat& oldRot, const Vec3& oldScale,
                     const Vec3& newPos, const Quat& newRot, const Vec3& newScale,
                     std::function<void(const Vec3&, const Quat&, const Vec3&)> applyFunc)
        : objectName_(objectName),
          oldPos_(oldPos), oldRot_(oldRot), oldScale_(oldScale),
          newPos_(newPos), newRot_(newRot), newScale_(newScale),
          applyFunc_(applyFunc) {}
    
    void execute() override {
        if (applyFunc_) applyFunc_(newPos_, newRot_, newScale_);
    }
    
    void undo() override {
        if (applyFunc_) applyFunc_(oldPos_, oldRot_, oldScale_);
    }
    
    std::string getDescription() const override {
        return "Transform " + objectName_;
    }
    
    std::string getType() const override {
        return "Transform:" + objectName_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const TransformCommand*>(other);
        return cmd && cmd->objectName_ == objectName_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const TransformCommand*>(other);
        if (cmd) {
            newPos_ = cmd->newPos_;
            newRot_ = cmd->newRot_;
            newScale_ = cmd->newScale_;
        }
    }
    
private:
    std::string objectName_;
    Vec3 oldPos_, newPos_;
    Quat oldRot_, newRot_;
    Vec3 oldScale_, newScale_;
    std::function<void(const Vec3&, const Quat&, const Vec3&)> applyFunc_;
};

// === Bone Rotation Command ===
class BoneRotationCommand : public ICommand {
public:
    BoneRotationCommand(const std::string& boneName,
                        const Quat& oldRot, const Quat& newRot,
                        std::function<void(const std::string&, const Quat&)> applyFunc)
        : boneName_(boneName), oldRot_(oldRot), newRot_(newRot), applyFunc_(applyFunc) {}
    
    void execute() override {
        if (applyFunc_) applyFunc_(boneName_, newRot_);
    }
    
    void undo() override {
        if (applyFunc_) applyFunc_(boneName_, oldRot_);
    }
    
    std::string getDescription() const override {
        return "Rotate Bone: " + boneName_;
    }
    
    std::string getType() const override {
        return "BoneRotation:" + boneName_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const BoneRotationCommand*>(other);
        return cmd && cmd->boneName_ == boneName_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const BoneRotationCommand*>(other);
        if (cmd) {
            newRot_ = cmd->newRot_;
        }
    }
    
private:
    std::string boneName_;
    Quat oldRot_;
    Quat newRot_;
    std::function<void(const std::string&, const Quat&)> applyFunc_;
};

// === BlendShape Command ===
class BlendShapeCommand : public ICommand {
public:
    BlendShapeCommand(const std::string& shapeName,
                      float oldWeight, float newWeight,
                      std::function<void(const std::string&, float)> applyFunc)
        : shapeName_(shapeName), oldWeight_(oldWeight), newWeight_(newWeight),
          applyFunc_(applyFunc) {}
    
    void execute() override {
        if (applyFunc_) applyFunc_(shapeName_, newWeight_);
    }
    
    void undo() override {
        if (applyFunc_) applyFunc_(shapeName_, oldWeight_);
    }
    
    std::string getDescription() const override {
        return "BlendShape: " + shapeName_;
    }
    
    std::string getType() const override {
        return "BlendShape:" + shapeName_;
    }
    
    bool canMergeWith(const ICommand* other) const override {
        auto* cmd = dynamic_cast<const BlendShapeCommand*>(other);
        return cmd && cmd->shapeName_ == shapeName_;
    }
    
    void mergeWith(const ICommand* other) override {
        auto* cmd = dynamic_cast<const BlendShapeCommand*>(other);
        if (cmd) {
            newWeight_ = cmd->newWeight_;
        }
    }
    
private:
    std::string shapeName_;
    float oldWeight_;
    float newWeight_;
    std::function<void(const std::string&, float)> applyFunc_;
};

// === Preset Apply Command ===
class PresetApplyCommand : public ICommand {
public:
    PresetApplyCommand(const std::string& presetName,
                       std::function<void()> applyFunc,
                       std::function<void()> revertFunc)
        : presetName_(presetName), applyFunc_(applyFunc), revertFunc_(revertFunc) {}
    
    void execute() override {
        if (applyFunc_) applyFunc_();
    }
    
    void undo() override {
        if (revertFunc_) revertFunc_();
    }
    
    std::string getDescription() const override {
        return "Apply Preset: " + presetName_;
    }
    
    std::string getType() const override {
        return "Preset";
    }
    
private:
    std::string presetName_;
    std::function<void()> applyFunc_;
    std::function<void()> revertFunc_;
};

// === Composite Command (for grouping multiple commands) ===
class CompositeCommand : public ICommand {
public:
    CompositeCommand(const std::string& description)
        : description_(description) {}
    
    void addCommand(CommandPtr cmd) {
        commands_.push_back(cmd);
    }
    
    void execute() override {
        for (auto& cmd : commands_) {
            cmd->execute();
        }
    }
    
    void undo() override {
        // Undo in reverse order
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo();
        }
    }
    
    std::string getDescription() const override {
        return description_;
    }
    
    std::string getType() const override {
        return "Composite";
    }
    
    size_t getMemorySize() const override {
        size_t size = sizeof(*this);
        for (const auto& cmd : commands_) {
            size += cmd->getMemorySize();
        }
        return size;
    }
    
private:
    std::string description_;
    std::vector<CommandPtr> commands_;
};

// === Lambda Command (for simple one-off commands) ===
class LambdaCommand : public ICommand {
public:
    LambdaCommand(const std::string& description,
                  std::function<void()> doFunc,
                  std::function<void()> undoFunc)
        : description_(description), doFunc_(doFunc), undoFunc_(undoFunc) {}
    
    void execute() override {
        if (doFunc_) doFunc_();
    }
    
    void undo() override {
        if (undoFunc_) undoFunc_();
    }
    
    std::string getDescription() const override {
        return description_;
    }
    
    std::string getType() const override {
        return "Lambda";
    }
    
private:
    std::string description_;
    std::function<void()> doFunc_;
    std::function<void()> undoFunc_;
};

// ============================================================================
// Command History Manager
// ============================================================================

class CommandHistory {
public:
    static CommandHistory& getInstance() {
        static CommandHistory instance;
        return instance;
    }
    
    // Execute a command and add to history
    void execute(CommandPtr command) {
        if (!command) return;
        
        command->timestamp = currentTime_;
        
        // Check if we can merge with the last command
        if (mergeEnabled_ && !undoStack_.empty() && 
            (currentTime_ - undoStack_.back()->timestamp) < mergeTimeWindow_) {
            
            if (undoStack_.back()->canMergeWith(command.get())) {
                undoStack_.back()->mergeWith(command.get());
                undoStack_.back()->execute();
                return;
            }
        }
        
        // Execute the command
        command->execute();
        
        // Clear redo stack
        redoStack_.clear();
        
        // Add to undo stack
        undoStack_.push_back(command);
        
        // Enforce memory limit
        enforceMemoryLimit();
        
        // Notify listeners
        notifyChange();
    }
    
    // Undo the last command
    bool undo() {
        if (undoStack_.empty()) return false;
        
        auto command = undoStack_.back();
        undoStack_.pop_back();
        
        command->undo();
        redoStack_.push_back(command);
        
        notifyChange();
        return true;
    }
    
    // Redo the last undone command
    bool redo() {
        if (redoStack_.empty()) return false;
        
        auto command = redoStack_.back();
        redoStack_.pop_back();
        
        command->execute();
        undoStack_.push_back(command);
        
        notifyChange();
        return true;
    }
    
    // Check if undo is available
    bool canUndo() const {
        return !undoStack_.empty();
    }
    
    // Check if redo is available
    bool canRedo() const {
        return !redoStack_.empty();
    }
    
    // Get description of next undo
    std::string getUndoDescription() const {
        if (undoStack_.empty()) return "";
        return undoStack_.back()->getDescription();
    }
    
    // Get description of next redo
    std::string getRedoDescription() const {
        if (redoStack_.empty()) return "";
        return redoStack_.back()->getDescription();
    }
    
    // Get history for display
    std::vector<std::string> getUndoHistory(int maxItems = 20) const {
        std::vector<std::string> history;
        int count = std::min(maxItems, static_cast<int>(undoStack_.size()));
        for (int i = 0; i < count; i++) {
            history.push_back(undoStack_[undoStack_.size() - 1 - i]->getDescription());
        }
        return history;
    }
    
    std::vector<std::string> getRedoHistory(int maxItems = 20) const {
        std::vector<std::string> history;
        int count = std::min(maxItems, static_cast<int>(redoStack_.size()));
        for (int i = 0; i < count; i++) {
            history.push_back(redoStack_[redoStack_.size() - 1 - i]->getDescription());
        }
        return history;
    }
    
    // Clear all history
    void clear() {
        undoStack_.clear();
        redoStack_.clear();
        notifyChange();
    }
    
    // Get stack sizes
    size_t getUndoCount() const { return undoStack_.size(); }
    size_t getRedoCount() const { return redoStack_.size(); }
    
    // Settings
    void setMaxUndoCount(size_t count) { maxUndoCount_ = count; }
    void setMaxMemoryBytes(size_t bytes) { maxMemoryBytes_ = bytes; }
    void setMergeEnabled(bool enabled) { mergeEnabled_ = enabled; }
    void setMergeTimeWindow(float seconds) { mergeTimeWindow_ = seconds; }
    
    // Update time (call each frame)
    void update(float deltaTime) {
        currentTime_ += deltaTime;
    }
    
    // Begin a compound command (groups multiple commands)
    void beginCompound(const std::string& description) {
        if (compoundCommand_) return;  // Already in compound mode
        compoundCommand_ = std::make_shared<CompositeCommand>(description);
    }
    
    // End compound command
    void endCompound() {
        if (!compoundCommand_) return;
        if (dynamic_cast<CompositeCommand*>(compoundCommand_.get())->getMemorySize() > sizeof(CompositeCommand)) {
            execute(compoundCommand_);
        }
        compoundCommand_ = nullptr;
    }
    
    // Add to current compound or execute directly
    void executeOrAddToCompound(CommandPtr command) {
        if (compoundCommand_) {
            command->execute();
            dynamic_cast<CompositeCommand*>(compoundCommand_.get())->addCommand(command);
        } else {
            execute(command);
        }
    }
    
    // Register change listener
    void addChangeListener(std::function<void()> listener) {
        changeListeners_.push_back(listener);
    }
    
    // Mark current state as saved (for dirty tracking)
    void markSaved() {
        savedIndex_ = undoStack_.size();
    }
    
    // Check if document is dirty (has unsaved changes)
    bool isDirty() const {
        return undoStack_.size() != savedIndex_;
    }
    
private:
    CommandHistory() = default;
    
    void enforceMemoryLimit() {
        // Enforce count limit
        while (undoStack_.size() > maxUndoCount_) {
            undoStack_.pop_front();
        }
        
        // Enforce memory limit
        size_t totalMemory = 0;
        for (const auto& cmd : undoStack_) {
            totalMemory += cmd->getMemorySize();
        }
        
        while (totalMemory > maxMemoryBytes_ && !undoStack_.empty()) {
            totalMemory -= undoStack_.front()->getMemorySize();
            undoStack_.pop_front();
        }
    }
    
    void notifyChange() {
        for (auto& listener : changeListeners_) {
            listener();
        }
    }
    
    std::deque<CommandPtr> undoStack_;
    std::deque<CommandPtr> redoStack_;
    
    CommandPtr compoundCommand_;
    
    size_t maxUndoCount_ = 100;
    size_t maxMemoryBytes_ = 100 * 1024 * 1024;  // 100 MB
    bool mergeEnabled_ = true;
    float mergeTimeWindow_ = 0.5f;  // Merge commands within 0.5 seconds
    float currentTime_ = 0;
    
    size_t savedIndex_ = 0;
    
    std::vector<std::function<void()>> changeListeners_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline CommandHistory& getCommandHistory() {
    return CommandHistory::getInstance();
}

// Execute a simple lambda command
inline void executeCommand(const std::string& description,
                           std::function<void()> doFunc,
                           std::function<void()> undoFunc) {
    getCommandHistory().execute(std::make_shared<LambdaCommand>(description, doFunc, undoFunc));
}

// Execute a float slider command
inline void executeSliderCommand(const std::string& name, float* target, 
                                  float oldValue, float newValue,
                                  std::function<void()> onChange = nullptr) {
    getCommandHistory().execute(
        std::make_shared<FloatSliderCommand>(name, target, oldValue, newValue, onChange));
}

// Execute a color change command
inline void executeColorCommand(const std::string& name, float* rgb,
                                 const Vec3& oldColor, const Vec3& newColor,
                                 std::function<void()> onChange = nullptr) {
    getCommandHistory().execute(
        std::make_shared<ColorChangeCommand>(name, rgb, oldColor, newColor, onChange));
}

// Macro for easy undo/redo integration with sliders
#define UNDO_SLIDER(name, target, onChange) \
    do { \
        static float _undoOldValue = target; \
        if (ImGui::IsItemActivated()) { \
            _undoOldValue = target; \
        } \
        if (ImGui::IsItemDeactivatedAfterEdit()) { \
            executeSliderCommand(name, &target, _undoOldValue, target, onChange); \
        } \
    } while(0)

}  // namespace luma
