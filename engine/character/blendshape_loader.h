// BlendShape Loader - Load pre-sculpted BlendShapes from Blender export
// Part of LUMA Character Creation System (MetaHuman-style workflow)
#pragma once

#include "engine/character/blend_shape.h"
#include <string>
#include <cstdio>
#include <cstdint>

namespace luma {

// ============================================================================
// BlendShape File Format (.blendshapes)
// ============================================================================
// 
// Header:
//     uint32: magic (0x42534850 = "BSHP")
//     uint32: version (1)
//     uint32: vertex_count
//     uint32: shape_count
// 
// For each shape:
//     uint32: name_length
//     char[name_length]: name (UTF-8)
//     float: min_weight
//     float: max_weight
//     float: default_weight
//     uint32: delta_count
//     For each delta:
//         uint32: vertex_index
//         float[3]: position_delta
//         float[3]: normal_delta
//
// ============================================================================

class BlendShapeLoader {
public:
    static constexpr uint32_t MAGIC = 0x42534850;  // "BSHP"
    static constexpr uint32_t VERSION = 1;
    
    struct LoadResult {
        bool success = false;
        uint32_t vertexCount = 0;
        uint32_t shapeCount = 0;
        std::string errorMessage;
    };
    
    // Load BlendShapes from binary file into a BlendShapeMesh
    static LoadResult load(const std::string& path, BlendShapeMesh& mesh) {
        LoadResult result;
        
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp) {
            result.errorMessage = "Failed to open file: " + path;
            printf("[BlendShapeLoader] %s\n", result.errorMessage.c_str());
            return result;
        }
        
        printf("[BlendShapeLoader] Loading from: %s\n", path.c_str());
        
        // Read header
        uint32_t magic, version, vertexCount, shapeCount;
        fread(&magic, sizeof(uint32_t), 1, fp);
        fread(&version, sizeof(uint32_t), 1, fp);
        fread(&vertexCount, sizeof(uint32_t), 1, fp);
        fread(&shapeCount, sizeof(uint32_t), 1, fp);
        
        // Validate header
        if (magic != MAGIC) {
            result.errorMessage = "Invalid magic number (not a .blendshapes file)";
            printf("[BlendShapeLoader] %s\n", result.errorMessage.c_str());
            fclose(fp);
            return result;
        }
        
        if (version != VERSION) {
            result.errorMessage = "Unsupported version: " + std::to_string(version);
            printf("[BlendShapeLoader] %s\n", result.errorMessage.c_str());
            fclose(fp);
            return result;
        }
        
        printf("[BlendShapeLoader] Header: %u vertices, %u shapes\n", vertexCount, shapeCount);
        
        result.vertexCount = vertexCount;
        result.shapeCount = shapeCount;
        
        // Read each shape
        for (uint32_t s = 0; s < shapeCount; s++) {
            // Read name
            uint32_t nameLength;
            fread(&nameLength, sizeof(uint32_t), 1, fp);
            
            std::string name(nameLength, '\0');
            fread(&name[0], 1, nameLength, fp);
            
            // Read weight range
            float minWeight, maxWeight, defaultWeight;
            fread(&minWeight, sizeof(float), 1, fp);
            fread(&maxWeight, sizeof(float), 1, fp);
            fread(&defaultWeight, sizeof(float), 1, fp);
            
            // Read delta count
            uint32_t deltaCount;
            fread(&deltaCount, sizeof(uint32_t), 1, fp);
            
            // Create BlendShape target
            BlendShapeTarget target(name);
            
            // Read deltas
            for (uint32_t d = 0; d < deltaCount; d++) {
                uint32_t vertexIndex;
                float positionDelta[3];
                float normalDelta[3];
                
                fread(&vertexIndex, sizeof(uint32_t), 1, fp);
                fread(positionDelta, sizeof(float), 3, fp);
                fread(normalDelta, sizeof(float), 3, fp);
                
                BlendShapeDelta delta;
                delta.vertexIndex = vertexIndex;
                delta.positionDelta = Vec3(positionDelta[0], positionDelta[1], positionDelta[2]);
                delta.normalDelta = Vec3(normalDelta[0], normalDelta[1], normalDelta[2]);
                
                target.addDelta(delta);
            }
            
            // Add target and create channel
            int targetIdx = mesh.addTarget(target);
            int channelIdx = mesh.createChannel(name, targetIdx);
            
            // Set channel properties
            if (auto* ch = mesh.getChannel(channelIdx)) {
                ch->minWeight = minWeight;
                ch->maxWeight = maxWeight;
                ch->defaultWeight = defaultWeight;
                ch->weight = defaultWeight;
                
                // Set category based on name prefix
                if (name.find("id_face") == 0) ch->group = "Face";
                else if (name.find("id_forehead") == 0) ch->group = "Forehead";
                else if (name.find("id_eye") == 0) ch->group = "Eyes";
                else if (name.find("id_brow") == 0) ch->group = "Eyebrows";
                else if (name.find("id_nose") == 0) ch->group = "Nose";
                else if (name.find("id_mouth") == 0 || name.find("id_lip") == 0 || name.find("id_philtrum") == 0) ch->group = "Mouth";
                else if (name.find("id_chin") == 0) ch->group = "Chin";
                else if (name.find("id_jaw") == 0) ch->group = "Jaw";
                else if (name.find("id_cheek") == 0 || name.find("id_nasolabial") == 0) ch->group = "Cheeks";
                else if (name.find("id_ear") == 0) ch->group = "Ears";
                else if (name.find("id_neck") == 0) ch->group = "Neck";
                else if (name.find("expr_") == 0) ch->group = "Expression";
                else ch->group = "Other";
            }
            
            printf("[BlendShapeLoader]   + %s: %u deltas\n", name.c_str(), deltaCount);
        }
        
        fclose(fp);
        
        result.success = true;
        printf("[BlendShapeLoader] Loaded %u shapes successfully\n", shapeCount);
        
        return result;
    }
    
    // Check if a blendshapes file exists
    static bool exists(const std::string& path) {
        FILE* fp = fopen(path.c_str(), "rb");
        if (fp) {
            fclose(fp);
            return true;
        }
        return false;
    }
    
    // Get info about a blendshapes file without fully loading it
    static LoadResult getInfo(const std::string& path) {
        LoadResult result;
        
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp) {
            result.errorMessage = "Failed to open file";
            return result;
        }
        
        uint32_t magic, version, vertexCount, shapeCount;
        fread(&magic, sizeof(uint32_t), 1, fp);
        fread(&version, sizeof(uint32_t), 1, fp);
        fread(&vertexCount, sizeof(uint32_t), 1, fp);
        fread(&shapeCount, sizeof(uint32_t), 1, fp);
        
        fclose(fp);
        
        if (magic != MAGIC) {
            result.errorMessage = "Invalid magic number";
            return result;
        }
        
        result.success = true;
        result.vertexCount = vertexCount;
        result.shapeCount = shapeCount;
        
        return result;
    }
};

} // namespace luma
