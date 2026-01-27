// Data-Driven System
// Hot reload, config tables, localization
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

namespace luma {

// ===== Config Value =====
using ConfigValue = std::variant<
    std::monostate,     // null
    bool,
    int64_t,
    double,
    std::string,
    std::vector<std::string>,
    std::vector<double>
>;

// ===== Config Table =====
class ConfigTable {
public:
    ConfigTable(const std::string& name = "") : name_(name) {}
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Set values
    void setBool(const std::string& key, bool value) { data_[key] = value; }
    void setInt(const std::string& key, int64_t value) { data_[key] = value; }
    void setFloat(const std::string& key, double value) { data_[key] = value; }
    void setString(const std::string& key, const std::string& value) { data_[key] = value; }
    void setStringArray(const std::string& key, const std::vector<std::string>& value) { data_[key] = value; }
    void setFloatArray(const std::string& key, const std::vector<double>& value) { data_[key] = value; }
    
    // Get values with defaults
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second);
        }
        return defaultValue;
    }
    
    int64_t getInt(const std::string& key, int64_t defaultValue = 0) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (std::holds_alternative<int64_t>(it->second)) {
                return std::get<int64_t>(it->second);
            }
            if (std::holds_alternative<double>(it->second)) {
                return (int64_t)std::get<double>(it->second);
            }
        }
        return defaultValue;
    }
    
    double getFloat(const std::string& key, double defaultValue = 0.0) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            if (std::holds_alternative<double>(it->second)) {
                return std::get<double>(it->second);
            }
            if (std::holds_alternative<int64_t>(it->second)) {
                return (double)std::get<int64_t>(it->second);
            }
        }
        return defaultValue;
    }
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<std::string>(it->second)) {
            return std::get<std::string>(it->second);
        }
        return defaultValue;
    }
    
    std::vector<std::string> getStringArray(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<std::vector<std::string>>(it->second)) {
            return std::get<std::vector<std::string>>(it->second);
        }
        return {};
    }
    
    std::vector<double> getFloatArray(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<std::vector<double>>(it->second)) {
            return std::get<std::vector<double>>(it->second);
        }
        return {};
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
    
    const std::unordered_map<std::string, ConfigValue>& getAllData() const {
        return data_;
    }
    
    // Parse from simple key=value format
    bool parseFromString(const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            
            // Find key=value
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            
            // Trim whitespace
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(key);
            trim(value);
            
            if (key.empty()) continue;
            
            // Detect type
            if (value == "true" || value == "false") {
                setBool(key, value == "true");
            } else if (value.find('.') != std::string::npos) {
                try {
                    setFloat(key, std::stod(value));
                } catch (...) {
                    setString(key, value);
                }
            } else {
                try {
                    setInt(key, std::stoll(value));
                } catch (...) {
                    setString(key, value);
                }
            }
        }
        
        return true;
    }
    
    // Serialize to string
    std::string serializeToString() const {
        std::ostringstream ss;
        
        for (const auto& [key, value] : data_) {
            ss << key << " = ";
            
            if (std::holds_alternative<bool>(value)) {
                ss << (std::get<bool>(value) ? "true" : "false");
            } else if (std::holds_alternative<int64_t>(value)) {
                ss << std::get<int64_t>(value);
            } else if (std::holds_alternative<double>(value)) {
                ss << std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                ss << std::get<std::string>(value);
            }
            
            ss << "\n";
        }
        
        return ss.str();
    }
    
private:
    std::string name_;
    std::unordered_map<std::string, ConfigValue> data_;
};

// ===== Localization =====
class Localization {
public:
    Localization(const std::string& language = "en") : currentLanguage_(language) {}
    
    // Set language
    void setLanguage(const std::string& language) {
        currentLanguage_ = language;
    }
    
    const std::string& getLanguage() const { return currentLanguage_; }
    
    // Get available languages
    std::vector<std::string> getAvailableLanguages() const {
        std::vector<std::string> langs;
        for (const auto& [lang, _] : strings_) {
            langs.push_back(lang);
        }
        return langs;
    }
    
    // Load strings for a language
    void loadStrings(const std::string& language,
                     const std::unordered_map<std::string, std::string>& strings) {
        strings_[language] = strings;
    }
    
    // Parse from simple format: key = value
    bool parseFromString(const std::string& language, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        
        auto& langStrings = strings_[language];
        
        while (std::getline(stream, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            
            // Trim
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(key);
            trim(value);
            
            // Handle escape sequences
            size_t pos = 0;
            while ((pos = value.find("\\n", pos)) != std::string::npos) {
                value.replace(pos, 2, "\n");
                pos++;
            }
            
            langStrings[key] = value;
        }
        
        return true;
    }
    
    // Get localized string
    std::string get(const std::string& key) const {
        auto langIt = strings_.find(currentLanguage_);
        if (langIt != strings_.end()) {
            auto strIt = langIt->second.find(key);
            if (strIt != langIt->second.end()) {
                return strIt->second;
            }
        }
        
        // Fallback to English
        if (currentLanguage_ != "en") {
            auto enIt = strings_.find("en");
            if (enIt != strings_.end()) {
                auto strIt = enIt->second.find(key);
                if (strIt != enIt->second.end()) {
                    return strIt->second;
                }
            }
        }
        
        return key;  // Return key if not found
    }
    
    // Get with format arguments
    template<typename... Args>
    std::string format(const std::string& key, Args... args) const {
        std::string str = get(key);
        return formatString(str, args...);
    }
    
    // Check if key exists
    bool has(const std::string& key) const {
        auto langIt = strings_.find(currentLanguage_);
        if (langIt != strings_.end()) {
            return langIt->second.find(key) != langIt->second.end();
        }
        return false;
    }
    
private:
    template<typename T, typename... Rest>
    std::string formatString(const std::string& fmt, T value, Rest... rest) const {
        std::string result = fmt;
        size_t pos = result.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream ss;
            ss << value;
            result.replace(pos, 2, ss.str());
        }
        if constexpr (sizeof...(rest) > 0) {
            return formatString(result, rest...);
        }
        return result;
    }
    
    std::string formatString(const std::string& fmt) const {
        return fmt;
    }
    
    std::string currentLanguage_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> strings_;
};

// ===== File Watcher =====
struct WatchedFile {
    std::string path;
    std::filesystem::file_time_type lastModified;
    std::function<void(const std::string&)> callback;
};

class FileWatcher {
public:
    void addWatch(const std::string& path,
                  std::function<void(const std::string&)> callback) {
        WatchedFile watch;
        watch.path = path;
        watch.callback = callback;
        
        try {
            if (std::filesystem::exists(path)) {
                watch.lastModified = std::filesystem::last_write_time(path);
            }
        } catch (...) {}
        
        watchedFiles_[path] = watch;
    }
    
    void removeWatch(const std::string& path) {
        watchedFiles_.erase(path);
    }
    
    void clearAllWatches() {
        watchedFiles_.clear();
    }
    
    // Check for changes (call periodically)
    void update() {
        for (auto& [path, watch] : watchedFiles_) {
            try {
                if (!std::filesystem::exists(path)) continue;
                
                auto currentTime = std::filesystem::last_write_time(path);
                if (currentTime != watch.lastModified) {
                    watch.lastModified = currentTime;
                    if (watch.callback) {
                        watch.callback(path);
                    }
                }
            } catch (...) {}
        }
    }
    
    size_t getWatchCount() const { return watchedFiles_.size(); }
    
private:
    std::unordered_map<std::string, WatchedFile> watchedFiles_;
};

// ===== Data Manager =====
class DataManager {
public:
    static DataManager& getInstance() {
        static DataManager instance;
        return instance;
    }
    
    // Initialize
    void initialize(const std::string& dataPath = "data/") {
        dataPath_ = dataPath;
        hotReloadEnabled_ = true;
    }
    
    void setDataPath(const std::string& path) { dataPath_ = path; }
    const std::string& getDataPath() const { return dataPath_; }
    
    // === Config Tables ===
    
    ConfigTable* loadConfig(const std::string& name, const std::string& path = "") {
        std::string fullPath = path.empty() ? (dataPath_ + "config/" + name + ".cfg") : path;
        
        auto config = std::make_unique<ConfigTable>(name);
        
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            config->parseFromString(buffer.str());
        }
        
        ConfigTable* ptr = config.get();
        configs_[name] = std::move(config);
        
        // Set up hot reload
        if (hotReloadEnabled_) {
            fileWatcher_.addWatch(fullPath, [this, name](const std::string& path) {
                reloadConfig(name, path);
            });
        }
        
        return ptr;
    }
    
    ConfigTable* getConfig(const std::string& name) {
        auto it = configs_.find(name);
        return it != configs_.end() ? it->second.get() : nullptr;
    }
    
    void saveConfig(const std::string& name, const std::string& path = "") {
        auto it = configs_.find(name);
        if (it == configs_.end()) return;
        
        std::string fullPath = path.empty() ? (dataPath_ + "config/" + name + ".cfg") : path;
        
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << it->second->serializeToString();
        }
    }
    
    void reloadConfig(const std::string& name, const std::string& path = "") {
        auto it = configs_.find(name);
        if (it == configs_.end()) return;
        
        std::string fullPath = path.empty() ? (dataPath_ + "config/" + name + ".cfg") : path;
        
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            it->second->clear();
            it->second->parseFromString(buffer.str());
            
            // Notify listeners
            auto listenerIt = configListeners_.find(name);
            if (listenerIt != configListeners_.end()) {
                for (auto& listener : listenerIt->second) {
                    listener(it->second.get());
                }
            }
        }
    }
    
    // Config change listeners
    void addConfigListener(const std::string& name,
                          std::function<void(ConfigTable*)> listener) {
        configListeners_[name].push_back(listener);
    }
    
    // === Localization ===
    
    Localization& getLocalization() { return localization_; }
    
    void loadLanguage(const std::string& language, const std::string& path = "") {
        std::string fullPath = path.empty() 
            ? (dataPath_ + "lang/" + language + ".txt") 
            : path;
        
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            localization_.parseFromString(language, buffer.str());
        }
        
        // Set up hot reload
        if (hotReloadEnabled_) {
            fileWatcher_.addWatch(fullPath, [this, language](const std::string& path) {
                std::ifstream f(path);
                if (f.is_open()) {
                    std::stringstream buffer;
                    buffer << f.rdbuf();
                    localization_.parseFromString(language, buffer.str());
                }
            });
        }
    }
    
    void setLanguage(const std::string& language) {
        localization_.setLanguage(language);
    }
    
    std::string localize(const std::string& key) const {
        return localization_.get(key);
    }
    
    template<typename... Args>
    std::string localizeFormat(const std::string& key, Args... args) const {
        return localization_.format(key, args...);
    }
    
    // === Hot Reload ===
    
    void setHotReloadEnabled(bool enabled) { hotReloadEnabled_ = enabled; }
    bool isHotReloadEnabled() const { return hotReloadEnabled_; }
    
    void update() {
        if (hotReloadEnabled_) {
            fileWatcher_.update();
        }
    }
    
    // === Generic Data Files ===
    
    std::string loadTextFile(const std::string& path) {
        std::string fullPath = dataPath_ + path;
        std::ifstream file(fullPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
        return "";
    }
    
    bool saveTextFile(const std::string& path, const std::string& content) {
        std::string fullPath = dataPath_ + path;
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << content;
            return true;
        }
        return false;
    }
    
    // Watched file count
    size_t getWatchedFileCount() const { return fileWatcher_.getWatchCount(); }
    
private:
    DataManager() = default;
    
    std::string dataPath_ = "data/";
    bool hotReloadEnabled_ = true;
    
    std::unordered_map<std::string, std::unique_ptr<ConfigTable>> configs_;
    std::unordered_map<std::string, std::vector<std::function<void(ConfigTable*)>>> configListeners_;
    
    Localization localization_;
    FileWatcher fileWatcher_;
};

// ===== Global Accessor =====
inline DataManager& getDataManager() {
    return DataManager::getInstance();
}

// ===== Convenience Macros =====
#define LOC(key) luma::getDataManager().localize(key)
#define LOC_FMT(key, ...) luma::getDataManager().localizeFormat(key, __VA_ARGS__)
#define CONFIG(name) luma::getDataManager().getConfig(name)

}  // namespace luma
