// History Panel - Visual undo/redo history
// Display and navigate command history
#pragma once

#include "engine/editor/undo_system.h"
#include <string>
#include <vector>
#include <functional>

namespace luma {

// ============================================================================
// History Entry - Visual representation of a command
// ============================================================================

struct HistoryEntry {
    int index = 0;
    std::string description;
    std::string type;
    std::string timestamp;
    
    bool isCurrent = false;
    bool isUndone = false;
    
    // Visual
    std::string icon;
    Vec3 color{0.8f, 0.8f, 0.8f};
};

// ============================================================================
// History Panel State
// ============================================================================

struct HistoryPanelState {
    // Display
    bool showTimestamps = false;
    bool showIcons = true;
    bool groupSimilar = true;
    int maxVisibleItems = 50;
    
    // Navigation
    int selectedIndex = -1;
    int scrollOffset = 0;
    
    // Filtering
    std::string filterText;
    std::vector<std::string> filterTypes;
    
    // Actions
    bool confirmBeforeJump = false;
};

// ============================================================================
// History Panel
// ============================================================================

class HistoryPanel {
public:
    static HistoryPanel& getInstance() {
        static HistoryPanel instance;
        return instance;
    }
    
    void initialize() {
        // Register with command history for updates
        getCommandHistory().addChangeListener([this]() {
            refresh();
        });
        
        initialized_ = true;
    }
    
    // Get display entries
    std::vector<HistoryEntry> getEntries() const {
        std::vector<HistoryEntry> entries;
        
        auto undoHistory = getCommandHistory().getUndoHistory(state_.maxVisibleItems);
        auto redoHistory = getCommandHistory().getRedoHistory(state_.maxVisibleItems);
        
        // Add redo items (future)
        int index = static_cast<int>(undoHistory.size()) + 
                    static_cast<int>(redoHistory.size()) - 1;
        
        for (auto it = redoHistory.rbegin(); it != redoHistory.rend(); ++it, --index) {
            HistoryEntry entry;
            entry.index = index;
            entry.description = *it;
            entry.isUndone = true;
            entry.isCurrent = false;
            entry.icon = getIconForType(entry.type);
            entry.color = Vec3(0.5f, 0.5f, 0.5f);  // Grayed out
            
            if (passesFilter(entry)) {
                entries.push_back(entry);
            }
        }
        
        // Add current state marker
        {
            HistoryEntry current;
            current.index = static_cast<int>(undoHistory.size());
            current.description = "Current State";
            current.isCurrent = true;
            current.color = Vec3(0.3f, 0.8f, 0.3f);
            entries.push_back(current);
        }
        
        // Add undo items (past)
        index = static_cast<int>(undoHistory.size()) - 1;
        for (const auto& desc : undoHistory) {
            HistoryEntry entry;
            entry.index = index--;
            entry.description = desc;
            entry.isUndone = false;
            entry.isCurrent = false;
            entry.icon = getIconForType(entry.type);
            entry.color = Vec3(0.8f, 0.8f, 0.8f);
            
            if (passesFilter(entry)) {
                entries.push_back(entry);
            }
        }
        
        return entries;
    }
    
    // Navigation
    void jumpToIndex(int index) {
        int currentIndex = static_cast<int>(getCommandHistory().getUndoCount());
        
        if (index < currentIndex) {
            // Undo multiple times
            int undoCount = currentIndex - index;
            for (int i = 0; i < undoCount; i++) {
                getCommandHistory().undo();
            }
        } else if (index > currentIndex) {
            // Redo multiple times
            int redoCount = index - currentIndex;
            for (int i = 0; i < redoCount; i++) {
                getCommandHistory().redo();
            }
        }
        
        if (onJump_) {
            onJump_(index);
        }
    }
    
    void undoToSelected() {
        if (state_.selectedIndex >= 0) {
            jumpToIndex(state_.selectedIndex);
        }
    }
    
    // Selection
    void select(int index) {
        state_.selectedIndex = index;
        
        if (onSelect_) {
            onSelect_(index);
        }
    }
    
    void clearSelection() {
        state_.selectedIndex = -1;
    }
    
    // Actions
    void clearHistory() {
        getCommandHistory().clear();
        refresh();
    }
    
    void markSavePoint() {
        getCommandHistory().markSaved();
        refresh();
    }
    
    // State
    HistoryPanelState& getState() { return state_; }
    const HistoryPanelState& getState() const { return state_; }
    
    // Statistics
    int getUndoCount() const {
        return static_cast<int>(getCommandHistory().getUndoCount());
    }
    
    int getRedoCount() const {
        return static_cast<int>(getCommandHistory().getRedoCount());
    }
    
    bool canUndo() const {
        return getCommandHistory().canUndo();
    }
    
    bool canRedo() const {
        return getCommandHistory().canRedo();
    }
    
    bool isDirty() const {
        return getCommandHistory().isDirty();
    }
    
    // Callbacks
    void setOnSelect(std::function<void(int)> callback) {
        onSelect_ = callback;
    }
    
    void setOnJump(std::function<void(int)> callback) {
        onJump_ = callback;
    }
    
    void setOnRefresh(std::function<void()> callback) {
        onRefresh_ = callback;
    }
    
private:
    HistoryPanel() = default;
    
    void refresh() {
        if (onRefresh_) {
            onRefresh_();
        }
    }
    
    bool passesFilter(const HistoryEntry& entry) const {
        // Text filter
        if (!state_.filterText.empty()) {
            std::string lowerDesc = entry.description;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
            
            std::string lowerFilter = state_.filterText;
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
            
            if (lowerDesc.find(lowerFilter) == std::string::npos) {
                return false;
            }
        }
        
        // Type filter
        if (!state_.filterTypes.empty()) {
            if (std::find(state_.filterTypes.begin(), state_.filterTypes.end(), entry.type) 
                == state_.filterTypes.end()) {
                return false;
            }
        }
        
        return true;
    }
    
    std::string getIconForType(const std::string& type) const {
        if (type.find("Slider") != std::string::npos) return "📊";
        if (type.find("Color") != std::string::npos) return "🎨";
        if (type.find("Transform") != std::string::npos) return "↔️";
        if (type.find("Bone") != std::string::npos) return "🦴";
        if (type.find("BlendShape") != std::string::npos) return "😀";
        if (type.find("Preset") != std::string::npos) return "📋";
        return "📝";
    }
    
    HistoryPanelState state_;
    bool initialized_ = false;
    
    std::function<void(int)> onSelect_;
    std::function<void(int)> onJump_;
    std::function<void()> onRefresh_;
};

inline HistoryPanel& getHistoryPanel() {
    return HistoryPanel::getInstance();
}

}  // namespace luma
