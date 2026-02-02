// Visual Scripting System - Node-based Programming
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <sstream>

namespace luma {

// ===== Pin Types =====
enum class PinType {
    Flow,       // Execution flow (white)
    Bool,       // Boolean (red)
    Int,        // Integer (cyan)
    Float,      // Float (green)
    String,     // String (magenta)
    Vec2,       // Vector2 (yellow)
    Vec3,       // Vector3 (yellow)
    Object,     // Game object reference (blue)
    Any         // Wildcard
};

// ===== Pin Direction =====
enum class PinDirection {
    Input,
    Output
};

// ===== Pin Value =====
using PinValue = std::variant<
    std::monostate,     // None/Flow
    bool,               // Bool
    int,                // Int
    float,              // Float
    std::string,        // String
    Vec2,               // Vec2
    Vec3,               // Vec3
    uint64_t            // Object ID
>;

// ===== Pin =====
struct Pin {
    uint32_t id = 0;
    std::string name;
    PinType type = PinType::Flow;
    PinDirection direction = PinDirection::Input;
    PinValue defaultValue;
    bool connected = false;
    
    // Visual position (relative to node)
    Vec2 position;
};

// ===== Node Category =====
enum class NodeCategory {
    Events,         // OnStart, OnUpdate, OnCollision, etc.
    Flow,           // Branch, Sequence, ForLoop, etc.
    Math,           // Add, Multiply, Lerp, etc.
    Logic,          // And, Or, Not, Compare
    Variables,      // Get, Set
    Transform,      // GetPosition, SetRotation, etc.
    Physics,        // AddForce, Raycast, etc.
    Audio,          // PlaySound, StopSound
    Animation,      // PlayAnimation, SetParameter
    Input,          // GetKey, GetAxis, GetMousePosition
    Debug,          // Print, DrawLine
    Custom          // User-defined
};

// ===== Link =====
struct Link {
    uint32_t id = 0;
    uint32_t fromNode = 0;
    uint32_t fromPin = 0;
    uint32_t toNode = 0;
    uint32_t toPin = 0;
};

// ===== Visual Script Node =====
class VisualScriptNode {
public:
    uint32_t id = 0;
    std::string name;
    std::string displayName;
    NodeCategory category = NodeCategory::Custom;
    Vec2 position;
    Vec2 size = {150, 100};
    
    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
    
    // Node color based on category
    uint32_t headerColor = 0xFF444444;
    
    // Custom data for specific node types
    std::unordered_map<std::string, PinValue> properties;
    
    // For compile/execute
    std::string comment;
    bool breakpoint = false;
    
    Pin* findPin(uint32_t pinId) {
        for (auto& pin : inputs) {
            if (pin.id == pinId) return &pin;
        }
        for (auto& pin : outputs) {
            if (pin.id == pinId) return &pin;
        }
        return nullptr;
    }
    
    Pin* findInputByName(const std::string& name) {
        for (auto& pin : inputs) {
            if (pin.name == name) return &pin;
        }
        return nullptr;
    }
    
    Pin* findOutputByName(const std::string& name) {
        for (auto& pin : outputs) {
            if (pin.name == name) return &pin;
        }
        return nullptr;
    }
};

// ===== Visual Script Graph =====
class VisualScriptGraph {
public:
    std::string name = "NewGraph";
    std::string description;
    
    std::vector<std::unique_ptr<VisualScriptNode>> nodes;
    std::vector<Link> links;
    
    // Variables
    struct Variable {
        std::string name;
        PinType type;
        PinValue defaultValue;
        bool isPublic = false;
    };
    std::vector<Variable> variables;
    
    // ID generators
    uint32_t nextNodeId = 1;
    uint32_t nextPinId = 1;
    uint32_t nextLinkId = 1;
    
    // Create node
    VisualScriptNode* createNode(const std::string& type) {
        auto node = std::make_unique<VisualScriptNode>();
        node->id = nextNodeId++;
        node->name = type;
        
        // Configure based on type
        configureNode(*node, type);
        
        nodes.push_back(std::move(node));
        return nodes.back().get();
    }
    
    // Delete node
    void deleteNode(uint32_t nodeId) {
        // Remove connected links
        links.erase(
            std::remove_if(links.begin(), links.end(), [nodeId](const Link& l) {
                return l.fromNode == nodeId || l.toNode == nodeId;
            }),
            links.end()
        );
        
        // Remove node
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(), [nodeId](const auto& n) {
                return n->id == nodeId;
            }),
            nodes.end()
        );
    }
    
    // Create link
    bool createLink(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin) {
        auto* srcNode = findNode(fromNode);
        auto* dstNode = findNode(toNode);
        if (!srcNode || !dstNode) return false;
        
        auto* srcPin = srcNode->findPin(fromPin);
        auto* dstPin = dstNode->findPin(toPin);
        if (!srcPin || !dstPin) return false;
        
        // Validate
        if (srcPin->direction != PinDirection::Output) return false;
        if (dstPin->direction != PinDirection::Input) return false;
        if (!canConnect(srcPin->type, dstPin->type)) return false;
        
        // Remove existing connection to input pin (if any)
        links.erase(
            std::remove_if(links.begin(), links.end(), [toPin](const Link& l) {
                return l.toPin == toPin;
            }),
            links.end()
        );
        
        Link link;
        link.id = nextLinkId++;
        link.fromNode = fromNode;
        link.fromPin = fromPin;
        link.toNode = toNode;
        link.toPin = toPin;
        links.push_back(link);
        
        srcPin->connected = true;
        dstPin->connected = true;
        
        return true;
    }
    
    // Delete link
    void deleteLink(uint32_t linkId) {
        auto it = std::find_if(links.begin(), links.end(), [linkId](const Link& l) {
            return l.id == linkId;
        });
        
        if (it != links.end()) {
            // Update pin connected state
            if (auto* node = findNode(it->fromNode)) {
                if (auto* pin = node->findPin(it->fromPin)) {
                    // Check if any other links use this pin
                    bool hasOther = false;
                    for (const auto& l : links) {
                        if (l.id != linkId && l.fromPin == it->fromPin) {
                            hasOther = true;
                            break;
                        }
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
        }
    }
    
    // Find node
    VisualScriptNode* findNode(uint32_t id) {
        for (auto& node : nodes) {
            if (node->id == id) return node.get();
        }
        return nullptr;
    }
    
    // Find link
    Link* findLink(uint32_t id) {
        for (auto& link : links) {
            if (link.id == id) return &link;
        }
        return nullptr;
    }
    
    // Add variable
    void addVariable(const std::string& name, PinType type, bool isPublic = false) {
        Variable var;
        var.name = name;
        var.type = type;
        var.isPublic = isPublic;
        variables.push_back(var);
    }
    
    // Compile to Lua
    std::string compileToLua() const {
        std::stringstream ss;
        
        ss << "-- Auto-generated from Visual Script: " << name << "\n";
        ss << "-- DO NOT EDIT MANUALLY\n\n";
        
        // Variables
        if (!variables.empty()) {
            ss << "-- Variables\n";
            for (const auto& var : variables) {
                ss << "local " << var.name << " = " << getDefaultLuaValue(var.type) << "\n";
            }
            ss << "\n";
        }
        
        // Find event nodes and generate functions
        for (const auto& node : nodes) {
            if (node->category == NodeCategory::Events) {
                ss << "-- " << node->displayName << "\n";
                ss << "function " << node->name << "()\n";
                
                // Follow execution flow
                std::string body = generateNodeCode(*node, "  ");
                ss << body;
                
                ss << "end\n\n";
            }
        }
        
        return ss.str();
    }
    
private:
    void configureNode(VisualScriptNode& node, const std::string& type) {
        // === Event Nodes ===
        if (type == "OnStart") {
            node.displayName = "On Start";
            node.category = NodeCategory::Events;
            node.headerColor = 0xFFCC3333; // Red
            addFlowOutput(node, "Exec");
        }
        else if (type == "OnUpdate") {
            node.displayName = "On Update";
            node.category = NodeCategory::Events;
            node.headerColor = 0xFFCC3333;
            addFlowOutput(node, "Exec");
            addOutput(node, "DeltaTime", PinType::Float);
        }
        else if (type == "OnCollision") {
            node.displayName = "On Collision";
            node.category = NodeCategory::Events;
            node.headerColor = 0xFFCC3333;
            addFlowOutput(node, "Exec");
            addOutput(node, "Other", PinType::Object);
        }
        // === Flow Nodes ===
        else if (type == "Branch") {
            node.displayName = "Branch";
            node.category = NodeCategory::Flow;
            node.headerColor = 0xFF666666;
            addFlowInput(node, "Exec");
            addInput(node, "Condition", PinType::Bool);
            addFlowOutput(node, "True");
            addFlowOutput(node, "False");
        }
        else if (type == "Sequence") {
            node.displayName = "Sequence";
            node.category = NodeCategory::Flow;
            node.headerColor = 0xFF666666;
            addFlowInput(node, "Exec");
            addFlowOutput(node, "Then 0");
            addFlowOutput(node, "Then 1");
        }
        else if (type == "ForLoop") {
            node.displayName = "For Loop";
            node.category = NodeCategory::Flow;
            node.headerColor = 0xFF666666;
            addFlowInput(node, "Exec");
            addInput(node, "Start", PinType::Int);
            addInput(node, "End", PinType::Int);
            addFlowOutput(node, "Loop Body");
            addOutput(node, "Index", PinType::Int);
            addFlowOutput(node, "Completed");
        }
        else if (type == "WhileLoop") {
            node.displayName = "While Loop";
            node.category = NodeCategory::Flow;
            node.headerColor = 0xFF666666;
            addFlowInput(node, "Exec");
            addInput(node, "Condition", PinType::Bool);
            addFlowOutput(node, "Loop Body");
            addFlowOutput(node, "Completed");
        }
        // === Math Nodes ===
        else if (type == "Add") {
            node.displayName = "Add";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33; // Green
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Subtract") {
            node.displayName = "Subtract";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Multiply") {
            node.displayName = "Multiply";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Divide") {
            node.displayName = "Divide";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Lerp") {
            node.displayName = "Lerp";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addInput(node, "Alpha", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Clamp") {
            node.displayName = "Clamp";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "Value", PinType::Float);
            addInput(node, "Min", PinType::Float);
            addInput(node, "Max", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "Random") {
            node.displayName = "Random";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "Min", PinType::Float);
            addInput(node, "Max", PinType::Float);
            addOutput(node, "Result", PinType::Float);
        }
        // === Logic Nodes ===
        else if (type == "And") {
            node.displayName = "And";
            node.category = NodeCategory::Logic;
            node.headerColor = 0xFF3366CC; // Blue
            addInput(node, "A", PinType::Bool);
            addInput(node, "B", PinType::Bool);
            addOutput(node, "Result", PinType::Bool);
        }
        else if (type == "Or") {
            node.displayName = "Or";
            node.category = NodeCategory::Logic;
            node.headerColor = 0xFF3366CC;
            addInput(node, "A", PinType::Bool);
            addInput(node, "B", PinType::Bool);
            addOutput(node, "Result", PinType::Bool);
        }
        else if (type == "Not") {
            node.displayName = "Not";
            node.category = NodeCategory::Logic;
            node.headerColor = 0xFF3366CC;
            addInput(node, "Input", PinType::Bool);
            addOutput(node, "Result", PinType::Bool);
        }
        else if (type == "Compare") {
            node.displayName = "Compare";
            node.category = NodeCategory::Logic;
            node.headerColor = 0xFF3366CC;
            addInput(node, "A", PinType::Float);
            addInput(node, "B", PinType::Float);
            addOutput(node, "==", PinType::Bool);
            addOutput(node, "!=", PinType::Bool);
            addOutput(node, "<", PinType::Bool);
            addOutput(node, ">", PinType::Bool);
        }
        // === Variable Nodes ===
        else if (type == "GetVariable") {
            node.displayName = "Get";
            node.category = NodeCategory::Variables;
            node.headerColor = 0xFF9933CC; // Purple
            addOutput(node, "Value", PinType::Any);
            node.properties["VariableName"] = std::string("");
        }
        else if (type == "SetVariable") {
            node.displayName = "Set";
            node.category = NodeCategory::Variables;
            node.headerColor = 0xFF9933CC;
            addFlowInput(node, "Exec");
            addInput(node, "Value", PinType::Any);
            addFlowOutput(node, "Exec");
            addOutput(node, "Value", PinType::Any);
            node.properties["VariableName"] = std::string("");
        }
        // === Transform Nodes ===
        else if (type == "GetPosition") {
            node.displayName = "Get Position";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933; // Orange
            addInput(node, "Object", PinType::Object);
            addOutput(node, "Position", PinType::Vec3);
        }
        else if (type == "SetPosition") {
            node.displayName = "Set Position";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Position", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "GetRotation") {
            node.displayName = "Get Rotation";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addInput(node, "Object", PinType::Object);
            addOutput(node, "Rotation", PinType::Vec3);
        }
        else if (type == "SetRotation") {
            node.displayName = "Set Rotation";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Rotation", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "Translate") {
            node.displayName = "Translate";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Delta", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "Rotate") {
            node.displayName = "Rotate";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Euler", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "LookAt") {
            node.displayName = "Look At";
            node.category = NodeCategory::Transform;
            node.headerColor = 0xFFCC9933;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Target", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        // === Physics Nodes ===
        else if (type == "AddForce") {
            node.displayName = "Add Force";
            node.category = NodeCategory::Physics;
            node.headerColor = 0xFF33CCCC; // Cyan
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Force", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "AddImpulse") {
            node.displayName = "Add Impulse";
            node.category = NodeCategory::Physics;
            node.headerColor = 0xFF33CCCC;
            addFlowInput(node, "Exec");
            addInput(node, "Object", PinType::Object);
            addInput(node, "Impulse", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        else if (type == "Raycast") {
            node.displayName = "Raycast";
            node.category = NodeCategory::Physics;
            node.headerColor = 0xFF33CCCC;
            addInput(node, "Origin", PinType::Vec3);
            addInput(node, "Direction", PinType::Vec3);
            addInput(node, "Distance", PinType::Float);
            addOutput(node, "Hit", PinType::Bool);
            addOutput(node, "HitPoint", PinType::Vec3);
            addOutput(node, "HitObject", PinType::Object);
        }
        // === Audio Nodes ===
        else if (type == "PlaySound") {
            node.displayName = "Play Sound";
            node.category = NodeCategory::Audio;
            node.headerColor = 0xFFCC33CC; // Magenta
            addFlowInput(node, "Exec");
            addInput(node, "Sound", PinType::String);
            addInput(node, "Volume", PinType::Float);
            addFlowOutput(node, "Exec");
        }
        else if (type == "StopSound") {
            node.displayName = "Stop Sound";
            node.category = NodeCategory::Audio;
            node.headerColor = 0xFFCC33CC;
            addFlowInput(node, "Exec");
            addInput(node, "Sound", PinType::String);
            addFlowOutput(node, "Exec");
        }
        // === Input Nodes ===
        else if (type == "GetKey") {
            node.displayName = "Get Key";
            node.category = NodeCategory::Input;
            node.headerColor = 0xFFCCCC33; // Yellow
            addInput(node, "Key", PinType::String);
            addOutput(node, "Pressed", PinType::Bool);
            addOutput(node, "Held", PinType::Bool);
            addOutput(node, "Released", PinType::Bool);
        }
        else if (type == "GetAxis") {
            node.displayName = "Get Axis";
            node.category = NodeCategory::Input;
            node.headerColor = 0xFFCCCC33;
            addInput(node, "Axis", PinType::String);
            addOutput(node, "Value", PinType::Float);
        }
        else if (type == "GetMousePosition") {
            node.displayName = "Get Mouse Position";
            node.category = NodeCategory::Input;
            node.headerColor = 0xFFCCCC33;
            addOutput(node, "Position", PinType::Vec2);
        }
        // === Debug Nodes ===
        else if (type == "Print") {
            node.displayName = "Print";
            node.category = NodeCategory::Debug;
            node.headerColor = 0xFF888888;
            addFlowInput(node, "Exec");
            addInput(node, "Message", PinType::String);
            addFlowOutput(node, "Exec");
        }
        else if (type == "DrawDebugLine") {
            node.displayName = "Draw Debug Line";
            node.category = NodeCategory::Debug;
            node.headerColor = 0xFF888888;
            addFlowInput(node, "Exec");
            addInput(node, "Start", PinType::Vec3);
            addInput(node, "End", PinType::Vec3);
            addFlowOutput(node, "Exec");
        }
        // === Vector Nodes ===
        else if (type == "MakeVec3") {
            node.displayName = "Make Vec3";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "X", PinType::Float);
            addInput(node, "Y", PinType::Float);
            addInput(node, "Z", PinType::Float);
            addOutput(node, "Vector", PinType::Vec3);
        }
        else if (type == "BreakVec3") {
            node.displayName = "Break Vec3";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "Vector", PinType::Vec3);
            addOutput(node, "X", PinType::Float);
            addOutput(node, "Y", PinType::Float);
            addOutput(node, "Z", PinType::Float);
        }
        else if (type == "VectorLength") {
            node.displayName = "Vector Length";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "Vector", PinType::Vec3);
            addOutput(node, "Length", PinType::Float);
        }
        else if (type == "Normalize") {
            node.displayName = "Normalize";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "Vector", PinType::Vec3);
            addOutput(node, "Result", PinType::Vec3);
        }
        else if (type == "DotProduct") {
            node.displayName = "Dot Product";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Vec3);
            addInput(node, "B", PinType::Vec3);
            addOutput(node, "Result", PinType::Float);
        }
        else if (type == "CrossProduct") {
            node.displayName = "Cross Product";
            node.category = NodeCategory::Math;
            node.headerColor = 0xFF33AA33;
            addInput(node, "A", PinType::Vec3);
            addInput(node, "B", PinType::Vec3);
            addOutput(node, "Result", PinType::Vec3);
        }
        else {
            // Unknown node
            node.displayName = type;
            node.headerColor = 0xFF444444;
        }
    }
    
    void addFlowInput(VisualScriptNode& node, const std::string& name) {
        Pin pin;
        pin.id = nextPinId++;
        pin.name = name;
        pin.type = PinType::Flow;
        pin.direction = PinDirection::Input;
        node.inputs.push_back(pin);
    }
    
    void addFlowOutput(VisualScriptNode& node, const std::string& name) {
        Pin pin;
        pin.id = nextPinId++;
        pin.name = name;
        pin.type = PinType::Flow;
        pin.direction = PinDirection::Output;
        node.outputs.push_back(pin);
    }
    
    void addInput(VisualScriptNode& node, const std::string& name, PinType type) {
        Pin pin;
        pin.id = nextPinId++;
        pin.name = name;
        pin.type = type;
        pin.direction = PinDirection::Input;
        node.inputs.push_back(pin);
    }
    
    void addOutput(VisualScriptNode& node, const std::string& name, PinType type) {
        Pin pin;
        pin.id = nextPinId++;
        pin.name = name;
        pin.type = type;
        pin.direction = PinDirection::Output;
        node.outputs.push_back(pin);
    }
    
    static bool canConnect(PinType from, PinType to) {
        if (from == to) return true;
        if (from == PinType::Any || to == PinType::Any) return true;
        
        // Allow int -> float
        if (from == PinType::Int && to == PinType::Float) return true;
        
        return false;
    }
    
    static std::string getDefaultLuaValue(PinType type) {
        switch (type) {
            case PinType::Bool:   return "false";
            case PinType::Int:    return "0";
            case PinType::Float:  return "0.0";
            case PinType::String: return "\"\"";
            case PinType::Vec2:   return "{x=0, y=0}";
            case PinType::Vec3:   return "{x=0, y=0, z=0}";
            case PinType::Object: return "nil";
            default:              return "nil";
        }
    }
    
    std::string generateNodeCode(const VisualScriptNode& node, const std::string& indent) const {
        std::stringstream ss;
        
        // Find connected flow output
        for (const auto& pin : node.outputs) {
            if (pin.type == PinType::Flow && pin.connected) {
                // Find link
                for (const auto& link : links) {
                    if (link.fromNode == node.id && link.fromPin == pin.id) {
                        // Find target node
                        for (const auto& n : nodes) {
                            if (n->id == link.toNode) {
                                ss << indent << "-- " << n->displayName << "\n";
                                // Generate code for this node
                                ss << generateActionCode(*n, indent);
                                // Continue flow
                                ss << generateNodeCode(*n, indent);
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
        
        return ss.str();
    }
    
    std::string generateActionCode(const VisualScriptNode& node, const std::string& indent) const {
        std::stringstream ss;
        
        if (node.name == "Print") {
            ss << indent << "print(\"message\")\n";
        }
        else if (node.name == "SetPosition") {
            ss << indent << "setPosition(obj, pos)\n";
        }
        else if (node.name == "Branch") {
            ss << indent << "if condition then\n";
            ss << indent << "  -- true branch\n";
            ss << indent << "else\n";
            ss << indent << "  -- false branch\n";
            ss << indent << "end\n";
        }
        
        return ss.str();
    }
};

// ===== Node Library =====
struct NodeDefinition {
    std::string name;
    std::string displayName;
    NodeCategory category;
    std::string description;
};

class NodeLibrary {
public:
    static NodeLibrary& getInstance() {
        static NodeLibrary instance;
        return instance;
    }
    
    const std::vector<NodeDefinition>& getNodes() const { return nodes_; }
    
    std::vector<NodeDefinition> getNodesInCategory(NodeCategory category) const {
        std::vector<NodeDefinition> result;
        for (const auto& node : nodes_) {
            if (node.category == category) {
                result.push_back(node);
            }
        }
        return result;
    }
    
    std::vector<NodeDefinition> searchNodes(const std::string& query) const {
        std::vector<NodeDefinition> result;
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (const auto& node : nodes_) {
            std::string lowerName = node.displayName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find(lowerQuery) != std::string::npos) {
                result.push_back(node);
            }
        }
        return result;
    }
    
private:
    NodeLibrary() {
        // Events
        nodes_.push_back({"OnStart", "On Start", NodeCategory::Events, "Called when the game starts"});
        nodes_.push_back({"OnUpdate", "On Update", NodeCategory::Events, "Called every frame"});
        nodes_.push_back({"OnCollision", "On Collision", NodeCategory::Events, "Called when collision occurs"});
        
        // Flow
        nodes_.push_back({"Branch", "Branch", NodeCategory::Flow, "Conditional execution"});
        nodes_.push_back({"Sequence", "Sequence", NodeCategory::Flow, "Execute in order"});
        nodes_.push_back({"ForLoop", "For Loop", NodeCategory::Flow, "Loop with counter"});
        nodes_.push_back({"WhileLoop", "While Loop", NodeCategory::Flow, "Loop while condition is true"});
        
        // Math
        nodes_.push_back({"Add", "Add", NodeCategory::Math, "Add two values"});
        nodes_.push_back({"Subtract", "Subtract", NodeCategory::Math, "Subtract two values"});
        nodes_.push_back({"Multiply", "Multiply", NodeCategory::Math, "Multiply two values"});
        nodes_.push_back({"Divide", "Divide", NodeCategory::Math, "Divide two values"});
        nodes_.push_back({"Lerp", "Lerp", NodeCategory::Math, "Linear interpolation"});
        nodes_.push_back({"Clamp", "Clamp", NodeCategory::Math, "Clamp value between min and max"});
        nodes_.push_back({"Random", "Random", NodeCategory::Math, "Random value between min and max"});
        nodes_.push_back({"MakeVec3", "Make Vec3", NodeCategory::Math, "Create a Vec3 from components"});
        nodes_.push_back({"BreakVec3", "Break Vec3", NodeCategory::Math, "Get Vec3 components"});
        nodes_.push_back({"VectorLength", "Vector Length", NodeCategory::Math, "Get length of vector"});
        nodes_.push_back({"Normalize", "Normalize", NodeCategory::Math, "Normalize vector"});
        nodes_.push_back({"DotProduct", "Dot Product", NodeCategory::Math, "Dot product of two vectors"});
        nodes_.push_back({"CrossProduct", "Cross Product", NodeCategory::Math, "Cross product of two vectors"});
        
        // Logic
        nodes_.push_back({"And", "And", NodeCategory::Logic, "Logical AND"});
        nodes_.push_back({"Or", "Or", NodeCategory::Logic, "Logical OR"});
        nodes_.push_back({"Not", "Not", NodeCategory::Logic, "Logical NOT"});
        nodes_.push_back({"Compare", "Compare", NodeCategory::Logic, "Compare two values"});
        
        // Variables
        nodes_.push_back({"GetVariable", "Get Variable", NodeCategory::Variables, "Get variable value"});
        nodes_.push_back({"SetVariable", "Set Variable", NodeCategory::Variables, "Set variable value"});
        
        // Transform
        nodes_.push_back({"GetPosition", "Get Position", NodeCategory::Transform, "Get object position"});
        nodes_.push_back({"SetPosition", "Set Position", NodeCategory::Transform, "Set object position"});
        nodes_.push_back({"GetRotation", "Get Rotation", NodeCategory::Transform, "Get object rotation"});
        nodes_.push_back({"SetRotation", "Set Rotation", NodeCategory::Transform, "Set object rotation"});
        nodes_.push_back({"Translate", "Translate", NodeCategory::Transform, "Move object"});
        nodes_.push_back({"Rotate", "Rotate", NodeCategory::Transform, "Rotate object"});
        nodes_.push_back({"LookAt", "Look At", NodeCategory::Transform, "Make object look at target"});
        
        // Physics
        nodes_.push_back({"AddForce", "Add Force", NodeCategory::Physics, "Apply force to object"});
        nodes_.push_back({"AddImpulse", "Add Impulse", NodeCategory::Physics, "Apply impulse to object"});
        nodes_.push_back({"Raycast", "Raycast", NodeCategory::Physics, "Cast ray and detect hit"});
        
        // Audio
        nodes_.push_back({"PlaySound", "Play Sound", NodeCategory::Audio, "Play audio clip"});
        nodes_.push_back({"StopSound", "Stop Sound", NodeCategory::Audio, "Stop audio clip"});
        
        // Input
        nodes_.push_back({"GetKey", "Get Key", NodeCategory::Input, "Check keyboard key state"});
        nodes_.push_back({"GetAxis", "Get Axis", NodeCategory::Input, "Get input axis value"});
        nodes_.push_back({"GetMousePosition", "Get Mouse Position", NodeCategory::Input, "Get mouse screen position"});
        
        // Debug
        nodes_.push_back({"Print", "Print", NodeCategory::Debug, "Print message to console"});
        nodes_.push_back({"DrawDebugLine", "Draw Debug Line", NodeCategory::Debug, "Draw debug line in world"});
    }
    
    std::vector<NodeDefinition> nodes_;
};

// ===== Helper Functions =====
inline const char* getCategoryName(NodeCategory category) {
    switch (category) {
        case NodeCategory::Events:    return "Events";
        case NodeCategory::Flow:      return "Flow Control";
        case NodeCategory::Math:      return "Math";
        case NodeCategory::Logic:     return "Logic";
        case NodeCategory::Variables: return "Variables";
        case NodeCategory::Transform: return "Transform";
        case NodeCategory::Physics:   return "Physics";
        case NodeCategory::Audio:     return "Audio";
        case NodeCategory::Animation: return "Animation";
        case NodeCategory::Input:     return "Input";
        case NodeCategory::Debug:     return "Debug";
        default:                      return "Custom";
    }
}

inline uint32_t getPinColor(PinType type) {
    switch (type) {
        case PinType::Flow:   return 0xFFFFFFFF; // White
        case PinType::Bool:   return 0xFF3333CC; // Red
        case PinType::Int:    return 0xFFCCCC33; // Cyan
        case PinType::Float:  return 0xFF33CC33; // Green
        case PinType::String: return 0xFFCC33CC; // Magenta
        case PinType::Vec2:   return 0xFF33CCCC; // Yellow
        case PinType::Vec3:   return 0xFF33CCCC; // Yellow
        case PinType::Object: return 0xFFCC9933; // Blue
        case PinType::Any:    return 0xFF888888; // Gray
        default:              return 0xFFFFFFFF;
    }
}

}  // namespace luma
