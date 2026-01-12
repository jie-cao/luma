// Universal model loader using Assimp
// Supports: FBX, OBJ, glTF, DAE, 3DS, and 40+ formats
#pragma once

#include <string>
#include <vector>
#include <optional>

#include "engine/renderer/mesh.h"

namespace luma {

// Loaded model data
struct Model {
    std::vector<Mesh> meshes;
    std::string name;
    
    // Bounding box
    float minBounds[3] = {0, 0, 0};
    float maxBounds[3] = {0, 0, 0};
    
    // Statistics
    size_t totalVertices = 0;
    size_t totalTriangles = 0;
};

// Load a 3D model file (FBX, OBJ, glTF, DAE, etc.)
std::optional<Model> load_model(const std::string& path);

// Get supported file extensions
std::vector<std::string> get_supported_extensions();

// Get file filter string for Windows file dialog
const char* get_file_filter();

}  // namespace luma
