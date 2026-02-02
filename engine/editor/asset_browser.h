// Asset Browser - Resource management and preview
// Browse, import, and manage project assets
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <memory>

namespace luma {

// ============================================================================
// Asset Types
// ============================================================================

enum class AssetType {
    Unknown,
    
    // 3D Assets
    Model,          // .fbx, .obj, .gltf, .glb
    Mesh,           // Internal mesh data
    Skeleton,       // Animation skeleton
    Animation,      // .fbx (animation), .bvh
    
    // Textures
    Texture,        // .png, .jpg, .tga, .exr, .hdr
    Cubemap,        // 6-face cubemap
    
    // Materials
    Material,       // .mat, .json
    Shader,         // .shader, .hlsl, .metal
    
    // Audio
    Audio,          // .wav, .mp3, .ogg
    
    // Data
    Prefab,         // .prefab
    Scene,          // .scene
    Project,        // .luma
    Config,         // .json, .yaml
    
    // Character
    Character,      // .char
    Clothing,       // .clothing
    HairStyle,      // .hair
    
    // Scripts
    Script,         // .lua
    
    COUNT
};

inline std::string assetTypeToString(AssetType type) {
    switch (type) {
        case AssetType::Model: return "Model";
        case AssetType::Mesh: return "Mesh";
        case AssetType::Skeleton: return "Skeleton";
        case AssetType::Animation: return "Animation";
        case AssetType::Texture: return "Texture";
        case AssetType::Cubemap: return "Cubemap";
        case AssetType::Material: return "Material";
        case AssetType::Shader: return "Shader";
        case AssetType::Audio: return "Audio";
        case AssetType::Prefab: return "Prefab";
        case AssetType::Scene: return "Scene";
        case AssetType::Project: return "Project";
        case AssetType::Config: return "Config";
        case AssetType::Character: return "Character";
        case AssetType::Clothing: return "Clothing";
        case AssetType::HairStyle: return "HairStyle";
        case AssetType::Script: return "Script";
        default: return "Unknown";
    }
}

inline std::string assetTypeToIcon(AssetType type) {
    switch (type) {
        case AssetType::Model: return "📦";
        case AssetType::Mesh: return "🔷";
        case AssetType::Skeleton: return "🦴";
        case AssetType::Animation: return "🎬";
        case AssetType::Texture: return "🖼️";
        case AssetType::Cubemap: return "🌐";
        case AssetType::Material: return "🎨";
        case AssetType::Shader: return "💎";
        case AssetType::Audio: return "🔊";
        case AssetType::Prefab: return "📋";
        case AssetType::Scene: return "🌍";
        case AssetType::Project: return "📁";
        case AssetType::Config: return "⚙️";
        case AssetType::Character: return "🧑";
        case AssetType::Clothing: return "👕";
        case AssetType::HairStyle: return "💇";
        case AssetType::Script: return "📜";
        default: return "❓";
    }
}

// ============================================================================
// Asset Info
// ============================================================================

struct AssetInfo {
    std::string id;              // Unique identifier
    std::string name;            // Display name
    std::string path;            // File path (relative to project)
    std::string absolutePath;    // Absolute path
    
    AssetType type = AssetType::Unknown;
    
    // Metadata
    size_t fileSize = 0;
    std::string lastModified;
    std::string createdDate;
    
    // Preview
    std::string thumbnailPath;
    bool hasThumbnail = false;
    
    // Import settings
    std::unordered_map<std::string, std::string> importSettings;
    
    // Tags for filtering
    std::vector<std::string> tags;
    
    // Dependencies
    std::vector<std::string> dependencies;
    std::vector<std::string> dependents;
    
    // Status
    bool isLoaded = false;
    bool isModified = false;
    bool isExternal = false;
};

// ============================================================================
// Asset Folder
// ============================================================================

struct AssetFolder {
    std::string name;
    std::string path;
    
    std::vector<AssetFolder> subfolders;
    std::vector<std::string> assetIds;  // Assets in this folder
    
    bool isExpanded = false;
};

// ============================================================================
// Import Settings
// ============================================================================

struct ModelImportSettings {
    float scale = 1.0f;
    bool importAnimations = true;
    bool importMaterials = true;
    bool importTextures = true;
    bool generateLOD = true;
    int lodLevels = 4;
    bool calculateTangents = true;
    bool flipUVs = false;
    bool combineMeshes = false;
};

struct TextureImportSettings {
    bool generateMipmaps = true;
    bool sRGB = true;
    int maxSize = 4096;
    bool compress = true;
    std::string format = "auto";  // "auto", "bc7", "bc3", "rgba8"
};

struct AudioImportSettings {
    bool compress = true;
    int sampleRate = 44100;
    bool mono = false;
    bool streaming = false;  // Large files
};

// ============================================================================
// Asset Importer
// ============================================================================

class AssetImporter {
public:
    // Import file to project
    static AssetInfo import(const std::string& sourcePath, 
                            const std::string& destFolder,
                            const ModelImportSettings& settings = {}) {
        AssetInfo info;
        
        // Determine type from extension
        std::filesystem::path p(sourcePath);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        info.type = getTypeFromExtension(ext);
        info.name = p.stem().string();
        info.absolutePath = sourcePath;
        info.path = destFolder + "/" + p.filename().string();
        info.id = generateAssetId(info.path);
        
        // Get file info
        if (std::filesystem::exists(sourcePath)) {
            info.fileSize = std::filesystem::file_size(sourcePath);
            auto ftime = std::filesystem::last_write_time(sourcePath);
            // Convert to string (simplified)
            info.lastModified = "2024-01-01";
        }
        
        // Store import settings
        info.importSettings["scale"] = std::to_string(settings.scale);
        info.importSettings["importAnimations"] = settings.importAnimations ? "true" : "false";
        
        return info;
    }
    
    // Get supported extensions
    static std::vector<std::string> getSupportedExtensions(AssetType type) {
        switch (type) {
            case AssetType::Model:
                return {".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds", ".blend"};
            case AssetType::Animation:
                return {".fbx", ".bvh", ".anim"};
            case AssetType::Texture:
                return {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".exr", ".hdr"};
            case AssetType::Audio:
                return {".wav", ".mp3", ".ogg", ".flac"};
            case AssetType::Material:
                return {".mat", ".material"};
            case AssetType::Shader:
                return {".shader", ".hlsl", ".glsl", ".metal"};
            case AssetType::Script:
                return {".lua"};
            default:
                return {};
        }
    }
    
private:
    static AssetType getTypeFromExtension(const std::string& ext) {
        static std::unordered_map<std::string, AssetType> extMap = {
            {".fbx", AssetType::Model}, {".obj", AssetType::Model},
            {".gltf", AssetType::Model}, {".glb", AssetType::Model},
            {".dae", AssetType::Model}, {".3ds", AssetType::Model},
            {".blend", AssetType::Model},
            
            {".bvh", AssetType::Animation}, {".anim", AssetType::Animation},
            
            {".png", AssetType::Texture}, {".jpg", AssetType::Texture},
            {".jpeg", AssetType::Texture}, {".tga", AssetType::Texture},
            {".bmp", AssetType::Texture}, {".psd", AssetType::Texture},
            {".exr", AssetType::Texture}, {".hdr", AssetType::Texture},
            
            {".wav", AssetType::Audio}, {".mp3", AssetType::Audio},
            {".ogg", AssetType::Audio}, {".flac", AssetType::Audio},
            
            {".mat", AssetType::Material}, {".material", AssetType::Material},
            
            {".shader", AssetType::Shader}, {".hlsl", AssetType::Shader},
            {".glsl", AssetType::Shader}, {".metal", AssetType::Shader},
            
            {".lua", AssetType::Script},
            
            {".luma", AssetType::Project},
            {".scene", AssetType::Scene},
            {".prefab", AssetType::Prefab}
        };
        
        auto it = extMap.find(ext);
        return (it != extMap.end()) ? it->second : AssetType::Unknown;
    }
    
    static std::string generateAssetId(const std::string& path) {
        // Simple hash-based ID
        std::hash<std::string> hasher;
        return "asset_" + std::to_string(hasher(path));
    }
};

// ============================================================================
// Asset Browser State
// ============================================================================

struct AssetBrowserState {
    // View mode
    enum class ViewMode { List, Grid, Thumbnails };
    ViewMode viewMode = ViewMode::Grid;
    int thumbnailSize = 80;
    
    // Navigation
    std::string currentPath = "/";
    std::vector<std::string> pathHistory;
    int historyIndex = 0;
    
    // Selection
    std::vector<std::string> selectedAssets;
    std::string lastSelectedAsset;
    
    // Filtering
    std::string searchQuery;
    AssetType filterType = AssetType::Unknown;  // Unknown = all
    std::vector<std::string> filterTags;
    
    // Sorting
    enum class SortBy { Name, Type, Size, Date };
    SortBy sortBy = SortBy::Name;
    bool sortAscending = true;
    
    // Context menu
    bool showContextMenu = false;
    std::string contextMenuTarget;
    
    // Drag and drop
    bool isDragging = false;
    std::string dragSource;
    
    // Preview
    bool showPreview = true;
    std::string previewAsset;
};

// ============================================================================
// Asset Browser Manager
// ============================================================================

class AssetBrowser {
public:
    static AssetBrowser& getInstance() {
        static AssetBrowser instance;
        return instance;
    }
    
    void initialize(const std::string& projectPath) {
        projectPath_ = projectPath;
        scanDirectory(projectPath);
        initialized_ = true;
    }
    
    // Navigation
    void setCurrentPath(const std::string& path) {
        if (state_.currentPath != path) {
            state_.pathHistory.push_back(state_.currentPath);
            state_.historyIndex = static_cast<int>(state_.pathHistory.size());
            state_.currentPath = path;
        }
    }
    
    void goBack() {
        if (state_.historyIndex > 0) {
            state_.historyIndex--;
            state_.currentPath = state_.pathHistory[state_.historyIndex];
        }
    }
    
    void goForward() {
        if (state_.historyIndex < static_cast<int>(state_.pathHistory.size()) - 1) {
            state_.historyIndex++;
            state_.currentPath = state_.pathHistory[state_.historyIndex];
        }
    }
    
    void goUp() {
        std::filesystem::path p(state_.currentPath);
        if (p.has_parent_path()) {
            setCurrentPath(p.parent_path().string());
        }
    }
    
    // Asset management
    void registerAsset(const AssetInfo& info) {
        assets_[info.id] = info;
    }
    
    AssetInfo* getAsset(const std::string& id) {
        auto it = assets_.find(id);
        return (it != assets_.end()) ? &it->second : nullptr;
    }
    
    std::vector<AssetInfo*> getAssetsInFolder(const std::string& folderPath) {
        std::vector<AssetInfo*> result;
        
        for (auto& [id, asset] : assets_) {
            std::filesystem::path assetPath(asset.path);
            std::filesystem::path parentPath = assetPath.parent_path();
            
            if (parentPath.string() == folderPath || 
                (folderPath == "/" && parentPath.empty())) {
                result.push_back(&asset);
            }
        }
        
        // Apply sorting
        sortAssets(result);
        
        return result;
    }
    
    std::vector<AssetInfo*> searchAssets(const std::string& query) {
        std::vector<AssetInfo*> result;
        
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (auto& [id, asset] : assets_) {
            std::string lowerName = asset.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find(lowerQuery) != std::string::npos) {
                // Apply type filter
                if (state_.filterType == AssetType::Unknown || 
                    asset.type == state_.filterType) {
                    result.push_back(&asset);
                }
            }
        }
        
        sortAssets(result);
        return result;
    }
    
    std::vector<AssetInfo*> getAssetsByType(AssetType type) {
        std::vector<AssetInfo*> result;
        
        for (auto& [id, asset] : assets_) {
            if (asset.type == type) {
                result.push_back(&asset);
            }
        }
        
        sortAssets(result);
        return result;
    }
    
    // Selection
    void selectAsset(const std::string& id, bool additive = false) {
        if (!additive) {
            state_.selectedAssets.clear();
        }
        
        if (std::find(state_.selectedAssets.begin(), state_.selectedAssets.end(), id) 
            == state_.selectedAssets.end()) {
            state_.selectedAssets.push_back(id);
        }
        
        state_.lastSelectedAsset = id;
        state_.previewAsset = id;
    }
    
    void deselectAsset(const std::string& id) {
        state_.selectedAssets.erase(
            std::remove(state_.selectedAssets.begin(), state_.selectedAssets.end(), id),
            state_.selectedAssets.end());
    }
    
    void clearSelection() {
        state_.selectedAssets.clear();
        state_.lastSelectedAsset = "";
    }
    
    // Import
    void importFile(const std::string& sourcePath) {
        AssetInfo info = AssetImporter::import(sourcePath, state_.currentPath);
        registerAsset(info);
        
        if (onAssetImported_) {
            onAssetImported_(info);
        }
    }
    
    void importFiles(const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            importFile(path);
        }
    }
    
    // Folder operations
    void createFolder(const std::string& name) {
        std::string fullPath = state_.currentPath + "/" + name;
        
        AssetFolder folder;
        folder.name = name;
        folder.path = fullPath;
        
        // In reality, would create on disk
        // std::filesystem::create_directory(projectPath_ + fullPath);
    }
    
    // State access
    AssetBrowserState& getState() { return state_; }
    const AssetBrowserState& getState() const { return state_; }
    
    const std::string& getProjectPath() const { return projectPath_; }
    
    // Callbacks
    void setOnAssetImported(std::function<void(const AssetInfo&)> callback) {
        onAssetImported_ = callback;
    }
    
    void setOnAssetSelected(std::function<void(const AssetInfo&)> callback) {
        onAssetSelected_ = callback;
    }
    
    void setOnAssetDoubleClicked(std::function<void(const AssetInfo&)> callback) {
        onAssetDoubleClicked_ = callback;
    }
    
    // Refresh
    void refresh() {
        scanDirectory(projectPath_);
    }
    
    // Get folder tree
    const AssetFolder& getRootFolder() const { return rootFolder_; }
    
private:
    AssetBrowser() = default;
    
    void scanDirectory(const std::string& path) {
        rootFolder_.name = "Assets";
        rootFolder_.path = "/";
        rootFolder_.subfolders.clear();
        rootFolder_.assetIds.clear();
        
        if (!std::filesystem::exists(path)) return;
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string relPath = std::filesystem::relative(entry.path(), path).string();
                    
                    AssetInfo info;
                    info.absolutePath = entry.path().string();
                    info.path = "/" + relPath;
                    info.name = entry.path().stem().string();
                    info.id = "asset_" + std::to_string(std::hash<std::string>{}(relPath));
                    
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    // Determine type
                    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                        info.type = AssetType::Model;
                    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") {
                        info.type = AssetType::Texture;
                    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
                        info.type = AssetType::Audio;
                    } else if (ext == ".luma") {
                        info.type = AssetType::Project;
                    }
                    
                    info.fileSize = entry.file_size();
                    
                    assets_[info.id] = info;
                }
            }
        } catch (...) {
            // Handle scan errors
        }
    }
    
    void sortAssets(std::vector<AssetInfo*>& assets) {
        auto comparator = [this](AssetInfo* a, AssetInfo* b) {
            int result = 0;
            
            switch (state_.sortBy) {
                case AssetBrowserState::SortBy::Name:
                    result = a->name.compare(b->name);
                    break;
                case AssetBrowserState::SortBy::Type:
                    result = static_cast<int>(a->type) - static_cast<int>(b->type);
                    break;
                case AssetBrowserState::SortBy::Size:
                    result = (a->fileSize < b->fileSize) ? -1 : (a->fileSize > b->fileSize ? 1 : 0);
                    break;
                case AssetBrowserState::SortBy::Date:
                    result = a->lastModified.compare(b->lastModified);
                    break;
            }
            
            return state_.sortAscending ? (result < 0) : (result > 0);
        };
        
        std::sort(assets.begin(), assets.end(), comparator);
    }
    
    std::string projectPath_;
    std::unordered_map<std::string, AssetInfo> assets_;
    AssetFolder rootFolder_;
    
    AssetBrowserState state_;
    bool initialized_ = false;
    
    std::function<void(const AssetInfo&)> onAssetImported_;
    std::function<void(const AssetInfo&)> onAssetSelected_;
    std::function<void(const AssetInfo&)> onAssetDoubleClicked_;
};

inline AssetBrowser& getAssetBrowser() {
    return AssetBrowser::getInstance();
}

}  // namespace luma
