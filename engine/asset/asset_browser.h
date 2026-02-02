// Asset Browser - Project Resource Management
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace luma {

// ===== Browser Asset Type =====
enum class BrowserAssetType {
    Unknown,
    Folder,
    Model,      // .fbx, .obj, .gltf, .glb
    Texture,    // .png, .jpg, .jpeg, .tga, .hdr
    Material,   // .mat
    Scene,      // .luma
    Script,     // .lua
    Audio,      // .wav, .mp3, .ogg
    Animation,  // .anim
    Prefab,     // .prefab
    Shader,     // .hlsl, .metal, .glsl
    Font        // .ttf, .otf
};

// ===== Asset Info =====
struct AssetInfo {
    std::string name;
    std::string path;           // Full path
    std::string relativePath;   // Relative to project root
    std::string extension;
    BrowserAssetType type = BrowserAssetType::Unknown;
    uint64_t size = 0;
    std::chrono::system_clock::time_point lastModified;
    bool isDirectory = false;
    
    // Thumbnail
    uint32_t thumbnailId = 0;
    bool thumbnailLoaded = false;
    
    // Metadata
    std::string uuid;
    std::unordered_map<std::string, std::string> metadata;
};

// ===== Asset Filter =====
struct AssetFilter {
    std::string searchText;
    std::vector<BrowserAssetType> allowedTypes;
    bool showHidden = false;
    bool caseSensitive = false;
    
    bool matches(const AssetInfo& asset) const {
        // Type filter
        if (!allowedTypes.empty()) {
            bool typeMatch = false;
            for (auto t : allowedTypes) {
                if (asset.type == t) { typeMatch = true; break; }
            }
            if (!typeMatch) return false;
        }
        
        // Text filter
        if (!searchText.empty()) {
            std::string name = asset.name;
            std::string search = searchText;
            if (!caseSensitive) {
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
            }
            if (name.find(search) == std::string::npos) return false;
        }
        
        // Hidden filter
        if (!showHidden && !asset.name.empty() && asset.name[0] == '.') {
            return false;
        }
        
        return true;
    }
};

// ===== Sort Mode =====
enum class AssetSortMode {
    Name,
    Type,
    Size,
    DateModified
};

// ===== View Mode =====
enum class AssetViewMode {
    Grid,
    List,
    Columns
};

// ===== Asset Browser Events =====
struct AssetBrowserEvents {
    std::function<void(const AssetInfo&)> onAssetSelected;
    std::function<void(const AssetInfo&)> onAssetDoubleClicked;
    std::function<void(const AssetInfo&)> onAssetDeleted;
    std::function<void(const AssetInfo&)> onAssetRenamed;
    std::function<void(const std::string&, const std::string&)> onAssetMoved;
    std::function<void(const std::vector<std::string>&)> onAssetsImported;
};

// ===== Asset Browser =====
class AssetBrowser {
public:
    AssetBrowser() = default;
    
    // Initialize with project root
    void initialize(const std::string& projectRoot) {
        projectRoot_ = projectRoot;
        assetsRoot_ = projectRoot + "/assets";
        currentPath_ = assetsRoot_;
        refresh();
    }
    
    // Navigation
    void setCurrentPath(const std::string& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            // Add to history
            if (!history_.empty() && historyIndex_ < history_.size() - 1) {
                history_.erase(history_.begin() + historyIndex_ + 1, history_.end());
            }
            history_.push_back(currentPath_);
            historyIndex_ = history_.size() - 1;
            
            currentPath_ = path;
            refresh();
        }
    }
    
    void navigateUp() {
        auto parent = std::filesystem::path(currentPath_).parent_path();
        if (parent.string().find(projectRoot_) == 0) {
            setCurrentPath(parent.string());
        }
    }
    
    void navigateBack() {
        if (historyIndex_ > 0) {
            historyIndex_--;
            currentPath_ = history_[historyIndex_];
            refresh();
        }
    }
    
    void navigateForward() {
        if (historyIndex_ < history_.size() - 1) {
            historyIndex_++;
            currentPath_ = history_[historyIndex_];
            refresh();
        }
    }
    
    bool canGoBack() const { return historyIndex_ > 0; }
    bool canGoForward() const { return historyIndex_ < history_.size() - 1; }
    bool canGoUp() const { 
        return currentPath_ != projectRoot_ && currentPath_.find(projectRoot_) == 0; 
    }
    
    // Refresh current directory
    void refresh() {
        assets_.clear();
        
        if (!std::filesystem::exists(currentPath_)) return;
        
        for (const auto& entry : std::filesystem::directory_iterator(currentPath_)) {
            AssetInfo info = createAssetInfo(entry);
            if (filter_.matches(info)) {
                assets_.push_back(info);
            }
        }
        
        sortAssets();
    }
    
    // Sorting
    void setSortMode(AssetSortMode mode, bool ascending = true) {
        sortMode_ = mode;
        sortAscending_ = ascending;
        sortAssets();
    }
    
    // Filtering
    void setFilter(const AssetFilter& filter) {
        filter_ = filter;
        refresh();
    }
    
    void setSearchText(const std::string& text) {
        filter_.searchText = text;
        refresh();
    }
    
    // View
    void setViewMode(AssetViewMode mode) { viewMode_ = mode; }
    AssetViewMode getViewMode() const { return viewMode_; }
    
    void setThumbnailSize(int size) { thumbnailSize_ = std::clamp(size, 32, 256); }
    int getThumbnailSize() const { return thumbnailSize_; }
    
    // Selection
    void selectAsset(size_t index) {
        if (index < assets_.size()) {
            selectedIndices_.clear();
            selectedIndices_.push_back(index);
            if (events_.onAssetSelected) {
                events_.onAssetSelected(assets_[index]);
            }
        }
    }
    
    void toggleSelection(size_t index) {
        auto it = std::find(selectedIndices_.begin(), selectedIndices_.end(), index);
        if (it != selectedIndices_.end()) {
            selectedIndices_.erase(it);
        } else {
            selectedIndices_.push_back(index);
        }
    }
    
    void selectRange(size_t start, size_t end) {
        selectedIndices_.clear();
        for (size_t i = std::min(start, end); i <= std::max(start, end); i++) {
            if (i < assets_.size()) {
                selectedIndices_.push_back(i);
            }
        }
    }
    
    void clearSelection() { selectedIndices_.clear(); }
    
    bool isSelected(size_t index) const {
        return std::find(selectedIndices_.begin(), selectedIndices_.end(), index) != selectedIndices_.end();
    }
    
    const std::vector<size_t>& getSelectedIndices() const { return selectedIndices_; }
    
    // Asset Operations
    bool createFolder(const std::string& name) {
        std::string path = currentPath_ + "/" + name;
        try {
            std::filesystem::create_directory(path);
            refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool deleteAsset(const AssetInfo& asset) {
        try {
            if (asset.isDirectory) {
                std::filesystem::remove_all(asset.path);
            } else {
                std::filesystem::remove(asset.path);
            }
            if (events_.onAssetDeleted) {
                events_.onAssetDeleted(asset);
            }
            refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool renameAsset(const AssetInfo& asset, const std::string& newName) {
        try {
            auto parent = std::filesystem::path(asset.path).parent_path();
            std::string newPath = (parent / newName).string();
            std::filesystem::rename(asset.path, newPath);
            if (events_.onAssetRenamed) {
                events_.onAssetRenamed(asset);
            }
            refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool moveAsset(const std::string& sourcePath, const std::string& destPath) {
        try {
            std::filesystem::rename(sourcePath, destPath);
            if (events_.onAssetMoved) {
                events_.onAssetMoved(sourcePath, destPath);
            }
            refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool copyAsset(const std::string& sourcePath, const std::string& destPath) {
        try {
            if (std::filesystem::is_directory(sourcePath)) {
                std::filesystem::copy(sourcePath, destPath, 
                    std::filesystem::copy_options::recursive);
            } else {
                std::filesystem::copy_file(sourcePath, destPath);
            }
            refresh();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // Import
    void importAssets(const std::vector<std::string>& paths) {
        std::vector<std::string> imported;
        for (const auto& path : paths) {
            auto filename = std::filesystem::path(path).filename();
            std::string destPath = currentPath_ + "/" + filename.string();
            try {
                std::filesystem::copy_file(path, destPath);
                imported.push_back(destPath);
            } catch (...) {}
        }
        if (!imported.empty() && events_.onAssetsImported) {
            events_.onAssetsImported(imported);
        }
        refresh();
    }
    
    // Getters
    const std::string& getProjectRoot() const { return projectRoot_; }
    const std::string& getCurrentPath() const { return currentPath_; }
    const std::vector<AssetInfo>& getAssets() const { return assets_; }
    const AssetFilter& getFilter() const { return filter_; }
    
    std::string getRelativePath(const std::string& fullPath) const {
        if (fullPath.find(projectRoot_) == 0) {
            return fullPath.substr(projectRoot_.length() + 1);
        }
        return fullPath;
    }
    
    // Events
    AssetBrowserEvents& events() { return events_; }
    
    // Get breadcrumb path components
    std::vector<std::pair<std::string, std::string>> getBreadcrumbs() const {
        std::vector<std::pair<std::string, std::string>> crumbs;
        std::filesystem::path path(currentPath_);
        std::filesystem::path root(projectRoot_);
        
        while (path.string().find(projectRoot_) == 0) {
            crumbs.insert(crumbs.begin(), {path.filename().string(), path.string()});
            if (path == root) break;
            path = path.parent_path();
        }
        
        return crumbs;
    }
    
    // Get folder tree for sidebar
    struct FolderNode {
        std::string name;
        std::string path;
        std::vector<FolderNode> children;
        bool expanded = false;
    };
    
    FolderNode getFolderTree() const {
        FolderNode root;
        root.name = std::filesystem::path(projectRoot_).filename().string();
        root.path = projectRoot_;
        buildFolderTree(root, projectRoot_, 3); // Max depth
        return root;
    }
    
private:
    AssetInfo createAssetInfo(const std::filesystem::directory_entry& entry) {
        AssetInfo info;
        info.path = entry.path().string();
        info.name = entry.path().filename().string();
        info.relativePath = getRelativePath(info.path);
        info.extension = entry.path().extension().string();
        info.isDirectory = entry.is_directory();
        
        if (info.isDirectory) {
            info.type = BrowserAssetType::Folder;
        } else {
            info.type = getBrowserAssetType(info.extension);
            try {
                info.size = entry.file_size();
                // Note: last_write_time conversion is simplified for compatibility
                auto ftime = entry.last_write_time();
                (void)ftime; // Unused for now - would need platform-specific conversion
            } catch (...) {}
        }
        
        return info;
    }
    
    static BrowserAssetType getBrowserAssetType(const std::string& ext) {
        std::string lower = ext;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == ".fbx" || lower == ".obj" || lower == ".gltf" || lower == ".glb") {
            return BrowserAssetType::Model;
        }
        if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" || 
            lower == ".tga" || lower == ".hdr" || lower == ".bmp") {
            return BrowserAssetType::Texture;
        }
        if (lower == ".mat") return BrowserAssetType::Material;
        if (lower == ".luma") return BrowserAssetType::Scene;
        if (lower == ".lua") return BrowserAssetType::Script;
        if (lower == ".wav" || lower == ".mp3" || lower == ".ogg") return BrowserAssetType::Audio;
        if (lower == ".anim") return BrowserAssetType::Animation;
        if (lower == ".prefab") return BrowserAssetType::Prefab;
        if (lower == ".hlsl" || lower == ".metal" || lower == ".glsl") return BrowserAssetType::Shader;
        if (lower == ".ttf" || lower == ".otf") return BrowserAssetType::Font;
        
        return BrowserAssetType::Unknown;
    }
    
    void sortAssets() {
        std::sort(assets_.begin(), assets_.end(), [this](const AssetInfo& a, const AssetInfo& b) {
            // Folders always first
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory;
            }
            
            bool result = false;
            switch (sortMode_) {
                case AssetSortMode::Name:
                    result = a.name < b.name;
                    break;
                case AssetSortMode::Type:
                    result = static_cast<int>(a.type) < static_cast<int>(b.type);
                    break;
                case AssetSortMode::Size:
                    result = a.size < b.size;
                    break;
                case AssetSortMode::DateModified:
                    result = a.lastModified < b.lastModified;
                    break;
            }
            
            return sortAscending_ ? result : !result;
        });
    }
    
    void buildFolderTree(FolderNode& node, const std::string& path, int depth) const {
        if (depth <= 0) return;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) {
                    std::string name = entry.path().filename().string();
                    if (name[0] == '.') continue; // Skip hidden
                    
                    FolderNode child;
                    child.name = name;
                    child.path = entry.path().string();
                    buildFolderTree(child, child.path, depth - 1);
                    node.children.push_back(child);
                }
            }
        } catch (...) {}
    }
    
    std::string projectRoot_;
    std::string assetsRoot_;
    std::string currentPath_;
    
    std::vector<AssetInfo> assets_;
    std::vector<size_t> selectedIndices_;
    
    AssetFilter filter_;
    AssetSortMode sortMode_ = AssetSortMode::Name;
    bool sortAscending_ = true;
    AssetViewMode viewMode_ = AssetViewMode::Grid;
    int thumbnailSize_ = 96;
    
    std::vector<std::string> history_;
    size_t historyIndex_ = 0;
    
    AssetBrowserEvents events_;
};

// ===== Asset Type Utilities =====
inline const char* getAssetTypeIcon(BrowserAssetType type) {
    switch (type) {
        case BrowserAssetType::Folder:    return "\xef\x81\xbc"; // folder icon
        case BrowserAssetType::Model:     return "\xef\x86\xb2"; // cube
        case BrowserAssetType::Texture:   return "\xef\x87\x85"; // image
        case BrowserAssetType::Material:  return "\xef\x83\xab"; // palette
        case BrowserAssetType::Scene:     return "\xef\x84\xa8"; // sitemap
        case BrowserAssetType::Script:    return "\xef\x84\xa1"; // code
        case BrowserAssetType::Audio:     return "\xef\x80\xa1"; // music
        case BrowserAssetType::Animation: return "\xef\x80\x88"; // play
        case BrowserAssetType::Prefab:    return "\xef\x86\xa9"; // box
        case BrowserAssetType::Shader:    return "\xef\x83\xa5"; // magic
        case BrowserAssetType::Font:      return "\xef\x80\xb1"; // font
        default:                          return "\xef\x85\x9b"; // file
    }
}

inline const char* getAssetTypeName(BrowserAssetType type) {
    switch (type) {
        case BrowserAssetType::Folder:    return "Folder";
        case BrowserAssetType::Model:     return "Model";
        case BrowserAssetType::Texture:   return "Texture";
        case BrowserAssetType::Material:  return "Material";
        case BrowserAssetType::Scene:     return "Scene";
        case BrowserAssetType::Script:    return "Script";
        case BrowserAssetType::Audio:     return "Audio";
        case BrowserAssetType::Animation: return "Animation";
        case BrowserAssetType::Prefab:    return "Prefab";
        case BrowserAssetType::Shader:    return "Shader";
        case BrowserAssetType::Font:      return "Font";
        default:                          return "Unknown";
    }
}

inline std::string formatFileSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    char buffer[32];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%llu B", bytes);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    }
    
    return buffer;
}

}  // namespace luma
