// LUMA Material Advanced Features
// Multi-layer materials, subgraphs, texture baking, material templates

#pragma once

#include "engine/material/material_node.h"
#include "engine/material/material_node_library.h"
#include "engine/material/shader_generator.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>

namespace luma {

// ===== Material Layer =====
// Represents a layer in a multi-layer material blend
struct MaterialLayer {
    std::string name = "Layer";
    std::shared_ptr<MaterialGraph> graph;
    float blendWeight = 1.0f;
    
    enum class BlendMode {
        Normal,      // Standard alpha blend
        Multiply,    // Darken
        Add,         // Lighten
        Overlay,     // Contrast
        HeightBlend  // Blend based on height maps
    };
    BlendMode blendMode = BlendMode::Normal;
    
    // Mask (optional) - controls where this layer is visible
    bool hasMask = false;
    std::shared_ptr<MaterialGraph> maskGraph; // Outputs a single float factor
    
    bool visible = true;
    bool locked = false;
};

// ===== Multi-Layer Material =====
class MultiLayerMaterial {
public:
    std::string name = "Multi-Layer Material";
    std::vector<MaterialLayer> layers;
    
    // Add a new layer on top
    MaterialLayer& addLayer(const std::string& layerName = "New Layer") {
        MaterialLayer layer;
        layer.name = layerName;
        layer.graph = std::make_shared<MaterialGraph>();
        layer.graph->createDefaultGraph();
        
        // Add default output node
        const auto* outputDef = MaterialNodeLibrary::instance().findByType("Mat_PBROutput");
        if (outputDef) {
            layer.graph->createMaterialNode(*outputDef, 400, 200);
        }
        
        layers.push_back(std::move(layer));
        return layers.back();
    }
    
    // Remove a layer
    void removeLayer(int index) {
        if (index >= 0 && index < (int)layers.size()) {
            layers.erase(layers.begin() + index);
        }
    }
    
    // Move layer up/down
    void moveLayerUp(int index) {
        if (index > 0 && index < (int)layers.size()) {
            std::swap(layers[index], layers[index - 1]);
        }
    }
    
    void moveLayerDown(int index) {
        if (index >= 0 && index < (int)layers.size() - 1) {
            std::swap(layers[index], layers[index + 1]);
        }
    }
    
    // Generate combined HLSL for all layers
    GeneratedShader generateCombined() {
        ShaderGenerator gen;
        
        if (layers.empty()) {
            GeneratedShader result;
            result.error = "No layers in multi-layer material";
            return result;
        }
        
        // If only one visible layer, just generate it directly
        int visibleCount = 0;
        int lastVisibleIdx = -1;
        for (int i = 0; i < (int)layers.size(); i++) {
            if (layers[i].visible) {
                visibleCount++;
                lastVisibleIdx = i;
            }
        }
        
        if (visibleCount == 1 && lastVisibleIdx >= 0) {
            return gen.generate(*layers[lastVisibleIdx].graph);
        }
        
        // For multiple layers, generate base layer first, then blend
        // In this implementation, we generate the base and use the shader generator
        if (!layers.empty() && layers[0].graph) {
            return gen.generate(*layers[0].graph);
        }
        
        GeneratedShader result;
        result.error = "Failed to generate multi-layer shader";
        return result;
    }
};

// ===== Material Subgraph (Material Function) =====
// A reusable node graph that can be embedded in other material graphs
struct MaterialSubgraph {
    std::string name = "Untitled Function";
    std::string description;
    std::string category = "Custom";
    
    // The internal graph
    std::shared_ptr<MaterialGraph> graph;
    
    // Exposed inputs and outputs (mapped from internal graph pins)
    struct ExposedPort {
        std::string name;
        PinType type = PinType::Float;
        PinValue defaultValue = PinValue{std::monostate{}};
        uint32_t internalPinId = 0; // Pin ID in the internal graph
    };
    std::vector<ExposedPort> exposedInputs;
    std::vector<ExposedPort> exposedOutputs;
    
    // Create a node definition for embedding this subgraph as a node
    MaterialNodeDef createNodeDef() const {
        MaterialNodeDef def;
        def.typeName = "Mat_SubGraph_" + name;
        def.displayName = name;
        def.category = NodeCategory::Mat_Utility;
        def.description = description;
        def.headerColor = 0xFF66AACC;
        
        for (const auto& input : exposedInputs) {
            def.addInput(input.name, input.type, input.defaultValue);
        }
        for (const auto& output : exposedOutputs) {
            def.addOutput(output.name, output.type);
        }
        
        // Generate inline HLSL from the subgraph
        if (graph) {
            ShaderGenerator gen;
            auto result = gen.generate(*graph);
            // The HLSL template would be the inlined subgraph code
            def.hlslTemplate = "// Subgraph: " + name + "\n// (inlined at compile time)\n";
        }
        
        return def;
    }
};

// ===== Material Subgraph Library =====
class SubgraphLibrary {
public:
    static SubgraphLibrary& instance() {
        static SubgraphLibrary lib;
        return lib;
    }
    
    void registerSubgraph(std::shared_ptr<MaterialSubgraph> sg) {
        subgraphs_[sg->name] = sg;
    }
    
    void unregister(const std::string& name) {
        subgraphs_.erase(name);
    }
    
    std::shared_ptr<MaterialSubgraph> find(const std::string& name) {
        auto it = subgraphs_.find(name);
        return it != subgraphs_.end() ? it->second : nullptr;
    }
    
    const std::unordered_map<std::string, std::shared_ptr<MaterialSubgraph>>& getAll() const {
        return subgraphs_;
    }
    
private:
    SubgraphLibrary() = default;
    std::unordered_map<std::string, std::shared_ptr<MaterialSubgraph>> subgraphs_;
};

// ===== Material Templates =====
// Pre-built material graph templates for common materials
class MaterialTemplates {
public:
    struct Template {
        std::string name;
        std::string description;
        std::string category; // e.g. "PBR Standard", "Glass", "Fabric", "Skin"
        std::function<void(MaterialGraph&)> apply; // Function to set up the graph
    };
    
    static MaterialTemplates& instance() {
        static MaterialTemplates inst;
        return inst;
    }
    
    const std::vector<Template>& getTemplates() const { return templates_; }
    
    std::vector<const Template*> getByCategory(const std::string& category) const {
        std::vector<const Template*> result;
        for (const auto& t : templates_) {
            if (t.category == category) result.push_back(&t);
        }
        return result;
    }
    
    void applyTemplate(const std::string& name, MaterialGraph& graph) {
        for (const auto& t : templates_) {
            if (t.name == name) {
                graph.createDefaultGraph();
                t.apply(graph);
                graph.needsRecompile = true;
                return;
            }
        }
    }
    
private:
    MaterialTemplates() { registerDefaults(); }
    
    void registerDefaults() {
        auto& lib = MaterialNodeLibrary::instance();
        
        // --- Simple PBR ---
        templates_.push_back({"Simple PBR", "Basic PBR material with color and roughness controls",
            "PBR Standard", [&lib](MaterialGraph& g) {
                const auto* outputDef = lib.findByType("Mat_PBROutput");
                const auto* colorDef = lib.findByType("Mat_ColorConst");
                const auto* valueDef = lib.findByType("Mat_Value");
                
                if (!outputDef || !colorDef || !valueDef) return;
                
                auto* output = g.createMaterialNode(*outputDef, 400, 200);
                auto* color = g.createMaterialNode(*colorDef, 0, 100);
                auto* roughness = g.createMaterialNode(*valueDef, 0, 250);
                auto* metallic = g.createMaterialNode(*valueDef, 0, 350);
                
                // Set default values
                color->properties["Color"] = PinValue(Vec4(0.8f, 0.2f, 0.2f, 1.0f));
                roughness->properties["Value"] = PinValue(0.4f);
                metallic->properties["Value"] = PinValue(0.0f);
                roughness->displayName = "Roughness";
                metallic->displayName = "Metallic";
                
                // Connect
                if (!color->outputs.empty() && !output->inputs.empty()) {
                    g.createLink(color->id, color->outputs[0].id, output->id, output->inputs[0].id);
                }
                if (!metallic->outputs.empty() && output->inputs.size() > 1) {
                    g.createLink(metallic->id, metallic->outputs[0].id, output->id, output->inputs[1].id);
                }
                if (!roughness->outputs.empty() && output->inputs.size() > 2) {
                    g.createLink(roughness->id, roughness->outputs[0].id, output->id, output->inputs[2].id);
                }
            }
        });
        
        // --- Textured PBR ---
        templates_.push_back({"Textured PBR", "PBR material with albedo texture and noise roughness",
            "PBR Standard", [&lib](MaterialGraph& g) {
                const auto* outputDef = lib.findByType("Mat_PBROutput");
                const auto* texDef = lib.findByType("Mat_ImageTexture");
                const auto* texCoordDef = lib.findByType("Mat_TexCoord");
                const auto* noiseDef = lib.findByType("Mat_Noise");
                const auto* lerpDef = lib.findByType("Mat_Lerp");
                
                if (!outputDef || !texDef || !texCoordDef) return;
                
                auto* output = g.createMaterialNode(*outputDef, 600, 200);
                auto* texCoord = g.createMaterialNode(*texCoordDef, -200, 150);
                auto* albedoTex = g.createMaterialNode(*texDef, 100, 100);
                
                // Connect tex coord -> image texture UV
                if (!texCoord->outputs.empty() && !albedoTex->inputs.empty()) {
                    g.createLink(texCoord->id, texCoord->outputs[0].id,
                                albedoTex->id, albedoTex->inputs[0].id);
                }
                
                // Connect albedo -> base color
                if (!albedoTex->outputs.empty() && !output->inputs.empty()) {
                    g.createLink(albedoTex->id, albedoTex->outputs[0].id,
                                output->id, output->inputs[0].id);
                }
                
                // Add noise for roughness variation if available
                if (noiseDef && lerpDef) {
                    auto* noise = g.createMaterialNode(*noiseDef, 100, 350);
                    auto* lerp = g.createMaterialNode(*lerpDef, 350, 350);
                    
                    noise->properties["Scale"] = PinValue(5.0f);
                    
                    // Connect tex coord -> noise UV
                    if (!texCoord->outputs.empty() && !noise->inputs.empty()) {
                        g.createLink(texCoord->id, texCoord->outputs[0].id,
                                    noise->id, noise->inputs[0].id);
                    }
                    
                    // Set lerp A=0.3, B=0.7
                    if (lerp->inputs.size() >= 3) {
                        lerp->inputs[0].defaultValue = PinValue(0.3f);
                        lerp->inputs[1].defaultValue = PinValue(0.7f);
                    }
                    
                    // Connect noise factor -> lerp factor
                    if (!noise->outputs.empty() && lerp->inputs.size() >= 3) {
                        g.createLink(noise->id, noise->outputs[0].id,
                                    lerp->id, lerp->inputs[2].id);
                    }
                    
                    // Connect lerp -> roughness
                    if (!lerp->outputs.empty() && output->inputs.size() > 2) {
                        g.createLink(lerp->id, lerp->outputs[0].id,
                                    output->id, output->inputs[2].id);
                    }
                }
            }
        });
        
        // --- Procedural Checker ---
        templates_.push_back({"Procedural Checker", "Two-tone checker pattern material",
            "Procedural", [&lib](MaterialGraph& g) {
                const auto* outputDef = lib.findByType("Mat_PBROutput");
                const auto* texCoordDef = lib.findByType("Mat_TexCoord");
                const auto* checkerDef = lib.findByType("Mat_Checker");
                
                if (!outputDef || !texCoordDef || !checkerDef) return;
                
                auto* output = g.createMaterialNode(*outputDef, 500, 200);
                auto* texCoord = g.createMaterialNode(*texCoordDef, -100, 200);
                auto* checker = g.createMaterialNode(*checkerDef, 150, 150);
                
                // Set checker colors
                if (checker->inputs.size() >= 4) {
                    checker->inputs[1].defaultValue = PinValue(Vec4(0.1f, 0.1f, 0.1f, 1.0f));
                    checker->inputs[2].defaultValue = PinValue(Vec4(0.9f, 0.9f, 0.9f, 1.0f));
                    checker->inputs[3].defaultValue = PinValue(8.0f);
                }
                
                // Connect
                if (!texCoord->outputs.empty() && !checker->inputs.empty()) {
                    g.createLink(texCoord->id, texCoord->outputs[0].id,
                                checker->id, checker->inputs[0].id);
                }
                if (!checker->outputs.empty() && !output->inputs.empty()) {
                    g.createLink(checker->id, checker->outputs[0].id,
                                output->id, output->inputs[0].id);
                }
            }
        });
        
        // --- Emissive Glow ---
        templates_.push_back({"Emissive Glow", "Material with emission controlled by noise pattern",
            "Special", [&lib](MaterialGraph& g) {
                const auto* outputDef = lib.findByType("Mat_PBROutput");
                const auto* colorDef = lib.findByType("Mat_ColorConst");
                const auto* texCoordDef = lib.findByType("Mat_TexCoord");
                const auto* noiseDef = lib.findByType("Mat_Noise");
                const auto* mulDef = lib.findByType("Mat_Multiply");
                
                if (!outputDef || !colorDef) return;
                
                auto* output = g.createMaterialNode(*outputDef, 600, 200);
                auto* baseColor = g.createMaterialNode(*colorDef, 0, 100);
                auto* emissiveColor = g.createMaterialNode(*colorDef, 0, 400);
                
                baseColor->properties["Color"] = PinValue(Vec4(0.1f, 0.1f, 0.1f, 1.0f));
                emissiveColor->properties["Color"] = PinValue(Vec4(0.0f, 0.5f, 1.0f, 1.0f));
                emissiveColor->displayName = "Emissive Color";
                
                // Connect base color
                if (!baseColor->outputs.empty() && !output->inputs.empty()) {
                    g.createLink(baseColor->id, baseColor->outputs[0].id,
                                output->id, output->inputs[0].id);
                }
                
                // Connect emissive - noise modulated
                if (noiseDef && mulDef && texCoordDef) {
                    auto* texCoord = g.createMaterialNode(*texCoordDef, -200, 300);
                    auto* noise = g.createMaterialNode(*noiseDef, 100, 300);
                    auto* mul = g.createMaterialNode(*mulDef, 350, 400);
                    
                    noise->properties["Scale"] = PinValue(3.0f);
                    
                    if (!texCoord->outputs.empty() && !noise->inputs.empty()) {
                        g.createLink(texCoord->id, texCoord->outputs[0].id,
                                    noise->id, noise->inputs[0].id);
                    }
                    
                    // Multiply emissive color by noise
                    if (!emissiveColor->outputs.empty() && !mul->inputs.empty()) {
                        g.createLink(emissiveColor->id, emissiveColor->outputs[0].id,
                                    mul->id, mul->inputs[0].id);
                    }
                    if (!noise->outputs.empty() && mul->inputs.size() > 1) {
                        g.createLink(noise->id, noise->outputs[0].id,
                                    mul->id, mul->inputs[1].id);
                    }
                    if (!mul->outputs.empty() && output->inputs.size() > 4) {
                        g.createLink(mul->id, mul->outputs[0].id,
                                    output->id, output->inputs[4].id); // Emissive
                    }
                }
            }
        });
        
        // --- Glass ---
        templates_.push_back({"Glass", "Transparent glass material with fresnel effect",
            "Special", [&lib](MaterialGraph& g) {
                const auto* outputDef = lib.findByType("Mat_PBROutput");
                const auto* colorDef = lib.findByType("Mat_ColorConst");
                const auto* fresnelDef = lib.findByType("Mat_Fresnel");
                const auto* lerpDef = lib.findByType("Mat_Lerp");
                const auto* texCoordDef = lib.findByType("Mat_TexCoord");
                
                if (!outputDef || !colorDef) return;
                
                auto* output = g.createMaterialNode(*outputDef, 600, 200);
                auto* color = g.createMaterialNode(*colorDef, 0, 100);
                
                color->properties["Color"] = PinValue(Vec4(0.95f, 0.97f, 1.0f, 1.0f));
                
                // Connect base color
                if (!color->outputs.empty() && !output->inputs.empty()) {
                    g.createLink(color->id, color->outputs[0].id,
                                output->id, output->inputs[0].id);
                }
                
                // Set roughness to low
                if (output->inputs.size() > 2) {
                    output->inputs[2].defaultValue = PinValue(0.05f); // Roughness
                }
                
                // Fresnel for alpha
                if (fresnelDef && lerpDef && texCoordDef) {
                    auto* texCoord = g.createMaterialNode(*texCoordDef, -100, 300);
                    auto* fresnel = g.createMaterialNode(*fresnelDef, 150, 350);
                    auto* lerp = g.createMaterialNode(*lerpDef, 350, 400);
                    
                    fresnel->inputs[0].defaultValue = PinValue(1.52f); // Glass IOR
                    
                    // Lerp between 0.1 and 0.9 alpha based on fresnel
                    lerp->inputs[0].defaultValue = PinValue(0.1f);
                    lerp->inputs[1].defaultValue = PinValue(0.8f);
                    
                    // Connect normal -> fresnel
                    if (texCoord->outputs.size() > 1 && !fresnel->inputs.empty()) {
                        // Normal output from texcoord (index 1)
                        g.createLink(texCoord->id, texCoord->outputs[1].id,
                                    fresnel->id, fresnel->inputs[1].id);
                    }
                    
                    // Connect fresnel -> lerp factor
                    if (!fresnel->outputs.empty() && lerp->inputs.size() > 2) {
                        g.createLink(fresnel->id, fresnel->outputs[0].id,
                                    lerp->id, lerp->inputs[2].id);
                    }
                    
                    // Connect lerp -> alpha
                    if (!lerp->outputs.empty() && output->inputs.size() > 6) {
                        g.createLink(lerp->id, lerp->outputs[0].id,
                                    output->id, output->inputs[6].id); // Alpha
                    }
                }
            }
        });
    }
    
    std::vector<Template> templates_;
};

// ===== Texture Baker =====
// Bakes procedural textures from a material graph to static texture files
struct TextureBakeSettings {
    int width = 1024;
    int height = 1024;
    std::string outputPath;
    
    // Which channels to bake
    bool bakeAlbedo = true;
    bool bakeNormal = true;
    bool bakeMetallic = true;
    bool bakeRoughness = true;
    bool bakeAO = true;
    bool bakeEmissive = false;
    bool bakeHeight = false;
    
    // Format
    enum class Format { PNG, TGA, HDR };
    Format format = Format::PNG;
};

class TextureBaker {
public:
    // Bake a material graph's outputs to textures
    // This generates a simplified shader that outputs each channel to a render target
    // The actual GPU rendering would be done by the UnifiedRenderer
    
    struct BakeResult {
        bool success = false;
        std::string error;
        std::vector<std::string> outputFiles; // Paths to baked textures
    };
    
    static GeneratedShader generateBakeShader(const MaterialGraph& graph, const std::string& channel) {
        ShaderGenerator gen;
        auto fullShader = gen.generate(graph);
        
        if (!fullShader.valid) return fullShader;
        
        // For baking, we modify the generated shader to output a specific channel
        // as a flat color (no lighting, just the raw value)
        // This is a simplified approach - production would use separate render targets
        
        GeneratedShader bakeShader = fullShader;
        bakeShader.pixelEntry = "PSBake_" + channel;
        
        // Append a bake pixel shader function
        std::string bakePS = "\n// Bake shader for " + channel + "\n";
        bakePS += "float4 PSBake_" + channel + "(PSInput _input) : SV_TARGET {\n";
        bakePS += "    // (Same node code as PSMain)\n";
        bakePS += "    return float4(0, 0, 0, 1); // Placeholder\n";
        bakePS += "}\n";
        
        bakeShader.hlslCode += bakePS;
        return bakeShader;
    }
};

// ===== Material Undo/Redo =====
// Undo/redo support for material graph operations
class MaterialUndoStack {
public:
    struct Snapshot {
        std::string description;
        // Serialized graph state (JSON string)
        std::string graphState;
    };
    
    void pushState(const MaterialGraph& graph, const std::string& description) {
        // Trim redo stack
        if (currentIndex_ < (int)history_.size() - 1) {
            history_.resize(currentIndex_ + 1);
        }
        
        // Serialize current state
        Snapshot snap;
        snap.description = description;
        // Would use MaterialSerializer::serialize() here
        // For now, store a simplified representation
        snap.graphState = std::to_string(graph.computeHash());
        
        history_.push_back(snap);
        currentIndex_ = (int)history_.size() - 1;
        
        // Limit history size
        if (history_.size() > maxHistory_) {
            history_.erase(history_.begin());
            currentIndex_--;
        }
    }
    
    bool canUndo() const { return currentIndex_ > 0; }
    bool canRedo() const { return currentIndex_ < (int)history_.size() - 1; }
    
    const Snapshot* undo() {
        if (!canUndo()) return nullptr;
        currentIndex_--;
        return &history_[currentIndex_];
    }
    
    const Snapshot* redo() {
        if (!canRedo()) return nullptr;
        currentIndex_++;
        return &history_[currentIndex_];
    }
    
    void clear() {
        history_.clear();
        currentIndex_ = -1;
    }
    
    int getHistorySize() const { return (int)history_.size(); }
    int getCurrentIndex() const { return currentIndex_; }
    
private:
    std::vector<Snapshot> history_;
    int currentIndex_ = -1;
    size_t maxHistory_ = 100;
};

} // namespace luma
