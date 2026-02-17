// LUMA Material Shader Generator
// Converts a MaterialGraph into HLSL shader code
// Performs topological sort, variable naming, type conversion, and code assembly

#pragma once

#include "engine/material/material_node.h"
#include "engine/material/material_node_library.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <functional>

namespace luma {

// ===== Generated Shader Result =====
struct GeneratedShader {
    std::string hlslCode;                      // Complete HLSL source
    std::string vertexEntry = "VSMain";        // VS entry point
    std::string pixelEntry = "PSMain";         // PS entry point
    std::vector<MaterialTextureSlot> textureSlots;  // Texture bindings
    std::vector<MaterialUniform> uniforms;          // Material uniforms
    uint64_t hash = 0;                         // For caching
    bool valid = false;
    std::string error;                         // Error message if !valid
};

// ===== Shader Generator =====
class ShaderGenerator {
public:
    
    GeneratedShader generate(const MaterialGraph& graph) {
        GeneratedShader result;
        
        // Validate graph
        if (graph.outputNodeId == 0) {
            result.error = "No output node in material graph";
            return result;
        }
        
        const VisualScriptNode* outputNode = graph.findNode(graph.outputNodeId);
        if (!outputNode) {
            result.error = "Output node not found";
            return result;
        }
        
        // Collect active nodes (those contributing to output)
        auto activeNodes = graph.collectActiveNodes();
        if (activeNodes.empty()) {
            result.error = "No active nodes connected to output";
            return result;
        }
        
        // Topological sort
        auto sortedIds = graph.topologicalSort();
        
        // Filter to only active nodes
        std::vector<uint32_t> activeSorted;
        for (uint32_t id : sortedIds) {
            if (activeNodes.count(id)) {
                activeSorted.push_back(id);
            }
        }
        
        // Reset state
        varNames_.clear();
        textureSlotCounter_ = 0;
        uniformCounter_ = 0;
        result.textureSlots.clear();
        result.uniforms.clear();
        needsProcedural_ = false;
        
        // Assign variable names to all output pins
        for (uint32_t nodeId : activeSorted) {
            const auto* node = graph.findNode(nodeId);
            if (!node) continue;
            
            // Check if this node's definition needs procedural includes
            const auto* def = MaterialNodeLibrary::instance().findByType(node->name);
            if (def && !def->hlslHelperIncludes.empty()) {
                needsProcedural_ = true;
            }
            
            for (const auto& pin : node->outputs) {
                std::string varName = "_n" + std::to_string(nodeId) + "_" + sanitizeName(pin.name);
                varNames_[pin.id] = varName;
            }
        }
        
        // Collect textures and uniforms
        for (uint32_t nodeId : activeSorted) {
            const auto* node = graph.findNode(nodeId);
            if (!node) continue;
            
            const auto* def = MaterialNodeLibrary::instance().findByType(node->name);
            if (!def) continue;
            
            // Check for texture properties
            for (const auto& propDef : def->properties) {
                if (propDef.type == PinType::Texture2D) {
                    MaterialTextureSlot slot;
                    slot.nodeId = nodeId;
                    slot.registerSlot = 10 + textureSlotCounter_;
                    slot.variableName = "matTex" + std::to_string(textureSlotCounter_);
                    
                    // Get texture path from node properties
                    auto it = node->properties.find(propDef.name);
                    if (it != node->properties.end() && std::holds_alternative<std::string>(it->second)) {
                        slot.texturePath = std::get<std::string>(it->second);
                    }
                    
                    result.textureSlots.push_back(slot);
                    textureSlotCounter_++;
                }
            }
            
            // Collect non-texture properties as uniforms
            for (const auto& propDef : def->properties) {
                if (propDef.type == PinType::Texture2D) continue;
                if (propDef.type == PinType::Int && !propDef.enumOptions.empty()) continue; // Enum props are compile-time
                
                MaterialUniform uniform;
                uniform.name = "matParam_" + std::to_string(nodeId) + "_" + sanitizeName(propDef.name);
                uniform.type = propDef.type;
                uniform.nodeId = nodeId;
                uniform.propertyName = propDef.name;
                
                // Get current value from node
                auto it = node->properties.find(propDef.name);
                if (it != node->properties.end()) {
                    uniform.value = it->second;
                } else {
                    uniform.value = propDef.defaultValue;
                }
                
                result.uniforms.push_back(uniform);
            }
        }
        
        // Generate HLSL
        std::stringstream ss;
        
        // Header
        ss << "// Auto-generated material shader from LUMA Material Node Editor\n";
        ss << "// DO NOT EDIT MANUALLY - changes will be overwritten\n\n";
        
        // Includes
        ss << "#include \"pbr_common.hlsli\"\n";
        if (needsProcedural_) {
            ss << "#include \"procedural.hlsli\"\n";
        }
        ss << "\n";
        
        // Scene constant buffer (matches existing layout)
        ss << "cbuffer SceneConstants : register(b0) {\n";
        ss << "    float4x4 worldViewProj;\n";
        ss << "    float4x4 world;\n";
        ss << "    float4x4 lightViewProj;\n";
        ss << "    float4 lightDirAndFlags;\n";
        ss << "    float4 cameraPosAndMetal;\n";
        ss << "    float4 baseColorAndRough;\n";
        ss << "    float4 shadowParams;\n";
        ss << "    float4 iblParams;\n";
        ss << "};\n\n";
        
        // Macros for scene constants
        ss << "#define lightDir lightDirAndFlags.xyz\n";
        ss << "#define cameraPos cameraPosAndMetal.xyz\n\n";
        
        // Material constant buffer
        if (!result.uniforms.empty()) {
            ss << "cbuffer MaterialConstants : register(b1) {\n";
            uint32_t offset = 0;
            for (auto& uniform : result.uniforms) {
                uniform.offsetInBuffer = offset;
                ss << "    " << pinTypeToHLSL(uniform.type) << " " << uniform.name << ";\n";
                
                // Calculate size (aligned to 4 bytes)
                int components = pinTypeComponents(uniform.type);
                offset += components * 4;
                // Align to 16 bytes for float4
                if (components > 2) offset = (offset + 15) & ~15;
            }
            ss << "};\n\n";
        }
        
        // Standard textures (shadow, IBL)
        ss << "Texture2D shadowMap : register(t3);\n";
        ss << "TextureCube irradianceMap : register(t4);\n";
        ss << "TextureCube prefilteredMap : register(t5);\n";
        ss << "Texture2D brdfLUT : register(t6);\n";
        ss << "SamplerState texSampler : register(s0);\n";
        ss << "SamplerComparisonState shadowSampler : register(s1);\n\n";
        
        // Material textures
        if (!result.textureSlots.empty()) {
            ss << "// Material textures\n";
            ss << "SamplerState matSampler : register(s2);\n";
            for (const auto& tex : result.textureSlots) {
                ss << "Texture2D " << tex.variableName << " : register(t" << tex.registerSlot << ");\n";
            }
            ss << "\n";
        }
        
        // Color ramp functions
        for (uint32_t nodeId : activeSorted) {
            const auto* node = graph.findNode(nodeId);
            if (!node || node->name != "Mat_ColorRamp") continue;
            
            auto rampIt = graph.colorRamps.find(nodeId);
            if (rampIt != graph.colorRamps.end() && !rampIt->second.empty()) {
                generateColorRampFunction(ss, nodeId, rampIt->second);
            } else {
                // Default ramp: black to white
                ss << "float4 colorRamp_" << nodeId << "(float t) {\n";
                ss << "    return float4(t, t, t, 1.0);\n";
                ss << "}\n\n";
            }
        }
        
        // Vertex shader input/output
        ss << "struct VSInput {\n";
        ss << "    float3 position : POSITION;\n";
        ss << "    float3 normal : NORMAL;\n";
        ss << "    float4 tangent : TANGENT;\n";
        ss << "    float2 uv : TEXCOORD;\n";
        ss << "    float3 color : COLOR;\n";
        ss << "};\n\n";
        
        ss << "struct PSInput {\n";
        ss << "    float4 position : SV_POSITION;\n";
        ss << "    float3 worldPos : TEXCOORD0;\n";
        ss << "    float3 normal : TEXCOORD1;\n";
        ss << "    float3 tangent : TEXCOORD2;\n";
        ss << "    float3 bitangent : TEXCOORD3;\n";
        ss << "    float2 uv : TEXCOORD4;\n";
        ss << "    float3 color : COLOR;\n";
        ss << "    float4 shadowCoord : TEXCOORD5;\n";
        ss << "};\n\n";
        
        // Vertex shader (standard, same as static PBR)
        ss << "PSInput VSMain(VSInput input) {\n";
        ss << "    PSInput output;\n";
        ss << "    float4 worldPos = mul(world, float4(input.position, 1.0));\n";
        ss << "    output.position = mul(worldViewProj, float4(input.position, 1.0));\n";
        ss << "    output.worldPos = worldPos.xyz;\n";
        ss << "    output.normal = normalize(mul((float3x3)world, input.normal));\n";
        ss << "    output.tangent = normalize(mul((float3x3)world, input.tangent.xyz));\n";
        ss << "    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;\n";
        ss << "    output.uv = input.uv;\n";
        ss << "    output.color = input.color;\n";
        ss << "    output.shadowCoord = mul(lightViewProj, worldPos);\n";
        ss << "    return output;\n";
        ss << "}\n\n";
        
        // Pixel shader
        ss << "float4 PSMain(PSInput _input) : SV_TARGET {\n";
        
        // Generate code for each active node in topological order
        for (uint32_t nodeId : activeSorted) {
            const auto* node = graph.findNode(nodeId);
            if (!node) continue;
            
            // Skip output node (handled separately)
            if (nodeId == graph.outputNodeId) continue;
            
            ss << "    // " << node->displayName << " (Node " << nodeId << ")\n";
            generateNodeCode(ss, graph, *node, result);
            ss << "\n";
        }
        
        // Generate output mapping
        ss << "    // === Material Output ===\n";
        generateOutputCode(ss, graph, *outputNode);
        
        // Shadow coordinates
        ss << "    float3 _shadowCoord = _input.shadowCoord.xyz / _input.shadowCoord.w;\n";
        ss << "    _shadowCoord.xy = _shadowCoord.xy * 0.5 + 0.5;\n";
        ss << "    _shadowCoord.y = 1.0 - _shadowCoord.y;\n\n";
        
        // Call PBR lighting function
        ss << "    return PBRLighting(\n";
        ss << "        _matBaseColor, _matMetallic, _matRoughness, _matNormal,\n";
        ss << "        _input.worldPos, cameraPos, lightDir, _shadowCoord,\n";
        ss << "        _matAO, _matEmissive, _matAlpha,\n";
        ss << "        shadowMap, shadowSampler,\n";
        ss << "        irradianceMap, prefilteredMap, brdfLUT, texSampler,\n";
        ss << "        shadowParams, iblParams);\n";
        ss << "}\n";
        
        result.hlslCode = ss.str();
        result.hash = graph.computeHash();
        result.valid = true;
        return result;
    }
    
private:
    std::unordered_map<uint32_t, std::string> varNames_;  // pinId -> variable name
    int textureSlotCounter_ = 0;
    int uniformCounter_ = 0;
    bool needsProcedural_ = false;
    
    static std::string sanitizeName(const std::string& name) {
        std::string result;
        for (char c : name) {
            if (std::isalnum(c)) result += c;
            else result += '_';
        }
        return result;
    }
    
    // Resolve an input pin to its HLSL expression
    std::string resolveInput(const MaterialGraph& graph, const VisualScriptNode& node,
                             const std::string& pinName, const GeneratedShader& shader) {
        const Pin* pin = nullptr;
        for (const auto& p : node.inputs) {
            if (p.name == pinName) { pin = &p; break; }
        }
        if (!pin) return "0.0";
        
        // Check if connected
        const Link* link = graph.findInputLink(pin->id);
        if (link) {
            // Find the source pin's variable
            auto it = varNames_.find(link->fromPin);
            if (it != varNames_.end()) {
                // Get source pin type for type conversion
                const auto* srcNode = graph.findNode(link->fromNode);
                if (srcNode) {
                    const Pin* srcPin = srcNode->findPin(link->fromPin);
                    if (srcPin && srcPin->type != pin->type) {
                        return generateTypeConversion(it->second, srcPin->type, pin->type);
                    }
                }
                return it->second;
            }
        }
        
        // Use default value
        return pinDefaultToHLSL(pin->type, pin->defaultValue);
    }
    
    // Get the output variable name for a pin
    std::string getOutputVar(uint32_t pinId) {
        auto it = varNames_.find(pinId);
        if (it != varNames_.end()) return it->second;
        return "_unknown";
    }
    
    // Generate HLSL code for a single node
    void generateNodeCode(std::stringstream& ss, const MaterialGraph& graph,
                          const VisualScriptNode& node, const GeneratedShader& shader) {
        const auto* def = MaterialNodeLibrary::instance().findByType(node.name);
        if (!def) {
            ss << "    // Unknown node type: " << node.name << "\n";
            return;
        }
        
        // Declare output variables
        for (const auto& pin : node.outputs) {
            auto it = varNames_.find(pin.id);
            if (it != varNames_.end()) {
                ss << "    " << pinTypeToHLSL(pin.type) << " " << it->second << ";\n";
            }
        }
        
        // Process HLSL template
        std::string code = def->hlslTemplate;
        
        // Replace {input:Name} with resolved expressions
        for (const auto& pinDef : def->pins) {
            if (pinDef.direction != PinDirection::Input) continue;
            std::string placeholder = "{input:" + pinDef.name + "}";
            std::string value = resolveInput(graph, node, pinDef.name, shader);
            replaceAll(code, placeholder, value);
        }
        
        // Replace {output:Name} with variable names
        for (const auto& pin : node.outputs) {
            std::string placeholder = "{output:" + pin.name + "}";
            auto it = varNames_.find(pin.id);
            if (it != varNames_.end()) {
                replaceAll(code, placeholder, it->second);
            }
        }
        
        // Replace {param:Name} with uniform names or literal values
        for (const auto& propDef : def->properties) {
            std::string placeholder = "{param:" + propDef.name + "}";
            
            if (propDef.type == PinType::Texture2D) continue;
            if (propDef.type == PinType::Int && !propDef.enumOptions.empty()) {
                // Enum: use literal value
                auto it = node.properties.find(propDef.name);
                int idx = 0;
                if (it != node.properties.end() && std::holds_alternative<int>(it->second)) {
                    idx = std::get<int>(it->second);
                }
                replaceAll(code, placeholder, std::to_string(idx));
                continue;
            }
            
            // Find the uniform name
            std::string uniformName = "matParam_" + std::to_string(node.id) + "_" + sanitizeName(propDef.name);
            replaceAll(code, placeholder, uniformName);
        }
        
        // Replace {texture} with texture variable (for image texture nodes)
        for (const auto& tex : shader.textureSlots) {
            if (tex.nodeId == node.id) {
                replaceAll(code, "{texture}", tex.variableName);
                break;
            }
        }
        
        // Replace {node} with node ID (for unique variable naming)
        replaceAll(code, "{node}", std::to_string(node.id));
        
        // Indent and output
        std::istringstream stream(code);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                ss << "    " << line << "\n";
            }
        }
    }
    
    // Generate the output mapping code
    void generateOutputCode(std::stringstream& ss, const MaterialGraph& graph,
                            const VisualScriptNode& outputNode) {
        // Map each output pin to its final variable
        auto resolveOutputPin = [&](const std::string& pinName, PinType pinType, const std::string& defaultVal) -> std::string {
            for (const auto& pin : outputNode.inputs) {
                if (pin.name == pinName) {
                    const Link* link = graph.findInputLink(pin.id);
                    if (link) {
                        auto it = varNames_.find(link->fromPin);
                        if (it != varNames_.end()) {
                            const auto* srcNode = graph.findNode(link->fromNode);
                            if (srcNode) {
                                const Pin* srcPin = srcNode->findPin(link->fromPin);
                                if (srcPin && srcPin->type != pinType) {
                                    return generateTypeConversion(it->second, srcPin->type, pinType);
                                }
                            }
                            return it->second;
                        }
                    }
                    return defaultVal;
                }
            }
            return defaultVal;
        };
        
        ss << "    float3 _matBaseColor = " << resolveOutputPin("Base Color", PinType::Vec3, "float3(0.8, 0.8, 0.8)") << ";\n";
        ss << "    float _matMetallic = " << resolveOutputPin("Metallic", PinType::Float, "0.0") << ";\n";
        ss << "    float _matRoughness = " << resolveOutputPin("Roughness", PinType::Float, "0.5") << ";\n";
        
        // Normal: check if connected, otherwise use geometry normal
        std::string normalExpr = resolveOutputPin("Normal", PinType::Vec3, "");
        if (normalExpr.empty()) {
            ss << "    float3 _matNormal = normalize(_input.normal);\n";
        } else {
            ss << "    float3 _matNormal = normalize(" << normalExpr << ");\n";
        }
        
        ss << "    float3 _matEmissive = " << resolveOutputPin("Emissive", PinType::Vec3, "float3(0.0, 0.0, 0.0)") << ";\n";
        ss << "    float _matAO = " << resolveOutputPin("AO", PinType::Float, "1.0") << ";\n";
        ss << "    float _matAlpha = " << resolveOutputPin("Alpha", PinType::Float, "1.0") << ";\n";
        ss << "\n";
    }
    
    // Generate a color ramp lookup function
    void generateColorRampFunction(std::stringstream& ss, uint32_t nodeId,
                                    const std::vector<ColorRampStop>& stops) {
        ss << "float4 colorRamp_" << nodeId << "(float t) {\n";
        ss << "    t = saturate(t);\n";
        
        if (stops.size() == 1) {
            ss << "    return float4(" << stops[0].color[0] << ", " << stops[0].color[1] 
               << ", " << stops[0].color[2] << ", " << stops[0].color[3] << ");\n";
        } else {
            // Sort stops by position
            auto sorted = stops;
            std::sort(sorted.begin(), sorted.end(), [](const ColorRampStop& a, const ColorRampStop& b) {
                return a.position < b.position;
            });
            
            // Generate piecewise linear interpolation
            for (size_t i = 0; i < sorted.size() - 1; i++) {
                const auto& a = sorted[i];
                const auto& b = sorted[i + 1];
                
                if (i == 0) {
                    ss << "    if (t <= " << b.position << ") {\n";
                } else {
                    ss << "    else if (t <= " << b.position << ") {\n";
                }
                
                float range = b.position - a.position;
                if (range < 0.0001f) range = 0.0001f;
                
                ss << "        float f = (t - " << a.position << ") / " << range << ";\n";
                ss << "        return lerp(float4(" << a.color[0] << "," << a.color[1] << "," << a.color[2] << "," << a.color[3] << "),\n";
                ss << "                    float4(" << b.color[0] << "," << b.color[1] << "," << b.color[2] << "," << b.color[3] << "), f);\n";
                ss << "    }\n";
            }
            
            // Last stop
            const auto& last = sorted.back();
            ss << "    return float4(" << last.color[0] << "," << last.color[1] 
               << "," << last.color[2] << "," << last.color[3] << ");\n";
        }
        
        ss << "}\n\n";
    }
    
    static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
};

// ===== Material Shader Cache =====
// Caches compiled material shaders by graph hash
class MaterialShaderCache {
public:
    static MaterialShaderCache& instance() {
        static MaterialShaderCache cache;
        return cache;
    }
    
    // Check if a shader with this hash is cached
    bool has(uint64_t hash) const {
        return cache_.count(hash) > 0;
    }
    
    // Get cached shader
    const GeneratedShader* get(uint64_t hash) const {
        auto it = cache_.find(hash);
        return it != cache_.end() ? &it->second : nullptr;
    }
    
    // Store generated shader
    void store(uint64_t hash, const GeneratedShader& shader) {
        cache_[hash] = shader;
    }
    
    // Remove from cache
    void invalidate(uint64_t hash) {
        cache_.erase(hash);
    }
    
    // Clear entire cache
    void clear() {
        cache_.clear();
    }
    
    // Get cache size
    size_t size() const { return cache_.size(); }
    
private:
    MaterialShaderCache() = default;
    std::unordered_map<uint64_t, GeneratedShader> cache_;
};

} // namespace luma
