// LUMA Shader Manager Implementation

#include "shader_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace luma {

ShaderManager& ShaderManager::instance() {
    static ShaderManager inst;
    return inst;
}

void ShaderManager::init(const std::string& path) {
    basePath = path;
    std::cout << "[ShaderManager] Initialized with base path: " << basePath << std::endl;
}

ShaderSource ShaderManager::loadSource(const std::string& name, const char* embeddedFallback) {
    // Check cache first
    auto it = sourceCache.find(name);
    if (it != sourceCache.end()) {
        return it->second;
    }
    
    ShaderSource source;
    source.name = name;
    source.filePath = basePath + name;
    source.loaded = false;
    
    // Try to load from file
    std::ifstream file(source.filePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        source.source = buffer.str();
        source.loaded = true;
        std::cout << "[ShaderManager] Loaded shader from file: " << source.filePath << std::endl;
    } else if (embeddedFallback) {
        // Use embedded fallback
        source.source = embeddedFallback;
        source.loaded = true;
        std::cout << "[ShaderManager] Using embedded fallback for: " << name << std::endl;
    } else {
        std::cerr << "[ShaderManager] Failed to load shader: " << name << std::endl;
    }
    
    // Cache the source
    sourceCache[name] = source;
    return source;
}

const char* ShaderManager::getShaderModel(ShaderType type) {
    switch (type) {
        case ShaderType::Vertex:   return "vs_5_0";
        case ShaderType::Pixel:    return "ps_5_0";
        case ShaderType::Compute:  return "cs_5_0";
        case ShaderType::Geometry: return "gs_5_0";
        case ShaderType::Hull:     return "hs_5_0";
        case ShaderType::Domain:   return "ds_5_0";
        default: return "vs_5_0";
    }
}

static std::string makeCacheKey(const std::string& name, const std::string& entryPoint, ShaderType type) {
    return name + ":" + entryPoint + ":" + std::to_string(static_cast<int>(type));
}

#if defined(_WIN32)

CompiledShader* ShaderManager::compile(const std::string& name, const std::string& source,
                                        const std::string& entryPoint, ShaderType type) {
    std::string key = makeCacheKey(name, entryPoint, type);
    
    // Check cache
    auto it = shaderCache.find(key);
    if (it != shaderCache.end() && it->second->valid) {
        return it->second.get();
    }
    
    // Create new compiled shader
    auto compiled = std::make_unique<CompiledShader>();
    compiled->name = name;
    compiled->type = type;
    compiled->valid = false;
    
    // Compile
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(
        source.c_str(), source.size(),
        name.c_str(),
        nullptr, nullptr,
        entryPoint.c_str(),
        getShaderModel(type),
        flags, 0,
        &shaderBlob,
        &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "[ShaderManager] Compile error (" << name << ":" << entryPoint << "): " 
                      << (char*)errorBlob->GetBufferPointer() << std::endl;
        }
        return nullptr;
    }
    
    // Copy bytecode to platform-agnostic storage
    compiled->bytecode.resize(shaderBlob->GetBufferSize());
    memcpy(compiled->bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
    compiled->valid = true;
    
    // Store in cache
    CompiledShader* result = compiled.get();
    shaderCache[key] = std::move(compiled);
    
    std::cout << "[ShaderManager] Compiled: " << name << ":" << entryPoint << std::endl;
    return result;
}

#else // Non-Windows platforms

CompiledShader* ShaderManager::compile(const std::string& name, const std::string& source,
                                        const std::string& entryPoint, ShaderType type) {
    std::cerr << "[ShaderManager] Compile not implemented for this platform" << std::endl;
    return nullptr;
}

#endif // _WIN32

CompiledShader* ShaderManager::compileFromFile(const std::string& filename, const std::string& entryPoint,
                                                ShaderType type, const char* embeddedFallback) {
    ShaderSource source = loadSource(filename, embeddedFallback);
    if (!source.loaded || source.source.empty()) {
        return nullptr;
    }
    return compile(filename, source.source, entryPoint, type);
}

CompiledShader* ShaderManager::getShader(const std::string& key) {
    auto it = shaderCache.find(key);
    if (it != shaderCache.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ShaderManager::watchForChanges() {
    // TODO: Implement file watching using FileWatcher
}

void ShaderManager::recompileAll() {
    std::cout << "[ShaderManager] Recompiling all shaders..." << std::endl;
    
    // Store keys to recompile
    std::vector<std::tuple<std::string, std::string, ShaderType>> toRecompile;
    
    for (const auto& pair : shaderCache) {
        const std::string& key = pair.first;
        size_t pos1 = key.find(':');
        size_t pos2 = key.rfind(':');
        
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos1 != pos2) {
            std::string name = key.substr(0, pos1);
            std::string entryPoint = key.substr(pos1 + 1, pos2 - pos1 - 1);
            ShaderType type = static_cast<ShaderType>(std::stoi(key.substr(pos2 + 1)));
            toRecompile.push_back({name, entryPoint, type});
        }
    }
    
    // Clear caches
    shaderCache.clear();
    sourceCache.clear();
    
    // Recompile
    for (const auto& [name, entryPoint, type] : toRecompile) {
        compileFromFile(name, entryPoint, type, nullptr);
    }
    
    if (recompileCallback) {
        recompileCallback();
    }
    
    std::cout << "[ShaderManager] Recompilation complete" << std::endl;
}

void ShaderManager::clearCache() {
    shaderCache.clear();
    sourceCache.clear();
    std::cout << "[ShaderManager] Cache cleared" << std::endl;
}

} // namespace luma
