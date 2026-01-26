# LUMA Studio API Reference

## Math Types (`engine/foundation/math_types.h`)

### Vec3

3D vector class.

```cpp
struct Vec3 {
    float x, y, z;
    
    Vec3();
    Vec3(float x, float y, float z);
    
    Vec3 operator+(const Vec3& other) const;
    Vec3 operator-(const Vec3& other) const;
    Vec3 operator*(float scalar) const;
    
    float length() const;
    float dot(const Vec3& other) const;
    Vec3 cross(const Vec3& other) const;
    Vec3 normalized() const;
};
```

### Quat

Quaternion for rotations.

```cpp
struct Quat {
    float x, y, z, w;
    
    Quat();  // Identity
    Quat(float x, float y, float z, float w);
    
    static Quat fromEuler(float pitch, float yaw, float roll);
    static Quat fromAxisAngle(const Vec3& axis, float angle);
    
    Vec3 toEuler() const;
    Vec3 rotate(const Vec3& v) const;
    Quat operator*(const Quat& other) const;
};

// Interpolation
Quat slerp(const Quat& a, const Quat& b, float t);
```

### Mat4

4x4 transformation matrix.

```cpp
struct Mat4 {
    float m[16];  // Column-major
    
    static Mat4 identity();
    static Mat4 translation(const Vec3& t);
    static Mat4 scale(const Vec3& s);
    static Mat4 fromQuat(const Quat& q);
    
    Mat4 operator*(const Mat4& other) const;
};
```

---

## Scene Graph (`engine/scene/`)

### SceneGraph

```cpp
class SceneGraph {
    Entity* createEntity(const std::string& name);
    void destroyEntity(EntityID id);
    Entity* findEntityByName(const std::string& name);
    Entity* getEntityById(EntityID id);
    
    void setParent(Entity* child, Entity* parent);
    std::vector<Entity*> getRootEntities();
    size_t getEntityCount() const;
    
    // Selection (multi-select support)
    void selectEntity(Entity* entity);
    void addToSelection(Entity* entity);
    void clearSelection();
    std::vector<Entity*> getSelectedEntities();
    
    // Clipboard
    void copySelection();
    void pasteClipboard();
    Entity* duplicateEntity(Entity* entity);
};
```

### Entity

```cpp
struct Entity {
    EntityID id;
    std::string name;
    Transform localTransform;
    
    Entity* parent;
    std::vector<Entity*> children;
    
    // Components
    std::shared_ptr<Material> material;
    bool hasLight;
    Light light;
    
    Mat4 getWorldMatrix() const;
};
```

### Transform

```cpp
struct Transform {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    
    Mat4 toMatrix() const;
    void setEulerDegrees(const Vec3& euler);
    Vec3 getEulerDegrees() const;
};
```

---

## Animation (`engine/animation/`)

### Skeleton

```cpp
class Skeleton {
    int addBone(const std::string& name, int parentIndex);
    int findBoneByName(const std::string& name) const;
    const Bone* getBone(int index) const;
    uint32_t getBoneCount() const;
    
    void setBoneLocalTransform(int index, Vec3 pos, Quat rot, Vec3 scale);
    void computeBoneMatrices(Mat4* outMatrices) const;
    void computeSkinningMatrices(Mat4* outMatrices) const;
};
```

### AnimationClip

```cpp
class AnimationClip {
    std::string name;
    float duration;
    bool looping;
    std::vector<AnimationChannel> channels;
    
    AnimationChannel& addChannel(const std::string& boneName);
    void resolveBoneIndices(const Skeleton& skeleton);
    void sample(float time, Vec3* pos, Quat* rot, Vec3* scale, int boneCount);
};
```

### Animator

```cpp
class Animator {
    void setSkeleton(Skeleton* skeleton);
    void addClip(const std::string& name, std::unique_ptr<AnimationClip> clip);
    AnimationClip* getClip(const std::string& name);
    
    void play(const std::string& clipName, float crossfadeDuration = 0.2f);
    void stop();
    void update(float deltaTime);
    
    void setPaused(bool paused);
    void setSpeed(float speed);
    void setTime(float time);
    
    float getCurrentTime() const;
    std::string getCurrentClipName() const;
    bool isPlaying() const;
    
    void getSkinningMatrices(Mat4* outMatrices) const;
};
```

### AnimationStateMachine

```cpp
class AnimationStateMachine {
    void addParameter(const std::string& name, ParameterType type);
    void setFloat(const std::string& name, float value);
    void setBool(const std::string& name, bool value);
    void setTrigger(const std::string& name);
    
    StateMachineState* createState(const std::string& name);
    void setDefaultState(const std::string& name);
    
    void start();
    void update(float deltaTime);
    void sample(Vec3* pos, Quat* rot, Vec3* scale, int boneCount) const;
    
    std::string getCurrentStateName() const;
    bool isTransitioning() const;
};
```

### IKManager

```cpp
class IKManager {
    int setupArmIK(int shoulder, int elbow, int hand);
    int setupLegIK(int hip, int knee, int foot, int pelvis = -1);
    int setupHeadLookAt(int head);
    int setupSpineChain(const std::vector<int>& spineIndices);
    
    void setHandTarget(int index, const Vec3& target, float weight = 1.0f);
    void setFootTarget(int index, const Vec3& ground, const Vec3& normal, float weight = 1.0f);
    void setLookAtTarget(int index, const Vec3& target, float weight = 1.0f);
    
    void solve(Skeleton& skeleton);
};
```

---

## Material (`engine/material/material.h`)

```cpp
struct Material {
    std::string name;
    uint32_t id;
    
    Vec3 baseColor;
    float alpha;
    float metallic;
    float roughness;
    float ao;
    
    Vec3 emissiveColor;
    float emissiveIntensity;
    float normalStrength;
    
    // Textures
    uint32_t textures[TEXTURE_SLOT_COUNT];
    
    bool twoSided;
    bool alphaBlend;
    bool alphaCutoff;
    float alphaCutoffValue;
    
    // Presets
    static Material createDefault();
    static Material createGold();
    static Material createSilver();
    static Material createPlastic(const Vec3& color);
    static Material createRubber(const Vec3& color);
    static Material createGlass();
};

class MaterialLibrary {
    static MaterialLibrary& get();
    void addPreset(const std::string& name, const Material& mat);
    Material* getPreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
};
```

---

## Lighting (`engine/lighting/light.h`)

```cpp
enum class LightType { Directional, Point, Spot };

struct Light {
    uint32_t id;
    std::string name;
    LightType type;
    bool enabled;
    
    Vec3 color;
    float intensity;
    Vec3 position;
    Vec3 direction;
    
    // Attenuation (point/spot)
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    
    // Spot light
    float innerConeAngle;
    float outerConeAngle;
    
    static Light createDirectional();
    static Light createPoint();
    static Light createSpot();
};

class LightManager {
    static LightManager& get();
    
    int addLight(const Light& light);
    void removeLight(uint32_t id);
    Light* getLight(uint32_t id);
    
    const std::vector<Light>& getAllLights() const;
    int getActiveLightCount() const;
};
```

---

## Rendering (`engine/rendering/`)

### CullingSystem

```cpp
class CullingSystem {
    static CullingSystem& get();
    
    void beginFrame(const Mat4& viewProjection);
    bool isVisible(const BoundingSphere& bounds) const;
    
    CullStats getStats() const;
};
```

### LODManager

```cpp
class LODManager {
    void registerLODGroup(const LODGroup& group);
    int selectLOD(const LODGroup& group, float screenSize);
    float calculateScreenSize(const Vec3& center, float radius, const Mat4& viewProj);
};
```

### SSAOEffect

```cpp
class SSAOEffect {
    SSAOSettings settings;
    SSAOKernel kernel;
    SSAONoise noise;
    
    void updateSettings(const SSAOSettings& settings);
    SSAOUniforms getUniforms(const Mat4& proj, int width, int height) const;
};

// Quality presets
SSAOSettings SSAOPresets::low();
SSAOSettings SSAOPresets::medium();
SSAOSettings SSAOPresets::high();
SSAOSettings SSAOPresets::ultra();
```

### CascadedShadowMap

```cpp
class CascadedShadowMap {
    CSMSettings settings;
    std::vector<ShadowCascade> cascades;
    
    void update(const Mat4& cameraView, const Mat4& cameraProj,
                const Vec3& lightDir, float near, float far);
    
    int getCascadeIndex(float viewSpaceDepth) const;
    float getCascadeBlendFactor(float depth, int cascade) const;
};

// Quality presets
CSMSettings ShadowPresets::low();
CSMSettings ShadowPresets::medium();
CSMSettings ShadowPresets::high();
CSMSettings ShadowPresets::ultra();
```

---

## Editor (`engine/editor/`)

### CommandHistory

```cpp
class CommandHistory {
    void execute(std::unique_ptr<Command> cmd);
    void undo();
    void redo();
    
    bool canUndo() const;
    bool canRedo() const;
    
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;
    
    bool isDirty() const;
    void markClean();
    
    void setOnHistoryChanged(std::function<void()> callback);
};
```

### Command Base

```cpp
class Command {
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() { execute(); }
    virtual std::string getDescription() const = 0;
    virtual const char* getTypeId() const = 0;
};
```

---

## Serialization (`engine/serialization/`)

### SceneSerializer

```cpp
class SceneSerializer {
    static JsonValue serializeScene(const SceneGraph& scene,
                                    const std::string& name,
                                    const RHICameraParams* camera = nullptr,
                                    const PostProcessSettings* pp = nullptr);
    
    static void deserializeScene(const JsonValue& json,
                                 SceneGraph& scene,
                                 RHICameraParams* camera = nullptr,
                                 PostProcessSettings* pp = nullptr);
    
    static bool saveToFile(const std::string& path, const JsonValue& json);
    static JsonValue loadFromFile(const std::string& path);
};
```

---

## Timeline (`engine/animation/timeline.h`)

### AnimationCurve<T>

```cpp
template<typename T>
class AnimationCurve {
    T defaultValue;
    
    int addKeyframe(float time, const T& value);
    void removeKeyframe(int index);
    int getKeyframeCount() const;
    
    T evaluate(float time) const;
    void autoComputeAllTangents();
};
```

### Timeline

```cpp
class Timeline {
    std::string name;
    float duration;
    float frameRate;
    float currentTime;
    bool playing;
    
    TimelineTrack* createTrack(const std::string& name, TrackType type);
    
    void play();
    void pause();
    void stop();
    void setTime(float time);
    void update(float deltaTime);
    
    void addMarker(float time, const std::string& name);
    void gotoMarker(const std::string& name);
    
    float snapToFrame(float time) const;
};
```
