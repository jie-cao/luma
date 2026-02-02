// LUMA Project File System - Save and load character projects
// File format: .luma (JSON-based)
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace luma {

// ============================================================================
// Project File Format Version
// ============================================================================

constexpr int PROJECT_FORMAT_VERSION_MAJOR = 1;
constexpr int PROJECT_FORMAT_VERSION_MINOR = 0;

// ============================================================================
// Character Project Data
// ============================================================================

struct CharacterProjectData {
    // Metadata
    std::string name = "Untitled";
    std::string author;
    std::string description;
    std::string createdDate;
    std::string modifiedDate;
    int formatVersionMajor = PROJECT_FORMAT_VERSION_MAJOR;
    int formatVersionMinor = PROJECT_FORMAT_VERSION_MINOR;
    
    // Character type
    int characterType = 0;  // 0=Human, 1=Cartoon, 2=Mascot, etc.
    
    // Body parameters
    struct BodyParams {
        int gender = 0;          // 0=Male, 1=Female, 2=Neutral
        int ageGroup = 3;        // 0=Child, 1=Teen, 2=YoungAdult, 3=Adult, 4=Senior
        float height = 0.5f;
        float weight = 0.5f;
        float muscularity = 0.3f;
        float bodyFat = 0.3f;
        float shoulderWidth = 0.5f;
        float chestSize = 0.5f;
        float waistSize = 0.5f;
        float hipWidth = 0.5f;
        float armLength = 0.5f;
        float armThickness = 0.5f;
        float legLength = 0.5f;
        float thighThickness = 0.5f;
        float bustSize = 0.5f;
        Vec3 skinColor{0.85f, 0.65f, 0.5f};
    } body;
    
    // Face parameters
    struct FaceParams {
        float faceWidth = 0.5f;
        float faceLength = 0.5f;
        float faceRoundness = 0.5f;
        float eyeSize = 0.5f;
        float eyeSpacing = 0.5f;
        float eyeHeight = 0.5f;
        float eyeAngle = 0.5f;
        Vec3 eyeColor{0.3f, 0.4f, 0.2f};
        float noseLength = 0.5f;
        float noseWidth = 0.5f;
        float noseHeight = 0.5f;
        float noseBridge = 0.5f;
        float mouthWidth = 0.5f;
        float upperLipThickness = 0.5f;
        float lowerLipThickness = 0.5f;
        float jawWidth = 0.5f;
        float jawLine = 0.5f;
        float chinLength = 0.5f;
        float chinWidth = 0.5f;
    } face;
    
    // Texture parameters
    struct TextureParams {
        int skinTonePreset = 0;
        Vec3 customSkinColor{0.85f, 0.65f, 0.5f};
        int eyeColorPreset = 0;
        Vec3 customEyeColor{0.3f, 0.4f, 0.2f};
        int lipColorPreset = 0;
        Vec3 customLipColor{0.75f, 0.45f, 0.45f};
        float skinRoughness = 0.5f;
        float skinSubsurface = 0.3f;
    } textures;
    
    // Hair
    struct HairParams {
        std::string styleId = "bald";
        int colorPreset = 1;  // 0=Black, 1=Brown, etc.
        Vec3 customColor{0.2f, 0.15f, 0.1f};
        bool useCustomColor = false;
    } hair;
    
    // Clothing
    struct ClothingItem {
        std::string assetId;
        std::string slot;
        Vec3 color{1, 1, 1};
        std::string customTexturePath;
    };
    std::vector<ClothingItem> clothing;
    
    // BlendShape weights
    std::unordered_map<std::string, float> blendShapeWeights;
    
    // Expression preset
    std::string expressionPreset = "neutral";
    
    // Camera/View settings
    struct ViewSettings {
        float cameraDistance = 3.0f;
        float cameraYaw = 0.0f;
        float cameraPitch = 0.0f;
        bool autoRotate = true;
    } view;
    
    // Thumbnail (base64 encoded PNG)
    std::string thumbnailBase64;
};

// ============================================================================
// Project File Writer
// ============================================================================

class ProjectWriter {
public:
    static bool save(const CharacterProjectData& project, const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << "{\n";
        
        // Metadata
        file << "  \"formatVersion\": \"" << project.formatVersionMajor << "." 
             << project.formatVersionMinor << "\",\n";
        file << "  \"name\": " << escapeString(project.name) << ",\n";
        file << "  \"author\": " << escapeString(project.author) << ",\n";
        file << "  \"description\": " << escapeString(project.description) << ",\n";
        file << "  \"createdDate\": " << escapeString(project.createdDate) << ",\n";
        file << "  \"modifiedDate\": " << escapeString(project.modifiedDate) << ",\n";
        file << "  \"characterType\": " << project.characterType << ",\n";
        
        // Body
        file << "  \"body\": {\n";
        file << "    \"gender\": " << project.body.gender << ",\n";
        file << "    \"ageGroup\": " << project.body.ageGroup << ",\n";
        file << "    \"height\": " << project.body.height << ",\n";
        file << "    \"weight\": " << project.body.weight << ",\n";
        file << "    \"muscularity\": " << project.body.muscularity << ",\n";
        file << "    \"bodyFat\": " << project.body.bodyFat << ",\n";
        file << "    \"shoulderWidth\": " << project.body.shoulderWidth << ",\n";
        file << "    \"chestSize\": " << project.body.chestSize << ",\n";
        file << "    \"waistSize\": " << project.body.waistSize << ",\n";
        file << "    \"hipWidth\": " << project.body.hipWidth << ",\n";
        file << "    \"armLength\": " << project.body.armLength << ",\n";
        file << "    \"armThickness\": " << project.body.armThickness << ",\n";
        file << "    \"legLength\": " << project.body.legLength << ",\n";
        file << "    \"thighThickness\": " << project.body.thighThickness << ",\n";
        file << "    \"bustSize\": " << project.body.bustSize << ",\n";
        file << "    \"skinColor\": " << vec3ToJson(project.body.skinColor) << "\n";
        file << "  },\n";
        
        // Face
        file << "  \"face\": {\n";
        file << "    \"faceWidth\": " << project.face.faceWidth << ",\n";
        file << "    \"faceLength\": " << project.face.faceLength << ",\n";
        file << "    \"faceRoundness\": " << project.face.faceRoundness << ",\n";
        file << "    \"eyeSize\": " << project.face.eyeSize << ",\n";
        file << "    \"eyeSpacing\": " << project.face.eyeSpacing << ",\n";
        file << "    \"eyeHeight\": " << project.face.eyeHeight << ",\n";
        file << "    \"eyeAngle\": " << project.face.eyeAngle << ",\n";
        file << "    \"eyeColor\": " << vec3ToJson(project.face.eyeColor) << ",\n";
        file << "    \"noseLength\": " << project.face.noseLength << ",\n";
        file << "    \"noseWidth\": " << project.face.noseWidth << ",\n";
        file << "    \"noseHeight\": " << project.face.noseHeight << ",\n";
        file << "    \"noseBridge\": " << project.face.noseBridge << ",\n";
        file << "    \"mouthWidth\": " << project.face.mouthWidth << ",\n";
        file << "    \"upperLipThickness\": " << project.face.upperLipThickness << ",\n";
        file << "    \"lowerLipThickness\": " << project.face.lowerLipThickness << ",\n";
        file << "    \"jawWidth\": " << project.face.jawWidth << ",\n";
        file << "    \"jawLine\": " << project.face.jawLine << ",\n";
        file << "    \"chinLength\": " << project.face.chinLength << ",\n";
        file << "    \"chinWidth\": " << project.face.chinWidth << "\n";
        file << "  },\n";
        
        // Textures
        file << "  \"textures\": {\n";
        file << "    \"skinTonePreset\": " << project.textures.skinTonePreset << ",\n";
        file << "    \"customSkinColor\": " << vec3ToJson(project.textures.customSkinColor) << ",\n";
        file << "    \"eyeColorPreset\": " << project.textures.eyeColorPreset << ",\n";
        file << "    \"customEyeColor\": " << vec3ToJson(project.textures.customEyeColor) << ",\n";
        file << "    \"lipColorPreset\": " << project.textures.lipColorPreset << ",\n";
        file << "    \"customLipColor\": " << vec3ToJson(project.textures.customLipColor) << ",\n";
        file << "    \"skinRoughness\": " << project.textures.skinRoughness << ",\n";
        file << "    \"skinSubsurface\": " << project.textures.skinSubsurface << "\n";
        file << "  },\n";
        
        // Hair
        file << "  \"hair\": {\n";
        file << "    \"styleId\": " << escapeString(project.hair.styleId) << ",\n";
        file << "    \"colorPreset\": " << project.hair.colorPreset << ",\n";
        file << "    \"customColor\": " << vec3ToJson(project.hair.customColor) << ",\n";
        file << "    \"useCustomColor\": " << (project.hair.useCustomColor ? "true" : "false") << "\n";
        file << "  },\n";
        
        // Clothing
        file << "  \"clothing\": [\n";
        for (size_t i = 0; i < project.clothing.size(); i++) {
            const auto& item = project.clothing[i];
            file << "    {\n";
            file << "      \"assetId\": " << escapeString(item.assetId) << ",\n";
            file << "      \"slot\": " << escapeString(item.slot) << ",\n";
            file << "      \"color\": " << vec3ToJson(item.color) << ",\n";
            file << "      \"customTexturePath\": " << escapeString(item.customTexturePath) << "\n";
            file << "    }";
            if (i < project.clothing.size() - 1) file << ",";
            file << "\n";
        }
        file << "  ],\n";
        
        // BlendShape weights
        file << "  \"blendShapeWeights\": {\n";
        size_t bsCount = 0;
        for (const auto& [name, weight] : project.blendShapeWeights) {
            file << "    " << escapeString(name) << ": " << weight;
            if (++bsCount < project.blendShapeWeights.size()) file << ",";
            file << "\n";
        }
        file << "  },\n";
        
        // Expression
        file << "  \"expressionPreset\": " << escapeString(project.expressionPreset) << ",\n";
        
        // View settings
        file << "  \"view\": {\n";
        file << "    \"cameraDistance\": " << project.view.cameraDistance << ",\n";
        file << "    \"cameraYaw\": " << project.view.cameraYaw << ",\n";
        file << "    \"cameraPitch\": " << project.view.cameraPitch << ",\n";
        file << "    \"autoRotate\": " << (project.view.autoRotate ? "true" : "false") << "\n";
        file << "  }";
        
        // Thumbnail
        if (!project.thumbnailBase64.empty()) {
            file << ",\n  \"thumbnail\": " << escapeString(project.thumbnailBase64);
        }
        
        file << "\n}\n";
        
        file.close();
        return true;
    }
    
private:
    static std::string escapeString(const std::string& s) {
        std::string result = "\"";
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        result += "\"";
        return result;
    }
    
    static std::string vec3ToJson(const Vec3& v) {
        std::ostringstream ss;
        ss << "[" << v.x << ", " << v.y << ", " << v.z << "]";
        return ss.str();
    }
};

// ============================================================================
// Project File Reader
// ============================================================================

class ProjectReader {
public:
    static bool load(const std::string& path, CharacterProjectData& project) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        // Simple JSON parsing
        project.name = getString(content, "name", "Untitled");
        project.author = getString(content, "author", "");
        project.description = getString(content, "description", "");
        project.createdDate = getString(content, "createdDate", "");
        project.modifiedDate = getString(content, "modifiedDate", "");
        project.characterType = getInt(content, "characterType", 0);
        
        // Body
        project.body.gender = getInt(content, "gender", 0);
        project.body.ageGroup = getInt(content, "ageGroup", 3);
        project.body.height = getFloat(content, "height", 0.5f);
        project.body.weight = getFloat(content, "weight", 0.5f);
        project.body.muscularity = getFloat(content, "muscularity", 0.3f);
        project.body.bodyFat = getFloat(content, "bodyFat", 0.3f);
        project.body.shoulderWidth = getFloat(content, "shoulderWidth", 0.5f);
        project.body.chestSize = getFloat(content, "chestSize", 0.5f);
        project.body.waistSize = getFloat(content, "waistSize", 0.5f);
        project.body.hipWidth = getFloat(content, "hipWidth", 0.5f);
        project.body.armLength = getFloat(content, "armLength", 0.5f);
        project.body.armThickness = getFloat(content, "armThickness", 0.5f);
        project.body.legLength = getFloat(content, "legLength", 0.5f);
        project.body.thighThickness = getFloat(content, "thighThickness", 0.5f);
        project.body.bustSize = getFloat(content, "bustSize", 0.5f);
        project.body.skinColor = getVec3(content, "skinColor", Vec3(0.85f, 0.65f, 0.5f));
        
        // Face
        project.face.faceWidth = getFloat(content, "faceWidth", 0.5f);
        project.face.faceLength = getFloat(content, "faceLength", 0.5f);
        project.face.faceRoundness = getFloat(content, "faceRoundness", 0.5f);
        project.face.eyeSize = getFloat(content, "eyeSize", 0.5f);
        project.face.eyeSpacing = getFloat(content, "eyeSpacing", 0.5f);
        project.face.eyeHeight = getFloat(content, "eyeHeight", 0.5f);
        project.face.eyeAngle = getFloat(content, "eyeAngle", 0.5f);
        project.face.eyeColor = getVec3(content, "eyeColor", Vec3(0.3f, 0.4f, 0.2f));
        project.face.noseLength = getFloat(content, "noseLength", 0.5f);
        project.face.noseWidth = getFloat(content, "noseWidth", 0.5f);
        project.face.noseHeight = getFloat(content, "noseHeight", 0.5f);
        project.face.noseBridge = getFloat(content, "noseBridge", 0.5f);
        project.face.mouthWidth = getFloat(content, "mouthWidth", 0.5f);
        project.face.upperLipThickness = getFloat(content, "upperLipThickness", 0.5f);
        project.face.lowerLipThickness = getFloat(content, "lowerLipThickness", 0.5f);
        project.face.jawWidth = getFloat(content, "jawWidth", 0.5f);
        project.face.jawLine = getFloat(content, "jawLine", 0.5f);
        project.face.chinLength = getFloat(content, "chinLength", 0.5f);
        project.face.chinWidth = getFloat(content, "chinWidth", 0.5f);
        
        // Hair
        project.hair.styleId = getString(content, "styleId", "bald");
        project.hair.colorPreset = getInt(content, "colorPreset", 1);
        project.hair.customColor = getVec3(content, "customColor", Vec3(0.2f, 0.15f, 0.1f));
        project.hair.useCustomColor = getBool(content, "useCustomColor", false);
        
        // Expression
        project.expressionPreset = getString(content, "expressionPreset", "neutral");
        
        // View
        project.view.cameraDistance = getFloat(content, "cameraDistance", 3.0f);
        project.view.cameraYaw = getFloat(content, "cameraYaw", 0.0f);
        project.view.cameraPitch = getFloat(content, "cameraPitch", 0.0f);
        project.view.autoRotate = getBool(content, "autoRotate", true);
        
        // Thumbnail
        project.thumbnailBase64 = getString(content, "thumbnail", "");
        
        return true;
    }
    
private:
    static std::string getString(const std::string& json, const std::string& key, const std::string& defaultVal) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        // Skip whitespace
        pos = json.find_first_not_of(" \t\n\r", pos + 1);
        if (pos == std::string::npos) return defaultVal;
        
        if (json[pos] != '"') return defaultVal;
        
        size_t end = pos + 1;
        while (end < json.size()) {
            if (json[end] == '"' && json[end - 1] != '\\') break;
            end++;
        }
        
        return json.substr(pos + 1, end - pos - 1);
    }
    
    static float getFloat(const std::string& json, const std::string& key, float defaultVal) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find_first_not_of(" \t\n\r", pos + 1);
        if (pos == std::string::npos) return defaultVal;
        
        try {
            return std::stof(json.substr(pos));
        } catch (...) {
            return defaultVal;
        }
    }
    
    static int getInt(const std::string& json, const std::string& key, int defaultVal) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find_first_not_of(" \t\n\r", pos + 1);
        if (pos == std::string::npos) return defaultVal;
        
        try {
            return std::stoi(json.substr(pos));
        } catch (...) {
            return defaultVal;
        }
    }
    
    static bool getBool(const std::string& json, const std::string& key, bool defaultVal) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find_first_not_of(" \t\n\r", pos + 1);
        if (pos == std::string::npos) return defaultVal;
        
        if (json.substr(pos, 4) == "true") return true;
        if (json.substr(pos, 5) == "false") return false;
        return defaultVal;
    }
    
    static Vec3 getVec3(const std::string& json, const std::string& key, const Vec3& defaultVal) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return defaultVal;
        
        pos = json.find("[", pos);
        if (pos == std::string::npos) return defaultVal;
        
        size_t end = json.find("]", pos);
        if (end == std::string::npos) return defaultVal;
        
        std::string arr = json.substr(pos + 1, end - pos - 1);
        
        Vec3 result = defaultVal;
        try {
            size_t comma1 = arr.find(",");
            size_t comma2 = arr.find(",", comma1 + 1);
            
            result.x = std::stof(arr.substr(0, comma1));
            result.y = std::stof(arr.substr(comma1 + 1, comma2 - comma1 - 1));
            result.z = std::stof(arr.substr(comma2 + 1));
        } catch (...) {}
        
        return result;
    }
};

// ============================================================================
// Project Manager - Handle recent projects and auto-save
// ============================================================================

class ProjectManager {
public:
    static ProjectManager& getInstance() {
        static ProjectManager instance;
        return instance;
    }
    
    // Current project
    CharacterProjectData currentProject;
    std::string currentProjectPath;
    bool hasUnsavedChanges = false;
    
    // Recent projects
    std::vector<std::string> recentProjects;
    static constexpr int MAX_RECENT_PROJECTS = 10;
    
    // Save current project
    bool saveProject() {
        if (currentProjectPath.empty()) {
            return false;  // Need to use saveProjectAs
        }
        return saveProjectAs(currentProjectPath);
    }
    
    bool saveProjectAs(const std::string& path) {
        // Update modification date
        currentProject.modifiedDate = getCurrentDateTime();
        if (currentProject.createdDate.empty()) {
            currentProject.createdDate = currentProject.modifiedDate;
        }
        
        if (ProjectWriter::save(currentProject, path)) {
            currentProjectPath = path;
            hasUnsavedChanges = false;
            addToRecent(path);
            return true;
        }
        return false;
    }
    
    // Load project
    bool loadProject(const std::string& path) {
        CharacterProjectData loaded;
        if (ProjectReader::load(path, loaded)) {
            currentProject = loaded;
            currentProjectPath = path;
            hasUnsavedChanges = false;
            addToRecent(path);
            return true;
        }
        return false;
    }
    
    // New project
    void newProject() {
        currentProject = CharacterProjectData();
        currentProjectPath.clear();
        hasUnsavedChanges = false;
    }
    
    // Mark as modified
    void markModified() {
        hasUnsavedChanges = true;
    }
    
    // Auto-save
    void enableAutoSave(bool enable, int intervalSeconds = 60) {
        autoSaveEnabled_ = enable;
        autoSaveInterval_ = intervalSeconds;
    }
    
    void updateAutoSave(float deltaTime) {
        if (!autoSaveEnabled_ || currentProjectPath.empty()) return;
        
        autoSaveTimer_ += deltaTime;
        if (autoSaveTimer_ >= autoSaveInterval_ && hasUnsavedChanges) {
            // Save to backup file
            std::string backupPath = currentProjectPath + ".backup";
            ProjectWriter::save(currentProject, backupPath);
            autoSaveTimer_ = 0;
        }
    }
    
    // Recent projects management
    void loadRecentProjects(const std::string& configPath) {
        std::ifstream file(configPath);
        if (!file.is_open()) return;
        
        recentProjects.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && std::filesystem::exists(line)) {
                recentProjects.push_back(line);
            }
        }
    }
    
    void saveRecentProjects(const std::string& configPath) {
        std::ofstream file(configPath);
        if (!file.is_open()) return;
        
        for (const auto& path : recentProjects) {
            file << path << "\n";
        }
    }
    
    void addToRecent(const std::string& path) {
        // Remove if already exists
        recentProjects.erase(
            std::remove(recentProjects.begin(), recentProjects.end(), path),
            recentProjects.end());
        
        // Add to front
        recentProjects.insert(recentProjects.begin(), path);
        
        // Trim to max
        if (recentProjects.size() > MAX_RECENT_PROJECTS) {
            recentProjects.resize(MAX_RECENT_PROJECTS);
        }
    }
    
    void clearRecentProjects() {
        recentProjects.clear();
    }
    
private:
    ProjectManager() = default;
    
    bool autoSaveEnabled_ = false;
    float autoSaveTimer_ = 0;
    int autoSaveInterval_ = 60;
    
    std::string getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline ProjectManager& getProjectManager() {
    return ProjectManager::getInstance();
}

}  // namespace luma
