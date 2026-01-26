// Command Pattern - Undo/Redo System
// Base infrastructure for reversible operations
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <deque>

namespace luma {

// ===== Command Base Class =====
// All undoable operations inherit from this
class Command {
public:
    virtual ~Command() = default;
    
    // Execute the command (do)
    virtual void execute() = 0;
    
    // Undo the command
    virtual void undo() = 0;
    
    // Redo the command (default: re-execute)
    virtual void redo() { execute(); }
    
    // Get description for UI display
    virtual std::string getDescription() const = 0;
    
    // Can this command be merged with previous? (for continuous drags)
    virtual bool canMergeWith(const Command* other) const { return false; }
    
    // Merge with previous command (for optimization)
    virtual void mergeWith(Command* other) {}
    
    // Get command type ID (for merging)
    virtual const char* getTypeId() const = 0;
};

// ===== Command History =====
// Manages undo/redo stack
class CommandHistory {
public:
    static constexpr size_t kDefaultMaxHistory = 100;
    
    CommandHistory(size_t maxHistory = kDefaultMaxHistory) 
        : maxHistory_(maxHistory) {}
    
    // Execute and record a command
    void execute(std::unique_ptr<Command> cmd) {
        if (!cmd) return;
        
        // Execute the command
        cmd->execute();
        
        // Try to merge with previous command
        if (!undoStack_.empty() && cmd->canMergeWith(undoStack_.back().get())) {
            cmd->mergeWith(undoStack_.back().get());
            undoStack_.back() = std::move(cmd);
        } else {
            // Add to undo stack
            undoStack_.push_back(std::move(cmd));
            
            // Limit history size
            while (undoStack_.size() > maxHistory_) {
                undoStack_.pop_front();
            }
        }
        
        // Clear redo stack (new action invalidates redo)
        redoStack_.clear();
        
        // Notify listeners
        if (onHistoryChanged_) onHistoryChanged_();
    }
    
    // Undo last command
    bool undo() {
        if (undoStack_.empty()) return false;
        
        auto cmd = std::move(undoStack_.back());
        undoStack_.pop_back();
        
        cmd->undo();
        
        redoStack_.push_back(std::move(cmd));
        
        if (onHistoryChanged_) onHistoryChanged_();
        return true;
    }
    
    // Redo last undone command
    bool redo() {
        if (redoStack_.empty()) return false;
        
        auto cmd = std::move(redoStack_.back());
        redoStack_.pop_back();
        
        cmd->redo();
        
        undoStack_.push_back(std::move(cmd));
        
        if (onHistoryChanged_) onHistoryChanged_();
        return true;
    }
    
    // Query state
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    
    size_t undoCount() const { return undoStack_.size(); }
    size_t redoCount() const { return redoStack_.size(); }
    
    // Get descriptions for UI
    std::string getUndoDescription() const {
        return undoStack_.empty() ? "" : undoStack_.back()->getDescription();
    }
    
    std::string getRedoDescription() const {
        return redoStack_.empty() ? "" : redoStack_.back()->getDescription();
    }
    
    // Get all undo descriptions (most recent first)
    std::vector<std::string> getUndoHistory() const {
        std::vector<std::string> result;
        for (auto it = undoStack_.rbegin(); it != undoStack_.rend(); ++it) {
            result.push_back((*it)->getDescription());
        }
        return result;
    }
    
    // Clear all history
    void clear() {
        undoStack_.clear();
        redoStack_.clear();
        if (onHistoryChanged_) onHistoryChanged_();
    }
    
    // Set callback for history changes
    void setOnHistoryChanged(std::function<void()> callback) {
        onHistoryChanged_ = std::move(callback);
    }
    
    // Mark current state as "saved" (for dirty tracking)
    void markSaved() {
        savedUndoCount_ = undoStack_.size();
    }
    
    bool isDirty() const {
        return undoStack_.size() != savedUndoCount_;
    }
    
private:
    std::deque<std::unique_ptr<Command>> undoStack_;
    std::deque<std::unique_ptr<Command>> redoStack_;
    size_t maxHistory_;
    size_t savedUndoCount_ = 0;
    std::function<void()> onHistoryChanged_;
};

// ===== Global Command History =====
inline CommandHistory& getCommandHistory() {
    static CommandHistory instance;
    return instance;
}

// ===== Helper Macros =====
#define EXECUTE_COMMAND(cmd) luma::getCommandHistory().execute(std::make_unique<cmd>)

}  // namespace luma
