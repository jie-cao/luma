# LUMA 角色创建系统使用指南

本文档介绍如何使用 LUMA 引擎的角色创建系统。

---

## 目录

1. [快速开始](#快速开始)
2. [基础用法](#基础用法)
3. [身体定制](#身体定制)
4. [脸部定制](#脸部定制)
5. [照片生成脸部](#照片生成脸部)
6. [BlendShape 直接控制](#blendshape-直接控制)
7. [渲染集成](#渲染集成)
8. [导出角色](#导出角色)
9. [API 参考](#api-参考)

---

## 快速开始

### 最小示例

```cpp
#include "engine/character/character.h"
#include "engine/character/base_human_loader.h"

// 1. 创建角色
auto character = luma::CharacterFactory::createBlank("MyCharacter");

// 2. 加载基础模型
auto& library = luma::BaseHumanModelLibrary::getInstance();
library.initializeDefaults();

const luma::BaseHumanModel* model = library.getModel("procedural_human");
if (model) {
    character->setBaseMesh(model->vertices, model->indices);
    // 复制 BlendShape 数据...
}

// 3. 调整参数
character->getBody().setGender(luma::Gender::Female);
character->getBody().setPreset(luma::BodyPreset::FemaleAthletic);

// 4. 获取变形后的网格用于渲染
std::vector<luma::Vertex> deformedVerts;
character->getDeformedVertices(deformedVerts);
```

### 使用演示应用

```cpp
#define LUMA_CHARACTER_CREATOR_DEMO_IMPLEMENTATION
#include "examples/07_character_creator.h"

// 初始化
luma::examples::CharacterCreatorDemo demo;
demo.initialize(&renderer);

// 主循环
while (running) {
    demo.update(deltaTime);
    
    renderer.beginFrame();
    demo.render();        // 渲染角色
    demo.renderUI();      // 渲染 ImGui UI
    renderer.endFrame();
}
```

---

## 基础用法

### 创建角色

```cpp
// 方法 1: 空白角色
auto character = CharacterFactory::createBlank("Name");

// 方法 2: 从预设创建
auto character = CharacterFactory::createFromPreset("Average Male");

// 方法 3: 随机生成
auto character = CharacterFactory::createRandom();

// 方法 4: 从照片创建 (需要 AI 模型)
auto character = CharacterFactory::createFromPhoto("photo.jpg");
```

### 角色组件

```cpp
// 获取各个组件
CharacterFace& face = character->getFace();
CharacterBody& body = character->getBody();
CharacterClothing& clothing = character->getClothing();
BlendShapeMesh& blendShapes = character->getBlendShapeMesh();
Skeleton& skeleton = character->getSkeleton();
```

---

## 身体定制

### 性别和年龄

```cpp
// 设置性别
body.setGender(Gender::Male);    // Male, Female, Neutral
body.setGender(Gender::Female);

// 设置年龄组
body.setAgeGroup(AgeGroup::Adult);  // Child, Teen, YoungAdult, Adult, Senior
```

### 体型预设

```cpp
// 男性预设
body.setPreset(BodyPreset::MaleSlim);
body.setPreset(BodyPreset::MaleAverage);
body.setPreset(BodyPreset::MaleMuscular);
body.setPreset(BodyPreset::MaleHeavy);
body.setPreset(BodyPreset::MaleElderly);

// 女性预设
body.setPreset(BodyPreset::FemaleSlim);
body.setPreset(BodyPreset::FemaleAverage);
body.setPreset(BodyPreset::FemaleCurvy);
body.setPreset(BodyPreset::FemaleAthletic);
body.setPreset(BodyPreset::FemaleElderly);

// 儿童预设
body.setPreset(BodyPreset::ChildToddler);
body.setPreset(BodyPreset::ChildYoung);
body.setPreset(BodyPreset::ChildTeen);
```

### 细节参数

```cpp
auto& m = body.getParams().measurements;

// 整体
m.height = 0.6f;        // 0=矮, 1=高
m.weight = 0.5f;        // 0=瘦, 1=胖
m.muscularity = 0.7f;   // 0=无肌肉, 1=肌肉发达
m.bodyFat = 0.3f;       // 0=低体脂, 1=高体脂

// 上身
m.shoulderWidth = 0.6f;
m.chestSize = 0.5f;
m.waistSize = 0.4f;
m.hipWidth = 0.5f;

// 手臂
m.armLength = 0.5f;
m.armThickness = 0.5f;

// 腿部
m.legLength = 0.5f;
m.thighThickness = 0.5f;

// 更新 BlendShape 权重
body.updateBlendShapeWeights();
```

### 肤色

```cpp
body.getParams().skinColor = Vec3(0.8f, 0.6f, 0.5f);

// 同步脸部和身体肤色
character->matchSkinColors();
```

---

## 脸部定制

### 脸型参数

```cpp
auto& shape = face.getShapeParams();

// 整体脸型
shape.faceWidth = 0.5f;      // 窄→宽
shape.faceLength = 0.5f;     // 短→长
shape.faceRoundness = 0.5f;  // 方→圆

// 眼睛
shape.eyeSize = 0.5f;
shape.eyeSpacing = 0.5f;     // 间距
shape.eyeHeight = 0.5f;      // 位置高度
shape.eyeAngle = 0.5f;       // 眼角角度
shape.eyeDepth = 0.5f;       // 深陷程度

// 眉毛
shape.browHeight = 0.5f;
shape.browAngle = 0.5f;
shape.browThickness = 0.5f;

// 鼻子
shape.noseLength = 0.5f;
shape.noseWidth = 0.5f;
shape.noseHeight = 0.5f;
shape.noseBridge = 0.5f;
shape.noseTip = 0.5f;

// 嘴巴
shape.mouthWidth = 0.5f;
shape.upperLipThickness = 0.5f;
shape.lowerLipThickness = 0.5f;

// 下巴/下颌
shape.chinLength = 0.5f;
shape.chinWidth = 0.5f;
shape.jawWidth = 0.5f;
shape.jawLine = 0.5f;

// 脸颊
shape.cheekboneProminence = 0.5f;
shape.cheekFullness = 0.5f;
```

### 表情

```cpp
// 预设表情
face.setExpression("neutral");
face.setExpression("smile", 0.8f);   // 带强度
face.setExpression("frown");
face.setExpression("surprise");
face.setExpression("angry");

// 手动控制 (ARKit 52 兼容)
auto& exp = face.getExpressionParams();
exp.mouthSmileLeft = 0.5f;
exp.mouthSmileRight = 0.5f;
exp.eyeBlinkLeft = 0.3f;
exp.browInnerUp = 0.4f;
```

### 贴图参数

```cpp
auto& tex = face.getTextureParams();

// 肤色
tex.skinTone = Vec3(0.85f, 0.65f, 0.5f);
tex.wrinkles = 0.2f;
tex.freckles = 0.1f;

// 眼睛
tex.eyeColor = Vec3(0.3f, 0.4f, 0.2f);  // 绿色

// 嘴唇
tex.lipColor = Vec3(0.7f, 0.4f, 0.4f);
tex.lipMoisture = 0.6f;

// 眉毛
tex.eyebrowColor = Vec3(0.2f, 0.15f, 0.1f);
tex.eyebrowDensity = 0.8f;
```

---

## 照片生成脸部

### 使用 PhotoToFacePipeline

```cpp
#include "engine/character/ai/face_reconstruction.h"

// 1. 初始化 Pipeline
PhotoToFacePipeline::Config config;
config.faceDetectorModelPath = "models/face_detector.onnx";     // 可选
config.faceMeshModelPath = "models/face_mesh.onnx";             // 可选
config.face3DMMModelPath = "models/deca.onnx";                  // 可选
config.extractTexture = true;
config.use3DMM = true;

PhotoToFacePipeline pipeline;
pipeline.initialize(config);

// 2. 处理照片
// imageData: RGBA 或 RGB 图像数据
// width, height: 图像尺寸
// channels: 3 (RGB) 或 4 (RGBA)
PhotoFaceResult result;
if (pipeline.process(imageData, width, height, channels, result)) {
    // 成功
    printf("Confidence: %.2f\n", result.overallConfidence);
    
    // 3. 应用到角色
    pipeline.applyToCharacterFace(result, character->getFace());
}
```

### PhotoFaceResult 内容

```cpp
struct PhotoFaceResult {
    bool success;
    std::string errorMessage;
    
    // 3DMM 参数
    std::vector<float> shapeParams;      // 形状参数 (~100维)
    std::vector<float> expressionParams; // 表情参数 (~50维)
    
    // 姿态
    Vec3 headRotation;    // 头部旋转
    Vec3 headTranslation; // 头部位置
    
    // 提取的贴图
    std::vector<uint8_t> textureData;
    int textureWidth, textureHeight;
    
    // 特征点 (468个3D点)
    std::vector<Vec3> landmarks;
    
    // 置信度
    float overallConfidence;
    float poseConfidence;
    float expressionConfidence;
};
```

---

## BlendShape 直接控制

### 获取和设置权重

```cpp
BlendShapeMesh& bs = character->getBlendShapeMesh();

// 按名称设置
bs.setWeight("body_height", 0.7f);
bs.setWeight("face_width", -0.3f);

// 按索引设置
bs.setWeight(0, 0.5f);

// 获取权重
float weight = bs.getWeight("body_height");

// 重置所有权重
bs.resetAllWeights();
```

### 获取通道信息

```cpp
// 遍历所有通道
const auto& channels = bs.getChannels();
for (const auto& ch : channels) {
    printf("Channel: %s, Weight: %.2f [%.2f ~ %.2f]\n",
           ch.name.c_str(), ch.weight, ch.minWeight, ch.maxWeight);
}

// 查找通道
int idx = bs.findChannelIndex("body_weight");
if (idx >= 0) {
    BlendShapeChannel* ch = bs.getChannel(idx);
    ch->setWeight(0.6f);
}
```

### 应用预设

```cpp
// 添加预设
BlendShapePreset happyPreset("Happy");
happyPreset.setWeight("mouthSmileLeft", 0.8f);
happyPreset.setWeight("mouthSmileRight", 0.8f);
happyPreset.setWeight("cheekSquintLeft", 0.3f);
happyPreset.setWeight("cheekSquintRight", 0.3f);
bs.addPreset(happyPreset);

// 应用预设
bs.applyPreset("Happy", 1.0f);  // 完全应用
bs.applyPreset("Happy", 0.5f);  // 50% 混合
```

---

## 渲染集成

### 使用 CharacterRenderer

```cpp
#include "engine/character/character_renderer.h"

CharacterRenderer charRenderer;
charRenderer.initialize(&renderer);
charRenderer.setupCharacter(character.get());

// 在渲染循环中
void renderLoop() {
    // 更新 BlendShape
    charRenderer.updateBlendShapes();
    
    // 检查是否需要 GPU 更新
    if (charRenderer.needsGPUUpdate()) {
        Mesh mesh = charRenderer.getCurrentMesh();
        gpuMesh = renderer.uploadMesh(mesh);
        charRenderer.markGPUUpdated();
    }
    
    // 渲染
    RHILoadedModel model;
    model.meshes.push_back(gpuMesh);
    model.radius = 1.0f;
    
    float worldMatrix[16];
    // ... 设置变换矩阵
    
    renderer.renderModel(model, worldMatrix);
}
```

### 手动渲染

```cpp
// 获取变形后的顶点
std::vector<Vertex> deformedVerts;
character->getDeformedVertices(deformedVerts);

// 创建 Mesh
Mesh mesh;
mesh.vertices = deformedVerts;
mesh.indices = character->getIndices();
mesh.baseColor[0] = character->getBody().getParams().skinColor.x;
mesh.baseColor[1] = character->getBody().getParams().skinColor.y;
mesh.baseColor[2] = character->getBody().getParams().skinColor.z;

// 上传并渲染
RHIGPUMesh gpuMesh = renderer.uploadMesh(mesh);
// ...
```

---

## 导出角色

### 序列化参数

```cpp
// 保存参数
std::unordered_map<std::string, std::string> data;
character->serialize(data);

// 保存到文件
std::ofstream file("character.json");
file << "{\n";
for (const auto& [key, value] : data) {
    file << "  \"" << key << "\": \"" << value << "\",\n";
}
file << "}\n";

// 加载参数
character->deserialize(data);
```

### 导出为 glTF (预留)

```cpp
// 导出接口 (待实现)
character->exportTo("character.glb", CharacterExportFormat::GLTF);
```

---

## API 参考

### 核心类

| 类 | 说明 |
|----|------|
| `Character` | 统一角色管理 |
| `CharacterFace` | 脸部参数 |
| `CharacterBody` | 身体参数 |
| `BlendShapeMesh` | BlendShape 数据 |
| `CharacterRenderer` | 渲染集成 |
| `PhotoToFacePipeline` | 照片处理 |

### 文件位置

```
engine/character/
├── blend_shape.h           # BlendShape 系统
├── character.h             # 角色整合
├── character_body.h        # 身体参数
├── character_face.h        # 脸部参数
├── character_renderer.h    # 渲染集成
├── character_creator_ui.h  # UI 界面
├── base_human_loader.h     # 模型加载
└── ai/
    ├── ai_inference.h      # AI 推理引擎
    └── face_reconstruction.h # 照片→脸部

examples/
└── 07_character_creator.h  # 演示应用

engine/renderer/shaders/
└── blend_shape.metal       # GPU 计算着色器
```

---

## 常见问题

### Q: 如何获取高质量的基础人体模型？

A: 目前提供程序化生成的简化模型用于测试。生产环境建议：
- 使用 MakeHuman 导出的模型 (CC0 许可)
- 购买商业模型
- 自己制作

### Q: AI 模型从哪里获取？

A: 推荐的开源模型：
- **人脸检测**: MediaPipe Face Detection
- **特征点**: MediaPipe Face Mesh (468点)
- **3DMM**: DECA, EMOCA, FaceVerse

需要将模型转换为 ONNX 格式。

### Q: 性能如何优化？

A: 
1. 使用 GPU BlendShape 计算 (`blend_shape.metal`)
2. 只在参数变化时更新网格
3. 减少活跃 BlendShape 数量 (最多 64 个)
4. 使用 LOD 系统

---

*最后更新: 2026-01-23*
