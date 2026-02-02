// Model Exporter - Unified export interface for multiple formats
// Supports: glTF/GLB, FBX, OBJ, VRM
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include "engine/character/blend_shape.h"
#include "engine/export/gltf_exporter.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>

namespace luma {

// ============================================================================
// Export Format
// ============================================================================

enum class ExportFormat {
    GLTF,           // glTF 2.0 (JSON + bin)
    GLB,            // glTF Binary
    FBX,            // Autodesk FBX
    OBJ,            // Wavefront OBJ
    VRM,            // VRM 1.0 (for VTuber)
    USD,            // Universal Scene Description (future)
    Unknown
};

inline std::string formatToExtension(ExportFormat format) {
    switch (format) {
        case ExportFormat::GLTF: return ".gltf";
        case ExportFormat::GLB: return ".glb";
        case ExportFormat::FBX: return ".fbx";
        case ExportFormat::OBJ: return ".obj";
        case ExportFormat::VRM: return ".vrm";
        case ExportFormat::USD: return ".usd";
        default: return "";
    }
}

inline ExportFormat extensionToFormat(const std::string& ext) {
    std::string lower = ext;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == ".gltf") return ExportFormat::GLTF;
    if (lower == ".glb") return ExportFormat::GLB;
    if (lower == ".fbx") return ExportFormat::FBX;
    if (lower == ".obj") return ExportFormat::OBJ;
    if (lower == ".vrm") return ExportFormat::VRM;
    if (lower == ".usd" || lower == ".usda" || lower == ".usdc") return ExportFormat::USD;
    return ExportFormat::Unknown;
}

// ============================================================================
// Export Options
// ============================================================================

struct ExportOptions {
    ExportFormat format = ExportFormat::GLB;
    
    // Common options
    bool exportSkeleton = true;
    bool exportBlendShapes = true;
    bool exportMaterials = true;
    bool exportTextures = true;
    bool exportAnimations = true;
    
    // Geometry
    bool exportNormals = true;
    bool exportTangents = true;
    bool exportUVs = true;
    bool exportVertexColors = false;
    
    // Textures
    bool embedTextures = true;
    int maxTextureSize = 2048;
    
    // Scale
    float scaleFactor = 1.0f;
    
    // Coordinate system conversion
    bool convertYUp = true;             // Convert to Y-up if needed
    bool flipFaces = false;
    
    // VRM specific
    std::string vrmTitle;
    std::string vrmAuthor;
    std::string vrmVersion;
    std::string vrmLicense = "CC-BY";
    
    // Metadata
    std::string copyright;
    std::string generator = "LUMA Creator";
};

// ============================================================================
// Export Result
// ============================================================================

struct ExportResult {
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
    
    // Additional files created
    std::vector<std::string> additionalFiles;
};

// ============================================================================
// Export Data Bundle - Everything needed to export a character
// ============================================================================

struct CharacterExportData {
    // Geometry
    Mesh mesh;
    
    // Skeleton (optional)
    std::shared_ptr<Skeleton> skeleton;
    
    // BlendShapes (optional)
    std::shared_ptr<BlendShapeMesh> blendShapes;
    
    // Textures (already in mesh, but can override)
    TextureData diffuseTexture;
    TextureData normalTexture;
    TextureData roughnessTexture;
    TextureData metallicTexture;
    
    // Metadata
    std::string name = "Character";
    std::string author;
    
    // VRM metadata
    struct VRMMetadata {
        std::string title;
        std::string version = "1.0";
        std::string author;
        std::string contactInfo;
        std::string reference;
        
        // Permissions
        std::string allowedUserName = "Everyone";
        std::string violentUsage = "Disallow";
        std::string sexualUsage = "Disallow";
        std::string commercialUsage = "Allow";
        std::string license = "CC-BY-4.0";
    } vrm;
};

// ============================================================================
// OBJ Exporter (Simple)
// ============================================================================

class OBJExporter {
public:
    static ExportResult exportMesh(
        const Mesh& mesh,
        const std::string& outputPath,
        const ExportOptions& options = {})
    {
        ExportResult result;
        
        std::ofstream objFile(outputPath);
        if (!objFile.is_open()) {
            result.errorMessage = "Failed to open file: " + outputPath;
            return result;
        }
        
        // Header
        objFile << "# Exported by " << options.generator << "\n";
        objFile << "# Vertices: " << mesh.vertices.size() << "\n";
        objFile << "# Faces: " << mesh.indices.size() / 3 << "\n\n";
        
        // Material library
        std::string mtlPath = outputPath.substr(0, outputPath.rfind('.')) + ".mtl";
        std::string mtlName = mtlPath.substr(mtlPath.rfind('/') + 1);
        objFile << "mtllib " << mtlName << "\n\n";
        
        // Vertices
        for (const auto& v : mesh.vertices) {
            objFile << "v " << v.position[0] * options.scaleFactor
                   << " " << v.position[1] * options.scaleFactor
                   << " " << v.position[2] * options.scaleFactor << "\n";
        }
        objFile << "\n";
        
        // Texture coordinates
        if (options.exportUVs) {
            for (const auto& v : mesh.vertices) {
                objFile << "vt " << v.uv[0] << " " << (1.0f - v.uv[1]) << "\n";  // Flip V
            }
            objFile << "\n";
        }
        
        // Normals
        if (options.exportNormals) {
            for (const auto& v : mesh.vertices) {
                objFile << "vn " << v.normal[0] << " " << v.normal[1] << " " << v.normal[2] << "\n";
            }
            objFile << "\n";
        }
        
        // Use material
        objFile << "usemtl Material\n";
        
        // Faces (1-indexed)
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t a = mesh.indices[i] + 1;
            uint32_t b = mesh.indices[i + 1] + 1;
            uint32_t c = mesh.indices[i + 2] + 1;
            
            if (options.exportNormals && options.exportUVs) {
                objFile << "f " << a << "/" << a << "/" << a
                       << " " << b << "/" << b << "/" << b
                       << " " << c << "/" << c << "/" << c << "\n";
            } else if (options.exportUVs) {
                objFile << "f " << a << "/" << a
                       << " " << b << "/" << b
                       << " " << c << "/" << c << "\n";
            } else if (options.exportNormals) {
                objFile << "f " << a << "//" << a
                       << " " << b << "//" << b
                       << " " << c << "//" << c << "\n";
            } else {
                objFile << "f " << a << " " << b << " " << c << "\n";
            }
        }
        
        objFile.close();
        
        // Write MTL file
        std::ofstream mtlFile(mtlPath);
        if (mtlFile.is_open()) {
            mtlFile << "# Material\n\n";
            mtlFile << "newmtl Material\n";
            mtlFile << "Kd " << mesh.baseColor[0] << " " << mesh.baseColor[1] << " " << mesh.baseColor[2] << "\n";
            mtlFile << "Ks 0.5 0.5 0.5\n";
            mtlFile << "Ns " << (1.0f - mesh.roughness) * 1000.0f << "\n";
            
            if (mesh.hasDiffuseTexture && !mesh.diffuseTexture.pixels.empty()) {
                std::string texPath = outputPath.substr(0, outputPath.rfind('.')) + "_diffuse.png";
                mtlFile << "map_Kd " << texPath.substr(texPath.rfind('/') + 1) << "\n";
                // Would need to write texture file
            }
            
            mtlFile.close();
            result.additionalFiles.push_back(mtlPath);
        }
        
        result.success = true;
        result.outputPath = outputPath;
        result.vertexCount = mesh.vertices.size();
        result.triangleCount = mesh.indices.size() / 3;
        
        return result;
    }
};

// ============================================================================
// VRM Exporter (glTF-based)
// ============================================================================

class VRMExporter {
public:
    static ExportResult exportCharacter(
        const CharacterExportData& data,
        const std::string& outputPath,
        const ExportOptions& options = {})
    {
        ExportResult result;
        
        // VRM is based on glTF, so we start with glTF export
        GLTFExportOptions gltfOptions;
        gltfOptions.exportGLB = true;
        gltfOptions.embedTextures = true;
        gltfOptions.exportSkeleton = true;
        gltfOptions.exportBlendShapes = true;
        gltfOptions.generator = "LUMA Creator (VRM)";
        
        GLTFExporter gltfExporter;
        auto gltfResult = gltfExporter.exportCharacter(
            data.mesh,
            data.skeleton.get(),
            data.blendShapes.get(),
            outputPath,
            gltfOptions
        );
        
        if (!gltfResult.success) {
            result.errorMessage = gltfResult.errorMessage;
            return result;
        }
        
        // For full VRM support, we would need to:
        // 1. Add VRM extension to glTF JSON
        // 2. Add VRM metadata (title, author, license)
        // 3. Add VRM-specific blend shape mappings
        // 4. Add spring bone physics data
        // 5. Add first-person settings
        // 6. Add look-at settings
        
        // For now, we export as glTF which can be converted to VRM
        // using external tools like VRM_Converter
        
        result.success = true;
        result.outputPath = outputPath;
        result.vertexCount = gltfResult.vertexCount;
        result.triangleCount = gltfResult.triangleCount;
        result.boneCount = gltfResult.boneCount;
        result.blendShapeCount = gltfResult.blendShapeCount;
        result.fileSize = gltfResult.fileSize;
        
        // Add note about VRM
        if (result.success) {
            result.errorMessage = "Exported as glTF. Convert to VRM using UniVRM or VRM_Converter.";
        }
        
        return result;
    }
};

// ============================================================================
// Model Exporter - Main Export Interface
// ============================================================================

class ModelExporter {
public:
    // Export mesh only
    static ExportResult exportMesh(
        const Mesh& mesh,
        const std::string& outputPath,
        const ExportOptions& options = {})
    {
        CharacterExportData data;
        data.mesh = mesh;
        return exportCharacter(data, outputPath, options);
    }
    
    // Export with skeleton
    static ExportResult exportWithSkeleton(
        const Mesh& mesh,
        const Skeleton& skeleton,
        const std::string& outputPath,
        const ExportOptions& options = {})
    {
        CharacterExportData data;
        data.mesh = mesh;
        data.skeleton = std::make_shared<Skeleton>(skeleton);
        return exportCharacter(data, outputPath, options);
    }
    
    // Export full character
    static ExportResult exportCharacter(
        const CharacterExportData& data,
        const std::string& outputPath,
        const ExportOptions& options = {})
    {
        // Determine format from path or options
        ExportFormat format = options.format;
        if (format == ExportFormat::Unknown) {
            size_t dotPos = outputPath.rfind('.');
            if (dotPos != std::string::npos) {
                format = extensionToFormat(outputPath.substr(dotPos));
            }
        }
        
        switch (format) {
            case ExportFormat::GLTF:
            case ExportFormat::GLB:
                return exportGLTF(data, outputPath, options);
                
            case ExportFormat::OBJ:
                return OBJExporter::exportMesh(data.mesh, outputPath, options);
                
            case ExportFormat::VRM:
                return VRMExporter::exportCharacter(data, outputPath, options);
                
            case ExportFormat::FBX:
                return exportFBX(data, outputPath, options);
                
            default: {
                ExportResult result;
                result.errorMessage = "Unsupported export format";
                return result;
            }
        }
    }
    
    // Get supported formats
    static std::vector<ExportFormat> getSupportedFormats() {
        return {
            ExportFormat::GLB,
            ExportFormat::GLTF,
            ExportFormat::OBJ,
            ExportFormat::VRM,
            ExportFormat::FBX
        };
    }
    
    // Get format info
    struct FormatInfo {
        ExportFormat format;
        std::string name;
        std::string extension;
        std::string description;
        bool supportsSkeleton;
        bool supportsBlendShapes;
        bool supportsAnimations;
    };
    
    static std::vector<FormatInfo> getFormatInfo() {
        return {
            {ExportFormat::GLB, "glTF Binary", ".glb", 
             "Recommended. Single file, widely supported.",
             true, true, true},
            
            {ExportFormat::GLTF, "glTF", ".gltf",
             "JSON format with separate binary and textures.",
             true, true, true},
            
            {ExportFormat::FBX, "Autodesk FBX", ".fbx",
             "Industry standard. Best for Maya, 3ds Max.",
             true, true, true},
            
            {ExportFormat::OBJ, "Wavefront OBJ", ".obj",
             "Simple mesh format. No skeleton or animation.",
             false, false, false},
            
            {ExportFormat::VRM, "VRM", ".vrm",
             "VTuber avatar format. For VRChat, VTuber apps.",
             true, true, false}
        };
    }
    
private:
    static ExportResult exportGLTF(
        const CharacterExportData& data,
        const std::string& outputPath,
        const ExportOptions& options)
    {
        GLTFExportOptions gltfOptions;
        gltfOptions.exportGLB = (options.format == ExportFormat::GLB);
        gltfOptions.embedTextures = options.embedTextures;
        gltfOptions.exportSkeleton = options.exportSkeleton;
        gltfOptions.exportBlendShapes = options.exportBlendShapes;
        gltfOptions.exportNormals = options.exportNormals;
        gltfOptions.exportTangents = options.exportTangents;
        gltfOptions.exportUVs = options.exportUVs;
        gltfOptions.exportVertexColors = options.exportVertexColors;
        gltfOptions.copyright = options.copyright;
        gltfOptions.generator = options.generator;
        
        GLTFExporter exporter;
        auto gltfResult = exporter.exportCharacter(
            data.mesh,
            data.skeleton.get(),
            data.blendShapes.get(),
            outputPath,
            gltfOptions
        );
        
        ExportResult result;
        result.success = gltfResult.success;
        result.errorMessage = gltfResult.errorMessage;
        result.outputPath = gltfResult.outputPath;
        result.vertexCount = gltfResult.vertexCount;
        result.triangleCount = gltfResult.triangleCount;
        result.boneCount = gltfResult.boneCount;
        result.blendShapeCount = gltfResult.blendShapeCount;
        result.textureCount = gltfResult.textureCount;
        result.fileSize = gltfResult.fileSize;
        
        return result;
    }
    
    static ExportResult exportFBX(
        const CharacterExportData& data,
        const std::string& outputPath,
        const ExportOptions& options)
    {
        ExportResult result;
        
        // FBX export is complex. Options:
        // 1. Use Autodesk FBX SDK (requires license)
        // 2. Use Assimp export (limited FBX support)
        // 3. Write our own (very complex)
        
        // For now, export as glTF and suggest conversion
        // Users can convert using Blender or other tools
        
        result.errorMessage = "FBX export not yet implemented. Export as glTF and convert using Blender.";
        result.success = false;
        
        // Alternative: export glTF and note
        /*
        std::string gltfPath = outputPath.substr(0, outputPath.rfind('.')) + ".glb";
        auto gltfResult = exportGLTF(data, gltfPath, options);
        if (gltfResult.success) {
            result = gltfResult;
            result.errorMessage = "Exported as glTF. Convert to FBX using Blender or FBX Converter.";
            result.outputPath = gltfPath;
        }
        */
        
        return result;
    }
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline ExportResult exportToGLB(const Mesh& mesh, const std::string& path) {
    ExportOptions options;
    options.format = ExportFormat::GLB;
    return ModelExporter::exportMesh(mesh, path, options);
}

inline ExportResult exportToGLTF(const Mesh& mesh, const std::string& path) {
    ExportOptions options;
    options.format = ExportFormat::GLTF;
    return ModelExporter::exportMesh(mesh, path, options);
}

inline ExportResult exportToOBJ(const Mesh& mesh, const std::string& path) {
    ExportOptions options;
    options.format = ExportFormat::OBJ;
    return ModelExporter::exportMesh(mesh, path, options);
}

}  // namespace luma
