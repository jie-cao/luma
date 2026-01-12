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
    stbi_set_flip_vertically_on_load(false);  // Don't flip for DirectX (origin is top-left)
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

// Load embedded texture by index
TextureData load_embedded_texture(const aiTexture* aiTex) {
    TextureData tex;
    if (!aiTex) return tex;
    
    if (aiTex->mHeight == 0) {
        // Compressed format (PNG, JPG, etc.) - mWidth is the size in bytes
        int w, h, channels;
        stbi_set_flip_vertically_on_load(false);  // Don't flip for DirectX
        unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(aiTex->pcData),
            aiTex->mWidth, &w, &h, &channels, 4);
        if (data) {
            tex.width = w;
            tex.height = h;
            tex.channels = 4;
            tex.pixels.assign(data, data + w * h * 4);
            tex.path = aiTex->mFilename.C_Str();
            stbi_image_free(data);
            std::cout << "[texture] Loaded embedded: " << w << "x" << h << " (" << tex.path << ")" << std::endl;
        }
    } else {
        // Uncompressed ARGB8888 format
        tex.width = aiTex->mWidth;
        tex.height = aiTex->mHeight;
        tex.channels = 4;
        tex.pixels.resize(tex.width * tex.height * 4);
        for (unsigned int i = 0; i < aiTex->mWidth * aiTex->mHeight; i++) {
            tex.pixels[i * 4 + 0] = aiTex->pcData[i].r;
            tex.pixels[i * 4 + 1] = aiTex->pcData[i].g;
            tex.pixels[i * 4 + 2] = aiTex->pcData[i].b;
            tex.pixels[i * 4 + 3] = aiTex->pcData[i].a;
        }
        tex.path = "[embedded raw]";
        std::cout << "[texture] Loaded embedded raw: " << tex.width << "x" << tex.height << std::endl;
    }
    return tex;
}

// Find embedded texture by matching filename
const aiTexture* find_embedded_texture(const aiScene* scene, const std::string& path) {
    if (!scene || scene->mNumTextures == 0) return nullptr;
    
    // Extract just the filename from the path
    std::filesystem::path p(path);
    std::string filename = p.filename().string();
    
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        const aiTexture* tex = scene->mTextures[i];
        if (!tex) continue;
        
        // Check if embedded texture filename matches
        std::string embeddedName = tex->mFilename.C_Str();
        std::filesystem::path embeddedPath(embeddedName);
        std::string embeddedFilename = embeddedPath.filename().string();
        
        if (embeddedFilename == filename) {
            std::cout << "[texture] Found embedded match: " << filename << " at index " << i << std::endl;
            return tex;
        }
    }
    
    // Also try matching by index if filename is empty but we have embedded textures
    // Return first available embedded texture as fallback
    return nullptr;
}

// Get texture from material (supports embedded and external textures)
TextureData get_material_texture(const aiMaterial* mat, aiTextureType type, const aiScene* scene) {
    TextureData tex;
    
    if (mat->GetTextureCount(type) > 0) {
        aiString texPath;
        if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
            std::string pathStr = texPath.C_Str();
            
            // Check if it's an embedded texture reference (starts with '*')
            if (pathStr.length() > 0 && pathStr[0] == '*') {
                int texIndex = std::atoi(pathStr.c_str() + 1);
                if (texIndex >= 0 && texIndex < (int)scene->mNumTextures) {
                    tex = load_embedded_texture(scene->mTextures[texIndex]);
                }
            } else {
                // Try to find external file first
                std::string externalPath = find_texture_file(pathStr);
                if (!externalPath.empty()) {
                    tex = load_texture(externalPath);
                } else {
                    // External file not found - try to find matching embedded texture
                    const aiTexture* embedded = find_embedded_texture(scene, pathStr);
                    if (embedded) {
                        tex = load_embedded_texture(embedded);
                    } else if (scene->mNumTextures > 0) {
                        // Last resort: try to use embedded texture by index based on material
                        // Some FBX files have embedded textures that don't match filenames
                        std::cerr << "[texture] External not found, trying embedded fallback for: " << pathStr << std::endl;
                    } else {
                        std::cerr << "[texture] Not found: " << pathStr << std::endl;
                    }
                }
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
        
        // Get material name
        aiString matName;
        if (mat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS) {
            mesh.materialName = matName.C_Str();
            std::cout << "[material] " << matName.C_Str() << std::endl;
        }
        
        // Get diffuse/base color
        aiColor3D diffuse(0.8f, 0.8f, 0.8f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            matColor[0] = diffuse.r;
            matColor[1] = diffuse.g;
            matColor[2] = diffuse.b;
            mesh.baseColor[0] = diffuse.r;
            mesh.baseColor[1] = diffuse.g;
            mesh.baseColor[2] = diffuse.b;
            std::cout << "  Diffuse color: (" << diffuse.r << ", " << diffuse.g << ", " << diffuse.b << ")" << std::endl;
        }
        
        // Get PBR parameters
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;
        
        // Try different property keys for metallic
        if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
            mesh.metallic = metallicFactor;
            std::cout << "  Metallic: " << metallicFactor << std::endl;
        } else {
            // Fallback: check reflectivity or shininess
            float shininess = 0.0f;
            if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                // Convert shininess to approximate metallic (higher shininess = more metallic-like)
                mesh.metallic = std::min(shininess / 100.0f, 1.0f);
            }
        }
        
        // Try different property keys for roughness
        if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
            mesh.roughness = roughnessFactor;
            std::cout << "  Roughness: " << roughnessFactor << std::endl;
        } else {
            // Fallback: invert shininess to get roughness
            float shininess = 0.0f;
            if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                mesh.roughness = 1.0f - std::min(shininess / 100.0f, 1.0f);
            } else {
                mesh.roughness = 0.5f;  // Default roughness
            }
        }
        
        std::cout << "  PBR: metallic=" << mesh.metallic << ", roughness=" << mesh.roughness << std::endl;
        
        // Debug: print all texture types present
        const aiTextureType texTypes[] = {
            aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR, aiTextureType_SPECULAR,
            aiTextureType_AMBIENT, aiTextureType_EMISSIVE, aiTextureType_NORMALS,
            aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_UNKNOWN
        };
        const char* texNames[] = {
            "DIFFUSE", "BASE_COLOR", "SPECULAR", "AMBIENT", "EMISSIVE", 
            "NORMALS", "METALNESS", "DIFFUSE_ROUGHNESS", "UNKNOWN"
        };
        for (int t = 0; t < 9; t++) {
            int count = mat->GetTextureCount(texTypes[t]);
            if (count > 0) {
                aiString path;
                mat->GetTexture(texTypes[t], 0, &path);
                std::cout << "  Texture " << texNames[t] << ": " << path.C_Str() << std::endl;
            }
        }
        
        // Try all possible texture types for diffuse/albedo
        TextureData diffuseTex;
        aiTextureType tryTypes[] = {
            aiTextureType_DIFFUSE,
            aiTextureType_BASE_COLOR,
            aiTextureType_AMBIENT,  // Some exporters put albedo here
            aiTextureType_UNKNOWN,  // Fallback
        };
        for (auto type : tryTypes) {
            if (diffuseTex.pixels.empty()) {
                diffuseTex = get_material_texture(mat, type, scene);
            }
        }
        
        if (!diffuseTex.pixels.empty()) {
            mesh.diffuseTexture = std::move(diffuseTex);
            mesh.hasDiffuseTexture = true;
        }
        
        // Load normal map
        TextureData normalTex = get_material_texture(mat, aiTextureType_NORMALS, scene);
        if (normalTex.pixels.empty()) {
            // Try height map as fallback
            normalTex = get_material_texture(mat, aiTextureType_HEIGHT, scene);
        }
        if (!normalTex.pixels.empty()) {
            mesh.normalTexture = std::move(normalTex);
            mesh.hasNormalTexture = true;
            std::cout << "  [loaded] Normal map" << std::endl;
        }
        
        // Load specular/metallic map
        TextureData specTex = get_material_texture(mat, aiTextureType_SPECULAR, scene);
        if (specTex.pixels.empty()) {
            specTex = get_material_texture(mat, aiTextureType_METALNESS, scene);
        }
        if (!specTex.pixels.empty()) {
            mesh.specularTexture = std::move(specTex);
            mesh.hasSpecularTexture = true;
            std::cout << "  [loaded] Specular/Metallic map" << std::endl;
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

        // Tangent (for normal mapping)
        if (aiMesh->HasTangentsAndBitangents()) {
            v.tangent[0] = aiMesh->mTangents[i].x;
            v.tangent[1] = aiMesh->mTangents[i].y;
            v.tangent[2] = aiMesh->mTangents[i].z;
            // Calculate handedness
            float bx = aiMesh->mBitangents[i].x;
            float by = aiMesh->mBitangents[i].y;
            float bz = aiMesh->mBitangents[i].z;
            float crossX = v.normal[1] * v.tangent[2] - v.normal[2] * v.tangent[1];
            float crossY = v.normal[2] * v.tangent[0] - v.normal[0] * v.tangent[2];
            float crossZ = v.normal[0] * v.tangent[1] - v.normal[1] * v.tangent[0];
            float dot = crossX * bx + crossY * by + crossZ * bz;
            v.tangent[3] = (dot < 0.0f) ? -1.0f : 1.0f;
        } else {
            v.tangent[0] = 1.0f;
            v.tangent[1] = 0.0f;
            v.tangent[2] = 0.0f;
            v.tangent[3] = 1.0f;
        }

        // UV coordinates (first set)
        // Flip V coordinate for DirectX (texture origin is top-left)
        if (aiMesh->HasTextureCoords(0)) {
            v.uv[0] = aiMesh->mTextureCoords[0][i].x;
            v.uv[1] = 1.0f - aiMesh->mTextureCoords[0][i].y;  // Flip V
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
        aiProcess_CalcTangentSpace |  // Required for normal mapping
        // Note: Don't use aiProcess_FlipUVs for DirectX (UV origin is top-left, same as textures)
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "[model] Assimp error: " << importer.GetErrorString() << std::endl;
        return std::nullopt;
    }

    std::cout << "[model] Loading: " << path << std::endl;
    std::cout << "[model] Meshes: " << scene->mNumMeshes << ", Materials: " << scene->mNumMaterials;
    if (scene->mNumTextures > 0) {
        std::cout << ", Embedded textures: " << scene->mNumTextures;
    }
    std::cout << std::endl;
    
    // Print embedded texture info
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        const aiTexture* tex = scene->mTextures[i];
        std::cout << "[embedded " << i << "] ";
        if (tex->mHeight == 0) {
            std::cout << "compressed " << tex->mWidth << " bytes";
        } else {
            std::cout << "raw " << tex->mWidth << "x" << tex->mHeight;
        }
        std::cout << " filename: \"" << tex->mFilename.C_Str() << "\"" << std::endl;
    }

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
