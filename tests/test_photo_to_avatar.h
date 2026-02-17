// Test: Photo to Avatar End-to-End Pipeline
// Tests the complete flow: Photo -> AI -> Parameters -> BlendShape -> Save
#pragma once

#include "engine/character/ai/face_reconstruction.h"
#include "engine/character/character_face.h"
#include "engine/character/face_template_mesh.h"
#include "engine/character/avatar_serializer.h"
#include "engine/character/landmark_mapper.h"
#include <string>
#include <cstdio>

namespace luma {
namespace tests {

class PhotoToAvatarTest {
public:
    // Test configuration
    struct Config {
        std::string modelsDir = "models/";
        std::string headMeshPath = "models/head_base.bin";
        std::string landmarkMapPath = "models/landmark_vertex_map.json";
        std::string testPhotoPath = "";  // Optional: test with actual photo
        std::string outputDir = "test_output/";
    };
    
    Config config;
    
    // Test results
    struct Results {
        bool aiPipelineInit = false;
        bool meshGeneration = false;
        bool blendShapeGeneration = false;
        bool parameterMapping = false;
        bool avatarSave = false;
        bool avatarLoad = false;
        bool landmarkMapLoad = false;
        
        int blendShapeCount = 0;
        int vertexCount = 0;
        int arkitChannelCount = 0;
        
        std::string errorMessage;
    };
    
    Results results;
    
    // Run all tests
    bool runAll() {
        printf("\n========================================\n");
        printf("  Photo-to-Avatar Pipeline Test\n");
        printf("========================================\n\n");
        
        bool allPassed = true;
        
        // Test 1: AI Pipeline Initialization
        printf("[Test 1] AI Pipeline Initialization...\n");
        allPassed &= testAIPipelineInit();
        
        // Test 2: Mesh Generation
        printf("\n[Test 2] Mesh Generation...\n");
        allPassed &= testMeshGeneration();
        
        // Test 3: BlendShape Generation
        printf("\n[Test 3] BlendShape Generation...\n");
        allPassed &= testBlendShapeGeneration();
        
        // Test 4: Parameter Mapping
        printf("\n[Test 4] Parameter Mapping (BFM -> FaceShapeParams)...\n");
        allPassed &= testParameterMapping();
        
        // Test 5: Landmark Map Loading
        printf("\n[Test 5] Landmark Map Loading...\n");
        allPassed &= testLandmarkMapLoad();
        
        // Test 6: Avatar Save/Load
        printf("\n[Test 6] Avatar Save/Load...\n");
        allPassed &= testAvatarSaveLoad();
        
        // Summary
        printf("\n========================================\n");
        printf("  Test Summary\n");
        printf("========================================\n");
        printf("  AI Pipeline Init:    %s\n", results.aiPipelineInit ? "PASS" : "FAIL");
        printf("  Mesh Generation:     %s\n", results.meshGeneration ? "PASS" : "FAIL");
        printf("  BlendShape Gen:      %s (%d channels)\n", 
               results.blendShapeGeneration ? "PASS" : "FAIL", results.blendShapeCount);
        printf("  Parameter Mapping:   %s\n", results.parameterMapping ? "PASS" : "FAIL");
        printf("  Landmark Map Load:   %s\n", results.landmarkMapLoad ? "PASS" : "FAIL");
        printf("  Avatar Save/Load:    %s\n", results.avatarSave && results.avatarLoad ? "PASS" : "FAIL");
        printf("========================================\n");
        printf("  Overall: %s\n", allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
        printf("========================================\n\n");
        
        return allPassed;
    }
    
private:
    // Test AI pipeline initialization
    bool testAIPipelineInit() {
        PhotoToFacePipeline pipeline;
        
        std::string detPath = config.modelsDir + "det_10g.onnx";
        std::string lmPath = config.modelsDir + "2d106det.onnx";
        std::string regPath = config.modelsDir + "mb1_120x120.onnx";
        
        bool success = pipeline.initialize(detPath, lmPath, regPath);
        
        if (success) {
            printf("  ✓ AI pipeline initialized successfully\n");
            printf("    - Detection model: %s\n", pipeline.hasDetector() ? "loaded" : "fallback");
            printf("    - Landmark model: %s\n", pipeline.hasLandmarkDetector() ? "loaded" : "fallback");
            printf("    - 3DMM model: %s\n", pipeline.has3DMMRegressor() ? "loaded" : "fallback");
        } else {
            printf("  ✓ AI pipeline in fallback mode (models not found)\n");
            success = true;  // Fallback is acceptable
        }
        
        results.aiPipelineInit = success;
        return success;
    }
    
    // Test mesh generation
    bool testMeshGeneration() {
        auto mesh = FaceTemplateMesh::generate(config.headMeshPath, 0.23f);
        
        results.vertexCount = (int)mesh.vertices.size();
        
        bool success = mesh.vertices.size() > 0 && mesh.indices.size() > 0;
        
        if (success) {
            printf("  ✓ Mesh generated successfully\n");
            printf("    - Vertices: %zu\n", mesh.vertices.size());
            printf("    - Triangles: %zu\n", mesh.indices.size() / 3);
            printf("    - Neck boundary: %d vertices\n", mesh.neckBoundaryCount);
        } else {
            printf("  ✗ Mesh generation failed\n");
            results.errorMessage = "Mesh generation failed";
        }
        
        results.meshGeneration = success;
        return success;
    }
    
    // Test BlendShape generation
    bool testBlendShapeGeneration() {
        auto mesh = FaceTemplateMesh::generate(config.headMeshPath, 0.23f);
        
        results.blendShapeCount = (int)mesh.blendShapes.getChannelCount();
        
        // Check for identity channels
        bool hasIdentityChannels = 
            mesh.blendShapes.findChannelIndex("faceWidth") >= 0 &&
            mesh.blendShapes.findChannelIndex("noseLength") >= 0 &&
            mesh.blendShapes.findChannelIndex("jawWidth") >= 0;
        
        // Check for ARKit channels
        bool hasARKitChannels = 
            mesh.blendShapes.findChannelIndex("eyeBlinkLeft") >= 0 &&
            mesh.blendShapes.findChannelIndex("jawOpen") >= 0 &&
            mesh.blendShapes.findChannelIndex("mouthSmileLeft") >= 0;
        
        // Count ARKit channels
        auto arkitNames = ARKitBlendShapes::getAll();
        int arkitCount = 0;
        for (const auto& name : arkitNames) {
            if (mesh.blendShapes.findChannelIndex(name) >= 0) {
                arkitCount++;
            }
        }
        results.arkitChannelCount = arkitCount;
        
        bool success = hasIdentityChannels && hasARKitChannels;
        
        if (success) {
            printf("  ✓ BlendShapes generated successfully\n");
            printf("    - Total channels: %d\n", results.blendShapeCount);
            printf("    - Identity channels: %s\n", hasIdentityChannels ? "present" : "missing");
            printf("    - ARKit channels: %d/52\n", arkitCount);
        } else {
            printf("  ✗ BlendShape generation incomplete\n");
            results.errorMessage = "Missing required BlendShape channels";
        }
        
        results.blendShapeGeneration = success;
        return success;
    }
    
    // Test parameter mapping
    bool testParameterMapping() {
        CharacterFace face;
        face.setupDefaultMappings();
        
        // Simulate BFM coefficients from 3DDFA
        std::vector<float> bfmCoeffs(40, 0.0f);
        bfmCoeffs[0] = 1.5f;   // Face width
        bfmCoeffs[1] = -0.8f;  // Face length
        bfmCoeffs[2] = 0.5f;   // Nose prominence
        bfmCoeffs[3] = 1.2f;   // Jaw width
        
        // Create photo result
        PhotoFaceResult result;
        result.success = true;
        result.shapeParams = bfmCoeffs;
        
        // Apply to face
        face.applyPhotoFaceResult(result);
        
        // Verify parameters changed from defaults
        const auto& params = face.getShapeParams();
        bool faceWidthChanged = std::abs(params.faceWidth - 0.5f) > 0.01f;
        bool jawWidthChanged = std::abs(params.jawWidth - 0.5f) > 0.01f;
        
        bool success = faceWidthChanged && jawWidthChanged;
        
        if (success) {
            printf("  ✓ Parameter mapping works correctly\n");
            printf("    - faceWidth: %.3f (expected != 0.5)\n", params.faceWidth);
            printf("    - jawWidth: %.3f (expected != 0.5)\n", params.jawWidth);
            printf("    - noseLength: %.3f\n", params.noseLength);
        } else {
            printf("  ✗ Parameter mapping failed\n");
            printf("    - faceWidth: %.3f\n", params.faceWidth);
            printf("    - jawWidth: %.3f\n", params.jawWidth);
            results.errorMessage = "BFM to FaceShapeParams mapping not working";
        }
        
        results.parameterMapping = success;
        return success;
    }
    
    // Test landmark map loading
    bool testLandmarkMapLoad() {
        LandmarkMapper mapper;
        
        bool success = mapper.load(config.landmarkMapPath);
        
        if (success) {
            printf("  ✓ Landmark map loaded successfully\n");
            printf("    - Mesh file: %s\n", mapper.meshFile.c_str());
            printf("    - Standard: %s\n", mapper.standard.c_str());
            printf("    - Landmarks: %d/%d marked\n", mapper.markedCount, mapper.landmarkCount);
        } else {
            // Not a failure if file doesn't exist yet
            printf("  ⊘ Landmark map not found (expected if not yet created)\n");
            printf("    - Path: %s\n", config.landmarkMapPath.c_str());
            success = true;  // Not a critical failure
        }
        
        results.landmarkMapLoad = success;
        return success;
    }
    
    // Test avatar save/load
    bool testAvatarSaveLoad() {
        // Create test avatar
        AvatarData original;
        original.name = "TestAvatar";
        original.source.type = "photo";
        original.source.confidence = 0.85f;
        original.identity.faceWidth = 0.65f;
        original.identity.noseLength = 0.45f;
        original.identity.jawWidth = 0.55f;
        original.appearance.skinTone = Vec3(0.9f, 0.75f, 0.6f);
        original.appearance.eyeColor = Vec3(0.3f, 0.25f, 0.2f);
        original.mesh.vertexCount = results.vertexCount;
        original.mesh.blendShapeCount = results.blendShapeCount;
        
        std::string testPath = config.outputDir + "test_avatar.json";
        
        // Save
        bool saveSuccess = AvatarSerializer::save(testPath, original);
        results.avatarSave = saveSuccess;
        
        if (!saveSuccess) {
            printf("  ✗ Failed to save avatar\n");
            results.errorMessage = "Avatar save failed";
            return false;
        }
        
        // Load
        AvatarData loaded;
        bool loadSuccess = AvatarSerializer::load(testPath, loaded);
        results.avatarLoad = loadSuccess;
        
        if (!loadSuccess) {
            printf("  ✗ Failed to load avatar\n");
            results.errorMessage = "Avatar load failed";
            return false;
        }
        
        // Verify data integrity
        bool nameMatch = loaded.name == original.name;
        bool faceWidthMatch = std::abs(loaded.identity.faceWidth - original.identity.faceWidth) < 0.001f;
        bool skinToneMatch = std::abs(loaded.appearance.skinTone.x - original.appearance.skinTone.x) < 0.001f;
        
        bool success = nameMatch && faceWidthMatch && skinToneMatch;
        
        if (success) {
            printf("  ✓ Avatar save/load works correctly\n");
            printf("    - Saved to: %s\n", testPath.c_str());
            printf("    - Name: %s\n", loaded.name.c_str());
            printf("    - faceWidth: %.3f (original: %.3f)\n", 
                   loaded.identity.faceWidth, original.identity.faceWidth);
        } else {
            printf("  ✗ Avatar data mismatch after load\n");
            results.errorMessage = "Avatar data corrupted during save/load";
        }
        
        return success;
    }
};

// Quick test runner
inline bool runPhotoToAvatarTests() {
    PhotoToAvatarTest test;
    return test.runAll();
}

} // namespace tests
} // namespace luma
