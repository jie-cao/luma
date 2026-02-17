// LUMA Material Graph Serializer
// JSON-based serialization/deserialization for material node graphs (.lmat format)
// Uses the engine's existing JSON system

#pragma once

#include "engine/material/material_node.h"
#include "engine/material/material_node_library.h"
#include "engine/serialization/json.h"
#include <string>
#include <fstream>
#include <sstream>

namespace luma {

class MaterialSerializer {
public:
    
    // ===== Serialize MaterialGraph to JSON =====
    static JsonValue serialize(const MaterialGraph& graph) {
        JsonObject root;
        root["version"] = JsonValue(2.0);
        root["name"] = JsonValue(graph.name);
        root["description"] = JsonValue(graph.description);
        root["outputNodeId"] = JsonValue(static_cast<double>(graph.outputNodeId));
        root["nextNodeId"] = JsonValue(static_cast<double>(graph.nextNodeId));
        root["nextPinId"] = JsonValue(static_cast<double>(graph.nextPinId));
        root["nextLinkId"] = JsonValue(static_cast<double>(graph.nextLinkId));
        
        // Serialize nodes
        JsonArray nodesArr;
        for (const auto& nodePtr : graph.nodes) {
            const auto& node = *nodePtr;
            JsonObject nodeObj;
            nodeObj["id"] = JsonValue(static_cast<double>(node.id));
            nodeObj["name"] = JsonValue(node.name);
            nodeObj["displayName"] = JsonValue(node.displayName);
            nodeObj["category"] = JsonValue(static_cast<double>(static_cast<int>(node.category)));
            nodeObj["headerColor"] = JsonValue(static_cast<double>(node.headerColor));
            nodeObj["posX"] = JsonValue(static_cast<double>(node.position.x));
            nodeObj["posY"] = JsonValue(static_cast<double>(node.position.y));
            nodeObj["sizeX"] = JsonValue(static_cast<double>(node.size.x));
            nodeObj["sizeY"] = JsonValue(static_cast<double>(node.size.y));
            nodeObj["comment"] = JsonValue(node.comment);
            
            // Serialize input pins
            JsonArray inputsArr;
            for (const auto& pin : node.inputs) {
                inputsArr.push_back(serializePin(pin));
            }
            nodeObj["inputs"] = JsonValue(inputsArr);
            
            // Serialize output pins
            JsonArray outputsArr;
            for (const auto& pin : node.outputs) {
                outputsArr.push_back(serializePin(pin));
            }
            nodeObj["outputs"] = JsonValue(outputsArr);
            
            // Serialize properties
            JsonObject propsObj;
            for (const auto& [key, value] : node.properties) {
                propsObj[key] = serializePinValue(value);
            }
            nodeObj["properties"] = JsonValue(propsObj);
            
            nodesArr.push_back(JsonValue(nodeObj));
        }
        root["nodes"] = JsonValue(nodesArr);
        
        // Serialize links
        JsonArray linksArr;
        for (const auto& link : graph.links) {
            JsonObject linkObj;
            linkObj["id"] = JsonValue(static_cast<double>(link.id));
            linkObj["fromNode"] = JsonValue(static_cast<double>(link.fromNode));
            linkObj["fromPin"] = JsonValue(static_cast<double>(link.fromPin));
            linkObj["toNode"] = JsonValue(static_cast<double>(link.toNode));
            linkObj["toPin"] = JsonValue(static_cast<double>(link.toPin));
            linksArr.push_back(JsonValue(linkObj));
        }
        root["links"] = JsonValue(linksArr);
        
        // Serialize color ramps
        JsonObject rampsObj;
        for (const auto& [nodeId, stops] : graph.colorRamps) {
            JsonArray stopsArr;
            for (const auto& stop : stops) {
                JsonObject stopObj;
                stopObj["position"] = JsonValue(static_cast<double>(stop.position));
                JsonArray colorArr;
                for (int i = 0; i < 4; i++) {
                    colorArr.push_back(JsonValue(static_cast<double>(stop.color[i])));
                }
                stopObj["color"] = JsonValue(colorArr);
                stopsArr.push_back(JsonValue(stopObj));
            }
            rampsObj[std::to_string(nodeId)] = JsonValue(stopsArr);
        }
        root["colorRamps"] = JsonValue(rampsObj);
        
        return JsonValue(root);
    }
    
    // ===== Deserialize MaterialGraph from JSON =====
    static bool deserialize(const JsonValue& json, MaterialGraph& graph) {
        if (!json.isObject()) return false;
        const auto& root = json.asObject();
        
        graph.nodes.clear();
        graph.links.clear();
        graph.colorRamps.clear();
        
        // Basic fields
        if (root.count("name")) graph.name = root.at("name").asString();
        if (root.count("description")) graph.description = root.at("description").asString();
        if (root.count("outputNodeId")) graph.outputNodeId = static_cast<uint32_t>(root.at("outputNodeId").asNumber());
        if (root.count("nextNodeId")) graph.nextNodeId = static_cast<uint32_t>(root.at("nextNodeId").asNumber());
        if (root.count("nextPinId")) graph.nextPinId = static_cast<uint32_t>(root.at("nextPinId").asNumber());
        if (root.count("nextLinkId")) graph.nextLinkId = static_cast<uint32_t>(root.at("nextLinkId").asNumber());
        
        // Deserialize nodes
        if (root.count("nodes") && root.at("nodes").isArray()) {
            for (const auto& nodeJson : root.at("nodes").asArray()) {
                if (!nodeJson.isObject()) continue;
                const auto& nodeObj = nodeJson.asObject();
                
                auto node = std::make_unique<VisualScriptNode>();
                if (nodeObj.count("id")) node->id = static_cast<uint32_t>(nodeObj.at("id").asNumber());
                if (nodeObj.count("name")) node->name = nodeObj.at("name").asString();
                if (nodeObj.count("displayName")) node->displayName = nodeObj.at("displayName").asString();
                if (nodeObj.count("category")) node->category = static_cast<NodeCategory>(static_cast<int>(nodeObj.at("category").asNumber()));
                if (nodeObj.count("headerColor")) node->headerColor = static_cast<uint32_t>(nodeObj.at("headerColor").asNumber());
                if (nodeObj.count("posX")) node->position.x = static_cast<float>(nodeObj.at("posX").asNumber());
                if (nodeObj.count("posY")) node->position.y = static_cast<float>(nodeObj.at("posY").asNumber());
                if (nodeObj.count("sizeX")) node->size.x = static_cast<float>(nodeObj.at("sizeX").asNumber());
                if (nodeObj.count("sizeY")) node->size.y = static_cast<float>(nodeObj.at("sizeY").asNumber());
                if (nodeObj.count("comment")) node->comment = nodeObj.at("comment").asString();
                
                // Deserialize pins
                if (nodeObj.count("inputs") && nodeObj.at("inputs").isArray()) {
                    for (const auto& pinJson : nodeObj.at("inputs").asArray()) {
                        Pin pin = deserializePin(pinJson);
                        pin.direction = PinDirection::Input;
                        node->inputs.push_back(pin);
                    }
                }
                if (nodeObj.count("outputs") && nodeObj.at("outputs").isArray()) {
                    for (const auto& pinJson : nodeObj.at("outputs").asArray()) {
                        Pin pin = deserializePin(pinJson);
                        pin.direction = PinDirection::Output;
                        node->outputs.push_back(pin);
                    }
                }
                
                // Deserialize properties
                if (nodeObj.count("properties") && nodeObj.at("properties").isObject()) {
                    for (const auto& [key, val] : nodeObj.at("properties").asObject()) {
                        node->properties[key] = deserializePinValue(val);
                    }
                }
                
                graph.nodes.push_back(std::move(node));
            }
        }
        
        // Deserialize links
        if (root.count("links") && root.at("links").isArray()) {
            for (const auto& linkJson : root.at("links").asArray()) {
                if (!linkJson.isObject()) continue;
                const auto& linkObj = linkJson.asObject();
                
                Link link;
                if (linkObj.count("id")) link.id = static_cast<uint32_t>(linkObj.at("id").asNumber());
                if (linkObj.count("fromNode")) link.fromNode = static_cast<uint32_t>(linkObj.at("fromNode").asNumber());
                if (linkObj.count("fromPin")) link.fromPin = static_cast<uint32_t>(linkObj.at("fromPin").asNumber());
                if (linkObj.count("toNode")) link.toNode = static_cast<uint32_t>(linkObj.at("toNode").asNumber());
                if (linkObj.count("toPin")) link.toPin = static_cast<uint32_t>(linkObj.at("toPin").asNumber());
                graph.links.push_back(link);
            }
        }
        
        // Deserialize color ramps
        if (root.count("colorRamps") && root.at("colorRamps").isObject()) {
            for (const auto& [nodeIdStr, stopsJson] : root.at("colorRamps").asObject()) {
                uint32_t nodeId = static_cast<uint32_t>(std::stoul(nodeIdStr));
                std::vector<ColorRampStop> stops;
                
                if (stopsJson.isArray()) {
                    for (const auto& stopJson : stopsJson.asArray()) {
                        if (!stopJson.isObject()) continue;
                        const auto& stopObj = stopJson.asObject();
                        
                        ColorRampStop stop;
                        if (stopObj.count("position")) stop.position = static_cast<float>(stopObj.at("position").asNumber());
                        if (stopObj.count("color") && stopObj.at("color").isArray()) {
                            const auto& colorArr = stopObj.at("color").asArray();
                            for (int i = 0; i < 4 && i < (int)colorArr.size(); i++) {
                                stop.color[i] = static_cast<float>(colorArr[i].asNumber());
                            }
                        }
                        stops.push_back(stop);
                    }
                }
                
                graph.colorRamps[nodeId] = stops;
            }
        }
        
        graph.needsRecompile = true;
        return true;
    }
    
    // ===== Save to .lmat file =====
    static bool saveToFile(const MaterialGraph& graph, const std::string& path) {
        JsonValue json = serialize(graph);
        std::string jsonStr = toJson(json);
        
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << jsonStr;
        file.close();
        return true;
    }
    
    // ===== Load from .lmat file =====
    static bool loadFromFile(const std::string& path, MaterialGraph& graph) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::stringstream ss;
        ss << file.rdbuf();
        std::string jsonStr = ss.str();
        file.close();
        
        JsonValue json = parseJson(jsonStr);
        if (json.isNull()) return false;
        
        return deserialize(json, graph);
    }
    
private:
    static JsonValue serializePin(const Pin& pin) {
        JsonObject obj;
        obj["id"] = JsonValue(static_cast<double>(pin.id));
        obj["name"] = JsonValue(pin.name);
        obj["type"] = JsonValue(static_cast<double>(static_cast<int>(pin.type)));
        obj["direction"] = JsonValue(static_cast<double>(static_cast<int>(pin.direction)));
        obj["connected"] = JsonValue(pin.connected);
        obj["defaultValue"] = serializePinValue(pin.defaultValue);
        return JsonValue(obj);
    }
    
    static Pin deserializePin(const JsonValue& json) {
        Pin pin;
        if (!json.isObject()) return pin;
        const auto& obj = json.asObject();
        
        if (obj.count("id")) pin.id = static_cast<uint32_t>(obj.at("id").asNumber());
        if (obj.count("name")) pin.name = obj.at("name").asString();
        if (obj.count("type")) pin.type = static_cast<PinType>(static_cast<int>(obj.at("type").asNumber()));
        if (obj.count("direction")) pin.direction = static_cast<PinDirection>(static_cast<int>(obj.at("direction").asNumber()));
        if (obj.count("connected")) pin.connected = obj.at("connected").asBool();
        if (obj.count("defaultValue")) pin.defaultValue = deserializePinValue(obj.at("defaultValue"));
        
        return pin;
    }
    
    static JsonValue serializePinValue(const PinValue& value) {
        if (std::holds_alternative<std::monostate>(value)) {
            return JsonValue(); // null
        }
        if (std::holds_alternative<bool>(value)) {
            return JsonValue(std::get<bool>(value));
        }
        if (std::holds_alternative<int>(value)) {
            return JsonValue(static_cast<double>(std::get<int>(value)));
        }
        if (std::holds_alternative<float>(value)) {
            return JsonValue(static_cast<double>(std::get<float>(value)));
        }
        if (std::holds_alternative<std::string>(value)) {
            return JsonValue(std::get<std::string>(value));
        }
        if (std::holds_alternative<Vec2>(value)) {
            Vec2 v = std::get<Vec2>(value);
            JsonObject obj;
            obj["_type"] = JsonValue(std::string("vec2"));
            obj["x"] = JsonValue(static_cast<double>(v.x));
            obj["y"] = JsonValue(static_cast<double>(v.y));
            return JsonValue(obj);
        }
        if (std::holds_alternative<Vec3>(value)) {
            Vec3 v = std::get<Vec3>(value);
            JsonObject obj;
            obj["_type"] = JsonValue(std::string("vec3"));
            obj["x"] = JsonValue(static_cast<double>(v.x));
            obj["y"] = JsonValue(static_cast<double>(v.y));
            obj["z"] = JsonValue(static_cast<double>(v.z));
            return JsonValue(obj);
        }
        if (std::holds_alternative<Vec4>(value)) {
            Vec4 v = std::get<Vec4>(value);
            JsonObject obj;
            obj["_type"] = JsonValue(std::string("vec4"));
            obj["x"] = JsonValue(static_cast<double>(v.x));
            obj["y"] = JsonValue(static_cast<double>(v.y));
            obj["z"] = JsonValue(static_cast<double>(v.z));
            obj["w"] = JsonValue(static_cast<double>(v.w));
            return JsonValue(obj);
        }
        if (std::holds_alternative<uint64_t>(value)) {
            return JsonValue(static_cast<double>(std::get<uint64_t>(value)));
        }
        return JsonValue(); // null
    }
    
    static PinValue deserializePinValue(const JsonValue& json) {
        if (json.isNull()) return PinValue{std::monostate{}};
        if (json.isBool()) return PinValue(json.asBool());
        if (json.isNumber()) return PinValue(static_cast<float>(json.asNumber()));
        if (json.isString()) {
            std::string s = json.asString();
            return PinValue(std::move(s));
        }
        if (json.isObject()) {
            const auto& obj = json.asObject();
            if (obj.count("_type")) {
                std::string type = obj.at("_type").asString();
                if (type == "vec2") {
                    return PinValue(Vec2{
                        static_cast<float>(obj.at("x").asNumber()),
                        static_cast<float>(obj.at("y").asNumber())
                    });
                }
                if (type == "vec3") {
                    return PinValue(Vec3{
                        static_cast<float>(obj.at("x").asNumber()),
                        static_cast<float>(obj.at("y").asNumber()),
                        static_cast<float>(obj.at("z").asNumber())
                    });
                }
                if (type == "vec4") {
                    return PinValue(Vec4(
                        static_cast<float>(obj.at("x").asNumber()),
                        static_cast<float>(obj.at("y").asNumber()),
                        static_cast<float>(obj.at("z").asNumber()),
                        static_cast<float>(obj.at("w").asNumber())
                    ));
                }
            }
        }
        return PinValue{std::monostate{}};
    }
};

} // namespace luma
