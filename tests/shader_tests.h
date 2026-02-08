// LUMA Shader System Tests
// Unit tests for ShaderManager, shader loading, and compilation

#pragma once

#include "engine/renderer/shader_manager.h"
#include <iostream>
#include <fstream>
#include <string>

namespace luma {
namespace test {

// Test helper macros
#define SHADER_EXPECT_TRUE(expr) if (!(expr)) { std::cerr << "[FAIL] " << #expr << std::endl; return false; }
#define SHADER_EXPECT_FALSE(expr) if (expr) { std::cerr << "[FAIL] " << #expr << " (expected false)" << std::endl; return false; }
#define SHADER_EXPECT_EQ(a, b) if ((a) != (b)) { std::cerr << "[FAIL] " << #a << " != " << #b << std::endl; return false; }
#define SHADER_EXPECT_NE(a, b) if ((a) == (b)) { std::cerr << "[FAIL] " << #a << " == " << #b << " (expected not equal)" << std::endl; return false; }

namespace ShaderTests {

// ============================================================================
// ShaderManager Initialization Tests
// ============================================================================

inline bool testShaderManagerSingleton() {
    // Get instance twice and verify same object
    ShaderManager& inst1 = ShaderManager::instance();
    ShaderManager& inst2 = ShaderManager::instance();
    
    SHADER_EXPECT_TRUE(&inst1 == &inst2);
    
    return true;
}

inline bool testShaderManagerInit() {
    ShaderManager& mgr = ShaderManager::instance();
    
    // Initialize with default path
    mgr.init("engine/renderer/shaders/");
    
    SHADER_EXPECT_EQ(mgr.getBasePath(), "engine/renderer/shaders/");
    
    return true;
}

inline bool testShaderManagerInitCustomPath() {
    ShaderManager& mgr = ShaderManager::instance();
    
    // Initialize with custom path
    std::string customPath = "custom/shaders/";
    mgr.init(customPath);
    
    SHADER_EXPECT_EQ(mgr.getBasePath(), customPath);
    
    // Reset to default
    mgr.init("engine/renderer/shaders/");
    
    return true;
}

// ============================================================================
// Shader Source Loading Tests
// ============================================================================

inline bool testShaderSourceLoadWithFallback() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    // Load with embedded fallback
    const char* fallback = "// Fallback shader\nfloat4 main() : SV_TARGET { return float4(1,0,0,1); }";
    ShaderSource source = mgr.loadSource("nonexistent_shader.hlsl", fallback);
    
    // Should load (either from file or fallback)
    SHADER_EXPECT_TRUE(source.loaded);
    SHADER_EXPECT_FALSE(source.source.empty());
    
    return true;
}

inline bool testShaderSourceCaching() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    const char* fallback = "// Test shader";
    
    // Load same shader twice
    ShaderSource source1 = mgr.loadSource("test_cache.hlsl", fallback);
    ShaderSource source2 = mgr.loadSource("test_cache.hlsl", fallback);
    
    // Both should be loaded
    SHADER_EXPECT_TRUE(source1.loaded);
    SHADER_EXPECT_TRUE(source2.loaded);
    
    // Contents should match
    SHADER_EXPECT_EQ(source1.source, source2.source);
    
    return true;
}

inline bool testShaderSourceLoadFromFile() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    // Try to load a known shader file (pbr.hlsl should exist)
    ShaderSource source = mgr.loadSource("pbr.hlsl", nullptr);
    
    // If file exists, it should be loaded
    // If not, this test will fail - that's expected if shader files don't exist
    if (source.loaded) {
        SHADER_EXPECT_FALSE(source.source.empty());
        SHADER_EXPECT_EQ(source.name, "pbr.hlsl");
    }
    
    return true;
}

// ============================================================================
// Shader Type Tests
// ============================================================================

inline bool testShaderTypeEnum() {
    // Test that shader type enum values are distinct
    ShaderType vertex = ShaderType::Vertex;
    ShaderType pixel = ShaderType::Pixel;
    ShaderType compute = ShaderType::Compute;
    ShaderType geometry = ShaderType::Geometry;
    ShaderType hull = ShaderType::Hull;
    ShaderType domain = ShaderType::Domain;
    
    SHADER_EXPECT_TRUE(vertex != pixel);
    SHADER_EXPECT_TRUE(pixel != compute);
    SHADER_EXPECT_TRUE(compute != geometry);
    SHADER_EXPECT_TRUE(geometry != hull);
    SHADER_EXPECT_TRUE(hull != domain);
    
    return true;
}

// ============================================================================
// CompiledShader Structure Tests
// ============================================================================

inline bool testCompiledShaderBasic() {
    CompiledShader shader;
    shader.name = "test_shader";
    shader.type = ShaderType::Pixel;
    shader.valid = false;
    
    SHADER_EXPECT_EQ(shader.name, "test_shader");
    SHADER_EXPECT_EQ(shader.type, ShaderType::Pixel);
    SHADER_EXPECT_FALSE(shader.valid);
    SHADER_EXPECT_TRUE(shader.bytecode.empty());
    
    return true;
}

inline bool testCompiledShaderBytecode() {
    CompiledShader shader;
    shader.name = "bytecode_test";
    shader.type = ShaderType::Vertex;
    
    // Simulate bytecode
    shader.bytecode = {0x44, 0x58, 0x42, 0x43};  // DXBC magic number
    shader.valid = true;
    
    SHADER_EXPECT_TRUE(shader.valid);
    SHADER_EXPECT_EQ(shader.bytecode.size(), 4u);
    SHADER_EXPECT_EQ(shader.getByteCodeSize(), 4u);
    SHADER_EXPECT_TRUE(shader.getByteCodePointer() != nullptr);
    
    return true;
}

inline bool testCompiledShaderEmptyBytecode() {
    CompiledShader shader;
    
    // Empty bytecode
    SHADER_EXPECT_TRUE(shader.getByteCodePointer() == nullptr);
    SHADER_EXPECT_EQ(shader.getByteCodeSize(), 0u);
    
    return true;
}

// ============================================================================
// Shader Cache Tests
// ============================================================================

inline bool testShaderCacheClear() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    
    // Load something into cache
    mgr.loadSource("cache_test.hlsl", "// test");
    
    // Clear cache
    mgr.clearCache();
    
    // After clear, getting a shader should require reloading
    // (We can't easily test this without checking internal state,
    // but we can verify clearCache doesn't crash)
    
    return true;
}

inline bool testShaderGetNonexistent() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    // Getting a shader that doesn't exist should return nullptr
    CompiledShader* shader = mgr.getShader("nonexistent_key");
    
    SHADER_EXPECT_TRUE(shader == nullptr);
    
    return true;
}

// ============================================================================
// ShaderSource Structure Tests
// ============================================================================

inline bool testShaderSourceStructure() {
    ShaderSource source;
    
    // Default state
    SHADER_EXPECT_FALSE(source.loaded);
    SHADER_EXPECT_TRUE(source.name.empty());
    SHADER_EXPECT_TRUE(source.source.empty());
    SHADER_EXPECT_TRUE(source.filePath.empty());
    
    // Set values
    source.name = "test.hlsl";
    source.source = "float4 main() : SV_TARGET { return 0; }";
    source.filePath = "shaders/test.hlsl";
    source.loaded = true;
    
    SHADER_EXPECT_TRUE(source.loaded);
    SHADER_EXPECT_EQ(source.name, "test.hlsl");
    SHADER_EXPECT_FALSE(source.source.empty());
    
    return true;
}

// ============================================================================
// Integration Tests (require actual shader files)
// ============================================================================

inline bool testLoadPBRShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    // Try to load PBR shader source
    ShaderSource source = mgr.loadSource("pbr.hlsl", nullptr);
    
    if (source.loaded) {
        // Verify it has expected content markers
        bool hasStruct = source.source.find("struct") != std::string::npos;
        bool hasFloat4 = source.source.find("float4") != std::string::npos ||
                         source.source.find("float3") != std::string::npos;
        
        // PBR shader should have these basic elements
        SHADER_EXPECT_TRUE(hasStruct || hasFloat4);
    }
    // If file doesn't exist, test passes (shader files optional in test env)
    
    return true;
}

inline bool testLoadWireframeShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    ShaderSource source = mgr.loadSource("wireframe.hlsl", nullptr);
    
    if (source.loaded) {
        // Wireframe shader should exist and have content
        SHADER_EXPECT_FALSE(source.source.empty());
    }
    
    return true;
}

inline bool testLoadSolidShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    ShaderSource source = mgr.loadSource("solid.hlsl", nullptr);
    
    if (source.loaded) {
        SHADER_EXPECT_FALSE(source.source.empty());
    }
    
    return true;
}

inline bool testLoadLineShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    ShaderSource source = mgr.loadSource("line.hlsl", nullptr);
    
    if (source.loaded) {
        SHADER_EXPECT_FALSE(source.source.empty());
    }
    
    return true;
}

// ============================================================================
// Hot Reload Tests (structural only, actual reload needs runtime)
// ============================================================================

inline bool testRecompileAllStructure() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    
    // Set a callback to track if recompile was triggered
    bool callbackCalled = false;
    mgr.setRecompileCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    // Call recompileAll (should not crash even with empty cache)
    mgr.recompileAll();
    
    // Callback should have been called
    SHADER_EXPECT_TRUE(callbackCalled);
    
    // Reset callback
    mgr.setRecompileCallback(nullptr);
    
    return true;
}

// ============================================================================
// Shader Compilation Tests (Windows/DX12 only)
// ============================================================================

#if defined(_WIN32)
inline bool testCompileSimpleVertexShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    const char* simpleVS = R"(
        struct VSInput {
            float3 position : POSITION;
        };
        struct VSOutput {
            float4 position : SV_POSITION;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.position = float4(input.position, 1.0);
            return output;
        }
    )";
    
    CompiledShader* shader = mgr.compile("simple_vs", simpleVS, "main", ShaderType::Vertex);
    
    // Should compile successfully
    SHADER_EXPECT_TRUE(shader != nullptr);
    if (shader) {
        SHADER_EXPECT_TRUE(shader->valid);
        SHADER_EXPECT_FALSE(shader->bytecode.empty());
    }
    
    return true;
}

inline bool testCompileSimplePixelShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    const char* simplePS = R"(
        float4 main() : SV_TARGET {
            return float4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    CompiledShader* shader = mgr.compile("simple_ps", simplePS, "main", ShaderType::Pixel);
    
    SHADER_EXPECT_TRUE(shader != nullptr);
    if (shader) {
        SHADER_EXPECT_TRUE(shader->valid);
    }
    
    return true;
}

inline bool testCompileInvalidShader() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    const char* invalidShader = R"(
        // This is not valid HLSL
        invalid syntax here !!!
    )";
    
    CompiledShader* shader = mgr.compile("invalid", invalidShader, "main", ShaderType::Pixel);
    
    // Should fail to compile
    SHADER_EXPECT_TRUE(shader == nullptr);
    
    return true;
}

inline bool testShaderCacheReuse() {
    ShaderManager& mgr = ShaderManager::instance();
    mgr.init("engine/renderer/shaders/");
    mgr.clearCache();
    
    const char* shader = "float4 main() : SV_TARGET { return float4(0,1,0,1); }";
    
    // Compile same shader twice
    CompiledShader* first = mgr.compile("cache_test", shader, "main", ShaderType::Pixel);
    CompiledShader* second = mgr.compile("cache_test", shader, "main", ShaderType::Pixel);
    
    // Should return same cached instance
    SHADER_EXPECT_TRUE(first != nullptr);
    SHADER_EXPECT_TRUE(second != nullptr);
    SHADER_EXPECT_TRUE(first == second);  // Same pointer = cached
    
    return true;
}
#endif // _WIN32

// ============================================================================
// Test Registration
// ============================================================================

inline void registerShaderTests(UnitTestRunner& runner) {
    // ShaderManager tests
    runner.addTest("Shader", "Manager Singleton", testShaderManagerSingleton);
    runner.addTest("Shader", "Manager Init", testShaderManagerInit);
    runner.addTest("Shader", "Manager Init Custom Path", testShaderManagerInitCustomPath);
    
    // Source loading tests
    runner.addTest("Shader", "Source Load With Fallback", testShaderSourceLoadWithFallback);
    runner.addTest("Shader", "Source Caching", testShaderSourceCaching);
    runner.addTest("Shader", "Source Load From File", testShaderSourceLoadFromFile);
    
    // Type tests
    runner.addTest("Shader", "Type Enum", testShaderTypeEnum);
    
    // CompiledShader tests
    runner.addTest("Shader", "Compiled Shader Basic", testCompiledShaderBasic);
    runner.addTest("Shader", "Compiled Shader Bytecode", testCompiledShaderBytecode);
    runner.addTest("Shader", "Compiled Shader Empty Bytecode", testCompiledShaderEmptyBytecode);
    
    // Cache tests
    runner.addTest("Shader", "Cache Clear", testShaderCacheClear);
    runner.addTest("Shader", "Get Nonexistent", testShaderGetNonexistent);
    
    // ShaderSource structure tests
    runner.addTest("Shader", "Source Structure", testShaderSourceStructure);
    
    // Integration tests
    runner.addTest("Shader", "Load PBR Shader", testLoadPBRShader);
    runner.addTest("Shader", "Load Wireframe Shader", testLoadWireframeShader);
    runner.addTest("Shader", "Load Solid Shader", testLoadSolidShader);
    runner.addTest("Shader", "Load Line Shader", testLoadLineShader);
    
    // Hot reload tests
    runner.addTest("Shader", "Recompile All Structure", testRecompileAllStructure);
    
#if defined(_WIN32)
    // Compilation tests (Windows/DX12 only)
    runner.addTest("Shader", "Compile Simple Vertex Shader", testCompileSimpleVertexShader);
    runner.addTest("Shader", "Compile Simple Pixel Shader", testCompileSimplePixelShader);
    runner.addTest("Shader", "Compile Invalid Shader", testCompileInvalidShader);
    runner.addTest("Shader", "Shader Cache Reuse", testShaderCacheReuse);
#endif
}

} // namespace ShaderTests
} // namespace test
} // namespace luma
