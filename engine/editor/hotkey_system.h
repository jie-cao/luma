// Hotkey System - Customizable keyboard shortcuts
// Supports key combinations, context-aware shortcuts, and user customization
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>

namespace luma {

// ============================================================================
// Key Codes (platform-independent)
// ============================================================================

enum class KeyCode {
    None = 0,
    
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Special keys
    Space, Enter, Escape, Tab, Backspace, Delete, Insert,
    Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    
    // Punctuation
    Comma, Period, Slash, Semicolon, Quote, 
    LeftBracket, RightBracket, Backslash, Grave, Minus, Equal,
    
    // Numpad
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide,
    NumpadEnter, NumpadDecimal,
    
    COUNT
};

// Modifier flags
enum class KeyModifier : uint8_t {
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3  // Cmd on macOS, Win on Windows
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasModifier(KeyModifier mods, KeyModifier check) {
    return (static_cast<uint8_t>(mods) & static_cast<uint8_t>(check)) != 0;
}

// ============================================================================
// Key Binding
// ============================================================================

struct KeyBinding {
    KeyCode key = KeyCode::None;
    KeyModifier modifiers = KeyModifier::None;
    
    bool isValid() const { return key != KeyCode::None; }
    
    bool matches(KeyCode k, KeyModifier mods) const {
        return key == k && modifiers == mods;
    }
    
    std::string toString() const;
    static KeyBinding fromString(const std::string& str);
    
    bool operator==(const KeyBinding& other) const {
        return key == other.key && modifiers == other.modifiers;
    }
};

// ============================================================================
// Hotkey Action Definition
// ============================================================================

struct HotkeyAction {
    std::string id;              // Unique identifier (e.g., "edit.undo")
    std::string name;            // Display name
    std::string nameCN;          // Chinese name
    std::string category;        // Category for grouping
    std::string description;
    
    KeyBinding defaultBinding;   // Default key binding
    KeyBinding userBinding;      // User-customized binding
    
    std::function<void()> callback;
    std::function<bool()> isEnabled;  // Optional check if action is available
    
    bool isGlobal = true;        // Works in any context
    std::string context;         // Specific context where this works
    
    KeyBinding getActiveBinding() const {
        return userBinding.isValid() ? userBinding : defaultBinding;
    }
};

// ============================================================================
// Hotkey Manager
// ============================================================================

class HotkeyManager {
public:
    static HotkeyManager& getInstance() {
        static HotkeyManager instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        registerDefaultHotkeys();
        initialized_ = true;
    }
    
    // Register a new action
    void registerAction(const HotkeyAction& action) {
        actions_[action.id] = action;
        rebuildLookup();
    }
    
    // Set callback for an action
    void setCallback(const std::string& actionId, std::function<void()> callback) {
        if (actions_.find(actionId) != actions_.end()) {
            actions_[actionId].callback = callback;
        }
    }
    
    // Set enabled check for an action
    void setEnabledCheck(const std::string& actionId, std::function<bool()> check) {
        if (actions_.find(actionId) != actions_.end()) {
            actions_[actionId].isEnabled = check;
        }
    }
    
    // Handle key press - returns true if handled
    bool handleKeyPress(KeyCode key, KeyModifier modifiers, const std::string& context = "") {
        KeyBinding binding{key, modifiers};
        
        // First check context-specific bindings
        if (!context.empty()) {
            auto it = bindingLookup_.find(binding);
            if (it != bindingLookup_.end()) {
                for (const auto& actionId : it->second) {
                    auto& action = actions_[actionId];
                    if (action.context == context || action.isGlobal) {
                        if (!action.isEnabled || action.isEnabled()) {
                            if (action.callback) {
                                action.callback();
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        // Then check global bindings
        auto it = bindingLookup_.find(binding);
        if (it != bindingLookup_.end()) {
            for (const auto& actionId : it->second) {
                auto& action = actions_[actionId];
                if (action.isGlobal) {
                    if (!action.isEnabled || action.isEnabled()) {
                        if (action.callback) {
                            action.callback();
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }
    
    // Get all actions
    const std::unordered_map<std::string, HotkeyAction>& getActions() const {
        return actions_;
    }
    
    // Get actions by category
    std::vector<const HotkeyAction*> getActionsByCategory(const std::string& category) const {
        std::vector<const HotkeyAction*> result;
        for (const auto& [id, action] : actions_) {
            if (action.category == category) {
                result.push_back(&action);
            }
        }
        return result;
    }
    
    // Get all categories
    std::vector<std::string> getCategories() const {
        std::vector<std::string> categories;
        for (const auto& [id, action] : actions_) {
            if (std::find(categories.begin(), categories.end(), action.category) == categories.end()) {
                categories.push_back(action.category);
            }
        }
        return categories;
    }
    
    // Set user binding
    void setUserBinding(const std::string& actionId, const KeyBinding& binding) {
        if (actions_.find(actionId) != actions_.end()) {
            actions_[actionId].userBinding = binding;
            rebuildLookup();
        }
    }
    
    // Reset to default
    void resetToDefault(const std::string& actionId) {
        if (actions_.find(actionId) != actions_.end()) {
            actions_[actionId].userBinding = KeyBinding{};
            rebuildLookup();
        }
    }
    
    // Reset all to defaults
    void resetAllToDefaults() {
        for (auto& [id, action] : actions_) {
            action.userBinding = KeyBinding{};
        }
        rebuildLookup();
    }
    
    // Check for conflicts
    std::vector<std::string> findConflicts(const std::string& actionId, 
                                            const KeyBinding& binding) const {
        std::vector<std::string> conflicts;
        for (const auto& [id, action] : actions_) {
            if (id != actionId && action.getActiveBinding() == binding) {
                conflicts.push_back(id);
            }
        }
        return conflicts;
    }
    
    // Save user bindings to file
    bool saveBindings(const std::string& path) const {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        
        file << "# LUMA Hotkey Bindings\n";
        file << "# Format: action_id = key_binding\n\n";
        
        for (const auto& [id, action] : actions_) {
            if (action.userBinding.isValid()) {
                file << id << " = " << action.userBinding.toString() << "\n";
            }
        }
        
        return true;
    }
    
    // Load user bindings from file
    bool loadBindings(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string actionId = line.substr(0, pos);
            std::string bindingStr = line.substr(pos + 1);
            
            // Trim whitespace
            while (!actionId.empty() && actionId.back() == ' ') actionId.pop_back();
            while (!bindingStr.empty() && bindingStr.front() == ' ') bindingStr = bindingStr.substr(1);
            
            if (actions_.find(actionId) != actions_.end()) {
                actions_[actionId].userBinding = KeyBinding::fromString(bindingStr);
            }
        }
        
        rebuildLookup();
        return true;
    }
    
    // Get binding display string for an action
    std::string getBindingString(const std::string& actionId) const {
        auto it = actions_.find(actionId);
        if (it == actions_.end()) return "";
        return it->second.getActiveBinding().toString();
    }
    
    // Set current context
    void setContext(const std::string& context) {
        currentContext_ = context;
    }
    
    const std::string& getContext() const {
        return currentContext_;
    }
    
private:
    HotkeyManager() = default;
    
    void registerDefaultHotkeys() {
        // === File Operations ===
        registerAction({
            "file.new", "New Project", "新建项目", "File",
            "Create a new project",
            {KeyCode::N, KeyModifier::Ctrl}
        });
        
        registerAction({
            "file.open", "Open Project", "打开项目", "File",
            "Open an existing project",
            {KeyCode::O, KeyModifier::Ctrl}
        });
        
        registerAction({
            "file.save", "Save Project", "保存项目", "File",
            "Save the current project",
            {KeyCode::S, KeyModifier::Ctrl}
        });
        
        registerAction({
            "file.save_as", "Save As", "另存为", "File",
            "Save with a new name",
            {KeyCode::S, KeyModifier::Ctrl | KeyModifier::Shift}
        });
        
        registerAction({
            "file.export", "Export", "导出", "File",
            "Export character",
            {KeyCode::E, KeyModifier::Ctrl}
        });
        
        // === Edit Operations ===
        registerAction({
            "edit.undo", "Undo", "撤销", "Edit",
            "Undo the last action",
            {KeyCode::Z, KeyModifier::Ctrl}
        });
        
        registerAction({
            "edit.redo", "Redo", "重做", "Edit",
            "Redo the last undone action",
            {KeyCode::Z, KeyModifier::Ctrl | KeyModifier::Shift}
        });
        
        registerAction({
            "edit.redo_alt", "Redo (Alt)", "重做", "Edit",
            "Redo the last undone action",
            {KeyCode::Y, KeyModifier::Ctrl}
        });
        
        registerAction({
            "edit.copy", "Copy", "复制", "Edit",
            "Copy selection",
            {KeyCode::C, KeyModifier::Ctrl}
        });
        
        registerAction({
            "edit.paste", "Paste", "粘贴", "Edit",
            "Paste from clipboard",
            {KeyCode::V, KeyModifier::Ctrl}
        });
        
        registerAction({
            "edit.delete", "Delete", "删除", "Edit",
            "Delete selection",
            {KeyCode::Delete, KeyModifier::None}
        });
        
        registerAction({
            "edit.select_all", "Select All", "全选", "Edit",
            "Select all",
            {KeyCode::A, KeyModifier::Ctrl}
        });
        
        registerAction({
            "edit.deselect", "Deselect", "取消选择", "Edit",
            "Deselect all",
            {KeyCode::D, KeyModifier::Ctrl}
        });
        
        // === View Operations ===
        registerAction({
            "view.reset_camera", "Reset Camera", "重置相机", "View",
            "Reset camera to default view",
            {KeyCode::Home, KeyModifier::None}
        });
        
        registerAction({
            "view.front", "Front View", "前视图", "View",
            "Switch to front view",
            {KeyCode::Numpad1, KeyModifier::None}
        });
        
        registerAction({
            "view.back", "Back View", "后视图", "View",
            "Switch to back view",
            {KeyCode::Numpad1, KeyModifier::Ctrl}
        });
        
        registerAction({
            "view.right", "Right View", "右视图", "View",
            "Switch to right view",
            {KeyCode::Numpad3, KeyModifier::None}
        });
        
        registerAction({
            "view.left", "Left View", "左视图", "View",
            "Switch to left view",
            {KeyCode::Numpad3, KeyModifier::Ctrl}
        });
        
        registerAction({
            "view.top", "Top View", "顶视图", "View",
            "Switch to top view",
            {KeyCode::Numpad7, KeyModifier::None}
        });
        
        registerAction({
            "view.bottom", "Bottom View", "底视图", "View",
            "Switch to bottom view",
            {KeyCode::Numpad7, KeyModifier::Ctrl}
        });
        
        registerAction({
            "view.perspective", "Perspective", "透视图", "View",
            "Switch to perspective view",
            {KeyCode::Numpad5, KeyModifier::None}
        });
        
        registerAction({
            "view.fullscreen", "Toggle Fullscreen", "全屏", "View",
            "Toggle fullscreen mode",
            {KeyCode::F11, KeyModifier::None}
        });
        
        registerAction({
            "view.wireframe", "Toggle Wireframe", "线框模式", "View",
            "Toggle wireframe display",
            {KeyCode::Z, KeyModifier::None}
        });
        
        // === Transform Tools ===
        registerAction({
            "tool.select", "Select Tool", "选择工具", "Tools",
            "Switch to select tool",
            {KeyCode::Q, KeyModifier::None}
        });
        
        registerAction({
            "tool.move", "Move Tool", "移动工具", "Tools",
            "Switch to move tool",
            {KeyCode::W, KeyModifier::None}
        });
        
        registerAction({
            "tool.rotate", "Rotate Tool", "旋转工具", "Tools",
            "Switch to rotate tool",
            {KeyCode::E, KeyModifier::None}
        });
        
        registerAction({
            "tool.scale", "Scale Tool", "缩放工具", "Tools",
            "Switch to scale tool",
            {KeyCode::R, KeyModifier::None}
        });
        
        registerAction({
            "tool.toggle_local", "Toggle Local/World", "切换局部/世界坐标", "Tools",
            "Toggle between local and world coordinate space",
            {KeyCode::X, KeyModifier::None}
        });
        
        registerAction({
            "tool.snap", "Toggle Snap", "切换吸附", "Tools",
            "Toggle snapping",
            {KeyCode::X, KeyModifier::Ctrl}
        });
        
        // === Animation ===
        registerAction({
            "anim.play_pause", "Play/Pause", "播放/暂停", "Animation",
            "Toggle animation playback",
            {KeyCode::Space, KeyModifier::None}
        });
        
        registerAction({
            "anim.stop", "Stop", "停止", "Animation",
            "Stop animation and reset",
            {KeyCode::Escape, KeyModifier::None}
        });
        
        registerAction({
            "anim.next_frame", "Next Frame", "下一帧", "Animation",
            "Go to next frame",
            {KeyCode::Right, KeyModifier::None}
        });
        
        registerAction({
            "anim.prev_frame", "Previous Frame", "上一帧", "Animation",
            "Go to previous frame",
            {KeyCode::Left, KeyModifier::None}
        });
        
        registerAction({
            "anim.first_frame", "First Frame", "第一帧", "Animation",
            "Go to first frame",
            {KeyCode::Home, KeyModifier::Ctrl}
        });
        
        registerAction({
            "anim.last_frame", "Last Frame", "最后一帧", "Animation",
            "Go to last frame",
            {KeyCode::End, KeyModifier::Ctrl}
        });
        
        registerAction({
            "anim.add_keyframe", "Add Keyframe", "添加关键帧", "Animation",
            "Add keyframe at current time",
            {KeyCode::K, KeyModifier::None}
        });
        
        registerAction({
            "anim.delete_keyframe", "Delete Keyframe", "删除关键帧", "Animation",
            "Delete selected keyframe",
            {KeyCode::K, KeyModifier::Shift}
        });
        
        // === Character Creator ===
        registerAction({
            "char.randomize", "Randomize", "随机生成", "Character",
            "Randomize character appearance",
            {KeyCode::R, KeyModifier::Ctrl | KeyModifier::Shift}
        });
        
        registerAction({
            "char.reset", "Reset", "重置", "Character",
            "Reset to default",
            {KeyCode::R, KeyModifier::Ctrl | KeyModifier::Alt}
        });
        
        registerAction({
            "char.mirror", "Mirror Pose", "镜像姿势", "Character",
            "Mirror the current pose",
            {KeyCode::M, KeyModifier::Ctrl}
        });
        
        // === Rendering ===
        registerAction({
            "render.screenshot", "Screenshot", "截图", "Render",
            "Take a screenshot",
            {KeyCode::F12, KeyModifier::None}
        });
        
        registerAction({
            "render.high_quality", "High Quality Render", "高质量渲染", "Render",
            "Render with high quality settings",
            {KeyCode::F12, KeyModifier::Shift}
        });
        
        // === Window ===
        registerAction({
            "window.hierarchy", "Toggle Hierarchy", "切换层级面板", "Window",
            "Toggle hierarchy panel",
            {KeyCode::H, KeyModifier::Ctrl | KeyModifier::Shift}
        });
        
        registerAction({
            "window.inspector", "Toggle Inspector", "切换检查器", "Window",
            "Toggle inspector panel",
            {KeyCode::I, KeyModifier::Ctrl | KeyModifier::Shift}
        });
        
        registerAction({
            "window.character", "Character Creator", "角色创建器", "Window",
            "Toggle character creator panel",
            {KeyCode::C, KeyModifier::Ctrl | KeyModifier::Shift}
        });
    }
    
    void rebuildLookup() {
        bindingLookup_.clear();
        for (const auto& [id, action] : actions_) {
            KeyBinding binding = action.getActiveBinding();
            if (binding.isValid()) {
                bindingLookup_[binding].push_back(id);
            }
        }
    }
    
    std::unordered_map<std::string, HotkeyAction> actions_;
    
    struct KeyBindingHash {
        size_t operator()(const KeyBinding& b) const {
            return std::hash<int>()(static_cast<int>(b.key)) ^
                   (std::hash<int>()(static_cast<int>(b.modifiers)) << 8);
        }
    };
    
    std::unordered_map<KeyBinding, std::vector<std::string>, KeyBindingHash> bindingLookup_;
    
    std::string currentContext_;
    bool initialized_ = false;
};

// ============================================================================
// KeyBinding String Conversion
// ============================================================================

inline std::string keyCodeToString(KeyCode key) {
    switch (key) {
        case KeyCode::A: return "A"; case KeyCode::B: return "B";
        case KeyCode::C: return "C"; case KeyCode::D: return "D";
        case KeyCode::E: return "E"; case KeyCode::F: return "F";
        case KeyCode::G: return "G"; case KeyCode::H: return "H";
        case KeyCode::I: return "I"; case KeyCode::J: return "J";
        case KeyCode::K: return "K"; case KeyCode::L: return "L";
        case KeyCode::M: return "M"; case KeyCode::N: return "N";
        case KeyCode::O: return "O"; case KeyCode::P: return "P";
        case KeyCode::Q: return "Q"; case KeyCode::R: return "R";
        case KeyCode::S: return "S"; case KeyCode::T: return "T";
        case KeyCode::U: return "U"; case KeyCode::V: return "V";
        case KeyCode::W: return "W"; case KeyCode::X: return "X";
        case KeyCode::Y: return "Y"; case KeyCode::Z: return "Z";
        
        case KeyCode::Num0: return "0"; case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2"; case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4"; case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6"; case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8"; case KeyCode::Num9: return "9";
        
        case KeyCode::F1: return "F1"; case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3"; case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5"; case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7"; case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9"; case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11"; case KeyCode::F12: return "F12";
        
        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Delete";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";
        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        
        case KeyCode::Numpad0: return "Num0";
        case KeyCode::Numpad1: return "Num1";
        case KeyCode::Numpad2: return "Num2";
        case KeyCode::Numpad3: return "Num3";
        case KeyCode::Numpad4: return "Num4";
        case KeyCode::Numpad5: return "Num5";
        case KeyCode::Numpad6: return "Num6";
        case KeyCode::Numpad7: return "Num7";
        case KeyCode::Numpad8: return "Num8";
        case KeyCode::Numpad9: return "Num9";
        
        default: return "Unknown";
    }
}

inline std::string KeyBinding::toString() const {
    std::string result;
    
    if (hasModifier(modifiers, KeyModifier::Ctrl)) result += "Ctrl+";
    if (hasModifier(modifiers, KeyModifier::Shift)) result += "Shift+";
    if (hasModifier(modifiers, KeyModifier::Alt)) result += "Alt+";
    if (hasModifier(modifiers, KeyModifier::Super)) {
        #ifdef __APPLE__
        result += "Cmd+";
        #else
        result += "Win+";
        #endif
    }
    
    result += keyCodeToString(key);
    return result;
}

inline KeyBinding KeyBinding::fromString(const std::string& str) {
    KeyBinding binding;
    
    std::string remaining = str;
    
    // Parse modifiers
    if (remaining.find("Ctrl+") == 0) {
        binding.modifiers = binding.modifiers | KeyModifier::Ctrl;
        remaining = remaining.substr(5);
    }
    if (remaining.find("Shift+") == 0) {
        binding.modifiers = binding.modifiers | KeyModifier::Shift;
        remaining = remaining.substr(6);
    }
    if (remaining.find("Alt+") == 0) {
        binding.modifiers = binding.modifiers | KeyModifier::Alt;
        remaining = remaining.substr(4);
    }
    if (remaining.find("Cmd+") == 0 || remaining.find("Win+") == 0) {
        binding.modifiers = binding.modifiers | KeyModifier::Super;
        remaining = remaining.substr(4);
    }
    
    // Parse key
    if (remaining.length() == 1) {
        char c = remaining[0];
        if (c >= 'A' && c <= 'Z') {
            binding.key = static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (c - 'A'));
        } else if (c >= '0' && c <= '9') {
            binding.key = static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (c - '0'));
        }
    } else {
        // Special keys
        if (remaining == "Space") binding.key = KeyCode::Space;
        else if (remaining == "Enter") binding.key = KeyCode::Enter;
        else if (remaining == "Escape") binding.key = KeyCode::Escape;
        else if (remaining == "Tab") binding.key = KeyCode::Tab;
        else if (remaining == "Delete") binding.key = KeyCode::Delete;
        else if (remaining == "Home") binding.key = KeyCode::Home;
        else if (remaining == "End") binding.key = KeyCode::End;
        else if (remaining == "Left") binding.key = KeyCode::Left;
        else if (remaining == "Right") binding.key = KeyCode::Right;
        else if (remaining == "Up") binding.key = KeyCode::Up;
        else if (remaining == "Down") binding.key = KeyCode::Down;
        else if (remaining.substr(0, 1) == "F") {
            int fnum = std::stoi(remaining.substr(1));
            if (fnum >= 1 && fnum <= 12) {
                binding.key = static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (fnum - 1));
            }
        }
    }
    
    return binding;
}

// ============================================================================
// Convenience
// ============================================================================

inline HotkeyManager& getHotkeyManager() {
    return HotkeyManager::getInstance();
}

}  // namespace luma
