// Scene Serializer - Save and load scenes to/from JSON
#pragma once

#include "json.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene_graph.h"
#include "engine/renderer/post_process.h"
#include "engine/renderer/unified_renderer.h"
#include <string>
#include <functional>

namespace luma {

// Callback for loading model assets (user provides implementation)
using ModelLoadCallback = std::function<bool(const std::string& path, RHILoadedModel& outModel)>;

// ===== Scene Serializer =====
class SceneSerializer {
public:
    // Serialize Vec3 to JSON
    static JsonValue serializeVec3(const Vec3& v) {
        JsonValue arr = JsonValue::array();
        arr.push(v.x);
        arr.push(v.y);
        arr.push(v.z);
        return arr;
    }
    
    // Deserialize Vec3 from JSON
    static Vec3 deserializeVec3(const JsonValue& json, Vec3 defaultVal = {0, 0, 0}) {
        if (!json.isArray() || json.size() < 3) return defaultVal;
        return Vec3(
            json[0].asFloat(),
            json[1].asFloat(),
            json[2].asFloat()
        );
    }
    
    // Serialize Quat to JSON
    static JsonValue serializeQuat(const Quat& q) {
        JsonValue arr = JsonValue::array();
        arr.push(q.x);
        arr.push(q.y);
        arr.push(q.z);
        arr.push(q.w);
        return arr;
    }
    
    // Deserialize Quat from JSON
    static Quat deserializeQuat(const JsonValue& json) {
        if (!json.isArray() || json.size() < 4) return Quat();
        return Quat(
            json[0].asFloat(),
            json[1].asFloat(),
            json[2].asFloat(),
            json[3].asFloat()
        );
    }
    
    // Serialize Transform to JSON
    static JsonValue serializeTransform(const Transform& t) {
        JsonValue obj = JsonValue::object();
        obj["position"] = serializeVec3(t.position);
        obj["rotation"] = serializeQuat(t.rotation);
        obj["scale"] = serializeVec3(t.scale);
        return obj;
    }
    
    // Deserialize Transform from JSON
    static Transform deserializeTransform(const JsonValue& json) {
        Transform t;
        if (json.has("position")) {
            t.position = deserializeVec3(json["position"]);
        }
        if (json.has("rotation")) {
            t.rotation = deserializeQuat(json["rotation"]);
        }
        if (json.has("scale")) {
            t.scale = deserializeVec3(json["scale"], {1, 1, 1});
        }
        return t;
    }
    
    // Serialize Entity to JSON
    static JsonValue serializeEntity(const Entity* entity) {
        JsonValue obj = JsonValue::object();
        
        obj["id"] = static_cast<int>(entity->id);
        obj["name"] = entity->name;
        obj["enabled"] = entity->enabled;
        obj["transform"] = serializeTransform(entity->localTransform);
        
        // Model reference (if any)
        if (entity->hasModel) {
            obj["hasModel"] = true;
            // Store model path for reloading
            obj["modelPath"] = entity->model.debugName;
        }
        
        // Animation info (if entity has skeleton/animations)
        if (entity->hasSkeleton()) {
            obj["hasSkeleton"] = true;
            // Store animation clip names
            JsonValue clipsArr = JsonValue::array();
            for (const auto& [name, clip] : entity->animationClips) {
                clipsArr.push(name);
            }
            obj["animationClips"] = clipsArr;
        }
        
        // Serialize children
        if (!entity->children.empty()) {
            JsonValue childrenArr = JsonValue::array();
            for (const Entity* child : entity->children) {
                childrenArr.push(serializeEntity(child));
            }
            obj["children"] = childrenArr;
        }
        
        return obj;
    }
    
    // Deserialize Entity from JSON (returns entity ID for hierarchy setup)
    static Entity* deserializeEntity(SceneGraph& scene, const JsonValue& json, 
                                     ModelLoadCallback loadModel = nullptr) {
        std::string name = json.get<std::string>("name", "Entity");
        Entity* entity = scene.createEntity(name);
        
        entity->enabled = json.get<bool>("enabled", true);
        entity->localTransform = deserializeTransform(json["transform"]);
        
        // Load model if specified
        if (json.get<bool>("hasModel", false) && loadModel) {
            std::string modelPath = json.get<std::string>("modelPath", "");
            if (!modelPath.empty()) {
                if (loadModel(modelPath, entity->model)) {
                    entity->hasModel = true;
                }
            }
        }
        
        // Deserialize children recursively
        if (json.has("children")) {
            const JsonArray& childrenArr = json["children"].asArray();
            for (const JsonValue& childJson : childrenArr) {
                Entity* child = deserializeEntity(scene, childJson, loadModel);
                if (child) {
                    entity->addChild(child);
                }
            }
        }
        
        return entity;
    }
    
    // ===== Camera Serialization =====
    
    static JsonValue serializeCameraParams(const RHICameraParams& camera) {
        JsonValue obj = JsonValue::object();
        obj["yaw"] = camera.yaw;
        obj["pitch"] = camera.pitch;
        obj["distance"] = camera.distance;
        obj["targetOffsetX"] = camera.targetOffsetX;
        obj["targetOffsetY"] = camera.targetOffsetY;
        obj["targetOffsetZ"] = camera.targetOffsetZ;
        return obj;
    }
    
    static RHICameraParams deserializeCameraParams(const JsonValue& json) {
        RHICameraParams camera;
        camera.yaw = json.get<float>("yaw", 0.78f);
        camera.pitch = json.get<float>("pitch", 0.5f);
        camera.distance = json.get<float>("distance", 1.0f);
        camera.targetOffsetX = json.get<float>("targetOffsetX", 0.0f);
        camera.targetOffsetY = json.get<float>("targetOffsetY", 0.0f);
        camera.targetOffsetZ = json.get<float>("targetOffsetZ", 0.0f);
        return camera;
    }
    
    // ===== Post-Process Serialization =====
    
    static JsonValue serializePostProcess(const PostProcessSettings& pp) {
        JsonValue obj = JsonValue::object();
        
        // Bloom
        JsonValue bloom = JsonValue::object();
        bloom["enabled"] = pp.bloom.enabled;
        bloom["threshold"] = pp.bloom.threshold;
        bloom["intensity"] = pp.bloom.intensity;
        bloom["radius"] = pp.bloom.radius;
        bloom["iterations"] = pp.bloom.iterations;
        bloom["softThreshold"] = pp.bloom.softThreshold;
        obj["bloom"] = bloom;
        
        // Tone mapping
        JsonValue tone = JsonValue::object();
        tone["enabled"] = pp.toneMapping.enabled;
        tone["mode"] = static_cast<int>(pp.toneMapping.mode);
        tone["exposure"] = pp.toneMapping.exposure;
        tone["gamma"] = pp.toneMapping.gamma;
        tone["contrast"] = pp.toneMapping.contrast;
        tone["saturation"] = pp.toneMapping.saturation;
        obj["toneMapping"] = tone;
        
        // Vignette
        JsonValue vignette = JsonValue::object();
        vignette["enabled"] = pp.vignette.enabled;
        vignette["intensity"] = pp.vignette.intensity;
        vignette["smoothness"] = pp.vignette.smoothness;
        vignette["roundness"] = pp.vignette.roundness;
        obj["vignette"] = vignette;
        
        // Chromatic aberration
        JsonValue chroma = JsonValue::object();
        chroma["enabled"] = pp.chromaticAberration.enabled;
        chroma["intensity"] = pp.chromaticAberration.intensity;
        obj["chromaticAberration"] = chroma;
        
        // Film grain
        JsonValue grain = JsonValue::object();
        grain["enabled"] = pp.filmGrain.enabled;
        grain["intensity"] = pp.filmGrain.intensity;
        grain["response"] = pp.filmGrain.response;
        obj["filmGrain"] = grain;
        
        // FXAA
        JsonValue fxaa = JsonValue::object();
        fxaa["enabled"] = pp.fxaa.enabled;
        obj["fxaa"] = fxaa;
        
        return obj;
    }
    
    static PostProcessSettings deserializePostProcess(const JsonValue& json) {
        PostProcessSettings pp;
        
        if (json.has("bloom")) {
            const auto& bloom = json["bloom"];
            pp.bloom.enabled = bloom.get<bool>("enabled", true);
            pp.bloom.threshold = bloom.get<float>("threshold", 1.0f);
            pp.bloom.intensity = bloom.get<float>("intensity", 1.0f);
            pp.bloom.radius = bloom.get<float>("radius", 4.0f);
            pp.bloom.iterations = bloom.get<int>("iterations", 5);
            pp.bloom.softThreshold = bloom.get<float>("softThreshold", 0.5f);
        }
        
        if (json.has("toneMapping")) {
            const auto& tone = json["toneMapping"];
            pp.toneMapping.enabled = tone.get<bool>("enabled", true);
            pp.toneMapping.mode = static_cast<ToneMappingSettings::Mode>(tone.get<int>("mode", 2));
            pp.toneMapping.exposure = tone.get<float>("exposure", 1.0f);
            pp.toneMapping.gamma = tone.get<float>("gamma", 2.2f);
            pp.toneMapping.contrast = tone.get<float>("contrast", 1.0f);
            pp.toneMapping.saturation = tone.get<float>("saturation", 1.0f);
        }
        
        if (json.has("vignette")) {
            const auto& vignette = json["vignette"];
            pp.vignette.enabled = vignette.get<bool>("enabled", false);
            pp.vignette.intensity = vignette.get<float>("intensity", 0.3f);
            pp.vignette.smoothness = vignette.get<float>("smoothness", 0.5f);
            pp.vignette.roundness = vignette.get<float>("roundness", 1.0f);
        }
        
        if (json.has("chromaticAberration")) {
            const auto& chroma = json["chromaticAberration"];
            pp.chromaticAberration.enabled = chroma.get<bool>("enabled", false);
            pp.chromaticAberration.intensity = chroma.get<float>("intensity", 0.01f);
        }
        
        if (json.has("filmGrain")) {
            const auto& grain = json["filmGrain"];
            pp.filmGrain.enabled = grain.get<bool>("enabled", false);
            pp.filmGrain.intensity = grain.get<float>("intensity", 0.1f);
            pp.filmGrain.response = grain.get<float>("response", 0.8f);
        }
        
        if (json.has("fxaa")) {
            const auto& fxaa = json["fxaa"];
            pp.fxaa.enabled = fxaa.get<bool>("enabled", true);
        }
        
        return pp;
    }
    
    // ===== Scene-level Serialization =====
    
    // Serialize entire scene to JSON (with optional camera and post-process)
    static JsonValue serializeScene(const SceneGraph& scene, const std::string& sceneName = "",
                                    const RHICameraParams* camera = nullptr,
                                    const PostProcessSettings* postProcess = nullptr) {
        JsonValue root = JsonValue::object();
        
        // Scene metadata
        root["version"] = 2;  // Updated version for new features
        root["name"] = sceneName.empty() ? "Untitled Scene" : sceneName;
        
        // Serialize camera settings
        if (camera) {
            root["camera"] = serializeCameraParams(*camera);
        }
        
        // Serialize post-process settings
        if (postProcess) {
            root["postProcess"] = serializePostProcess(*postProcess);
        }
        
        // Serialize root entities
        JsonValue entitiesArr = JsonValue::array();
        for (Entity* entity : scene.getRootEntities()) {
            entitiesArr.push(serializeEntity(entity));
        }
        root["entities"] = entitiesArr;
        
        return root;
    }
    
    // Deserialize scene from JSON (with optional camera and post-process output)
    static bool deserializeScene(SceneGraph& scene, const JsonValue& json,
                                 ModelLoadCallback loadModel = nullptr,
                                 RHICameraParams* outCamera = nullptr,
                                 PostProcessSettings* outPostProcess = nullptr) {
        if (!json.isObject()) return false;
        
        // Check version
        int version = json.get<int>("version", 1);
        if (version > 2) {
            // Future version - might not be compatible
            return false;
        }
        
        // Clear existing scene
        scene.clear();
        
        // Deserialize camera settings
        if (outCamera && json.has("camera")) {
            *outCamera = deserializeCameraParams(json["camera"]);
        }
        
        // Deserialize post-process settings
        if (outPostProcess && json.has("postProcess")) {
            *outPostProcess = deserializePostProcess(json["postProcess"]);
        }
        
        // Deserialize entities
        if (json.has("entities")) {
            const JsonArray& entitiesArr = json["entities"].asArray();
            for (const JsonValue& entityJson : entitiesArr) {
                deserializeEntity(scene, entityJson, loadModel);
            }
        }
        
        // Update all world matrices
        scene.updateAllWorldMatrices();
        
        return true;
    }
    
    // ===== File Operations =====
    
    // Save scene to file (basic version for compatibility)
    static bool saveScene(const SceneGraph& scene, const std::string& path,
                         const std::string& sceneName = "") {
        JsonValue json = serializeScene(scene, sceneName);
        return saveJsonFile(path, json, true);
    }
    
    // Save scene to file (with camera and post-process)
    static bool saveSceneFull(const SceneGraph& scene, const std::string& path,
                              const RHICameraParams& camera,
                              const PostProcessSettings& postProcess,
                              const std::string& sceneName = "") {
        JsonValue json = serializeScene(scene, sceneName, &camera, &postProcess);
        return saveJsonFile(path, json, true);
    }
    
    // Load scene from file (basic version for compatibility)
    static bool loadScene(SceneGraph& scene, const std::string& path,
                         ModelLoadCallback loadModel = nullptr) {
        try {
            JsonValue json = loadJsonFile(path);
            return deserializeScene(scene, json, loadModel);
        } catch (const std::exception&) {
            return false;
        }
    }
    
    // Load scene from file (with camera and post-process)
    static bool loadSceneFull(SceneGraph& scene, const std::string& path,
                              RHICameraParams& outCamera,
                              PostProcessSettings& outPostProcess,
                              ModelLoadCallback loadModel = nullptr) {
        try {
            JsonValue json = loadJsonFile(path);
            return deserializeScene(scene, json, loadModel, &outCamera, &outPostProcess);
        } catch (const std::exception&) {
            return false;
        }
    }
};

// ===== Settings Serialization =====

// Generic settings serializer for renderer/editor settings
class SettingsSerializer {
public:
    // Save settings to JSON file
    template<typename T>
    static bool save(const std::string& path, const T& settings);
    
    // Load settings from JSON file
    template<typename T>
    static bool load(const std::string& path, T& settings);
};

// Example: Camera settings
struct CameraSettings {
    Vec3 position{0, 2, 5};
    Vec3 target{0, 0, 0};
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    JsonValue toJson() const {
        JsonValue obj = JsonValue::object();
        obj["position"] = SceneSerializer::serializeVec3(position);
        obj["target"] = SceneSerializer::serializeVec3(target);
        obj["fov"] = fov;
        obj["nearPlane"] = nearPlane;
        obj["farPlane"] = farPlane;
        return obj;
    }
    
    static CameraSettings fromJson(const JsonValue& json) {
        CameraSettings s;
        s.position = SceneSerializer::deserializeVec3(json["position"], s.position);
        s.target = SceneSerializer::deserializeVec3(json["target"], s.target);
        s.fov = json.get<float>("fov", 45.0f);
        s.nearPlane = json.get<float>("nearPlane", 0.1f);
        s.farPlane = json.get<float>("farPlane", 1000.0f);
        return s;
    }
};

// Editor layout settings
struct EditorLayout {
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAssetBrowser = true;
    bool showConsole = false;
    float hierarchyWidth = 250.0f;
    float inspectorWidth = 300.0f;
    
    JsonValue toJson() const {
        JsonValue obj = JsonValue::object();
        obj["showHierarchy"] = showHierarchy;
        obj["showInspector"] = showInspector;
        obj["showAssetBrowser"] = showAssetBrowser;
        obj["showConsole"] = showConsole;
        obj["hierarchyWidth"] = hierarchyWidth;
        obj["inspectorWidth"] = inspectorWidth;
        return obj;
    }
    
    static EditorLayout fromJson(const JsonValue& json) {
        EditorLayout l;
        l.showHierarchy = json.get<bool>("showHierarchy", true);
        l.showInspector = json.get<bool>("showInspector", true);
        l.showAssetBrowser = json.get<bool>("showAssetBrowser", true);
        l.showConsole = json.get<bool>("showConsole", false);
        l.hierarchyWidth = json.get<float>("hierarchyWidth", 250.0f);
        l.inspectorWidth = json.get<float>("inspectorWidth", 300.0f);
        return l;
    }
};

}  // namespace luma
