// glTF/GLB Exporter - Export characters to glTF 2.0 format
// Supports: Meshes, Skeletons, BlendShapes, Textures, Materials
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace luma {

// ============================================================================
// Export Options
// ============================================================================

struct GLTFExportOptions {
    // Output format
    bool exportGLB = true;              // GLB (binary) vs glTF (JSON + files)
    bool embedTextures = true;          // Embed textures in GLB
    bool embedBuffers = true;           // Embed buffers in glTF (data URIs)
    
    // Geometry
    bool exportNormals = true;
    bool exportTangents = true;
    bool exportUVs = true;
    bool exportVertexColors = true;
    
    // Skeleton
    bool exportSkeleton = true;
    bool exportSkinWeights = true;
    
    // BlendShapes
    bool exportBlendShapes = true;
    int maxBlendShapes = 64;            // Limit for performance
    
    // Textures
    int maxTextureSize = 2048;
    bool compressTextures = false;
    
    // Metadata
    std::string copyright;
    std::string generator = "LUMA Creator";
};

// ============================================================================
// Export Result
// ============================================================================

struct GLTFExportResult {
    bool success = false;
    std::string errorMessage;
    std::string outputPath;
    
    // Statistics
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    size_t boneCount = 0;
    size_t blendShapeCount = 0;
    size_t textureCount = 0;
    size_t fileSize = 0;
};

// ============================================================================
// glTF Data Structures (simplified)
// ============================================================================

namespace gltf {

struct Asset {
    std::string version = "2.0";
    std::string generator;
    std::string copyright;
};

struct Buffer {
    std::vector<uint8_t> data;
    std::string uri;                    // For external buffers
};

struct BufferView {
    int buffer = 0;
    size_t byteOffset = 0;
    size_t byteLength = 0;
    size_t byteStride = 0;              // For vertex data
    int target = 0;                     // 34962=ARRAY_BUFFER, 34963=ELEMENT_ARRAY_BUFFER
};

struct Accessor {
    int bufferView = -1;
    size_t byteOffset = 0;
    int componentType = 0;              // 5126=FLOAT, 5123=UNSIGNED_SHORT, etc.
    size_t count = 0;
    std::string type;                   // "SCALAR", "VEC2", "VEC3", "VEC4", "MAT4"
    std::vector<float> min;
    std::vector<float> max;
};

struct Primitive {
    std::unordered_map<std::string, int> attributes;  // "POSITION", "NORMAL", etc.
    int indices = -1;
    int material = -1;
    std::vector<std::unordered_map<std::string, int>> targets;  // BlendShape targets
};

struct MeshData {
    std::string name;
    std::vector<Primitive> primitives;
    std::vector<float> weights;         // Default blend shape weights
};

struct Skin {
    std::string name;
    int skeleton = -1;                  // Root node
    std::vector<int> joints;            // Node indices
    int inverseBindMatrices = -1;       // Accessor index
};

struct Node {
    std::string name;
    std::vector<int> children;
    int mesh = -1;
    int skin = -1;
    float translation[3] = {0, 0, 0};
    float rotation[4] = {0, 0, 0, 1};   // Quaternion (x, y, z, w)
    float scale[3] = {1, 1, 1};
    bool hasTranslation = false;
    bool hasRotation = false;
    bool hasScale = false;
};

struct TextureInfo {
    int index = -1;
    int texCoord = 0;
};

struct PBRMetallicRoughness {
    float baseColorFactor[4] = {1, 1, 1, 1};
    TextureInfo baseColorTexture;
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
    TextureInfo metallicRoughnessTexture;
};

struct Material {
    std::string name;
    PBRMetallicRoughness pbrMetallicRoughness;
    TextureInfo normalTexture;
    TextureInfo occlusionTexture;
    TextureInfo emissiveTexture;
    float emissiveFactor[3] = {0, 0, 0};
    bool doubleSided = false;
    std::string alphaMode = "OPAQUE";   // "OPAQUE", "MASK", "BLEND"
    float alphaCutoff = 0.5f;
};

struct Image {
    std::string uri;
    std::string mimeType;
    int bufferView = -1;                // For embedded images
};

struct Sampler {
    int magFilter = 9729;               // LINEAR
    int minFilter = 9987;               // LINEAR_MIPMAP_LINEAR
    int wrapS = 10497;                  // REPEAT
    int wrapT = 10497;
};

struct Texture {
    int sampler = -1;
    int source = -1;                    // Image index
};

struct Scene {
    std::string name;
    std::vector<int> nodes;
};

}  // namespace gltf

// ============================================================================
// glTF Exporter
// ============================================================================

class GLTFExporter {
public:
    GLTFExporter() = default;
    
    // === Main Export Functions ===
    
    // Export mesh only
    GLTFExportResult exportMesh(
        const Mesh& mesh,
        const std::string& outputPath,
        const GLTFExportOptions& options = {})
    {
        return exportCharacter(mesh, nullptr, nullptr, outputPath, options);
    }
    
    // Export mesh with skeleton
    GLTFExportResult exportWithSkeleton(
        const Mesh& mesh,
        const Skeleton& skeleton,
        const std::string& outputPath,
        const GLTFExportOptions& options = {})
    {
        return exportCharacter(mesh, &skeleton, nullptr, outputPath, options);
    }
    
    // Export complete character (mesh + skeleton + blend shapes)
    GLTFExportResult exportCharacter(
        const Mesh& mesh,
        const Skeleton* skeleton,
        const BlendShapeMesh* blendShapes,
        const std::string& outputPath,
        const GLTFExportOptions& options = {})
    {
        GLTFExportResult result;
        options_ = options;
        
        // Clear previous data
        clearData();
        
        // Set asset info
        asset_.generator = options.generator;
        asset_.copyright = options.copyright;
        
        // Create nodes
        int meshNodeIndex = createMeshNode(mesh, skeleton, blendShapes);
        
        // Create skeleton nodes if present
        int skinIndex = -1;
        if (skeleton && options.exportSkeleton) {
            skinIndex = createSkeleton(*skeleton);
            if (skinIndex >= 0) {
                nodes_[meshNodeIndex].skin = skinIndex;
            }
        }
        
        // Create scene
        gltf::Scene scene;
        scene.name = "Scene";
        scene.nodes.push_back(meshNodeIndex);
        
        // Add skeleton root nodes to scene
        if (skeleton && skeletonRootNode_ >= 0) {
            scene.nodes.push_back(skeletonRootNode_);
        }
        
        scenes_.push_back(scene);
        
        // Add textures
        if (!mesh.diffuseTexture.pixels.empty()) {
            addTexture(mesh.diffuseTexture, "diffuse");
        }
        if (!mesh.normalTexture.pixels.empty()) {
            addTexture(mesh.normalTexture, "normal");
        }
        if (!mesh.specularTexture.pixels.empty()) {
            addTexture(mesh.specularTexture, "roughness");
        }
        
        // Create material
        createMaterial(mesh);
        
        // Write output
        if (options.exportGLB) {
            result = writeGLB(outputPath);
        } else {
            result = writeGLTF(outputPath);
        }
        
        // Fill statistics
        result.vertexCount = mesh.vertices.size();
        result.triangleCount = mesh.indices.size() / 3;
        result.boneCount = skeleton ? skeleton->getBoneCount() : 0;
        result.blendShapeCount = blendShapes ? blendShapes->getTargetCount() : 0;
        result.textureCount = textures_.size();
        
        return result;
    }
    
private:
    // Options
    GLTFExportOptions options_;
    
    // glTF data
    gltf::Asset asset_;
    std::vector<gltf::Buffer> buffers_;
    std::vector<gltf::BufferView> bufferViews_;
    std::vector<gltf::Accessor> accessors_;
    std::vector<gltf::MeshData> meshes_;
    std::vector<gltf::Node> nodes_;
    std::vector<gltf::Skin> skins_;
    std::vector<gltf::Material> materials_;
    std::vector<gltf::Image> images_;
    std::vector<gltf::Sampler> samplers_;
    std::vector<gltf::Texture> textures_;
    std::vector<gltf::Scene> scenes_;
    
    // Bone node mapping
    std::unordered_map<int, int> boneToNodeIndex_;
    int skeletonRootNode_ = -1;
    
    // Main buffer
    std::vector<uint8_t> mainBuffer_;
    
    void clearData() {
        buffers_.clear();
        bufferViews_.clear();
        accessors_.clear();
        meshes_.clear();
        nodes_.clear();
        skins_.clear();
        materials_.clear();
        images_.clear();
        samplers_.clear();
        textures_.clear();
        scenes_.clear();
        boneToNodeIndex_.clear();
        skeletonRootNode_ = -1;
        mainBuffer_.clear();
    }
    
    // === Node Creation ===
    
    int createMeshNode(const Mesh& mesh, const Skeleton* skeleton, const BlendShapeMesh* blendShapes) {
        gltf::Node node;
        node.name = mesh.name.empty() ? "Character" : mesh.name;
        node.mesh = createMesh(mesh, skeleton, blendShapes);
        
        int nodeIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(node);
        return nodeIndex;
    }
    
    int createMesh(const Mesh& mesh, const Skeleton* skeleton, const BlendShapeMesh* blendShapes) {
        gltf::MeshData meshData;
        meshData.name = mesh.name.empty() ? "Mesh" : mesh.name;
        
        gltf::Primitive prim;
        
        // Position accessor
        prim.attributes["POSITION"] = createPositionAccessor(mesh.vertices);
        
        // Normal accessor
        if (options_.exportNormals) {
            prim.attributes["NORMAL"] = createNormalAccessor(mesh.vertices);
        }
        
        // Tangent accessor
        if (options_.exportTangents) {
            int tangentAcc = createTangentAccessor(mesh.vertices);
            if (tangentAcc >= 0) {
                prim.attributes["TANGENT"] = tangentAcc;
            }
        }
        
        // UV accessor
        if (options_.exportUVs) {
            prim.attributes["TEXCOORD_0"] = createUVAccessor(mesh.vertices);
        }
        
        // Vertex color accessor
        if (options_.exportVertexColors && hasVertexColors(mesh.vertices)) {
            prim.attributes["COLOR_0"] = createColorAccessor(mesh.vertices);
        }
        
        // Skin weights
        if (skeleton && options_.exportSkinWeights) {
            prim.attributes["JOINTS_0"] = createJointsAccessor(mesh.vertices);
            prim.attributes["WEIGHTS_0"] = createWeightsAccessor(mesh.vertices);
        }
        
        // Indices
        prim.indices = createIndicesAccessor(mesh.indices);
        
        // Material
        prim.material = 0;  // We'll create one material
        
        // BlendShapes (morph targets)
        if (blendShapes && options_.exportBlendShapes) {
            createMorphTargets(mesh, *blendShapes, prim, meshData.weights);
        }
        
        meshData.primitives.push_back(prim);
        
        int meshIndex = static_cast<int>(meshes_.size());
        meshes_.push_back(meshData);
        return meshIndex;
    }
    
    // === Accessor Creation ===
    
    int createPositionAccessor(const std::vector<Vertex>& vertices) {
        std::vector<float> positions;
        positions.reserve(vertices.size() * 3);
        
        Vec3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
        Vec3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for (const auto& v : vertices) {
            positions.push_back(v.position[0]);
            positions.push_back(v.position[1]);
            positions.push_back(v.position[2]);
            
            minPos.x = std::min(minPos.x, v.position[0]);
            minPos.y = std::min(minPos.y, v.position[1]);
            minPos.z = std::min(minPos.z, v.position[2]);
            maxPos.x = std::max(maxPos.x, v.position[0]);
            maxPos.y = std::max(maxPos.y, v.position[1]);
            maxPos.z = std::max(maxPos.z, v.position[2]);
        }
        
        int bufferView = addBufferView(positions.data(), positions.size() * sizeof(float), 12, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;  // FLOAT
        acc.count = vertices.size();
        acc.type = "VEC3";
        acc.min = {minPos.x, minPos.y, minPos.z};
        acc.max = {maxPos.x, maxPos.y, maxPos.z};
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createNormalAccessor(const std::vector<Vertex>& vertices) {
        std::vector<float> normals;
        normals.reserve(vertices.size() * 3);
        
        for (const auto& v : vertices) {
            normals.push_back(v.normal[0]);
            normals.push_back(v.normal[1]);
            normals.push_back(v.normal[2]);
        }
        
        int bufferView = addBufferView(normals.data(), normals.size() * sizeof(float), 12, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;
        acc.count = vertices.size();
        acc.type = "VEC3";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createTangentAccessor(const std::vector<Vertex>& vertices) {
        // Check if we have tangent data
        bool hasTangents = false;
        for (const auto& v : vertices) {
            if (v.tangent[0] != 0 || v.tangent[1] != 0 || v.tangent[2] != 0) {
                hasTangents = true;
                break;
            }
        }
        if (!hasTangents) return -1;
        
        std::vector<float> tangents;
        tangents.reserve(vertices.size() * 4);
        
        for (const auto& v : vertices) {
            tangents.push_back(v.tangent[0]);
            tangents.push_back(v.tangent[1]);
            tangents.push_back(v.tangent[2]);
            tangents.push_back(v.tangent[3]);
        }
        
        int bufferView = addBufferView(tangents.data(), tangents.size() * sizeof(float), 16, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;
        acc.count = vertices.size();
        acc.type = "VEC4";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createUVAccessor(const std::vector<Vertex>& vertices) {
        std::vector<float> uvs;
        uvs.reserve(vertices.size() * 2);
        
        for (const auto& v : vertices) {
            uvs.push_back(v.uv[0]);
            uvs.push_back(v.uv[1]);
        }
        
        int bufferView = addBufferView(uvs.data(), uvs.size() * sizeof(float), 8, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;
        acc.count = vertices.size();
        acc.type = "VEC2";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createColorAccessor(const std::vector<Vertex>& vertices) {
        std::vector<float> colors;
        colors.reserve(vertices.size() * 4);
        
        for (const auto& v : vertices) {
            colors.push_back(v.color[0]);
            colors.push_back(v.color[1]);
            colors.push_back(v.color[2]);
            colors.push_back(v.color[3]);
        }
        
        int bufferView = addBufferView(colors.data(), colors.size() * sizeof(float), 16, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;
        acc.count = vertices.size();
        acc.type = "VEC4";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createJointsAccessor(const std::vector<Vertex>& vertices) {
        std::vector<uint16_t> joints;
        joints.reserve(vertices.size() * 4);
        
        for (const auto& v : vertices) {
            joints.push_back(static_cast<uint16_t>(v.boneIndices[0]));
            joints.push_back(static_cast<uint16_t>(v.boneIndices[1]));
            joints.push_back(static_cast<uint16_t>(v.boneIndices[2]));
            joints.push_back(static_cast<uint16_t>(v.boneIndices[3]));
        }
        
        int bufferView = addBufferView(joints.data(), joints.size() * sizeof(uint16_t), 8, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5123;  // UNSIGNED_SHORT
        acc.count = vertices.size();
        acc.type = "VEC4";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createWeightsAccessor(const std::vector<Vertex>& vertices) {
        std::vector<float> weights;
        weights.reserve(vertices.size() * 4);
        
        for (const auto& v : vertices) {
            weights.push_back(v.boneWeights[0]);
            weights.push_back(v.boneWeights[1]);
            weights.push_back(v.boneWeights[2]);
            weights.push_back(v.boneWeights[3]);
        }
        
        int bufferView = addBufferView(weights.data(), weights.size() * sizeof(float), 16, 34962);
        
        gltf::Accessor acc;
        acc.bufferView = bufferView;
        acc.componentType = 5126;
        acc.count = vertices.size();
        acc.type = "VEC4";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    int createIndicesAccessor(const std::vector<uint32_t>& indices) {
        // Use 16-bit if possible
        bool use32bit = indices.size() > 65535;
        
        int bufferView;
        gltf::Accessor acc;
        
        if (use32bit) {
            bufferView = addBufferView(indices.data(), indices.size() * sizeof(uint32_t), 0, 34963);
            acc.componentType = 5125;  // UNSIGNED_INT
        } else {
            std::vector<uint16_t> indices16;
            indices16.reserve(indices.size());
            for (uint32_t i : indices) {
                indices16.push_back(static_cast<uint16_t>(i));
            }
            bufferView = addBufferView(indices16.data(), indices16.size() * sizeof(uint16_t), 0, 34963);
            acc.componentType = 5123;  // UNSIGNED_SHORT
        }
        
        acc.bufferView = bufferView;
        acc.count = indices.size();
        acc.type = "SCALAR";
        
        int accIndex = static_cast<int>(accessors_.size());
        accessors_.push_back(acc);
        return accIndex;
    }
    
    // === Skeleton ===
    
    int createSkeleton(const Skeleton& skeleton) {
        if (skeleton.getBoneCount() == 0) return -1;
        
        // Create node for each bone
        std::vector<int> jointIndices;
        
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone) continue;
            
            gltf::Node node;
            node.name = bone->name;
            
            // Set local transform
            node.translation[0] = bone->localPosition.x;
            node.translation[1] = bone->localPosition.y;
            node.translation[2] = bone->localPosition.z;
            node.hasTranslation = true;
            
            // Quaternion: glTF uses (x, y, z, w) order
            node.rotation[0] = bone->localRotation.x;
            node.rotation[1] = bone->localRotation.y;
            node.rotation[2] = bone->localRotation.z;
            node.rotation[3] = bone->localRotation.w;
            node.hasRotation = true;
            
            node.scale[0] = bone->localScale.x;
            node.scale[1] = bone->localScale.y;
            node.scale[2] = bone->localScale.z;
            if (bone->localScale.x != 1 || bone->localScale.y != 1 || bone->localScale.z != 1) {
                node.hasScale = true;
            }
            
            int nodeIndex = static_cast<int>(nodes_.size());
            nodes_.push_back(node);
            
            boneToNodeIndex_[i] = nodeIndex;
            jointIndices.push_back(nodeIndex);
            
            // Track root
            if (bone->parentIndex < 0) {
                skeletonRootNode_ = nodeIndex;
            }
        }
        
        // Set up parent-child relationships
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone || bone->parentIndex < 0) continue;
            
            int parentNodeIdx = boneToNodeIndex_[bone->parentIndex];
            int childNodeIdx = boneToNodeIndex_[i];
            nodes_[parentNodeIdx].children.push_back(childNodeIdx);
        }
        
        // Create inverse bind matrices accessor
        std::vector<float> invBindMatrices;
        invBindMatrices.reserve(skeleton.getBoneCount() * 16);
        
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone) continue;
            
            const Mat4& mat = bone->inverseBindMatrix;
            // glTF uses column-major order
            for (int col = 0; col < 4; col++) {
                for (int row = 0; row < 4; row++) {
                    invBindMatrices.push_back(mat.m[row * 4 + col]);
                }
            }
        }
        
        int invBindView = addBufferView(invBindMatrices.data(), 
                                        invBindMatrices.size() * sizeof(float), 0, 0);
        
        gltf::Accessor invBindAcc;
        invBindAcc.bufferView = invBindView;
        invBindAcc.componentType = 5126;
        invBindAcc.count = skeleton.getBoneCount();
        invBindAcc.type = "MAT4";
        
        int invBindAccIdx = static_cast<int>(accessors_.size());
        accessors_.push_back(invBindAcc);
        
        // Create skin
        gltf::Skin skin;
        skin.name = "Armature";
        skin.skeleton = skeletonRootNode_;
        skin.joints = jointIndices;
        skin.inverseBindMatrices = invBindAccIdx;
        
        int skinIndex = static_cast<int>(skins_.size());
        skins_.push_back(skin);
        
        return skinIndex;
    }
    
    // === Morph Targets ===
    
    void createMorphTargets(const Mesh& mesh, const BlendShapeMesh& blendShapes,
                           gltf::Primitive& prim, std::vector<float>& weights) {
        int count = std::min(static_cast<int>(blendShapes.getTargetCount()), options_.maxBlendShapes);
        
        for (int i = 0; i < count; i++) {
            const BlendShapeTarget* target = blendShapes.getTarget(i);
            if (!target) continue;
            
            // Create position deltas
            std::vector<float> posDeltas(mesh.vertices.size() * 3, 0.0f);
            std::vector<float> normDeltas(mesh.vertices.size() * 3, 0.0f);
            
            for (const auto& delta : target->deltas) {
                if (delta.vertexIndex < mesh.vertices.size()) {
                    size_t idx = delta.vertexIndex * 3;
                    posDeltas[idx + 0] = delta.positionDelta.x;
                    posDeltas[idx + 1] = delta.positionDelta.y;
                    posDeltas[idx + 2] = delta.positionDelta.z;
                    normDeltas[idx + 0] = delta.normalDelta.x;
                    normDeltas[idx + 1] = delta.normalDelta.y;
                    normDeltas[idx + 2] = delta.normalDelta.z;
                }
            }
            
            // Position delta accessor
            int posView = addBufferView(posDeltas.data(), posDeltas.size() * sizeof(float), 12, 34962);
            gltf::Accessor posAcc;
            posAcc.bufferView = posView;
            posAcc.componentType = 5126;
            posAcc.count = mesh.vertices.size();
            posAcc.type = "VEC3";
            int posAccIdx = static_cast<int>(accessors_.size());
            accessors_.push_back(posAcc);
            
            // Normal delta accessor
            int normView = addBufferView(normDeltas.data(), normDeltas.size() * sizeof(float), 12, 34962);
            gltf::Accessor normAcc;
            normAcc.bufferView = normView;
            normAcc.componentType = 5126;
            normAcc.count = mesh.vertices.size();
            normAcc.type = "VEC3";
            int normAccIdx = static_cast<int>(accessors_.size());
            accessors_.push_back(normAcc);
            
            // Add target
            std::unordered_map<std::string, int> targetAttrs;
            targetAttrs["POSITION"] = posAccIdx;
            targetAttrs["NORMAL"] = normAccIdx;
            prim.targets.push_back(targetAttrs);
            
            // Default weight
            weights.push_back(0.0f);
        }
    }
    
    // === Materials and Textures ===
    
    void createMaterial(const Mesh& mesh) {
        gltf::Material mat;
        mat.name = "Material";
        
        mat.pbrMetallicRoughness.baseColorFactor[0] = mesh.baseColor[0];
        mat.pbrMetallicRoughness.baseColorFactor[1] = mesh.baseColor[1];
        mat.pbrMetallicRoughness.baseColorFactor[2] = mesh.baseColor[2];
        mat.pbrMetallicRoughness.baseColorFactor[3] = 1.0f;
        
        mat.pbrMetallicRoughness.metallicFactor = mesh.metallic;
        mat.pbrMetallicRoughness.roughnessFactor = mesh.roughness;
        
        // Link textures
        if (!textures_.empty()) {
            mat.pbrMetallicRoughness.baseColorTexture.index = 0;
        }
        if (textures_.size() > 1) {
            mat.normalTexture.index = 1;
        }
        
        materials_.push_back(mat);
    }
    
    void addTexture(const TextureData& texData, const std::string& name) {
        // Create sampler if not exists
        if (samplers_.empty()) {
            gltf::Sampler sampler;
            samplers_.push_back(sampler);
        }
        
        // Create image
        gltf::Image image;
        
        if (options_.embedTextures && options_.exportGLB) {
            // Embed in buffer
            std::vector<uint8_t> pngData = encodePNG(texData);
            int imageView = addBufferView(pngData.data(), pngData.size(), 0, 0);
            image.bufferView = imageView;
            image.mimeType = "image/png";
        } else {
            // External file
            image.uri = name + ".png";
            // Would need to write the texture file separately
        }
        
        int imageIndex = static_cast<int>(images_.size());
        images_.push_back(image);
        
        // Create texture
        gltf::Texture tex;
        tex.sampler = 0;
        tex.source = imageIndex;
        textures_.push_back(tex);
    }
    
    // === Buffer Management ===
    
    int addBufferView(const void* data, size_t byteLength, size_t byteStride, int target) {
        // Align to 4 bytes
        size_t offset = mainBuffer_.size();
        size_t padding = (4 - (offset % 4)) % 4;
        mainBuffer_.resize(offset + padding);
        offset = mainBuffer_.size();
        
        // Copy data
        mainBuffer_.resize(offset + byteLength);
        memcpy(mainBuffer_.data() + offset, data, byteLength);
        
        // Create buffer view
        gltf::BufferView view;
        view.buffer = 0;
        view.byteOffset = offset;
        view.byteLength = byteLength;
        view.byteStride = byteStride;
        view.target = target;
        
        int viewIndex = static_cast<int>(bufferViews_.size());
        bufferViews_.push_back(view);
        return viewIndex;
    }
    
    // === Helpers ===
    
    bool hasVertexColors(const std::vector<Vertex>& vertices) {
        for (const auto& v : vertices) {
            if (v.color[0] != 1 || v.color[1] != 1 || v.color[2] != 1 || v.color[3] != 1) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<uint8_t> encodePNG(const TextureData& tex) {
        // Simplified: just return raw RGBA for now
        // In production, would use stb_image_write or libpng
        
        // Create a minimal PNG header + raw data
        // For now, just store raw data (not valid PNG)
        std::vector<uint8_t> data;
        data.insert(data.end(), tex.pixels.begin(), tex.pixels.end());
        return data;
    }
    
    // === JSON Generation ===
    
    std::string generateJSON() {
        std::ostringstream ss;
        ss << "{\n";
        
        // Asset
        ss << "  \"asset\": {\n";
        ss << "    \"version\": \"" << asset_.version << "\"";
        if (!asset_.generator.empty()) {
            ss << ",\n    \"generator\": \"" << asset_.generator << "\"";
        }
        if (!asset_.copyright.empty()) {
            ss << ",\n    \"copyright\": \"" << asset_.copyright << "\"";
        }
        ss << "\n  }";
        
        // Scene
        ss << ",\n  \"scene\": 0";
        
        // Scenes
        ss << ",\n  \"scenes\": [";
        for (size_t i = 0; i < scenes_.size(); i++) {
            ss << "\n    {\"name\": \"" << scenes_[i].name << "\", \"nodes\": [";
            for (size_t j = 0; j < scenes_[i].nodes.size(); j++) {
                ss << scenes_[i].nodes[j];
                if (j < scenes_[i].nodes.size() - 1) ss << ", ";
            }
            ss << "]}";
            if (i < scenes_.size() - 1) ss << ",";
        }
        ss << "\n  ]";
        
        // Nodes
        ss << ",\n  \"nodes\": [";
        for (size_t i = 0; i < nodes_.size(); i++) {
            const auto& node = nodes_[i];
            ss << "\n    {\"name\": \"" << node.name << "\"";
            
            if (!node.children.empty()) {
                ss << ", \"children\": [";
                for (size_t j = 0; j < node.children.size(); j++) {
                    ss << node.children[j];
                    if (j < node.children.size() - 1) ss << ", ";
                }
                ss << "]";
            }
            
            if (node.mesh >= 0) ss << ", \"mesh\": " << node.mesh;
            if (node.skin >= 0) ss << ", \"skin\": " << node.skin;
            
            if (node.hasTranslation) {
                ss << ", \"translation\": [" << node.translation[0] << ", " 
                   << node.translation[1] << ", " << node.translation[2] << "]";
            }
            if (node.hasRotation) {
                ss << ", \"rotation\": [" << node.rotation[0] << ", " << node.rotation[1] 
                   << ", " << node.rotation[2] << ", " << node.rotation[3] << "]";
            }
            if (node.hasScale) {
                ss << ", \"scale\": [" << node.scale[0] << ", " 
                   << node.scale[1] << ", " << node.scale[2] << "]";
            }
            
            ss << "}";
            if (i < nodes_.size() - 1) ss << ",";
        }
        ss << "\n  ]";
        
        // Meshes
        if (!meshes_.empty()) {
            ss << ",\n  \"meshes\": [";
            for (size_t i = 0; i < meshes_.size(); i++) {
                const auto& mesh = meshes_[i];
                ss << "\n    {\"name\": \"" << mesh.name << "\", \"primitives\": [";
                
                for (size_t p = 0; p < mesh.primitives.size(); p++) {
                    const auto& prim = mesh.primitives[p];
                    ss << "{\"attributes\": {";
                    
                    bool first = true;
                    for (const auto& [name, idx] : prim.attributes) {
                        if (!first) ss << ", ";
                        ss << "\"" << name << "\": " << idx;
                        first = false;
                    }
                    ss << "}";
                    
                    if (prim.indices >= 0) ss << ", \"indices\": " << prim.indices;
                    if (prim.material >= 0) ss << ", \"material\": " << prim.material;
                    
                    if (!prim.targets.empty()) {
                        ss << ", \"targets\": [";
                        for (size_t t = 0; t < prim.targets.size(); t++) {
                            ss << "{";
                            bool firstT = true;
                            for (const auto& [name, idx] : prim.targets[t]) {
                                if (!firstT) ss << ", ";
                                ss << "\"" << name << "\": " << idx;
                                firstT = false;
                            }
                            ss << "}";
                            if (t < prim.targets.size() - 1) ss << ", ";
                        }
                        ss << "]";
                    }
                    
                    ss << "}";
                    if (p < mesh.primitives.size() - 1) ss << ", ";
                }
                ss << "]";
                
                if (!mesh.weights.empty()) {
                    ss << ", \"weights\": [";
                    for (size_t w = 0; w < mesh.weights.size(); w++) {
                        ss << mesh.weights[w];
                        if (w < mesh.weights.size() - 1) ss << ", ";
                    }
                    ss << "]";
                }
                
                ss << "}";
                if (i < meshes_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Skins
        if (!skins_.empty()) {
            ss << ",\n  \"skins\": [";
            for (size_t i = 0; i < skins_.size(); i++) {
                const auto& skin = skins_[i];
                ss << "\n    {\"name\": \"" << skin.name << "\"";
                if (skin.skeleton >= 0) ss << ", \"skeleton\": " << skin.skeleton;
                ss << ", \"joints\": [";
                for (size_t j = 0; j < skin.joints.size(); j++) {
                    ss << skin.joints[j];
                    if (j < skin.joints.size() - 1) ss << ", ";
                }
                ss << "]";
                if (skin.inverseBindMatrices >= 0) {
                    ss << ", \"inverseBindMatrices\": " << skin.inverseBindMatrices;
                }
                ss << "}";
                if (i < skins_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Materials
        if (!materials_.empty()) {
            ss << ",\n  \"materials\": [";
            for (size_t i = 0; i < materials_.size(); i++) {
                const auto& mat = materials_[i];
                ss << "\n    {\"name\": \"" << mat.name << "\"";
                ss << ", \"pbrMetallicRoughness\": {";
                ss << "\"baseColorFactor\": [" << mat.pbrMetallicRoughness.baseColorFactor[0] 
                   << ", " << mat.pbrMetallicRoughness.baseColorFactor[1]
                   << ", " << mat.pbrMetallicRoughness.baseColorFactor[2]
                   << ", " << mat.pbrMetallicRoughness.baseColorFactor[3] << "]";
                ss << ", \"metallicFactor\": " << mat.pbrMetallicRoughness.metallicFactor;
                ss << ", \"roughnessFactor\": " << mat.pbrMetallicRoughness.roughnessFactor;
                
                if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                    ss << ", \"baseColorTexture\": {\"index\": " 
                       << mat.pbrMetallicRoughness.baseColorTexture.index << "}";
                }
                ss << "}";
                
                if (mat.normalTexture.index >= 0) {
                    ss << ", \"normalTexture\": {\"index\": " << mat.normalTexture.index << "}";
                }
                
                ss << "}";
                if (i < materials_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Textures
        if (!textures_.empty()) {
            ss << ",\n  \"textures\": [";
            for (size_t i = 0; i < textures_.size(); i++) {
                ss << "\n    {\"sampler\": " << textures_[i].sampler 
                   << ", \"source\": " << textures_[i].source << "}";
                if (i < textures_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Images
        if (!images_.empty()) {
            ss << ",\n  \"images\": [";
            for (size_t i = 0; i < images_.size(); i++) {
                const auto& img = images_[i];
                ss << "\n    {";
                if (img.bufferView >= 0) {
                    ss << "\"bufferView\": " << img.bufferView;
                    ss << ", \"mimeType\": \"" << img.mimeType << "\"";
                } else {
                    ss << "\"uri\": \"" << img.uri << "\"";
                }
                ss << "}";
                if (i < images_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Samplers
        if (!samplers_.empty()) {
            ss << ",\n  \"samplers\": [";
            for (size_t i = 0; i < samplers_.size(); i++) {
                const auto& samp = samplers_[i];
                ss << "\n    {\"magFilter\": " << samp.magFilter 
                   << ", \"minFilter\": " << samp.minFilter
                   << ", \"wrapS\": " << samp.wrapS
                   << ", \"wrapT\": " << samp.wrapT << "}";
                if (i < samplers_.size() - 1) ss << ",";
            }
            ss << "\n  ]";
        }
        
        // Accessors
        ss << ",\n  \"accessors\": [";
        for (size_t i = 0; i < accessors_.size(); i++) {
            const auto& acc = accessors_[i];
            ss << "\n    {\"bufferView\": " << acc.bufferView;
            if (acc.byteOffset > 0) ss << ", \"byteOffset\": " << acc.byteOffset;
            ss << ", \"componentType\": " << acc.componentType;
            ss << ", \"count\": " << acc.count;
            ss << ", \"type\": \"" << acc.type << "\"";
            
            if (!acc.min.empty()) {
                ss << ", \"min\": [";
                for (size_t m = 0; m < acc.min.size(); m++) {
                    ss << acc.min[m];
                    if (m < acc.min.size() - 1) ss << ", ";
                }
                ss << "]";
            }
            if (!acc.max.empty()) {
                ss << ", \"max\": [";
                for (size_t m = 0; m < acc.max.size(); m++) {
                    ss << acc.max[m];
                    if (m < acc.max.size() - 1) ss << ", ";
                }
                ss << "]";
            }
            
            ss << "}";
            if (i < accessors_.size() - 1) ss << ",";
        }
        ss << "\n  ]";
        
        // Buffer Views
        ss << ",\n  \"bufferViews\": [";
        for (size_t i = 0; i < bufferViews_.size(); i++) {
            const auto& view = bufferViews_[i];
            ss << "\n    {\"buffer\": " << view.buffer;
            ss << ", \"byteOffset\": " << view.byteOffset;
            ss << ", \"byteLength\": " << view.byteLength;
            if (view.byteStride > 0) ss << ", \"byteStride\": " << view.byteStride;
            if (view.target > 0) ss << ", \"target\": " << view.target;
            ss << "}";
            if (i < bufferViews_.size() - 1) ss << ",";
        }
        ss << "\n  ]";
        
        // Buffers
        ss << ",\n  \"buffers\": [";
        ss << "\n    {\"byteLength\": " << mainBuffer_.size();
        if (!options_.exportGLB && options_.embedBuffers) {
            // Data URI
            ss << ", \"uri\": \"data:application/octet-stream;base64,";
            ss << base64Encode(mainBuffer_.data(), mainBuffer_.size());
            ss << "\"";
        } else if (!options_.exportGLB) {
            ss << ", \"uri\": \"buffer.bin\"";
        }
        ss << "}";
        ss << "\n  ]";
        
        ss << "\n}\n";
        return ss.str();
    }
    
    std::string base64Encode(const uint8_t* data, size_t length) {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve((length + 2) / 3 * 4);
        
        for (size_t i = 0; i < length; i += 3) {
            uint32_t n = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < length) n |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < length) n |= static_cast<uint32_t>(data[i + 2]);
            
            result += chars[(n >> 18) & 0x3F];
            result += chars[(n >> 12) & 0x3F];
            result += (i + 1 < length) ? chars[(n >> 6) & 0x3F] : '=';
            result += (i + 2 < length) ? chars[n & 0x3F] : '=';
        }
        
        return result;
    }
    
    // === File Writing ===
    
    GLTFExportResult writeGLTF(const std::string& outputPath) {
        GLTFExportResult result;
        
        std::string json = generateJSON();
        
        // Write JSON
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            result.errorMessage = "Failed to open file for writing: " + outputPath;
            return result;
        }
        
        file << json;
        file.close();
        
        // Write binary buffer if not embedded
        if (!options_.embedBuffers) {
            std::string binPath = outputPath.substr(0, outputPath.rfind('.')) + ".bin";
            std::ofstream binFile(binPath, std::ios::binary);
            if (binFile.is_open()) {
                binFile.write(reinterpret_cast<const char*>(mainBuffer_.data()), mainBuffer_.size());
                binFile.close();
            }
        }
        
        result.success = true;
        result.outputPath = outputPath;
        return result;
    }
    
    GLTFExportResult writeGLB(const std::string& outputPath) {
        GLTFExportResult result;
        
        std::string json = generateJSON();
        
        // Pad JSON to 4-byte alignment
        while (json.size() % 4 != 0) {
            json += ' ';
        }
        
        // Pad buffer to 4-byte alignment
        while (mainBuffer_.size() % 4 != 0) {
            mainBuffer_.push_back(0);
        }
        
        // GLB structure:
        // Header (12 bytes)
        // JSON chunk (8 + jsonLength)
        // BIN chunk (8 + binLength)
        
        uint32_t jsonLength = static_cast<uint32_t>(json.size());
        uint32_t binLength = static_cast<uint32_t>(mainBuffer_.size());
        uint32_t totalLength = 12 + 8 + jsonLength + 8 + binLength;
        
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            result.errorMessage = "Failed to open file for writing: " + outputPath;
            return result;
        }
        
        // Header
        uint32_t magic = 0x46546C67;  // "glTF"
        uint32_t version = 2;
        file.write(reinterpret_cast<const char*>(&magic), 4);
        file.write(reinterpret_cast<const char*>(&version), 4);
        file.write(reinterpret_cast<const char*>(&totalLength), 4);
        
        // JSON chunk
        uint32_t jsonChunkType = 0x4E4F534A;  // "JSON"
        file.write(reinterpret_cast<const char*>(&jsonLength), 4);
        file.write(reinterpret_cast<const char*>(&jsonChunkType), 4);
        file.write(json.data(), jsonLength);
        
        // BIN chunk
        uint32_t binChunkType = 0x004E4942;  // "BIN\0"
        file.write(reinterpret_cast<const char*>(&binLength), 4);
        file.write(reinterpret_cast<const char*>(&binChunkType), 4);
        file.write(reinterpret_cast<const char*>(mainBuffer_.data()), binLength);
        
        file.close();
        
        result.success = true;
        result.outputPath = outputPath;
        result.fileSize = totalLength;
        return result;
    }
};

}  // namespace luma
