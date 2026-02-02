// LUMA Plugin Manager - Load, manage, and query plugins
#pragma once

#include "engine/plugin/plugin_system.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace luma {

// ============================================================================
// Plugin Package Format (.lumapkg)
// ============================================================================

/*
 * Plugin Package Structure:
 * 
 * my-plugin.lumapkg/
 * ├── manifest.json          # Plugin metadata
 * ├── thumbnail.png          # Preview image
 * ├── assets/
 * │   ├── meshes/           # 3D models (.obj, .fbx, .gltf)
 * │   ├── textures/         # Textures (.png, .jpg)
 * │   ├── materials/        # Material definitions (.json)
 * │   └── configs/          # Asset configs (.json)
 * ├── scripts/              # Lua scripts (optional)
 * │   └── main.lua
 * └── lib/                  # Native libraries (optional)
 *     ├── windows/
 *     │   └── plugin.dll
 *     ├── macos/
 *     │   └── plugin.dylib
 *     └── linux/
 *         └── plugin.so
 */

// ============================================================================
// Manifest Parser
// ============================================================================

class ManifestParser {
public:
    // Simple JSON-like parser for manifest.json
    static PluginMetadata parse(const std::string& manifestContent) {
        PluginMetadata meta;
        
        // Parse key-value pairs (simplified JSON parsing)
        auto getValue = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\"";
            size_t pos = manifestContent.find(search);
            if (pos == std::string::npos) return "";
            
            pos = manifestContent.find(":", pos);
            if (pos == std::string::npos) return "";
            
            pos = manifestContent.find("\"", pos);
            if (pos == std::string::npos) return "";
            
            size_t end = manifestContent.find("\"", pos + 1);
            if (end == std::string::npos) return "";
            
            return manifestContent.substr(pos + 1, end - pos - 1);
        };
        
        auto getArray = [&](const std::string& key) -> std::vector<std::string> {
            std::vector<std::string> result;
            std::string search = "\"" + key + "\"";
            size_t pos = manifestContent.find(search);
            if (pos == std::string::npos) return result;
            
            pos = manifestContent.find("[", pos);
            if (pos == std::string::npos) return result;
            
            size_t end = manifestContent.find("]", pos);
            if (end == std::string::npos) return result;
            
            std::string arrayContent = manifestContent.substr(pos + 1, end - pos - 1);
            
            size_t start = 0;
            while ((start = arrayContent.find("\"", start)) != std::string::npos) {
                size_t itemEnd = arrayContent.find("\"", start + 1);
                if (itemEnd != std::string::npos) {
                    result.push_back(arrayContent.substr(start + 1, itemEnd - start - 1));
                    start = itemEnd + 1;
                } else {
                    break;
                }
            }
            
            return result;
        };
        
        meta.id = getValue("id");
        meta.name = getValue("name");
        meta.description = getValue("description");
        meta.author = getValue("author");
        meta.website = getValue("website");
        meta.license = getValue("license");
        meta.version = PluginVersion::parse(getValue("version"));
        meta.minEngineVersion = PluginVersion::parse(getValue("minEngineVersion"));
        meta.type = stringToPluginType(getValue("type"));
        meta.thumbnailPath = getValue("thumbnail");
        meta.entryPoint = getValue("entryPoint");
        meta.tags = getArray("tags");
        meta.dependencies = getArray("dependencies");
        
        return meta;
    }
    
    // Generate manifest JSON
    static std::string generate(const PluginMetadata& meta) {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"id\": \"" << meta.id << "\",\n";
        ss << "  \"name\": \"" << meta.name << "\",\n";
        ss << "  \"description\": \"" << meta.description << "\",\n";
        ss << "  \"author\": \"" << meta.author << "\",\n";
        ss << "  \"website\": \"" << meta.website << "\",\n";
        ss << "  \"license\": \"" << meta.license << "\",\n";
        ss << "  \"version\": \"" << meta.version.toString() << "\",\n";
        ss << "  \"minEngineVersion\": \"" << meta.minEngineVersion.toString() << "\",\n";
        ss << "  \"type\": \"" << pluginTypeToString(meta.type) << "\",\n";
        ss << "  \"thumbnail\": \"" << meta.thumbnailPath << "\",\n";
        ss << "  \"entryPoint\": \"" << meta.entryPoint << "\",\n";
        
        ss << "  \"tags\": [";
        for (size_t i = 0; i < meta.tags.size(); i++) {
            ss << "\"" << meta.tags[i] << "\"";
            if (i < meta.tags.size() - 1) ss << ", ";
        }
        ss << "],\n";
        
        ss << "  \"dependencies\": [";
        for (size_t i = 0; i < meta.dependencies.size(); i++) {
            ss << "\"" << meta.dependencies[i] << "\"";
            if (i < meta.dependencies.size() - 1) ss << ", ";
        }
        ss << "]\n";
        
        ss << "}\n";
        return ss.str();
    }
};

// ============================================================================
// Plugin Loader Result
// ============================================================================

struct PluginLoadResult {
    bool success = false;
    std::string errorMessage;
    std::shared_ptr<IPlugin> plugin;
    PluginMetadata metadata;
    std::string packagePath;
};

// ============================================================================
// Plugin Manager
// ============================================================================

class PluginManager {
public:
    static PluginManager& getInstance() {
        static PluginManager instance;
        return instance;
    }
    
    // === Plugin Directories ===
    
    void addPluginDirectory(const std::string& path) {
        pluginDirs_.push_back(path);
    }
    
    const std::vector<std::string>& getPluginDirectories() const {
        return pluginDirs_;
    }
    
    // === Plugin Discovery ===
    
    // Scan all plugin directories for packages
    std::vector<PluginMetadata> discoverPlugins() {
        std::vector<PluginMetadata> discovered;
        
        for (const auto& dir : pluginDirs_) {
            if (!std::filesystem::exists(dir)) continue;
            
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_directory()) {
                    // Check for manifest.json
                    auto manifestPath = entry.path() / "manifest.json";
                    if (std::filesystem::exists(manifestPath)) {
                        auto meta = loadManifest(manifestPath.string());
                        if (meta.isValid()) {
                            discovered.push_back(meta);
                            packagePaths_[meta.id] = entry.path().string();
                        }
                    }
                }
            }
        }
        
        return discovered;
    }
    
    // === Plugin Loading ===
    
    // Load a plugin by ID
    PluginLoadResult loadPlugin(const std::string& pluginId) {
        PluginLoadResult result;
        
        // Check if already loaded
        if (loadedPlugins_.find(pluginId) != loadedPlugins_.end()) {
            result.success = true;
            result.plugin = loadedPlugins_[pluginId];
            result.metadata = result.plugin->getMetadata();
            return result;
        }
        
        // Find package path
        auto pathIt = packagePaths_.find(pluginId);
        if (pathIt == packagePaths_.end()) {
            result.errorMessage = "Plugin not found: " + pluginId;
            return result;
        }
        
        std::string packagePath = pathIt->second;
        
        // Load manifest
        auto manifestPath = std::filesystem::path(packagePath) / "manifest.json";
        result.metadata = loadManifest(manifestPath.string());
        result.packagePath = packagePath;
        
        if (!result.metadata.isValid()) {
            result.errorMessage = "Invalid manifest for plugin: " + pluginId;
            return result;
        }
        
        // Check dependencies
        for (const auto& dep : result.metadata.dependencies) {
            if (loadedPlugins_.find(dep) == loadedPlugins_.end()) {
                auto depResult = loadPlugin(dep);
                if (!depResult.success) {
                    result.errorMessage = "Missing dependency: " + dep;
                    return result;
                }
            }
        }
        
        // Try to create plugin from factory (for built-in plugins)
        result.plugin = PluginFactory::getInstance().createPlugin(pluginId);
        
        // If no factory, try to load as asset-only plugin
        if (!result.plugin) {
            result.plugin = std::make_shared<AssetOnlyPlugin>(result.metadata, packagePath);
        }
        
        // Initialize
        if (!result.plugin->initialize()) {
            result.errorMessage = "Failed to initialize plugin: " + pluginId;
            result.plugin.reset();
            return result;
        }
        
        // Store
        loadedPlugins_[pluginId] = result.plugin;
        result.success = true;
        
        // Notify listeners
        for (auto& listener : listeners_) {
            listener->onPluginLoaded(pluginId);
        }
        
        return result;
    }
    
    // Unload a plugin
    bool unloadPlugin(const std::string& pluginId) {
        auto it = loadedPlugins_.find(pluginId);
        if (it == loadedPlugins_.end()) return false;
        
        // Check if other plugins depend on this
        for (const auto& [id, plugin] : loadedPlugins_) {
            if (id == pluginId) continue;
            const auto& deps = plugin->getMetadata().dependencies;
            if (std::find(deps.begin(), deps.end(), pluginId) != deps.end()) {
                return false;  // Cannot unload, has dependents
            }
        }
        
        it->second->shutdown();
        loadedPlugins_.erase(it);
        
        for (auto& listener : listeners_) {
            listener->onPluginUnloaded(pluginId);
        }
        
        return true;
    }
    
    // === Plugin Queries ===
    
    std::shared_ptr<IPlugin> getPlugin(const std::string& pluginId) const {
        auto it = loadedPlugins_.find(pluginId);
        return (it != loadedPlugins_.end()) ? it->second : nullptr;
    }
    
    bool isPluginLoaded(const std::string& pluginId) const {
        return loadedPlugins_.find(pluginId) != loadedPlugins_.end();
    }
    
    std::vector<std::shared_ptr<IPlugin>> getLoadedPlugins() const {
        std::vector<std::shared_ptr<IPlugin>> result;
        for (const auto& [id, plugin] : loadedPlugins_) {
            result.push_back(plugin);
        }
        return result;
    }
    
    // Get plugins by type
    std::vector<std::shared_ptr<IPlugin>> getPluginsByType(PluginType type) const {
        std::vector<std::shared_ptr<IPlugin>> result;
        for (const auto& [id, plugin] : loadedPlugins_) {
            if (plugin->getType() == type) {
                result.push_back(plugin);
            }
        }
        return result;
    }
    
    // === Asset Queries ===
    
    // Get all assets of a specific type from all loaded plugins
    std::vector<std::pair<std::string, PluginAsset>> getAllAssets(PluginType type) const {
        std::vector<std::pair<std::string, PluginAsset>> result;
        
        for (const auto& [pluginId, plugin] : loadedPlugins_) {
            if (plugin->getType() == type) {
                for (const auto& asset : plugin->getAssets()) {
                    result.push_back({pluginId, asset});
                }
            }
        }
        
        return result;
    }
    
    // Search assets by tag
    std::vector<std::pair<std::string, PluginAsset>> searchAssets(
        const std::string& query, 
        PluginType type = PluginType::Unknown) const 
    {
        std::vector<std::pair<std::string, PluginAsset>> result;
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (const auto& [pluginId, plugin] : loadedPlugins_) {
            if (type != PluginType::Unknown && plugin->getType() != type) continue;
            
            for (const auto& asset : plugin->getAssets()) {
                // Check name
                std::string lowerName = asset.name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                
                if (lowerName.find(lowerQuery) != std::string::npos) {
                    result.push_back({pluginId, asset});
                    continue;
                }
                
                // Check tags
                for (const auto& tag : asset.tags) {
                    std::string lowerTag = tag;
                    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                    if (lowerTag.find(lowerQuery) != std::string::npos) {
                        result.push_back({pluginId, asset});
                        break;
                    }
                }
            }
        }
        
        return result;
    }
    
    // === Event Listeners ===
    
    class IPluginListener {
    public:
        virtual ~IPluginListener() = default;
        virtual void onPluginLoaded(const std::string& pluginId) {}
        virtual void onPluginUnloaded(const std::string& pluginId) {}
        virtual void onPluginError(const std::string& pluginId, const std::string& error) {}
    };
    
    void addListener(std::shared_ptr<IPluginListener> listener) {
        listeners_.push_back(listener);
    }
    
    void removeListener(std::shared_ptr<IPluginListener> listener) {
        listeners_.erase(
            std::remove(listeners_.begin(), listeners_.end(), listener),
            listeners_.end());
    }
    
    // === Package Path ===
    
    std::string getPluginPath(const std::string& pluginId) const {
        auto it = packagePaths_.find(pluginId);
        return (it != packagePaths_.end()) ? it->second : "";
    }
    
    std::string resolveAssetPath(const std::string& pluginId, const std::string& relativePath) const {
        std::string basePath = getPluginPath(pluginId);
        if (basePath.empty()) return relativePath;
        return (std::filesystem::path(basePath) / relativePath).string();
    }
    
private:
    PluginManager() {
        // Default plugin directories
        pluginDirs_.push_back("plugins");
        pluginDirs_.push_back("~/.luma/plugins");
        
#ifdef __APPLE__
        pluginDirs_.push_back("/Library/Application Support/LUMA/plugins");
#elif _WIN32
        pluginDirs_.push_back("%APPDATA%/LUMA/plugins");
#else
        pluginDirs_.push_back("/usr/share/luma/plugins");
#endif
    }
    
    PluginMetadata loadManifest(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return {};
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return ManifestParser::parse(buffer.str());
    }
    
    std::vector<std::string> pluginDirs_;
    std::unordered_map<std::string, std::string> packagePaths_;
    std::unordered_map<std::string, std::shared_ptr<IPlugin>> loadedPlugins_;
    std::vector<std::shared_ptr<IPluginListener>> listeners_;
    
    // === Asset-Only Plugin (for content-only packages) ===
    
    class AssetOnlyPlugin : public IPlugin {
    public:
        AssetOnlyPlugin(const PluginMetadata& meta, const std::string& basePath)
            : metadata_(meta), basePath_(basePath) {}
        
        const PluginMetadata& getMetadata() const override { return metadata_; }
        
        bool initialize() override {
            // Load assets from assets/ directory
            loadAssetsFromDirectory();
            return true;
        }
        
        void shutdown() override {
            assets_.clear();
        }
        
        std::vector<PluginAsset> getAssets() const override {
            return assets_;
        }
        
        const PluginAsset* getAsset(const std::string& assetId) const override {
            for (const auto& asset : assets_) {
                if (asset.id == assetId) return &asset;
            }
            return nullptr;
        }
        
    private:
        void loadAssetsFromDirectory() {
            auto assetsDir = std::filesystem::path(basePath_) / "assets";
            if (!std::filesystem::exists(assetsDir)) return;
            
            // Look for asset config files
            auto configsDir = assetsDir / "configs";
            if (std::filesystem::exists(configsDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(configsDir)) {
                    if (entry.path().extension() == ".json") {
                        loadAssetConfig(entry.path().string());
                    }
                }
            }
            
            // Auto-discover meshes without configs
            auto meshesDir = assetsDir / "meshes";
            if (std::filesystem::exists(meshesDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(meshesDir)) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
                        std::string id = entry.path().stem().string();
                        
                        // Check if already loaded via config
                        bool found = false;
                        for (const auto& asset : assets_) {
                            if (asset.id == id) { found = true; break; }
                        }
                        
                        if (!found) {
                            PluginAsset asset;
                            asset.id = id;
                            asset.name = id;
                            asset.meshPath = "assets/meshes/" + entry.path().filename().string();
                            
                            // Look for matching texture
                            auto texPath = assetsDir / "textures" / (id + ".png");
                            if (std::filesystem::exists(texPath)) {
                                asset.texturePath = "assets/textures/" + id + ".png";
                            }
                            
                            assets_.push_back(asset);
                        }
                    }
                }
            }
        }
        
        void loadAssetConfig(const std::string& configPath) {
            std::ifstream file(configPath);
            if (!file.is_open()) return;
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            
            // Simple JSON parsing for asset config
            auto getValue = [&](const std::string& key) -> std::string {
                std::string search = "\"" + key + "\"";
                size_t pos = content.find(search);
                if (pos == std::string::npos) return "";
                pos = content.find(":", pos);
                if (pos == std::string::npos) return "";
                pos = content.find("\"", pos);
                if (pos == std::string::npos) return "";
                size_t end = content.find("\"", pos + 1);
                if (end == std::string::npos) return "";
                return content.substr(pos + 1, end - pos - 1);
            };
            
            PluginAsset asset;
            asset.id = getValue("id");
            asset.name = getValue("name");
            asset.category = getValue("category");
            asset.description = getValue("description");
            asset.meshPath = getValue("mesh");
            asset.texturePath = getValue("texture");
            asset.thumbnailPath = getValue("thumbnail");
            
            if (!asset.id.empty()) {
                assets_.push_back(asset);
            }
        }
        
        PluginMetadata metadata_;
        std::string basePath_;
        std::vector<PluginAsset> assets_;
    };
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline PluginManager& getPluginManager() {
    return PluginManager::getInstance();
}

// Get all character template plugins
inline std::vector<std::shared_ptr<ICharacterTemplatePlugin>> getCharacterTemplatePlugins() {
    std::vector<std::shared_ptr<ICharacterTemplatePlugin>> result;
    for (auto& plugin : getPluginManager().getPluginsByType(PluginType::CharacterTemplate)) {
        auto templatePlugin = std::dynamic_pointer_cast<ICharacterTemplatePlugin>(plugin);
        if (templatePlugin) {
            result.push_back(templatePlugin);
        }
    }
    return result;
}

// Get all clothing plugins
inline std::vector<std::shared_ptr<IClothingPlugin>> getClothingPlugins() {
    std::vector<std::shared_ptr<IClothingPlugin>> result;
    for (auto& plugin : getPluginManager().getPluginsByType(PluginType::Clothing)) {
        auto clothingPlugin = std::dynamic_pointer_cast<IClothingPlugin>(plugin);
        if (clothingPlugin) {
            result.push_back(clothingPlugin);
        }
    }
    return result;
}

// Get all hair plugins
inline std::vector<std::shared_ptr<IHairPlugin>> getHairPlugins() {
    std::vector<std::shared_ptr<IHairPlugin>> result;
    for (auto& plugin : getPluginManager().getPluginsByType(PluginType::Hair)) {
        auto hairPlugin = std::dynamic_pointer_cast<IHairPlugin>(plugin);
        if (hairPlugin) {
            result.push_back(hairPlugin);
        }
    }
    return result;
}

}  // namespace luma
