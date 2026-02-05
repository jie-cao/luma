// LUMA Editor Mode System
// Multi-mode workflow inspired by Blender/Unreal Engine
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace luma {
namespace editor {

// ===== Editor Modes =====
// Each mode provides different functionality and UI layout
enum class EditorMode {
    Welcome,    // Initial startup screen
    Scene,      // Scene overview - add/arrange objects
    Character,  // Character customization mode
    Edit,       // Geometry/Material editing (mesh-level)
    Animation,  // Animation editing
    Play        // Runtime preview (interactive mode)
};

// ===== View/Shading Modes (like Blender/Maya) =====
enum class ViewMode {
    Material,       // Full PBR material rendering (default)
    Solid,          // Solid gray shading (clay render)
    Wireframe       // Wireframe only (no solid)
};

// ===== Object Types =====
// Determines which modes are available for selected object
enum class ObjectType {
    None,
    Primitive,      // Basic geometry (cube, sphere, etc.)
    Model,          // Imported 3D model
    Character,      // Character with customizable parts
    Light,          // Light source
    Camera,         // Camera
    Terrain,        // Terrain object
    Particle,       // Particle system
    Audio           // Audio source
};

// ===== Mode Availability =====
struct ModeAvailability {
    bool scene = true;       // Always available
    bool character = false;  // Only when character selected
    bool edit = false;       // When any object selected
    bool animation = false;  // When object has skeleton
    bool play = true;        // Always available
    
    // Reasons for unavailability (shown as tooltip)
    std::string characterReason = "请先选择一个角色对象";
    std::string editReason = "请先选择一个对象";
    std::string animationReason = "选中的对象没有骨骼绑定";
};

// ===== Editor Mode Manager =====
class EditorModeManager {
public:
    // Current state
    EditorMode currentMode = EditorMode::Welcome;
    EditorMode previousMode = EditorMode::Scene;
    
    // First-time user experience
    bool isFirstLaunch = true;
    bool showWelcomeOnStartup = true;
    
    // Mode-specific state
    int selectedMeshIndex = -1;        // In Edit mode, which mesh is selected
    bool showMaterialNodeEditor = false;
    
    // View mode (Edit mode shading)
    ViewMode viewMode = ViewMode::Material;
    bool showWireframeOverlay = false;   // Show wireframe on top of solid
    float highlightColor[4] = {1.0f, 0.5f, 0.0f, 0.8f};  // Orange highlight for selected mesh
    
    // Recent projects for welcome screen
    struct RecentProject {
        std::string name;
        std::string path;
        std::string lastOpened;  // ISO date string
    };
    std::vector<RecentProject> recentProjects;
    
    // Scene presets for quick start
    struct ScenePreset {
        std::string name;
        std::string icon;        // Unicode icon
        std::string description;
        std::function<void()> createFunc;
    };
    std::vector<ScenePreset> scenePresets;
    
    // Callbacks
    std::function<void(EditorMode)> onModeChanged;
    std::function<void()> onNewScene;
    std::function<void(const std::string&)> onOpenProject;
    
public:
    EditorModeManager() {
        initializePresets();
        loadRecentProjects();
    }
    
    // Mode switching
    bool canSwitchToMode(EditorMode mode, ObjectType objType, bool hasSkeleton) const {
        switch (mode) {
            case EditorMode::Welcome:
                return true;
            case EditorMode::Scene:
                return true;
            case EditorMode::Character:
                return objType == ObjectType::Character;
            case EditorMode::Edit:
                return objType != ObjectType::None && 
                       objType != ObjectType::Light && 
                       objType != ObjectType::Camera &&
                       objType != ObjectType::Audio;
            case EditorMode::Animation:
                return hasSkeleton;
            case EditorMode::Play:
                return true;
            default:
                return false;
        }
    }
    
    ModeAvailability getAvailability(ObjectType objType, bool hasSkeleton) const {
        ModeAvailability avail;
        
        // Character mode
        avail.character = (objType == ObjectType::Character);
        if (!avail.character) {
            avail.characterReason = "请先选择一个角色对象";
        }
        
        // Edit mode
        avail.edit = (objType != ObjectType::None && 
                      objType != ObjectType::Light && 
                      objType != ObjectType::Camera &&
                      objType != ObjectType::Audio);
        if (!avail.edit) {
            if (objType == ObjectType::None) {
                avail.editReason = "请先选择一个对象";
            } else {
                avail.editReason = "此类型对象不支持编辑模式";
            }
        }
        
        // Animation mode
        avail.animation = hasSkeleton;
        if (!avail.animation) {
            if (objType == ObjectType::None) {
                avail.animationReason = "请先选择一个对象";
            } else {
                avail.animationReason = "选中的对象没有骨骼绑定";
            }
        }
        
        return avail;
    }
    
    void switchMode(EditorMode newMode) {
        if (newMode == currentMode) return;
        
        previousMode = currentMode;
        currentMode = newMode;
        
        // Reset mode-specific state
        if (newMode == EditorMode::Scene) {
            selectedMeshIndex = -1;
        }
        
        if (onModeChanged) {
            onModeChanged(newMode);
        }
    }
    
    void returnToPreviousMode() {
        if (previousMode != currentMode) {
            switchMode(previousMode);
        } else {
            switchMode(EditorMode::Scene);
        }
    }
    
    // Mode info
    static const char* getModeName(EditorMode mode) {
        switch (mode) {
            case EditorMode::Welcome:   return "欢迎";
            case EditorMode::Scene:     return "场景";
            case EditorMode::Character: return "角色";
            case EditorMode::Edit:      return "编辑";
            case EditorMode::Animation: return "动画";
            case EditorMode::Play:      return "互动";
            default: return "未知";
        }
    }
    
    static const char* getModeIcon(EditorMode mode) {
        switch (mode) {
            case EditorMode::Welcome:   return "[H]";
            case EditorMode::Scene:     return "[S]";
            case EditorMode::Character: return "[C]";
            case EditorMode::Edit:      return "[E]";
            case EditorMode::Animation: return "[A]";
            case EditorMode::Play:      return "[>]";
            default: return "[?]";
        }
    }
    
    static const char* getModeShortcut(EditorMode mode) {
        switch (mode) {
            case EditorMode::Scene:     return "1";
            case EditorMode::Character: return "2";
            case EditorMode::Edit:      return "Tab";
            case EditorMode::Animation: return "3";
            case EditorMode::Play:      return "F5";
            default: return "";
        }
    }
    
private:
    void initializePresets() {
        scenePresets = {
            { "空白场景", " ", "一个干净的空场景", nullptr },
            { "摄影棚", " ", "带有灯光的摄影棚环境", nullptr },
            { "户外公园", " ", "阳光明媚的公园场景", nullptr },
            { "中世纪城堡", " ", "奇幻风格的城堡场景", nullptr },
            { "科幻太空船", " ", "未来风格的太空船内部", nullptr },
        };
    }
    
    void loadRecentProjects() {
        // TODO: Load from config file
        // For now, use placeholder data
        recentProjects = {
            // { "MyProject", "C:/projects/my_project.luma", "2024-01-15" },
        };
    }
    
public:
    void saveRecentProjects() {
        // TODO: Save to config file
    }
    
    void addRecentProject(const std::string& name, const std::string& path) {
        // Remove if already exists
        recentProjects.erase(
            std::remove_if(recentProjects.begin(), recentProjects.end(),
                [&](const RecentProject& p) { return p.path == path; }),
            recentProjects.end()
        );
        
        // Add to front
        RecentProject proj;
        proj.name = name;
        proj.path = path;
        // TODO: Get current date
        proj.lastOpened = "今天";
        recentProjects.insert(recentProjects.begin(), proj);
        
        // Keep only last 10
        if (recentProjects.size() > 10) {
            recentProjects.resize(10);
        }
        
        saveRecentProjects();
    }
};

// ===== Add Object Menu =====
// Categories and items for the "+ Add" menu
struct AddObjectCategory {
    std::string name;
    std::string icon;
    std::vector<struct AddObjectItem> items;
};

struct AddObjectItem {
    std::string name;
    std::string icon;
    std::string tooltip;
    std::function<void()> createFunc;
    bool hasSubmenu = false;
    std::vector<AddObjectItem> submenu;
};

class AddObjectMenu {
public:
    std::vector<AddObjectCategory> categories;
    
    // Callbacks
    std::function<void(const std::string&)> onCreatePrimitive;  // "Cube", "Sphere", etc.
    std::function<void()> onCreateCharacter;
    std::function<void()> onImportModel;
    std::function<void(const std::string&)> onCreateLight;      // "Directional", "Point", etc.
    std::function<void()> onCreateCamera;
    std::function<void(const std::string&)> onLoadPreset;       // Preset name
    
    AddObjectMenu() {
        initializeMenu();
    }
    
private:
    void initializeMenu() {
        // Character category
        AddObjectCategory charCategory;
        charCategory.name = "角色";
        charCategory.icon = " ";
        charCategory.items = {
            { "从模板创建...", " ", "使用预设模板创建角色", nullptr },
            { "从照片生成 (AI)...", " ", "上传照片自动生成角色", nullptr },
            { "从预设创建...", " ", "从角色预设库选择", nullptr },
        };
        categories.push_back(charCategory);
        
        // Object category
        AddObjectCategory objCategory;
        objCategory.name = "物体";
        objCategory.icon = " ";
        objCategory.items = {
            { "基础几何体", " ", "立方体、球、圆柱等", nullptr, true, {
                { "立方体", " ", "创建立方体", nullptr },
                { "球体", " ", "创建球体", nullptr },
                { "圆柱体", " ", "创建圆柱体", nullptr },
                { "平面", " ", "创建平面", nullptr },
                { "圆环", " ", "创建圆环", nullptr },
            }},
            { "从照片生成 (AI)...", " ", "上传照片自动生成3D物体", nullptr },
            { "地形", " ", "创建可编辑地形", nullptr },
            { "植被", " ", "添加草木等植被", nullptr },
            { "水面", " ", "创建水面效果", nullptr },
        };
        categories.push_back(objCategory);
        
        // Light category
        AddObjectCategory lightCategory;
        lightCategory.name = "光源";
        lightCategory.icon = " ";
        lightCategory.items = {
            { "方向光", " ", "模拟太阳光的平行光源", nullptr },
            { "点光源", " ", "向四周发光的点光源", nullptr },
            { "聚光灯", " ", "锥形区域的聚光灯", nullptr },
            { "区域光", " ", "矩形区域光源", nullptr },
        };
        categories.push_back(lightCategory);
        
        // Camera
        AddObjectCategory cameraCategory;
        cameraCategory.name = "摄像机";
        cameraCategory.icon = " ";
        cameraCategory.items = {
            { "透视摄像机", " ", "标准透视投影摄像机", nullptr },
            { "正交摄像机", " ", "正交投影摄像机", nullptr },
        };
        categories.push_back(cameraCategory);
        
        // Import
        AddObjectCategory importCategory;
        importCategory.name = "导入模型";
        importCategory.icon = " ";
        importCategory.items = {
            { "导入模型...", " ", "导入 FBX/OBJ/glTF 文件", nullptr },
        };
        categories.push_back(importCategory);
        
        // Scene presets
        AddObjectCategory presetCategory;
        presetCategory.name = "场景预设";
        presetCategory.icon = " ";
        presetCategory.items = {
            { "摄影棚", " ", "带有灯光的摄影棚", nullptr },
            { "户外公园", " ", "阳光明媚的公园", nullptr },
            { "中世纪城堡", " ", "奇幻城堡场景", nullptr },
            { "科幻太空船", " ", "未来风格太空船", nullptr },
        };
        categories.push_back(presetCategory);
    }
};

} // namespace editor
} // namespace luma
