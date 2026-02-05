// LUMA Shader Manager
// Unified shader loading, compilation, caching and hot-reload support

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

namespace luma {

// Shader types
enum class ShaderType {
    Vertex,
    Pixel,
    Compute,
    Geometry,
    Hull,
    Domain
};

// Compiled shader data (platform-agnostic interface)
struct CompiledShader {
    std::string name;
    ShaderType type;
    std::vector<uint8_t> bytecode;
    bool valid = false;
    
    const void* getByteCodePointer() const {
        return bytecode.empty() ? nullptr : bytecode.data();
    }
    
    size_t getByteCodeSize() const {
        return bytecode.size();
    }
};

// Shader source with metadata
struct ShaderSource {
    std::string name;
    std::string source;
    std::string filePath;  // For hot-reload tracking
    bool loaded = false;
};

// Shader Manager singleton
class ShaderManager {
public:
    static ShaderManager& instance();
    
    // Initialize with base path for shader files
    void init(const std::string& basePath = "engine/renderer/shaders/");
    
    // Load shader source from file or return embedded fallback
    ShaderSource loadSource(const std::string& name, const char* embeddedFallback = nullptr);
    
    // Compile shader (platform-specific implementation in .cpp)
    CompiledShader* compile(const std::string& name, const std::string& source, 
                           const std::string& entryPoint, ShaderType type);
    
    // Compile from file with optional fallback
    CompiledShader* compileFromFile(const std::string& filename, const std::string& entryPoint,
                                   ShaderType type, const char* embeddedFallback = nullptr);
    
    // Get cached compiled shader
    CompiledShader* getShader(const std::string& key);
    
    // Hot-reload support
    void watchForChanges();
    void recompileAll();
    void setRecompileCallback(std::function<void()> callback) { recompileCallback = callback; }
    
    // Clear all cached shaders
    void clearCache();
    
    // Get base path
    const std::string& getBasePath() const { return basePath; }
    
private:
    ShaderManager() = default;
    ~ShaderManager() = default;
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    std::string basePath;
    std::unordered_map<std::string, std::unique_ptr<CompiledShader>> shaderCache;
    std::unordered_map<std::string, ShaderSource> sourceCache;
    std::function<void()> recompileCallback;
    
    static const char* getShaderModel(ShaderType type);
};

} // namespace luma
