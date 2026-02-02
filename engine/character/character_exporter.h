// Character Exporter - Export characters to various 3D formats
// Part of LUMA Character Creation System
#pragma once

#include "character.h"
#include "blend_shape.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace luma {

// ============================================================================
// Export Format
// ============================================================================

// CharacterExportFormat is defined in character.h

// ============================================================================
// Export Options
// ============================================================================

struct CharacterExportOptions {
    bool includeSkeleton = true;
    bool includeBlendShapes = true;
    bool includeTextures = true;
    bool applyCurrentPose = false;
    
    // Transform
    float scale = 1.0f;
    bool flipYZ = false;  // Convert Y-up to Z-up
    
    // Quality
    int textureSize = 1024;
    bool compressTextures = false;
};

// ============================================================================
// OBJ Exporter
// ============================================================================

class OBJExporter {
public:
    static bool exportCharacter(
        const Character& character,
        const std::string& outputPath,
        const CharacterExportOptions& options = CharacterExportOptions())
    {
        // Get deformed vertices
        std::vector<Vertex> vertices;
        character.getDeformedVertices(vertices);
        const auto& indices = character.getIndices();
        
        if (vertices.empty() || indices.empty()) {
            return false;
        }
        
        // Open OBJ file
        std::ofstream objFile(outputPath);
        if (!objFile.is_open()) {
            return false;
        }
        
        // Write header
        objFile << "# LUMA Character Export\n";
        objFile << "# Character: " << character.getName() << "\n";
        objFile << "# Vertices: " << vertices.size() << "\n";
        objFile << "# Triangles: " << indices.size() / 3 << "\n\n";
        
        // Write material library reference
        std::string mtlPath = outputPath;
        size_t dotPos = mtlPath.rfind('.');
        if (dotPos != std::string::npos) {
            mtlPath = mtlPath.substr(0, dotPos) + ".mtl";
        }
        
        // Extract just filename for mtllib
        size_t slashPos = mtlPath.rfind('/');
        std::string mtlFilename = (slashPos != std::string::npos) ? 
            mtlPath.substr(slashPos + 1) : mtlPath;
        
        objFile << "mtllib " << mtlFilename << "\n\n";
        
        // Write vertices
        objFile << "# Vertices\n";
        for (const auto& v : vertices) {
            float x = v.position[0] * options.scale;
            float y = v.position[1] * options.scale;
            float z = v.position[2] * options.scale;
            
            if (options.flipYZ) {
                std::swap(y, z);
                z = -z;
            }
            
            objFile << "v " << std::fixed << std::setprecision(6)
                   << x << " " << y << " " << z << "\n";
        }
        objFile << "\n";
        
        // Write texture coordinates
        objFile << "# Texture Coordinates\n";
        for (const auto& v : vertices) {
            objFile << "vt " << std::fixed << std::setprecision(6)
                   << v.uv[0] << " " << (1.0f - v.uv[1]) << "\n";
        }
        objFile << "\n";
        
        // Write normals
        objFile << "# Normals\n";
        for (const auto& v : vertices) {
            float nx = v.normal[0];
            float ny = v.normal[1];
            float nz = v.normal[2];
            
            if (options.flipYZ) {
                std::swap(ny, nz);
                nz = -nz;
            }
            
            objFile << "vn " << std::fixed << std::setprecision(6)
                   << nx << " " << ny << " " << nz << "\n";
        }
        objFile << "\n";
        
        // Write faces
        objFile << "# Faces\n";
        objFile << "usemtl skin\n";
        for (size_t i = 0; i < indices.size(); i += 3) {
            // OBJ indices are 1-based
            uint32_t i0 = indices[i + 0] + 1;
            uint32_t i1 = indices[i + 1] + 1;
            uint32_t i2 = indices[i + 2] + 1;
            
            objFile << "f " << i0 << "/" << i0 << "/" << i0 << " "
                   << i1 << "/" << i1 << "/" << i1 << " "
                   << i2 << "/" << i2 << "/" << i2 << "\n";
        }
        
        objFile.close();
        
        // Write MTL file
        std::ofstream mtlFile(mtlPath);
        if (mtlFile.is_open()) {
            const auto& skinColor = character.getBody().getParams().skinColor;
            
            mtlFile << "# LUMA Character Material\n\n";
            mtlFile << "newmtl skin\n";
            mtlFile << "Kd " << skinColor.x << " " << skinColor.y << " " << skinColor.z << "\n";
            mtlFile << "Ka 0.1 0.1 0.1\n";
            mtlFile << "Ks 0.2 0.2 0.2\n";
            mtlFile << "Ns 20\n";
            mtlFile << "d 1.0\n";
            
            mtlFile.close();
        }
        
        return true;
    }
};

// ============================================================================
// glTF Exporter (Basic Implementation)
// ============================================================================

class GLTFExporter {
public:
    static bool exportCharacter(
        const Character& character,
        const std::string& outputPath,
        const CharacterExportOptions& options = CharacterExportOptions())
    {
        // Get mesh data
        std::vector<Vertex> vertices;
        character.getDeformedVertices(vertices);
        const auto& indices = character.getIndices();
        
        if (vertices.empty() || indices.empty()) {
            return false;
        }
        
        // Build binary buffer
        std::vector<uint8_t> buffer;
        
        // Add positions
        size_t positionOffset = buffer.size();
        for (const auto& v : vertices) {
            appendFloat(buffer, v.position[0] * options.scale);
            appendFloat(buffer, v.position[1] * options.scale);
            appendFloat(buffer, v.position[2] * options.scale);
        }
        size_t positionLength = buffer.size() - positionOffset;
        
        // Add normals
        size_t normalOffset = buffer.size();
        for (const auto& v : vertices) {
            appendFloat(buffer, v.normal[0]);
            appendFloat(buffer, v.normal[1]);
            appendFloat(buffer, v.normal[2]);
        }
        size_t normalLength = buffer.size() - normalOffset;
        
        // Add texcoords
        size_t texcoordOffset = buffer.size();
        for (const auto& v : vertices) {
            appendFloat(buffer, v.uv[0]);
            appendFloat(buffer, 1.0f - v.uv[1]);  // Flip V
        }
        size_t texcoordLength = buffer.size() - texcoordOffset;
        
        // Add indices
        size_t indexOffset = buffer.size();
        for (uint32_t idx : indices) {
            appendUint32(buffer, idx);
        }
        size_t indexLength = buffer.size() - indexOffset;
        
        // Calculate bounds
        float minPos[3] = {1e10f, 1e10f, 1e10f};
        float maxPos[3] = {-1e10f, -1e10f, -1e10f};
        for (const auto& v : vertices) {
            for (int i = 0; i < 3; i++) {
                float p = v.position[i] * options.scale;
                minPos[i] = std::min(minPos[i], p);
                maxPos[i] = std::max(maxPos[i], p);
            }
        }
        
        // Build JSON
        std::stringstream json;
        json << std::fixed << std::setprecision(6);
        
        json << "{\n";
        json << "  \"asset\": {\n";
        json << "    \"generator\": \"LUMA Character Exporter\",\n";
        json << "    \"version\": \"2.0\"\n";
        json << "  },\n";
        
        // Scene
        json << "  \"scene\": 0,\n";
        json << "  \"scenes\": [{\"nodes\": [0]}],\n";
        
        // Nodes
        json << "  \"nodes\": [{\"mesh\": 0, \"name\": \"" << character.getName() << "\"}],\n";
        
        // Meshes
        json << "  \"meshes\": [{\n";
        json << "    \"name\": \"CharacterMesh\",\n";
        json << "    \"primitives\": [{\n";
        json << "      \"attributes\": {\n";
        json << "        \"POSITION\": 0,\n";
        json << "        \"NORMAL\": 1,\n";
        json << "        \"TEXCOORD_0\": 2\n";
        json << "      },\n";
        json << "      \"indices\": 3,\n";
        json << "      \"material\": 0\n";
        json << "    }]\n";
        json << "  }],\n";
        
        // Materials
        const auto& skinColor = character.getBody().getParams().skinColor;
        json << "  \"materials\": [{\n";
        json << "    \"name\": \"Skin\",\n";
        json << "    \"pbrMetallicRoughness\": {\n";
        json << "      \"baseColorFactor\": [" 
             << skinColor.x << ", " << skinColor.y << ", " << skinColor.z << ", 1.0],\n";
        json << "      \"metallicFactor\": 0.0,\n";
        json << "      \"roughnessFactor\": 0.6\n";
        json << "    }\n";
        json << "  }],\n";
        
        // Accessors
        json << "  \"accessors\": [\n";
        // Position accessor
        json << "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": " << vertices.size()
             << ", \"type\": \"VEC3\", \"min\": [" 
             << minPos[0] << ", " << minPos[1] << ", " << minPos[2] << "], \"max\": ["
             << maxPos[0] << ", " << maxPos[1] << ", " << maxPos[2] << "]},\n";
        // Normal accessor
        json << "    {\"bufferView\": 1, \"componentType\": 5126, \"count\": " << vertices.size()
             << ", \"type\": \"VEC3\"},\n";
        // Texcoord accessor
        json << "    {\"bufferView\": 2, \"componentType\": 5126, \"count\": " << vertices.size()
             << ", \"type\": \"VEC2\"},\n";
        // Index accessor
        json << "    {\"bufferView\": 3, \"componentType\": 5125, \"count\": " << indices.size()
             << ", \"type\": \"SCALAR\"}\n";
        json << "  ],\n";
        
        // Buffer views
        json << "  \"bufferViews\": [\n";
        json << "    {\"buffer\": 0, \"byteOffset\": " << positionOffset << ", \"byteLength\": " << positionLength << ", \"target\": 34962},\n";
        json << "    {\"buffer\": 0, \"byteOffset\": " << normalOffset << ", \"byteLength\": " << normalLength << ", \"target\": 34962},\n";
        json << "    {\"buffer\": 0, \"byteOffset\": " << texcoordOffset << ", \"byteLength\": " << texcoordLength << ", \"target\": 34962},\n";
        json << "    {\"buffer\": 0, \"byteOffset\": " << indexOffset << ", \"byteLength\": " << indexLength << ", \"target\": 34963}\n";
        json << "  ],\n";
        
        // Buffers
        json << "  \"buffers\": [{\"byteLength\": " << buffer.size() << "}]\n";
        json << "}\n";
        
        std::string jsonStr = json.str();
        
        // Pad JSON to 4-byte alignment
        while (jsonStr.size() % 4 != 0) {
            jsonStr += ' ';
        }
        
        // Pad buffer to 4-byte alignment
        while (buffer.size() % 4 != 0) {
            buffer.push_back(0);
        }
        
        // Write GLB file
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // GLB header
        uint32_t magic = 0x46546C67;  // "glTF"
        uint32_t version = 2;
        uint32_t length = 12 + 8 + jsonStr.size() + 8 + buffer.size();
        
        file.write(reinterpret_cast<const char*>(&magic), 4);
        file.write(reinterpret_cast<const char*>(&version), 4);
        file.write(reinterpret_cast<const char*>(&length), 4);
        
        // JSON chunk
        uint32_t jsonLength = static_cast<uint32_t>(jsonStr.size());
        uint32_t jsonType = 0x4E4F534A;  // "JSON"
        file.write(reinterpret_cast<const char*>(&jsonLength), 4);
        file.write(reinterpret_cast<const char*>(&jsonType), 4);
        file.write(jsonStr.c_str(), jsonStr.size());
        
        // Binary chunk
        uint32_t binLength = static_cast<uint32_t>(buffer.size());
        uint32_t binType = 0x004E4942;  // "BIN\0"
        file.write(reinterpret_cast<const char*>(&binLength), 4);
        file.write(reinterpret_cast<const char*>(&binType), 4);
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        
        file.close();
        return true;
    }
    
private:
    static void appendFloat(std::vector<uint8_t>& buffer, float value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + 4);
    }
    
    static void appendUint32(std::vector<uint8_t>& buffer, uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + 4);
    }
};

// ============================================================================
// Character Exporter - Unified Interface
// ============================================================================

class CharacterExporter {
public:
    static bool exportCharacter(
        const Character& character,
        const std::string& outputPath,
        CharacterExportFormat format,
        const CharacterExportOptions& options = CharacterExportOptions())
    {
        switch (format) {
            case CharacterExportFormat::OBJ:
                return OBJExporter::exportCharacter(character, outputPath, options);
                
            case CharacterExportFormat::GLTF:
                return GLTFExporter::exportCharacter(character, outputPath, options);
                
            case CharacterExportFormat::FBX:
                // FBX requires external library (Autodesk FBX SDK)
                return false;
        }
        return false;
    }
    
    static std::string getExtension(CharacterExportFormat format) {
        switch (format) {
            case CharacterExportFormat::OBJ: return ".obj";
            case CharacterExportFormat::GLTF: return ".glb";
            case CharacterExportFormat::FBX: return ".fbx";
        }
        return "";
    }
};

} // namespace luma
