// Model loader implementation using Assimp + stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "model_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <filesystem>

namespace luma {

namespace {

// Directory of the model file (for resolving relative texture paths)
std::string g_modelDir;

// Load texture from file
TextureData load_texture(const std::string& path) {
    TextureData tex;
    tex.path = path;
    
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);  // Force RGBA
    
    if (data) {
        tex.width = w;
        tex.height = h;
        tex.channels = 4;
        tex.pixels.assign(data, data + w * h * 4);
        stbi_image_free(data);
        std::cout << "[texture] Loaded: " << path << " (" << w << "x" << h << ")" << std::endl;
    } else {
        std::cerr << "[texture] Failed to load: " << path << std::endl;
    }
    
    return tex;
}

// Try to find texture file
std::string find_texture_file(const std::string& texPath) {
    // Try as-is
    if (std::filesystem::exists(texPath)) {
        return texPath;
    }
    
    // Try relative to model directory
    std::string relPath = g_modelDir + "/" + texPath;
    if (std::filesystem::exists(relPath)) {
        return relPath;
    }
    
    // Try just the filename in model directory
    std::filesystem::path p(texPath);
    std::string filename = p.filename().string();
    std::string inModelDir = g_modelDir + "/" + filename;
    if (std::filesystem::exists(inModelDir)) {
        return inModelDir;
    }
    
    // Try common texture subdirectories
    std::vector<std::string> subdirs = {"textures", "Textures", "tex", "maps", "Materials"};
    for (const auto& subdir : subdirs) {
        std::string subPath = g_modelDir + "/" + subdir + "/" + filename;
        if (std::filesystem::exists(subPath)) {
            return subPath;
        }
    }
    
    return "";  // Not found
}

// Get texture from material
TextureData get_material_texture(const aiMaterial* mat, aiTextureType type) {
    TextureData tex;
    
    if (mat->GetTextureCount(type) > 0) {
        aiString texPath;
        if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
            std::string path = find_texture_file(texPath.C_Str());
            if (!path.empty()) {
                tex = load_texture(path);
            } else {
                std::cerr << "[texture] Not found: " << texPath.C_Str() << std::endl;
            }
        }
    }
    
    return tex;
}

// Process a single mesh
Mesh process_mesh(const aiMesh* aiMesh, const aiScene* scene, Model& model) {
    Mesh mesh;
    mesh.vertices.reserve(aiMesh->mNumVertices);
    mesh.indices.reserve(aiMesh->mNumFaces * 3);

    // Get material
    float matColor[3] = {0.8f, 0.8f, 0.8f};
    if (aiMesh->mMaterialIndex < scene->mNumMaterials) {
        const aiMaterial* mat = scene->mMaterials[aiMesh->mMaterialIndex];
        
        // Get diffuse color
        aiColor3D diffuse(0.8f, 0.8f, 0.8f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            matColor[0] = diffuse.r;
            matColor[1] = diffuse.g;
            matColor[2] = diffuse.b;
        }
        
        // Try to get diffuse texture
        TextureData diffuseTex = get_material_texture(mat, aiTextureType_DIFFUSE);
        if (!diffuseTex.pixels.empty()) {
            mesh.diffuseTexture = std::move(diffuseTex);
            mesh.hasDiffuseTexture = true;
        } else {
            // Try base color (PBR)
            diffuseTex = get_material_texture(mat, aiTextureType_BASE_COLOR);
            if (!diffuseTex.pixels.empty()) {
                mesh.diffuseTexture = std::move(diffuseTex);
                mesh.hasDiffuseTexture = true;
            }
        }
    }

    // Process vertices
    for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
        Vertex v{};

        // Position
        v.position[0] = aiMesh->mVertices[i].x;
        v.position[1] = aiMesh->mVertices[i].y;
        v.position[2] = aiMesh->mVertices[i].z;

        // Bounds
        model.minBounds[0] = std::min(model.minBounds[0], v.position[0]);
        model.minBounds[1] = std::min(model.minBounds[1], v.position[1]);
        model.minBounds[2] = std::min(model.minBounds[2], v.position[2]);
        model.maxBounds[0] = std::max(model.maxBounds[0], v.position[0]);
        model.maxBounds[1] = std::max(model.maxBounds[1], v.position[1]);
        model.maxBounds[2] = std::max(model.maxBounds[2], v.position[2]);

        // Normal
        if (aiMesh->HasNormals()) {
            v.normal[0] = aiMesh->mNormals[i].x;
            v.normal[1] = aiMesh->mNormals[i].y;
            v.normal[2] = aiMesh->mNormals[i].z;
        } else {
            v.normal[0] = 0.0f;
            v.normal[1] = 1.0f;
            v.normal[2] = 0.0f;
        }

        // UV coordinates (first set)
        if (aiMesh->HasTextureCoords(0)) {
            v.uv[0] = aiMesh->mTextureCoords[0][i].x;
            v.uv[1] = aiMesh->mTextureCoords[0][i].y;
        } else {
            v.uv[0] = 0.0f;
            v.uv[1] = 0.0f;
        }

        // Color
        if (aiMesh->HasVertexColors(0)) {
            v.color[0] = aiMesh->mColors[0][i].r;
            v.color[1] = aiMesh->mColors[0][i].g;
            v.color[2] = aiMesh->mColors[0][i].b;
        } else {
            v.color[0] = matColor[0];
            v.color[1] = matColor[1];
            v.color[2] = matColor[2];
        }

        mesh.vertices.push_back(v);
    }

    // Process indices
    for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
        const aiFace& face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            mesh.indices.push_back(face.mIndices[j]);
        }
    }

    model.totalVertices += mesh.vertices.size();
    model.totalTriangles += aiMesh->mNumFaces;

    return mesh;
}

// Recursively process nodes
void process_node(const aiNode* node, const aiScene* scene, Model& model) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(process_mesh(mesh, scene, model));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], scene, model);
    }
}

}  // namespace

std::optional<Model> load_model(const std::string& path) {
    // Store model directory for texture loading
    std::filesystem::path fsPath(path);
    g_modelDir = fsPath.parent_path().string();
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "[model] Assimp error: " << importer.GetErrorString() << std::endl;
        return std::nullopt;
    }

    std::cout << "[model] Loading: " << path << std::endl;
    std::cout << "[model] Meshes: " << scene->mNumMeshes << ", Materials: " << scene->mNumMaterials << std::endl;

    Model model;
    model.name = path;
    model.minBounds[0] = model.minBounds[1] = model.minBounds[2] = std::numeric_limits<float>::max();
    model.maxBounds[0] = model.maxBounds[1] = model.maxBounds[2] = std::numeric_limits<float>::lowest();

    process_node(scene->mRootNode, scene, model);

    if (model.meshes.empty()) {
        std::cerr << "[model] No valid meshes found" << std::endl;
        return std::nullopt;
    }

    // Extract filename
    model.name = fsPath.filename().string();

    // Count textures
    int texCount = 0;
    for (const auto& m : model.meshes) {
        if (m.hasDiffuseTexture) texCount++;
    }

    std::cout << "[model] Loaded: " << model.name << std::endl;
    std::cout << "[model] Meshes: " << model.meshes.size() << ", Textures: " << texCount << std::endl;
    std::cout << "[model] Vertices: " << model.totalVertices << ", Triangles: " << model.totalTriangles << std::endl;

    return model;
}

std::vector<std::string> get_supported_extensions() {
    return {".fbx", ".FBX", ".obj", ".OBJ", ".gltf", ".glb", ".GLTF", ".GLB", ".dae", ".DAE", ".3ds", ".3DS"};
}

const char* get_file_filter() {
    return "3D Models (*.fbx;*.obj;*.gltf;*.glb;*.dae)\0*.fbx;*.FBX;*.obj;*.OBJ;*.gltf;*.glb;*.GLTF;*.GLB;*.dae;*.DAE;*.3ds;*.3DS\0"
           "FBX (*.fbx)\0*.fbx;*.FBX\0"
           "OBJ (*.obj)\0*.obj;*.OBJ\0"
           "glTF (*.gltf;*.glb)\0*.gltf;*.glb;*.GLTF;*.GLB\0"
           "All Files (*.*)\0*.*\0";
}

}  // namespace luma
