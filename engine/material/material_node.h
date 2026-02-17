// LUMA Material Node System
// Node-based material graph for dynamic shader generation
// Extends the VisualScript node infrastructure for PBR material authoring

#pragma once

#include "engine/script/visual_script.h"
#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstring>
#include <sstream>

namespace luma {

// ===== HLSL Type Mapping =====
// Maps PinType to the HLSL type string used in code generation
inline const char* pinTypeToHLSL(PinType type) {
    switch (type) {
        case PinType::Float:          return "float";
        case PinType::Vec2:           return "float2";
        case PinType::Vec3:           return "float3";
        case PinType::Vec4:           return "float4";
        case PinType::Color:          return "float4";
        case PinType::UV:             return "float2";
        case PinType::Normal:         return "float3";
        case PinType::Bool:           return "bool";
        case PinType::Int:            return "int";
        case PinType::Texture2D:      return "Texture2D";
        case PinType::Sampler:        return "SamplerState";
        case PinType::MaterialOutput: return "void";
        default:                      return "float";
    }
}

// Number of components in a pin type (for swizzle/broadcast)
inline int pinTypeComponents(PinType type) {
    switch (type) {
        case PinType::Float:  return 1;
        case PinType::Vec2:   return 2;
        case PinType::UV:     return 2;
        case PinType::Vec3:   return 3;
        case PinType::Normal: return 3;
        case PinType::Vec4:   return 4;
        case PinType::Color:  return 4;
        default:              return 1;
    }
}

// Generate HLSL cast/conversion code between pin types
inline std::string generateTypeConversion(const std::string& expr, PinType from, PinType to) {
    if (from == to) return expr;
    
    int fromC = pinTypeComponents(from);
    int toC = pinTypeComponents(to);
    
    if (fromC == toC) return expr; // Same dimension, just different semantic
    
    if (fromC == 1 && toC == 2) return "float2(" + expr + ", " + expr + ")";
    if (fromC == 1 && toC == 3) return "float3(" + expr + ", " + expr + ", " + expr + ")";
    if (fromC == 1 && toC == 4) return "float4(" + expr + ", " + expr + ", " + expr + ", 1.0)";
    if (fromC == 2 && toC == 1) return "(" + expr + ").x";
    if (fromC == 2 && toC == 3) return "float3(" + expr + ", 0.0)";
    if (fromC == 2 && toC == 4) return "float4(" + expr + ", 0.0, 1.0)";
    if (fromC == 3 && toC == 1) return "(" + expr + ").x";
    if (fromC == 3 && toC == 2) return "(" + expr + ").xy";
    if (fromC == 3 && toC == 4) return "float4(" + expr + ", 1.0)";
    if (fromC == 4 && toC == 1) return "(" + expr + ").x";
    if (fromC == 4 && toC == 2) return "(" + expr + ").xy";
    if (fromC == 4 && toC == 3) return "(" + expr + ").xyz";
    
    return expr;
}

// ===== Default value to HLSL literal =====
inline std::string pinDefaultToHLSL(PinType type, const PinValue& value) {
    if (std::holds_alternative<float>(value)) {
        float v = std::get<float>(value);
        std::ostringstream ss;
        ss << v;
        std::string s = ss.str();
        // Ensure it has a decimal point for HLSL
        if (s.find('.') == std::string::npos) s += ".0";
        return s;
    }
    if (std::holds_alternative<Vec3>(value)) {
        Vec3 v = std::get<Vec3>(value);
        std::ostringstream ss;
        ss << "float3(" << v.x << ", " << v.y << ", " << v.z << ")";
        return ss.str();
    }
    if (std::holds_alternative<Vec4>(value)) {
        Vec4 v = std::get<Vec4>(value);
        std::ostringstream ss;
        ss << "float4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return ss.str();
    }
    if (std::holds_alternative<Vec2>(value)) {
        Vec2 v = std::get<Vec2>(value);
        std::ostringstream ss;
        ss << "float2(" << v.x << ", " << v.y << ")";
        return ss.str();
    }
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    
    // Defaults per type
    switch (type) {
        case PinType::Float:  return "0.0";
        case PinType::Vec2:   return "float2(0.0, 0.0)";
        case PinType::UV:     return "float2(0.0, 0.0)";
        case PinType::Vec3:   return "float3(0.0, 0.0, 0.0)";
        case PinType::Normal: return "float3(0.0, 0.0, 1.0)";
        case PinType::Vec4:   return "float4(0.0, 0.0, 0.0, 0.0)";
        case PinType::Color:  return "float4(0.0, 0.0, 0.0, 1.0)";
        case PinType::Bool:   return "false";
        case PinType::Int:    return "0";
        default:              return "0.0";
    }
}

// ===== Material Node Definition =====
// Describes a type of material node (template, not instance)
struct MaterialNodeDef {
    std::string typeName;         // Unique type identifier, e.g. "Mat_ImageTexture"
    std::string displayName;      // Display name, e.g. "Image Texture"
    NodeCategory category;
    std::string description;
    uint32_t headerColor = 0xFF444444;
    
    // Pin definitions
    struct PinDef {
        std::string name;
        PinType type = PinType::Float;
        PinDirection direction = PinDirection::Input;
        PinValue defaultValue = PinValue{std::monostate{}};
    };
    std::vector<PinDef> pins;
    
    // HLSL code template for this node
    // Uses placeholders: {input:Name}, {output:Name}, {param:Name}
    // Example: "{output:Result} = {input:A} + {input:B};"
    std::string hlslTemplate;
    
    // For nodes that need helper functions (noise, etc.)
    std::string hlslHelperIncludes;  // e.g. "procedural" to include procedural.hlsli
    
    // Node properties (user-configurable parameters shown in UI)
    struct PropertyDef {
        std::string name;
        std::string displayName;
        PinType type = PinType::Float;  // Type for UI display
        PinValue defaultValue = PinValue{std::monostate{}};
        float minValue = 0.0f;   // For sliders
        float maxValue = 1.0f;
        std::vector<std::string> enumOptions; // For dropdown
    };
    std::vector<PropertyDef> properties;
    
    // Whether this is an output node (only one per graph)
    bool isOutput = false;
    
    // Helper to add pins - base version with explicit PinValue
    void addInput(const std::string& name, PinType type, PinValue defaultVal) {
        pins.push_back({name, type, PinDirection::Input, defaultVal});
    }
    // Overload: no default value
    void addInput(const std::string& name, PinType type) {
        pins.push_back({name, type, PinDirection::Input, PinValue{std::monostate{}}});
    }
    // Overloads for MSVC implicit variant conversion workaround
    void addInput(const std::string& name, PinType type, float defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addInput(const std::string& name, PinType type, int defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addInput(const std::string& name, PinType type, bool defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addInput(const std::string& name, PinType type, const Vec3& defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addInput(const std::string& name, PinType type, const Vec4& defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addInput(const std::string& name, PinType type, const Vec2& defaultVal) {
        pins.push_back({name, type, PinDirection::Input, PinValue(defaultVal)});
    }
    void addOutput(const std::string& name, PinType type) {
        pins.push_back({name, type, PinDirection::Output, PinValue{std::monostate{}}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type, 
                     PinValue defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, defaultVal, minV, maxV, {}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type) {
        properties.push_back({name, display, type, PinValue{std::monostate{}}, 0.0f, 1.0f, {}});
    }
    // Overloads for common property types  
    void addProperty(const std::string& name, const std::string& display, PinType type,
                     float defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, PinValue(defaultVal), minV, maxV, {}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type,
                     int defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, PinValue(defaultVal), minV, maxV, {}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type,
                     const Vec3& defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, PinValue(defaultVal), minV, maxV, {}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type,
                     const Vec4& defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, PinValue(defaultVal), minV, maxV, {}});
    }
    void addProperty(const std::string& name, const std::string& display, PinType type,
                     const std::string& defaultVal, float minV = 0.0f, float maxV = 1.0f) {
        properties.push_back({name, display, type, PinValue(defaultVal), minV, maxV, {}});
    }
    void addEnumProperty(const std::string& name, const std::string& display,
                         const std::vector<std::string>& options, int defaultIdx = 0) {
        PropertyDef prop;
        prop.name = name;
        prop.displayName = display;
        prop.type = PinType::Int;
        prop.defaultValue = PinValue(defaultIdx);
        prop.enumOptions = options;
        properties.push_back(prop);
    }
};

// ===== Texture Slot Info =====
// Describes a texture bound by a material node graph
struct MaterialTextureSlot {
    uint32_t nodeId = 0;            // Which node uses this texture
    std::string texturePath;         // Path to texture file
    int registerSlot = 0;            // HLSL register t-slot (t10+)
    std::string variableName;        // HLSL variable name
    void* gpuHandle = nullptr;       // Runtime GPU texture handle
};

// ===== Uniform Info =====
// Describes a custom uniform parameter in the generated shader
struct MaterialUniform {
    std::string name;                // HLSL variable name
    PinType type = PinType::Float;
    PinValue value = PinValue{std::monostate{}};
    uint32_t nodeId = 0;             // Owning node
    std::string propertyName;        // Property name in the node
    uint32_t offsetInBuffer = 0;     // Byte offset in material constant buffer
};

// ===== Color Ramp Stop =====
struct ColorRampStop {
    float position = 0.0f;   // [0, 1]
    float color[4] = {0, 0, 0, 1}; // RGBA
};

// ===== Material Graph =====
// Specialization of VisualScriptGraph for material node editing
class MaterialGraph {
public:
    std::string name = "New Material";
    std::string description;
    
    // Node and link storage (reuses visual script types)
    std::vector<std::unique_ptr<VisualScriptNode>> nodes;
    std::vector<Link> links;
    
    // ID generators
    uint32_t nextNodeId = 1;
    uint32_t nextPinId = 1;
    uint32_t nextLinkId = 1;
    
    // Output node reference
    uint32_t outputNodeId = 0;
    
    // Texture slots used by the graph
    std::vector<MaterialTextureSlot> textureSlots;
    
    // Custom uniforms (node parameters exposed to shader)
    std::vector<MaterialUniform> uniforms;
    
    // Color ramp data (per-node storage)
    std::unordered_map<uint32_t, std::vector<ColorRampStop>> colorRamps;
    
    // Dirty flag for recompilation
    bool needsRecompile = true;
    uint64_t lastCompiledHash = 0;
    
    // ===== Node Management =====
    
    VisualScriptNode* createMaterialNode(const MaterialNodeDef& def, float x = 0, float y = 0) {
        auto node = std::make_unique<VisualScriptNode>();
        node->id = nextNodeId++;
        node->name = def.typeName;
        node->displayName = def.displayName;
        node->category = def.category;
        node->headerColor = def.headerColor;
        node->position = {x, y};
        
        // Create pins from definition
        for (const auto& pinDef : def.pins) {
            Pin pin;
            pin.id = nextPinId++;
            pin.name = pinDef.name;
            pin.type = pinDef.type;
            pin.direction = pinDef.direction;
            pin.defaultValue = pinDef.defaultValue;
            
            if (pinDef.direction == PinDirection::Input) {
                node->inputs.push_back(pin);
            } else {
                node->outputs.push_back(pin);
            }
        }
        
        // Store default property values
        for (const auto& propDef : def.properties) {
            node->properties[propDef.name] = propDef.defaultValue;
        }
        
        // Store HLSL template in comment field for code generation
        node->comment = def.hlslTemplate;
        
        // Track output node
        if (def.isOutput) {
            outputNodeId = node->id;
        }
        
        // Auto-size based on pin count
        int maxPins = std::max(node->inputs.size(), node->outputs.size());
        node->size = {200.0f, static_cast<float>(60 + maxPins * 24)};
        
        auto* ptr = node.get();
        nodes.push_back(std::move(node));
        needsRecompile = true;
        return ptr;
    }
    
    void deleteNode(uint32_t nodeId) {
        // Don't allow deleting the output node
        if (nodeId == outputNodeId) return;
        
        // Remove connected links
        links.erase(
            std::remove_if(links.begin(), links.end(), [nodeId](const Link& l) {
                return l.fromNode == nodeId || l.toNode == nodeId;
            }),
            links.end()
        );
        
        // Remove color ramp data
        colorRamps.erase(nodeId);
        
        // Remove node
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(), [nodeId](const auto& n) {
                return n->id == nodeId;
            }),
            nodes.end()
        );
        
        needsRecompile = true;
    }
    
    bool createLink(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin) {
        auto* srcNode = findNode(fromNode);
        auto* dstNode = findNode(toNode);
        if (!srcNode || !dstNode) return false;
        
        auto* srcPin = srcNode->findPin(fromPin);
        auto* dstPin = dstNode->findPin(toPin);
        if (!srcPin || !dstPin) return false;
        
        if (srcPin->direction != PinDirection::Output) return false;
        if (dstPin->direction != PinDirection::Input) return false;
        if (!VisualScriptGraph::canConnect(srcPin->type, dstPin->type)) return false;
        
        // Prevent self-links
        if (fromNode == toNode) return false;
        
        // Prevent cycles
        if (wouldCreateCycle(fromNode, toNode)) return false;
        
        // Remove existing connection to input pin
        for (auto it = links.begin(); it != links.end(); ) {
            if (it->toPin == toPin) {
                // Update old source pin's connected state
                if (auto* oldSrcNode = findNode(it->fromNode)) {
                    if (auto* oldSrcPin = oldSrcNode->findPin(it->fromPin)) {
                        bool hasOther = false;
                        for (const auto& l : links) {
                            if (l.id != it->id && l.fromPin == it->fromPin) { hasOther = true; break; }
                        }
                        if (!hasOther) oldSrcPin->connected = false;
                    }
                }
                it = links.erase(it);
            } else {
                ++it;
            }
        }
        
        Link link;
        link.id = nextLinkId++;
        link.fromNode = fromNode;
        link.fromPin = fromPin;
        link.toNode = toNode;
        link.toPin = toPin;
        links.push_back(link);
        
        srcPin->connected = true;
        dstPin->connected = true;
        
        needsRecompile = true;
        return true;
    }
    
    void deleteLink(uint32_t linkId) {
        auto it = std::find_if(links.begin(), links.end(), [linkId](const Link& l) {
            return l.id == linkId;
        });
        if (it != links.end()) {
            if (auto* node = findNode(it->fromNode)) {
                if (auto* pin = node->findPin(it->fromPin)) {
                    bool hasOther = false;
                    for (const auto& l : links) {
                        if (l.id != linkId && l.fromPin == it->fromPin) { hasOther = true; break; }
                    }
                    if (!hasOther) pin->connected = false;
                }
            }
            if (auto* node = findNode(it->toNode)) {
                if (auto* pin = node->findPin(it->toPin)) {
                    pin->connected = false;
                }
            }
            links.erase(it);
            needsRecompile = true;
        }
    }
    
    VisualScriptNode* findNode(uint32_t id) {
        for (auto& n : nodes) {
            if (n->id == id) return n.get();
        }
        return nullptr;
    }
    
    const VisualScriptNode* findNode(uint32_t id) const {
        for (auto& n : nodes) {
            if (n->id == id) return n.get();
        }
        return nullptr;
    }
    
    Link* findLink(uint32_t id) {
        for (auto& l : links) {
            if (l.id == id) return &l;
        }
        return nullptr;
    }
    
    // Find the link connected to an input pin
    const Link* findInputLink(uint32_t pinId) const {
        for (const auto& l : links) {
            if (l.toPin == pinId) return &l;
        }
        return nullptr;
    }
    
    // Find all links from an output pin
    std::vector<const Link*> findOutputLinks(uint32_t pinId) const {
        std::vector<const Link*> result;
        for (const auto& l : links) {
            if (l.fromPin == pinId) result.push_back(&l);
        }
        return result;
    }
    
    // ===== Graph Analysis =====
    
    // Check if creating a link from -> to would create a cycle
    bool wouldCreateCycle(uint32_t fromNodeId, uint32_t toNodeId) const {
        // DFS from toNode's outputs to see if we can reach fromNode
        std::unordered_set<uint32_t> visited;
        return canReach(toNodeId, fromNodeId, visited);
    }
    
    // Topological sort of nodes (output node last)
    // Returns ordered list of node IDs
    std::vector<uint32_t> topologicalSort() const {
        // Build adjacency list (from -> to)
        std::unordered_map<uint32_t, std::vector<uint32_t>> adj;
        std::unordered_map<uint32_t, int> inDegree;
        
        for (const auto& n : nodes) {
            adj[n->id] = {};
            inDegree[n->id] = 0;
        }
        
        for (const auto& link : links) {
            adj[link.fromNode].push_back(link.toNode);
            inDegree[link.toNode]++;
        }
        
        // Kahn's algorithm
        std::vector<uint32_t> queue;
        for (const auto& [id, deg] : inDegree) {
            if (deg == 0) queue.push_back(id);
        }
        
        std::vector<uint32_t> sorted;
        while (!queue.empty()) {
            uint32_t curr = queue.back();
            queue.pop_back();
            sorted.push_back(curr);
            
            for (uint32_t next : adj[curr]) {
                inDegree[next]--;
                if (inDegree[next] == 0) {
                    queue.push_back(next);
                }
            }
        }
        
        return sorted;
    }
    
    // Collect all nodes that contribute to the output
    std::unordered_set<uint32_t> collectActiveNodes() const {
        std::unordered_set<uint32_t> active;
        if (outputNodeId == 0) return active;
        collectUpstream(outputNodeId, active);
        return active;
    }
    
    // ===== Hashing =====
    
    // Compute a hash of the graph topology and parameters for caching
    uint64_t computeHash() const {
        uint64_t hash = 0x12345678ABCDEF01ULL;
        
        // Hash node types and properties
        for (const auto& node : nodes) {
            hash ^= std::hash<std::string>{}(node->name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<uint32_t>{}(node->id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        
        // Hash links
        for (const auto& link : links) {
            hash ^= std::hash<uint32_t>{}(link.fromNode * 1000 + link.toNode) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<uint32_t>{}(link.fromPin * 1000 + link.toPin) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        
        return hash;
    }
    
    // ===== Default Graph =====
    
    // Create a default graph with just an output node
    void createDefaultGraph() {
        nodes.clear();
        links.clear();
        nextNodeId = 1;
        nextPinId = 1;
        nextLinkId = 1;
        outputNodeId = 0;
        textureSlots.clear();
        uniforms.clear();
        colorRamps.clear();
        needsRecompile = true;
    }
    
private:
    bool canReach(uint32_t from, uint32_t target, std::unordered_set<uint32_t>& visited) const {
        if (from == target) return true;
        if (visited.count(from)) return false;
        visited.insert(from);
        
        for (const auto& link : links) {
            if (link.fromNode == from) {
                if (canReach(link.toNode, target, visited)) return true;
            }
        }
        return false;
    }
    
    void collectUpstream(uint32_t nodeId, std::unordered_set<uint32_t>& active) const {
        if (active.count(nodeId)) return;
        active.insert(nodeId);
        
        const auto* node = findNode(nodeId);
        if (!node) return;
        
        // Find all input links to this node
        for (const auto& pin : node->inputs) {
            for (const auto& link : links) {
                if (link.toPin == pin.id) {
                    collectUpstream(link.fromNode, active);
                }
            }
        }
    }
};

// ===== Material Node Editor State =====
// Runtime state for the material node editor UI
struct MaterialNodeEditorState {
    std::unique_ptr<MaterialGraph> graph;
    
    // View state
    Vec2 scrollOffset = {0, 0};
    float zoom = 1.0f;
    
    // Selection state
    int selectedNodeId = -1;
    int hoveredNodeId = -1;
    int draggingNodeId = -1;
    Vec2 dragOffset;
    
    // Link creation
    bool creatingLink = false;
    uint32_t linkStartNode = 0;
    uint32_t linkStartPin = 0;
    bool linkStartIsOutput = true;
    
    // Context menu
    bool showContextMenu = false;
    Vec2 contextMenuPos;
    char searchBuffer[256] = {0};
    
    // Preview
    int previewShape = 0; // 0=Sphere, 1=Cube, 2=Plane, 3=Cylinder
    float previewYaw = 0.0f;
    float previewPitch = 0.3f;
    
    // Compilation
    bool isCompiling = false;
    std::string lastError;
    std::string generatedHLSL;
    bool showGeneratedCode = false;
    
    // Material instance
    uint32_t materialId = 0;
    
    void init() {
        graph = std::make_unique<MaterialGraph>();
        graph->createDefaultGraph();
    }
    
    void reset() {
        selectedNodeId = -1;
        hoveredNodeId = -1;
        draggingNodeId = -1;
        creatingLink = false;
        showContextMenu = false;
        searchBuffer[0] = 0;
    }
};

} // namespace luma
