// Avatar Serializer - Save and load avatar data to/from JSON
// Part of LUMA Photo-to-Avatar System
#pragma once

#include "engine/character/character_face.h"
#include "engine/serialization/json.h"
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace luma {

// ============================================================================
// Avatar Data Structure
// ============================================================================

struct AvatarData {
    // Metadata
    std::string version = "1.0";
    std::string name = "Unnamed";
    std::string createdAt;
    std::string modifiedAt;
    
    // Source info
    struct {
        std::string type = "manual";  // "manual", "photo", "preset"
        std::vector<std::string> photoFiles;
        float confidence = 0.0f;
    } source;
    
    // Face shape parameters
    FaceShapeParams identity;
    
    // Appearance
    struct {
        Vec3 skinTone{0.85f, 0.65f, 0.5f};
        Vec3 hairColor{0.2f, 0.15f, 0.1f};
        Vec3 eyeColor{0.4f, 0.3f, 0.2f};
    } appearance;
    
    // Mesh info
    struct {
        std::string baseMesh = "head_base.bin";
        int vertexCount = 0;
        int blendShapeCount = 0;
    } mesh;
};

// ============================================================================
// Avatar Serializer
// ============================================================================

class AvatarSerializer {
public:
    // Get current timestamp in ISO format
    static std::string getCurrentTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm);
        return std::string(buf);
    }
    
    // Save avatar to JSON file
    static bool save(const std::string& filepath, const AvatarData& avatar) {
        JsonValue root;
        
        // Metadata
        root["version"] = avatar.version;
        root["name"] = avatar.name;
        root["createdAt"] = avatar.createdAt.empty() ? getCurrentTimestamp() : avatar.createdAt;
        root["modifiedAt"] = getCurrentTimestamp();
        
        // Source
        JsonValue source;
        source["type"] = avatar.source.type;
        if (!avatar.source.photoFiles.empty()) {
            JsonValue photos;
            for (const auto& f : avatar.source.photoFiles) {
                photos.pushBack(f);
            }
            source["photos"] = photos;
        }
        source["confidence"] = avatar.source.confidence;
        root["source"] = source;
        
        // Identity (face shape params)
        JsonValue identity;
        auto params = const_cast<FaceShapeParams&>(avatar.identity).getAllParams();
        for (const auto& [name, ptr] : params) {
            identity[name] = *ptr;
        }
        root["identity"] = identity;
        
        // Appearance
        JsonValue appearance;
        JsonValue skinTone;
        skinTone.pushBack(avatar.appearance.skinTone.x);
        skinTone.pushBack(avatar.appearance.skinTone.y);
        skinTone.pushBack(avatar.appearance.skinTone.z);
        appearance["skinTone"] = skinTone;
        
        JsonValue hairColor;
        hairColor.pushBack(avatar.appearance.hairColor.x);
        hairColor.pushBack(avatar.appearance.hairColor.y);
        hairColor.pushBack(avatar.appearance.hairColor.z);
        appearance["hairColor"] = hairColor;
        
        JsonValue eyeColor;
        eyeColor.pushBack(avatar.appearance.eyeColor.x);
        eyeColor.pushBack(avatar.appearance.eyeColor.y);
        eyeColor.pushBack(avatar.appearance.eyeColor.z);
        appearance["eyeColor"] = eyeColor;
        
        root["appearance"] = appearance;
        
        // Mesh info
        JsonValue mesh;
        mesh["baseMesh"] = avatar.mesh.baseMesh;
        mesh["vertexCount"] = avatar.mesh.vertexCount;
        mesh["blendShapeCount"] = avatar.mesh.blendShapeCount;
        root["mesh"] = mesh;
        
        // Write to file
        std::string jsonStr = JsonWriter::write(root, true);
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            printf("[AvatarSerializer] Failed to open file for writing: %s\n", filepath.c_str());
            return false;
        }
        
        file << jsonStr;
        file.close();
        
        printf("[AvatarSerializer] Saved avatar to: %s\n", filepath.c_str());
        return true;
    }
    
    // Load avatar from JSON file
    static bool load(const std::string& filepath, AvatarData& avatar) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            printf("[AvatarSerializer] Failed to open file: %s\n", filepath.c_str());
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();
        
        JsonValue root;
        if (!JsonParser::parse(jsonStr, root)) {
            printf("[AvatarSerializer] Failed to parse JSON\n");
            return false;
        }
        
        // Metadata
        if (root.hasKey("version")) avatar.version = root["version"].asString();
        if (root.hasKey("name")) avatar.name = root["name"].asString();
        if (root.hasKey("createdAt")) avatar.createdAt = root["createdAt"].asString();
        if (root.hasKey("modifiedAt")) avatar.modifiedAt = root["modifiedAt"].asString();
        
        // Source
        if (root.hasKey("source")) {
            const auto& source = root["source"];
            if (source.hasKey("type")) avatar.source.type = source["type"].asString();
            if (source.hasKey("confidence")) avatar.source.confidence = source["confidence"].asFloat();
            if (source.hasKey("photos") && source["photos"].isArray()) {
                avatar.source.photoFiles.clear();
                for (const auto& p : source["photos"].asArray()) {
                    avatar.source.photoFiles.push_back(p.asString());
                }
            }
        }
        
        // Identity
        if (root.hasKey("identity")) {
            const auto& identity = root["identity"];
            auto params = avatar.identity.getAllParams();
            for (auto& [name, ptr] : params) {
                if (identity.hasKey(name)) {
                    *ptr = identity[name].asFloat();
                }
            }
        }
        
        // Appearance
        if (root.hasKey("appearance")) {
            const auto& appearance = root["appearance"];
            
            if (appearance.hasKey("skinTone") && appearance["skinTone"].isArray()) {
                const auto& arr = appearance["skinTone"].asArray();
                if (arr.size() >= 3) {
                    avatar.appearance.skinTone.x = arr[0].asFloat();
                    avatar.appearance.skinTone.y = arr[1].asFloat();
                    avatar.appearance.skinTone.z = arr[2].asFloat();
                }
            }
            
            if (appearance.hasKey("hairColor") && appearance["hairColor"].isArray()) {
                const auto& arr = appearance["hairColor"].asArray();
                if (arr.size() >= 3) {
                    avatar.appearance.hairColor.x = arr[0].asFloat();
                    avatar.appearance.hairColor.y = arr[1].asFloat();
                    avatar.appearance.hairColor.z = arr[2].asFloat();
                }
            }
            
            if (appearance.hasKey("eyeColor") && appearance["eyeColor"].isArray()) {
                const auto& arr = appearance["eyeColor"].asArray();
                if (arr.size() >= 3) {
                    avatar.appearance.eyeColor.x = arr[0].asFloat();
                    avatar.appearance.eyeColor.y = arr[1].asFloat();
                    avatar.appearance.eyeColor.z = arr[2].asFloat();
                }
            }
        }
        
        // Mesh
        if (root.hasKey("mesh")) {
            const auto& mesh = root["mesh"];
            if (mesh.hasKey("baseMesh")) avatar.mesh.baseMesh = mesh["baseMesh"].asString();
            if (mesh.hasKey("vertexCount")) avatar.mesh.vertexCount = mesh["vertexCount"].asInt();
            if (mesh.hasKey("blendShapeCount")) avatar.mesh.blendShapeCount = mesh["blendShapeCount"].asInt();
        }
        
        printf("[AvatarSerializer] Loaded avatar from: %s\n", filepath.c_str());
        return true;
    }
    
    // Create avatar data from CharacterFace
    static AvatarData fromCharacterFace(const CharacterFace& face, 
                                         const std::string& name = "Unnamed") {
        AvatarData avatar;
        avatar.name = name;
        avatar.createdAt = getCurrentTimestamp();
        avatar.identity = face.getShapeParams();
        
        const auto& tex = face.getTextureParams();
        avatar.appearance.skinTone = tex.skinTone;
        avatar.appearance.eyeColor = tex.eyeColor;
        avatar.appearance.hairColor = tex.eyebrowColor;
        
        return avatar;
    }
    
    // Apply avatar data to CharacterFace
    static void applyToCharacterFace(const AvatarData& avatar, CharacterFace& face) {
        face.setShapeParams(avatar.identity);
        
        FaceTextureParams tex = face.getTextureParams();
        tex.skinTone = avatar.appearance.skinTone;
        tex.eyeColor = avatar.appearance.eyeColor;
        tex.eyebrowColor = avatar.appearance.hairColor;
        face.setTextureParams(tex);
    }
};

} // namespace luma
