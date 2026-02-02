# LUMA Creator 开发日志

本文档记录 LUMA Creator 的开发进度和计划，持续更新。

---

## 📋 已完成功能

### 角色创建系统 (Phase 21-32)
- [x] **角色模板系统**
  - CharacterTemplate 抽象接口
  - Human/Cartoon/Mascot 模板实现
  - BodyProportions 身体比例预设

- [x] **身体部件系统**
  - 可组合的头/躯干/四肢/尾巴/翅膀
  - 程序化形状生成（球/椭球/胶囊/圆柱/圆锥/盒子）
  - 自动骨骼生成

- [x] **卡通特征系统**
  - 多种眼睛样式（Circle/Oval/Dot/Button/Anime/Disney）
  - 多种耳朵样式（MouseRound/CatPointed/BunnyLong/BearRound）
  - 鼻子和嘴巴样式（支持无嘴设计）
  - 配饰（蝴蝶结/项圈等）

- [x] **服装系统**
  - 服装加载器（OBJ/glTF/FBX）
  - 布料物理模拟（质点-弹簧）
  - 程序化布料纹理（棉/牛仔/丝绸/皮革/羊毛）
  - 自动骨骼蒙皮

- [x] **贴图系统**
  - 皮肤/眼睛/嘴唇程序化纹理
  - UV 映射优化
  - 法线贴图和粗糙度贴图

- [x] **发型系统**
  - 发型库（平头/中长发/马尾等）
  - 13 种颜色预设
  - 程序化发型生成

- [x] **标准绑定系统（Standard Rig）**
  - 支持 Mixamo / Unity Humanoid / VRM / Unreal 骨骼标准
  - 自动骨骼命名转换和动画重定向
  - ARKit 52 BlendShapes 面部绑定
  - VRM 表情和 Viseme（口型同步）支持
  - 自动骨骼权重生成（距离/热扩散）

### 编辑器系统 (Phase 31-32)
- [x] **撤销/重做系统**
  - Command Pattern 实现
  - 命令合并（连续滑块调整）
  - 复合命令支持
  - 内存限制管理

- [x] **快捷键系统**
  - 60+ 预设快捷键
  - 用户自定义绑定
  - 冲突检测和配置保存

- [x] **渲染预设系统**
  - 5 档质量预设（Preview → Ultra）
  - 完整渲染参数控制
  - 高质量输出设置

- [x] **Lip Sync 口型同步**
  - 27 种 Viseme 口型
  - 音频实时分析驱动
  - ARKit BlendShape 映射

- [x] **场景布置系统**
  - 多物体层级管理
  - 6 种图层分类
  - 6 套场景预设
  - 对齐/分布工具

- [x] **相机动画系统**
  - 关键帧时间轴
  - 9 种预设运动
  - Bezier 曲线插值
  - 景深动画支持

### 核心渲染系统
- [x] **DirectX 12 渲染后端**
  - DX12 设备初始化、命令队列、命令列表
  - 描述符堆管理（SRV、RTV、DSV）
  - 根签名和管线状态对象（PSO）
  - 常量缓冲区（256 字节对齐）
  - 深度测试和深度缓冲区

- [x] **PBR 渲染管线**
  - Cook-Torrance BRDF（GGX 分布、Smith 几何函数、Fresnel-Schlick）
  - 能量守恒（金属度/粗糙度）
  - ACES Filmic 色调映射
  - Gamma 校正（sRGB ↔ Linear）
  - 方向光 + 半球环境光

- [x] **纹理支持**
  - 基础色贴图（Albedo/Diffuse）
  - 法线贴图（Normal Mapping，TBN 矩阵）
  - ORM 贴图（Occlusion-Roughness-Metallic）
  - 纹理自动检测（通过采样亮度判断）
  - 嵌入纹理支持（FBX 内嵌纹理）
  - 各向异性过滤

- [x] **模型加载**
  - Assimp 集成（支持 FBX/OBJ/glTF/DAE/3DS 等 40+ 格式）
  - 顶点数据（位置、法线、切线、UV、颜色）
  - 材质参数读取（Metallic、Roughness、Base Color）
  - 模型边界计算（中心点、半径）
  - 多网格支持

### 视口与相机
- [x] **Maya 风格相机控制**
  - 旋转（Alt + 左键）：围绕目标点轨道旋转
  - 平移（Alt + 中键）：上下左右平移视口
  - 缩放（Alt + 右键 / 滚轮）：推拉相机距离
  - 重置相机（F 键）
  - 自动旋转（可选）

- [x] **动态裁剪平面**
  - 根据相机距离自动调整近/远裁剪平面
  - 支持大场景缩放（避免物体消失）

- [x] **视觉辅助**
  - 世界坐标轴（原点 XYZ 轴指示器）
  - 地面网格（可切换显示，G 键）
  - 3D 方向指示器（视口角落）

### 用户界面
- [x] **ImGui 集成**
  - 深色主题
  - 主菜单栏（File、View）
  - 模型信息面板（显示模型名、网格数、顶点数等）
  - 相机控制面板（距离、旋转角度、目标偏移）
  - 视口设置面板（网格、坐标轴开关）
  - 帮助覆盖层（F1 键，快捷键说明）
  - 状态栏（FPS 显示）

- [x] **文件对话框**
  - Windows 原生文件选择对话框
  - 支持多格式过滤

### 架构设计
- [x] **模块化架构**
  - `UnifiedRenderer`：跨平台渲染器（DX12/Metal）
  - `OrbitCamera`：相机状态管理
  - `Viewport`：视口控制器（输入处理、渲染编排）
  - `UIPanels`：UI 面板模块化
  - `AsyncTextureLoader`：异步纹理加载系统

- [x] **资源管理**
  - GPU 缓冲区管理（顶点、索引）
  - 纹理资源管理（SRV 索引分配）
  - 命令列表同步（Fence、等待 GPU）

---

## 🎯 开发路线图

按优先级排序的功能开发计划：

### 阶段 1：场景图 + 多对象支持 ✅ [已完成]

这是所有后续功能的基础。

- [x] **场景图系统**
  - Entity 实体类（唯一 ID、名称、启用状态）
  - Transform 组件（位置、旋转、缩放、矩阵计算）
  - 父子层级关系（局部/世界变换）
  - SceneGraph 管理器（创建、删除、遍历）

- [x] **多对象渲染**
  - 每个 Entity 关联 RHILoadedModel
  - 按 Entity Transform 渲染模型
  - 支持多个模型同时显示
  - UnifiedRenderer::setCamera() + renderModel() API

- [x] **场景编辑器 UI**
  - 场景层次面板（Scene Hierarchy）
  - 添加/删除对象
  - Inspector 面板（Transform 编辑）

### 阶段 2：阴影系统 ✅ [已完成]

PBR 之后最重要的视觉特性。

- [x] **Shadow Mapping**
  - 方向光深度贴图 (2048x2048 默认)
  - 阴影矩阵计算 (正交投影)
  - Shader 阴影采样

- [x] **阴影质量**
  - PCF 3x3 软阴影
  - 深度偏置 + 法线偏置
  - 可配置阴影距离

**API:**
```cpp
ShadowSettings settings;
settings.enabled = true;
settings.mapSize = 2048;
settings.bias = 0.005f;
settings.pcfSamples = 3;
renderer.setShadowSettings(settings);

// In render loop:
renderer.beginShadowPass(sceneRadius);
scene.traverseRenderables([&](Entity* e) {
    renderer.renderModelShadow(e->model, e->worldMatrix.m);
});
renderer.endShadowPass();
```

### 阶段 3：IBL 环境光照 ✅ [已完成]

让 PBR 材质效果更完整。

- [x] **Image-Based Lighting**
  - HDR 环境贴图加载（.hdr 格式）
  - 辐照度贴图（Diffuse IBL）- CPU 预计算
  - 预过滤环境贴图（Specular IBL）- CPU 预计算
  - BRDF LUT (Split-Sum 近似)

**API:**
```cpp
IBLSettings ibl;
ibl.enabled = true;
ibl.intensity = 1.0f;
ibl.rotation = 0.0f;
renderer.setIBLSettings(ibl);
renderer.loadEnvironmentMap("path/to/environment.hdr");

// Shader 自动使用 IBL 替代简单环境光
```

### 阶段 4：Shader 热重载 ✅ [已完成]

提升开发迭代效率。

- [x] **文件监控**
  - 跨平台文件监视器 (FileWatcher)
  - 监控 shaders/ 目录变化
  - 检测到修改时自动触发重编译

- [x] **Pipeline 热替换**
  - DX12: 从外部 .hlsl 文件加载和编译
  - Metal: 从外部 .metal 文件重新编译
  - 验证编译成功后替换 Pipeline
  - 失败时保持旧版本并输出错误

**API:**
```cpp
// 启用热重载
renderer.setShaderHotReload(true);

// 每帧检查文件变化
renderer.checkShaderReload();

// 手动触发重载
if (!renderer.reloadShaders()) {
    std::cout << "Error: " << renderer.getShaderError() << std::endl;
}

// 检查状态
bool enabled = renderer.isShaderHotReloadEnabled();
```

**文件位置:**
- `engine/renderer/shaders/pbr.hlsl` (DX12)
- `engine/renderer/shaders/shadow.hlsl` (DX12)
- `engine/renderer/shaders/pbr.metal` (Metal)

### 阶段 5：对象选择与操作 ✅ [已完成]

编辑器核心交互。

- [x] **鼠标拾取**
  - 射线-AABB 相交检测
  - 屏幕坐标转世界空间射线
  - 场景遍历找最近命中

- [x] **Transform Gizmo**
  - 移动手柄（XYZ 轴）
  - 旋转手柄（三个平面圆环）
  - 缩放手柄（轴向 + 均匀缩放）
  - 悬停/激活状态高亮

**文件:**
- `engine/scene/picking.h` - 射线、AABB、拾取
- `engine/editor/gizmo.h` - Transform Gizmo

**API:**
```cpp
// 射线拾取
float vpInverse[16];
renderer.getViewProjectionInverse(vpInverse);

float ndcX, ndcY;
pixelToNDC(mouseX, mouseY, width, height, ndcX, ndcY);
Ray ray = screenToWorldRay(ndcX, ndcY, vpInverse);

PickResult result = pickEntity(scene, ray);
if (result.hit()) {
    scene.setSelectedEntity(result.entity);
}

// Gizmo 使用
TransformGizmo gizmo;
gizmo.setMode(GizmoMode::Translate);  // 或 Rotate, Scale
gizmo.setTarget(selectedEntity);

// 检测悬停
gizmo.testHover(ray, screenScale);

// 拖拽交互
if (mouseDown) gizmo.beginDrag(ray, screenScale);
if (dragging) gizmo.updateDrag(ray);
if (mouseUp) gizmo.endDrag();

// 渲染 Gizmo
GizmoRenderData data = gizmo.generateRenderData(screenScale);
std::vector<float> lines;
for (auto& line : data.lines) {
    lines.insert(lines.end(), {line.start.x, line.start.y, line.start.z,
                               line.end.x, line.end.y, line.end.z,
                               line.color[0], line.color[1], line.color[2], line.color[3]});
}
renderer.renderGizmoLines(lines.data(), data.lines.size());
```

### 阶段 6：动画系统 ✅ [已完成]

角色和物体动画。

- [x] **骨骼动画**
  - 骨骼层次结构 (Skeleton)
  - 逆绑定矩阵 (Inverse Bind Matrix)
  - 骨骼局部变换

- [x] **动画剪辑 (AnimationClip)**
  - 位置/旋转/缩放关键帧
  - 线性/球面插值
  - 循环/单次播放

- [x] **动画播放器 (Animator)**
  - 动画状态管理
  - 交叉淡入淡出 (Crossfade)
  - 多动画混合

- [x] **GPU Skinning**
  - HLSL/MSL Skinning Shader
  - 每顶点 4 骨骼影响
  - 骨骼矩阵缓冲区

**文件:**
- `engine/animation/skeleton.h` - 骨骼层次
- `engine/animation/animation_clip.h` - 动画数据
- `engine/animation/animator.h` - 播放和混合
- `engine/animation/animation.h` - 统一头文件
- `engine/renderer/shaders/skinned.hlsl` - DX12 蒙皮着色器
- `engine/renderer/shaders/skinned.metal` - Metal 蒙皮着色器

**API:**
```cpp
// 创建骨骼
Skeleton skeleton;
int rootBone = skeleton.addBone("root", -1);
int spineBone = skeleton.addBone("spine", rootBone);
skeleton.setInverseBindMatrix(rootBone, inverseBindMat);

// 创建动画剪辑
auto clip = std::make_unique<AnimationClip>();
clip->name = "walk";
clip->duration = 1.0f;
auto& channel = clip->addChannel("spine");
channel.rotationKeys.push_back({0.0f, Quat::fromEuler(0, 0, 0)});
channel.rotationKeys.push_back({1.0f, Quat::fromEuler(0, 0.5f, 0)});

// 播放动画
Animator animator;
animator.setSkeleton(&skeleton);
animator.addClip("walk", std::move(clip));
animator.play("walk", 0.2f);  // 0.2s crossfade

// 每帧更新
animator.update(deltaTime);

// 获取蒙皮矩阵
Mat4 skinningMatrices[MAX_BONES];
animator.getSkinningMatrices(skinningMatrices);
// 上传到 GPU 并渲染
```

### 阶段 7：后处理效果 ✅ [已完成]

- [x] **Bloom 泛光**
  - 亮度阈值提取
  - 多级高斯模糊
  - 软阈值过渡

- [x] **色调映射 (Tone Mapping)**
  - Reinhard
  - ACES Filmic
  - Filmic
  - Uncharted 2
  - 曝光调整

- [x] **色彩调整 (Color Grading)**
  - Lift/Gamma/Gain
  - 色温/色调
  - 对比度/饱和度

- [x] **FXAA 抗锯齿**
  - 边缘检测
  - 亚像素混合

- [x] **其他效果**
  - Vignette 暗角
  - Chromatic Aberration 色差
  - Film Grain 胶片颗粒

**文件:**
- `engine/renderer/post_process.h` - 后处理设置和常量
- `engine/renderer/shaders/post_process.hlsl` - DX12 后处理着色器
- `engine/renderer/shaders/post_process.metal` - Metal 后处理着色器

**API:**
```cpp
PostProcessSettings pp;

// Bloom
pp.bloom.enabled = true;
pp.bloom.threshold = 1.0f;
pp.bloom.intensity = 0.8f;

// Tone Mapping
pp.toneMapping.mode = ToneMappingSettings::Mode::ACES;
pp.toneMapping.exposure = 1.2f;

// Color Grading
pp.colorGrading.saturation = 1.1f;
pp.colorGrading.temperature = 0.1f;  // 偏暖

// Vignette
pp.vignette.enabled = true;
pp.vignette.intensity = 0.3f;

// FXAA
pp.fxaa.enabled = true;

// 应用到渲染器
renderer.setPostProcessSettings(pp);
```

### 阶段 8：序列化与资源管理 ✅ [已完成]

- [x] **JSON 序列化库**
  - 轻量级 header-only 实现
  - 支持 null/bool/number/string/array/object
  - 解析器和格式化输出
  - 文件读写

- [x] **场景序列化**
  - Entity/Transform/Vec3/Quat 序列化
  - 层级关系保存
  - 模型引用持久化
  - 场景文件加载/保存

- [x] **资源管理器**
  - 资产缓存 (Asset Cache)
  - 引用计数
  - 自动垃圾回收 (GC)
  - 缓存统计

**文件:**
- `engine/serialization/json.h` - JSON 解析器/写入器
- `engine/serialization/scene_serializer.h` - 场景序列化
- `engine/asset/asset_manager.h` - 资源管理器

**API:**

```cpp
// === JSON ===
JsonValue obj = JsonValue::object();
obj["name"] = "MyScene";
obj["entities"] = JsonValue::array();

std::string jsonStr = toJson(obj, true);  // 格式化输出
JsonValue parsed = parseJson(jsonStr);

// 文件操作
saveJsonFile("scene.json", obj);
JsonValue loaded = loadJsonFile("scene.json");

// === 场景序列化 ===
SceneGraph scene;
// ... 创建实体 ...

// 保存场景
SceneSerializer::saveScene(scene, "level1.scene", "Level 1");

// 加载场景
SceneSerializer::loadScene(scene, "level1.scene", 
    [&](const std::string& path, RHILoadedModel& model) {
        return renderer.loadModel(path, model);
    });

// === 资源管理器 ===
AssetManager& assets = getAssetManager();

// 设置加载器
assets.setModelLoader([&](const std::string& path) {
    return std::make_shared<Model>(loadModel(path));
});

// 加载资源（自动缓存）
auto model = assets.loadModel<Model>("models/player.fbx");

// 引用计数
assets.addRef("models/player.fbx");
assets.release("models/player.fbx");

// 垃圾回收
assets.setUnusedTimeout(std::chrono::seconds(300));
assets.collectGarbage();

// 统计
auto stats = assets.getStatistics();
// stats.cacheHits, stats.cacheMisses, stats.hitRate
```

### 阶段 9：编辑器 UI 系统 ✅ [已完成]

完整的 ImGui 编辑器界面系统，支持现代编辑器工作流。

**文件:**
- `engine/ui/editor_ui.h` - 完整的编辑器 UI 系统

**UI 布局:**
```
┌──────────────────────────────────────────────────────────────────────┐
│  File  Edit  View  Window                              FPS: 60      │ <- Menu Bar
├──────────────────────────────────────────────────────────────────────┤
│  [Move] [Rotate] [Scale] | [Local/World] | [x] Snap  | [>] [||]     │ <- Toolbar
├─────────────┬────────────────────────────────────┬───────────────────┤
│ Hierarchy   │                                    │ Inspector         │
│             │                                    │                   │
│ ◆ Player    │         3D Viewport                │ Name: [Player  ]  │
│   ◆ Weapon  │                                    │                   │
│ ◆ Enemy     │                                    │ ▼ Transform       │
│ ○ Light     │                                    │   Position [x,y,z]│
│             │                                    │   Rotation [x,y,z]│
│             │                                    │   Scale    [x,y,z]│
│             │                                    │                   │
├─────────────┤                                    │ ▼ Model           │
│ Post Proc.  │                                    │   Meshes: 3       │
│ ▼ Bloom     │                                    │   Verts: 12450    │
│   [x] On    │                                    │                   │
│   Thresh 1.0│                                    │ [Add Component]   │
├─────────────┴─────────────────────┬──────────────┴───────────────────┤
│ Assets                            │ Animation Timeline               │
│ [D] models/  [M] player.fbx      │ Clip: [Walk    v]  [Loop] Speed  │
│ [T] diffuse.png                  │      |<  <  [>]  >  >|           │
│                                  │ Time: 0.50 / 2.00                │
│                                  │ ▓▓▓▓▓▓▓|░░░░░░░░░░░░░░░░░       │
├──────────────────────────────────┴───────────────────────────────────┤
│ W/E/R: Transform | Alt+Mouse: Camera | F: Focus | G: Grid           │ <- Status Bar
└──────────────────────────────────────────────────────────────────────┘
```

**功能面板:**

| 面板 | 功能 |
|------|------|
| **Toolbar** | 变换工具 (移动/旋转/缩放)、坐标空间、吸附、动画播放 |
| **Hierarchy** | 场景树、搜索、拖拽排序、右键菜单、添加实体 |
| **Inspector** | 实体属性、Transform 编辑、组件管理 |
| **Post Processing** | Bloom、Tone Mapping、Color Grading、Vignette、FXAA 等 |
| **Render Settings** | 阴影设置、IBL 设置、调试可视化 |
| **Lighting** | 方向光、环境光设置 |
| **Animation Timeline** | 动画剪辑选择、播放控制、时间轴拖动 |
| **Asset Browser** | 文件浏览、拖拽加载、资源预览 |
| **Console** | 日志输出、错误信息 |
| **Statistics** | FPS、Draw Calls、三角形/顶点数 |
| **Shader Status** | Hot-Reload 状态、编译错误 |

**快捷键:**
- `W` - 移动工具
- `E` - 旋转工具  
- `R` - 缩放工具
- `F` - 聚焦选中对象
- `G` - 切换网格
- `Del` - 删除选中
- `Ctrl+D` - 复制
- `Alt+LMB` - 轨道旋转
- `Alt+MMB` - 平移
- `Alt+RMB` - 缩放

**使用示例:**
```cpp
#include "engine/ui/editor_ui.h"

// 初始化
ui::applyEditorTheme();

ui::EditorState editorState;
ui::PostProcessSettings postProcess;
ui::RenderSettings renderSettings;
ui::LightSettings lighting;
ui::AnimationState animation;

editorState.onModelLoad = [&](const std::string& path) {
    // 加载模型
};

// 渲染循环
bool shouldQuit = false;
ui::drawMainMenuBar(editorState, viewport, shouldQuit);
ui::drawToolbar(editorState, gizmo);
ui::drawHierarchyPanel(scene, editorState);
ui::drawInspectorPanel(scene, editorState);
ui::drawPostProcessPanel(postProcess, editorState);
ui::drawRenderSettingsPanel(renderSettings, editorState);
ui::drawLightingPanel(lighting, editorState);
ui::drawAnimationTimeline(animation, editorState);
ui::drawAssetBrowser(editorState);
ui::drawConsole(editorState);
ui::drawStatsPanel(editorState);
ui::drawShaderStatus(shaderError, hotReload, onReload, editorState);
ui::drawStatusBar(width, height);
```

### 阶段 10：整合测试与应用重构 🚧 [进行中]

将所有已实现的功能整合到主应用中，验证协同工作。

#### 10.1 应用重命名 ✅
- [x] `creator_imgui` → `luma_studio` (LUMA Studio)
- [x] `creator_macos` → `luma_studio_macos`
- [x] 更新 CMakeLists.txt 中的目标名称
- [x] 更新窗口标题为 "LUMA Studio"

#### 10.2 新 UI 系统接入 ✅
- [x] 替换旧的 `panels.h` 为新的 `editor_ui.h`
- [x] 接入 EditorState 状态管理
- [x] 接入 Toolbar 和 Gizmo 控制
- [x] 接入完整的 Inspector 面板
- [x] 接入 Post-Processing / Render Settings / Lighting 面板
- [x] 接入 Animation Timeline
- [x] 接入 Asset Browser 和 Console
- [x] Windows (DX12) 和 macOS (Metal) 双平台更新

#### 10.3 后处理管线接入 ✅
- [x] 后处理 API 已添加到 UnifiedRenderer
  - `setPostProcessEnabled()` / `isPostProcessEnabled()`
  - `setPostProcessParams()` / `getFrameTime()`
- [x] Frame time 跟踪（用于动画效果）
- [x] UI 参数传递到渲染器（Windows + macOS）
- [x] **HDR 渲染目标** (RGBA16Float)
- [x] **Bloom 管线**:
  - Bloom 阈值提取 pass
  - 水平高斯模糊 pass (9-tap)
  - 垂直高斯模糊 pass (9-tap)
  - 最终合成 pass (scene + bloom + tone mapping + vignette + film grain)
- [x] DX12 实现 (`unified_renderer_dx12.cpp`)
- [x] Metal 实现 (`unified_renderer_metal.mm`)

**后处理管线架构:**
```
Scene ──▶ HDR Target (RGBA16F) ──▶ Bloom Threshold ──▶ Blur H ──▶ Blur V ──▶ Composite ──▶ Swapchain
                                        │                                         │
                                        └── 提取亮度 > threshold 的像素 ─────────────┘
                                                                                   │
                                                                        + ACES Tone Mapping
                                                                        + Vignette
                                                                        + Film Grain
                                                                        + Gamma Correction
```

#### 10.4 动画系统接入 ✅
**已完成:**
- [x] 动画数据结构 (Skeleton, AnimationClip, Animator)
- [x] 模型加载器支持骨骼和动画 (`load_model_with_animations`)
- [x] SkinnedVertex 结构 (bone indices + weights)
- [x] Skinned shader (HLSL + Metal)
- [x] Entity 添加 Skeleton 和 Animator 成员
- [x] UnifiedRenderer 添加 `renderSkinnedModel()` 方法
- [x] DX12 skinned pipeline (root signature + PSO + bone buffer)
- [x] Metal skinned pipeline (pipeline state + bone buffer)
- [x] 应用中使用 `load_model_with_animations` (Windows + macOS)
- [x] 连接 Timeline UI 到 Animator (播放/暂停/时间同步)
- [x] 动画系统单元测试 (`animation_test.h`)
- [x] Animator 添加 `setLooping()` 和 `setTime()` 方法

**文件:**
- `engine/animation/skeleton.h` - 骨骼层次结构
- `engine/animation/animation_clip.h` - 关键帧动画数据
- `engine/animation/animator.h` - 动画播放和混合
- `engine/animation/animation.h` - 统一头文件
- `engine/animation/animation_test.h` - 动画系统测试
- `engine/scene/entity.h` - Entity 支持 skeleton + animator
- `engine/renderer/mesh.h` - SkinnedVertex 结构
- `engine/renderer/unified_renderer.h` - renderSkinnedModel API
- `engine/renderer/shaders/skinned.hlsl` - GPU 蒙皮着色器 (DX12)
- `engine/renderer/shaders/skinned.metal` - GPU 蒙皮着色器 (Metal)
- `apps/luma_studio/main.cpp` - Windows 应用集成
- `apps/luma_studio_macos/LumaView.mm` - macOS 应用集成

**测试:**
运行动画系统测试: `cmake --build build && ./build/luma_anim_test`
测试内容:
1. 骨骼创建 (3-bone arm skeleton)
2. 动画剪辑创建 (wave animation)
3. Animator 播放和更新
4. 时间追踪
5. Skinned mesh 创建
6. 循环播放
7. 停止/重置

#### 10.5 场景序列化接入 ✅
- [x] File → Save Scene 功能（已连接回调）
- [x] File → Open Scene 功能（已连接回调）
- [x] 模型路径持久化
- [x] 保存/恢复相机状态 (yaw, pitch, distance, target offset)
- [x] 保存/恢复后处理设置 (bloom, tone mapping, vignette, chromatic aberration, film grain, FXAA)
- [x] 场景文件版本升级 (v1 → v2)
- [x] 新增 `saveSceneFull()` / `loadSceneFull()` API

**文件:**
- `engine/serialization/scene_serializer.h` - 扩展序列化支持
  - `serializeCameraParams()` / `deserializeCameraParams()`
  - `serializePostProcess()` / `deserializePostProcess()`
- `apps/luma_studio/main.cpp` - Windows 应用使用新 API
- `apps/luma_studio_macos/LumaView.mm` - macOS 应用使用新 API

**场景文件格式 (v2):**
```json
{
  "version": 2,
  "name": "My Scene",
  "camera": {
    "yaw": 0.78, "pitch": 0.5, "distance": 1.0,
    "targetOffsetX": 0, "targetOffsetY": 0, "targetOffsetZ": 0
  },
  "postProcess": {
    "bloom": { "enabled": true, "threshold": 1.0, ... },
    "toneMapping": { "mode": 2, "exposure": 1.0, ... },
    "vignette": { ... }, "filmGrain": { ... }, "fxaa": { ... }
  },
  "entities": [...]
}
```

#### 10.6 资源管理接入 ✅
- [x] Asset Browser 连接到 AssetManager
- [x] 拖拽模型到 Viewport
- [x] 缓存统计显示 (新增 Cache tab)
- [x] AssetManager 初始化 (model loader, cache settings)
- [x] Windows + macOS 双平台支持

**新增 UI 功能:**
- Asset Browser 新增 "Cache" tab 显示缓存统计
  - Total Loads / Cache Hits / Cache Misses
  - Hit Rate 进度条
  - Cached Assets 数量 / Cache Size (MB)
- 文件浏览器增强 (文件大小 tooltip)
- Viewport 拖拽接收 (从 Asset Browser 拖拽文件到场景)

**文件:**
- `engine/ui/editor_ui.h`:
  - `AssetCacheStats` 结构
  - `drawAssetBrowserExtended()` - 带缓存统计的 Asset Browser
  - `handleViewportDragDrop()` - Viewport 拖拽处理
- `apps/luma_studio/main.cpp` - Windows AssetManager 集成
- `apps/luma_studio_macos/LumaView.mm` - macOS AssetManager 集成

#### 10.7 功能验证清单 ✅

**自动化测试** (`luma_integration_test`):
- [x] Math Types: Vec3/Quat/Mat4 运算
- [x] Transform: 矩阵构建、欧拉角转换
- [x] SceneGraph: 创建/删除/层级/查找
- [x] Animation: Skeleton/Clip/Animator
- [x] Serialization: JSON 解析/写入/场景序列化
- [x] PostProcess: 设置结构验证

**手动测试清单** (运行 `--manual` 参数查看):
- [x] 场景图：创建、删除、重命名、父子关系
- [x] Transform：位置、旋转、缩放编辑
- [x] Gizmo：移动、旋转、缩放操作
- [x] 阴影：方向光阴影、PCF 软阴影
- [x] IBL：环境贴图加载、反射效果
- [x] 后处理：Bloom、ToneMapping、FXAA
- [x] 动画：播放、暂停、时间轴拖动
- [x] 序列化：保存、加载场景（含相机和后处理设置）
- [x] Shader 热重载：修改 .hlsl/.metal 自动更新

**测试文件:**
- `tests/integration_test.h` - 测试套件
- `tests/run_tests.cpp` - 测试运行器
- 编译: `cmake --build build --target luma_integration_test`
- 运行: `./build/luma_integration_test [--manual]`

---

## 阶段 11：产品功能完善 ✅

> 目标：将 LUMA Studio 从技术演示提升为可用的产品
> 
> **完成状态**: 11.1-11.6 已完成，11.7 平台扩展待后续实现

### 11.1 Undo/Redo 系统 ✅
- [x] Command 模式基础架构 (`engine/editor/command.h`)
- [x] Transform 操作撤销 (`engine/editor/commands/transform_commands.h`)
- [x] 场景图操作撤销 (`engine/editor/commands/scene_commands.h`)
- [x] 快捷键绑定 (Ctrl+Z / Ctrl+Shift+Z, Cmd+Z / Cmd+Shift+Z on macOS)
- [x] History Panel UI (Edit > History Panel)
- [x] 对象复制 Ctrl+D / Cmd+D
- [ ] 材质修改撤销 (待材质编辑器实现后添加)

### 11.2 材质编辑器 ✅
- [x] PBR Material 结构 (`engine/material/material.h`)
- [x] MaterialLibrary 管理器
- [x] Inspector 材质属性面板
- [x] Base Color 颜色选择器 (RGBA)
- [x] Metallic / Roughness / AO 滑块
- [x] Emissive 颜色与强度控制
- [x] Advanced 属性 (Normal Strength, IOR, Two-Sided, Alpha)
- [x] 纹理槽位 UI (拖拽支持)
- [x] 材质预设 (Gold, Silver, Copper, Plastic, Rubber, Glass, Emissive)
- [x] Material Commands for Undo/Redo (`engine/editor/commands/material_commands.h`)

### 11.3 截图与导出 ✅
- [x] ScreenshotExporter 系统 (`engine/export/screenshot.h`)
- [x] PNG/JPG 格式支持
- [x] 分辨率预设 (HD, FHD, 4K, Square)
- [x] 自定义分辨率
- [x] 透明背景选项
- [x] Supersampling (1x/2x/4x)
- [x] Screenshot Settings Dialog UI
- [x] File 菜单集成 (F12 快捷键)
- [x] 动画序列导出 API
- [ ] 视频编码 (MP4/GIF) - 需第三方库

### 11.4 多光源系统 ✅
- [x] Light 类型定义 (`engine/lighting/light.h`)
- [x] 支持 Directional / Point / Spot 光源
- [x] LightManager 管理器 (最多 16 盏灯)
- [x] AmbientLight 设置 (含 IBL 选项)
- [x] Entity Light 组件集成
- [x] Lighting Panel UI (添加/删除/编辑光源)
- [x] Inspector 光源属性编辑
- [x] 光源衰减计算 (constant/linear/quadratic)
- [x] Spot 光锥计算 (inner/outer cone)
- [x] 阴影参数 (bias, softness, map size)
- [x] GPU Light Data 打包 API
- [ ] 光源 Gizmo 可视化 (待实现)
- [ ] 多光源阴影渲染 (待实现)

### 11.5 编辑器增强 ✅
- [x] 多选支持 (`SceneGraph::selectedEntities_`)
- [x] 对象复制/粘贴 (`copySelection/pasteClipboard`)
- [x] 实体复制 (`duplicateEntity`)
- [x] 相机预设 (Front/Back/Left/Right/Top/Bottom/Perspective)
- [x] 相机书签保存/加载
- [x] View 菜单 Camera View 子菜单
- [x] View 菜单 Camera Bookmarks 子菜单
- [x] 线框模式选项
- [x] 正交视图选项
- [ ] 悬停高亮 (待实现)
- [ ] 框选支持 (待实现)

### 11.6 性能优化 ✅
- [x] **Frustum Culling** (`engine/rendering/culling.h`)
  - Plane/Frustum 数学库
  - BoundingSphere / AABB 包围体
  - FrustumCuller 视锥裁剪器
  - CullingSystem 统一管理
- [x] **LOD System** (`engine/rendering/lod.h`)
  - LODLevel / LODGroup 定义
  - 距离/屏幕尺寸选择
  - 平滑过渡支持
  - LODManager 全局管理
  - Quality 预设 (Low/Medium/High/Ultra)
- [x] **GPU Instancing** (`engine/rendering/instancing.h`)
  - InstanceData GPU 数据结构
  - InstanceBatch 批次管理
  - InstancingManager 实例化管理器
  - IndirectDrawCommand 间接绘制支持
- [x] **Occlusion Culling** (`engine/rendering/culling.h`)
  - OcclusionCuller 遮挡查询辅助
  - 像素阈值可配置
- [x] **Render Optimizer** (`engine/rendering/render_optimizer.h`)
  - 统一优化管线
  - RenderQueue 渲染队列 (前后排序)
  - 透明/不透明分离
  - 帧统计信息

### 11.7 平台扩展 (移至 Phase 14)
- [ ] iOS 查看器
- [ ] Android 查看器
- [ ] WebGL/WebGPU 版本

---

## 阶段 12：动画系统增强 ✅

> 目标：完善动画系统，支持复杂动画混合和状态管理

### 12.1 动画混合系统 ✅
- [x] 动画层 (`engine/animation/animation_layer.h`)
  - AnimationLayer 类
  - 多层动画播放
  - Override/Additive/Multiply 混合模式
- [x] 骨骼蒙版 (BoneMask)
  - 按骨骼名称过滤
  - 递归子骨骼包含
  - 预设蒙版 (UpperBody/LowerBody/Arms)
- [x] 混合树 (`engine/animation/blend_tree.h`)
  - BlendTree1D (1D 参数混合)
  - BlendTree2D (2D 参数混合)
  - 工厂方法 (Locomotion/Directional)
- [x] Crossfade 过渡
- [x] 加法混合 (Additive Blending)
- [x] AnimationLayerManager

### 12.2 动画状态机 ✅
- [x] AnimationStateMachine (`engine/animation/state_machine.h`)
- [x] AnimationState 状态节点
- [x] StateTransition 转换
- [x] TransitionCondition 条件
  - If/IfNot/Greater/Less/GreaterEqual/LessEqual
- [x] AnimationParameter 参数
  - Float/Int/Bool/Trigger 类型
- [x] Any State 转换
- [x] Exit Time 支持
- [x] 优先级排序
- [x] 预设状态机 (Locomotion/Combat)

### 12.3 逆向运动学 (IK) ✅
- [x] `engine/animation/ik_system.h`
- [x] TwoBoneIK (手臂/腿)
- [x] LookAtIK (头部跟踪)
- [x] FootIK (地形适应)
- [x] FABRIK (迭代式 IK 链)
- [x] IKManager (多 IK 管理)
- [x] IKRigHelper (人形骨骼自动设置)

### 12.4 Timeline 编辑器增强 ✅
- [x] `engine/animation/timeline.h`
- [x] AnimationCurve<T> (泛型曲线)
- [x] CurveKeyframe (Bezier 切线)
- [x] TimelineTrack (多轨道)
- [x] Timeline (播放控制)
- [x] 关键帧复制/粘贴
- [x] CurveEditorState (曲线编辑器 UI 状态)
- [x] 标记系统 (Markers)
- [x] 吸附 (Snap to frame/marker)

### 12.5 动画工具 ✅
- [x] `engine/animation/animation_tools.h`
- [x] AnimationRetargeter (动画重定向)
  - 骨骼映射
  - 自动映射生成
  - 旋转/缩放/镜像调整
- [x] AnimationCompressor (动画压缩)
  - 线性冗余关键帧删除
  - 可配置容差
  - 压缩统计
- [x] RootMotionExtractor (根运动)
  - 多种提取模式 (XZ/XYZ/Y/Rotation)
  - 烘焙/移除根运动
- [x] AnimationNotifyTrack (动画通知)
  - 事件/音效/粒子/脚步触发

---

## 阶段 13：高级渲染特效 ✅

> 目标：实现现代游戏引擎级别的渲染效果

### 13.1 SSAO (屏幕空间环境光遮蔽) ✅
- [x] `engine/rendering/ssao.h`
- [x] SSAOSettings (采样数/半径/强度等)
- [x] SSAOKernel (半球采样核)
- [x] SSAONoise (4x4 旋转噪声)
- [x] Metal Shader (SSAO + Blur + Apply)
- [x] 质量预设 (Low/Medium/High/Ultra)

### 13.2 HDR 环境光照 (IBL) ✅
- [x] `engine/rendering/ibl.h`
- [x] HDRImage (浮点 HDR 数据)
- [x] HDRLoader (Radiance 格式)
- [x] EnvironmentMap
  - 等距柱状图转立方体贴图
  - 漫反射辐照度卷积
  - 预滤波镜面反射贴图
  - BRDF LUT 生成
- [x] IBLSettings

### 13.3 屏幕空间反射 (SSR) ✅
- [x] `engine/rendering/ssr.h`
- [x] SSRSettings (步数/距离/厚度等)
- [x] SSRTracer (CPU 射线行进)
- [x] SSRHit 结果结构
- [x] 二分搜索精细化
- [x] 边缘淡出
- [x] Metal Shader (SSR + Blur + Composite)
- [x] 质量预设 (Low/Medium/High)

### 13.4 体积效果 ✅
- [x] `engine/rendering/volumetrics.h`
- [x] VolumetricFog
  - 高度衰减密度
  - Henyey-Greenstein 相位函数
  - 散射/吸收计算
- [x] GodRays (屏幕空间光线)
- [x] AtmosphericScattering
  - Rayleigh 散射 (蓝天)
  - Mie 散射 (太阳光晕)
  - 行星大气模拟
- [x] 预设 (Earth/Mars/LightFog/DenseFog)
- [x] Metal Shader (Fog + GodRay)

### 13.5 高级阴影 ✅
- [x] `engine/rendering/advanced_shadows.h`
- [x] Cascaded Shadow Maps (CSM)
  - 多级级联 (1-4)
  - 自动分割距离计算
  - 阴影稳定化 (防止游泳)
  - 级联混合
- [x] PCSS (Percentage Closer Soft Shadows)
  - Blocker 搜索
  - 可变半影大小
  - Poisson Disk 采样
- [x] 接触硬化阴影 (Contact Hardening)
- [x] CSM/PCF/PCSS Metal Shader
- [x] 质量预设 (Low/Medium/High/Ultra)

---

## 阶段 14：平台扩展 ✅

> 目标：将 LUMA Studio 扩展到多个平台

### 14.1 iOS 查看器 ✅
- [x] iOS Metal 渲染器 (`apps/luma_viewer_ios/`)
- [x] 触摸手势 (单指旋转/双指平移/捏合缩放/双击重置)
- [x] 移动端 UI 适配 (Info Label + Toolbar)
- [x] 性能优化 (Frustum Culling + LOD)
- [ ] ARKit 支持 (待实现)

**文件:**
- `apps/luma_viewer_ios/LumaViewController.mm` - 主视图控制器
- `apps/luma_viewer_ios/Info.plist` - iOS 应用配置
- `apps/luma_viewer_ios/LaunchScreen.storyboard` - 启动屏幕

**构建:**
```bash
# 生成 iOS Xcode 项目
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -B build-ios
# 在 Xcode 中打开并运行
open build-ios/luma.xcodeproj
```

### 14.2 Android 查看器 ✅
- [x] Vulkan 渲染器 (`apps/luma_viewer_android/`)
- [x] 触摸交互 (单指轨道/双指平移/捏合缩放)
- [x] JNI 桥接 (Kotlin ↔ C++)
- [x] 基础 UI (信息面板 + 工具栏)

**文件:**
- `apps/luma_viewer_android/app/src/main/cpp/vulkan_renderer.cpp` - Vulkan 渲染器
- `apps/luma_viewer_android/app/src/main/cpp/jni_bridge.cpp` - JNI 接口
- `apps/luma_viewer_android/app/src/main/java/com/luma/viewer/MainActivity.kt` - 主 Activity

**构建:**
```bash
cd apps/luma_viewer_android
./gradlew assembleDebug
# APK 输出: app/build/outputs/apk/debug/app-debug.apk
```

### 14.3 Web 版本 ✅
- [x] WebGPU 渲染器 (`apps/luma_viewer_web/`)
- [x] WGSL 着色器 (PBR 光照)
- [x] 鼠标/触摸交互 (轨道/平移/缩放)
- [x] 现代 Web UI (HTML5 + CSS3)
- [ ] WebGL 2.0 回退 (待实现)
- [ ] 资源流式加载 (待实现)

**文件:**
- `apps/luma_viewer_web/src/renderer.ts` - WebGPU 渲染器
- `apps/luma_viewer_web/src/camera.ts` - 轨道相机
- `apps/luma_viewer_web/src/geometry.ts` - 几何体工具

**构建:**
```bash
cd apps/luma_viewer_web
npm install
npm run dev    # 开发模式
npm run build  # 生产构建
```

---

## 阶段 15：测试与文档与示例 ✅

> 目标：确保代码质量和可维护性，提供完整示例

### 15.1 测试覆盖 ✅
- [x] `tests/unit_tests.h` - 完整单元测试套件
  - Math Tests (Vec3, Quat, Mat4)
  - Animation Tests (Skeleton, Clip, Animator, BlendTree, StateMachine)
  - Rendering Tests (Culling, LOD, SSAO, IBL, CSM, PCSS, Volumetrics)
  - IK Tests (TwoBoneIK, LookAtIK, IKManager)
  - Timeline Tests (Curve, Playback, Markers)
- [x] `tests/integration_test.h` - 集成测试
  - SceneGraph Tests
  - Transform Tests
  - Serialization Tests
- [x] `tests/run_tests.cpp` - 测试运行器
  - 命令行参数支持
  - 单元/集成测试选择
  - 手动测试清单

### 15.2 文档 ✅
- [x] `docs/ARCHITECTURE.md` - 架构设计文档
  - 目录结构
  - 核心系统概述
  - 渲染管线流程
  - 平台抽象层
  - 扩展点
- [x] `docs/API_REFERENCE.md` - API 参考文档
  - Math Types (Vec3, Quat, Mat4)
  - Scene Graph API
  - Animation API
  - Material & Lighting API
  - Rendering API
  - Editor API
  - Serialization API
  - Timeline API

### 15.3 示例项目 ✅

**A. 代码示例 (`examples/`)**
- [x] `01_basic_scene.h` - 场景创建、Transform、选择
- [x] `02_animation.h` - 骨骼动画、状态机、混合树、IK
- [x] `03_materials.h` - PBR 材质、预设、透明/发光
- [x] `04_lighting.h` - 光源类型、三点照明、色温
- [x] `05_post_processing.h` - Bloom、SSAO、SSR、体积效果
- [x] `06_performance.h` - Culling、LOD、Instancing、Benchmark
- [x] `examples.h` - 统一入口、分类目录

**B. 场景文件 (`assets/scenes/*.luma`)**
- [x] `demo_basic.luma` - 基础场景 (3 对象 + 双光源)
- [x] `demo_materials.luma` - 5×5 材质网格演示
- [x] `demo_lighting.luma` - 多光源 + 聚光灯演示

**C. 教程文档 (`docs/tutorials/`)**
- [x] `01_getting_started.md` - 快速入门
- [x] `02_scene_setup.md` - 场景组织
- [x] `03_materials.md` - PBR 材质详解
- [x] `04_animation.md` - 动画系统使用
- [x] `05_lighting.md` - 光照设置
- [x] `06_optimization.md` - 性能优化指南

**D. 内置演示模式 (`engine/editor/demo_mode.h`)**
- [x] DemoMode 类 (10 种演示场景)
- [x] DemoInfo 结构 (id/name/description/category)
- [x] 演示场景生成器:
  - basic - 基础场景
  - materials - 材质网格
  - lighting - 多光源
  - hierarchy - 层级关系 (太阳系)
  - animation_ready - 动画准备场景
  - post_process - 后处理演示 (发光)
  - stress_test - 压力测试 (900 对象)
  - material_presets - 所有材质预设
  - emissive - 发光/霓虹效果
  - three_point - 三点照明
- [x] DemoMenuState (Help 菜单集成)

### 15.4 UI 功能完善 ✅

**新增 UI 面板 (`engine/ui/editor_ui.h`):**

| 面板 | 功能 |
|------|------|
| **Advanced Post-Process** | SSAO 设置、SSR 设置、体积雾、God Rays |
| **Advanced Shadows** | CSM 级联设置、PCSS 软阴影 |
| **Environment** | HDR 环境贴图加载、IBL 强度/旋转 |
| **State Machine Editor** | 参数管理、状态列表、转换编辑 |
| **Blend Tree Editor** | 1D/2D 混合树可视化、参数控制 |
| **IK Settings** | IK 链管理、目标/权重控制 |
| **Animation Layers** | 图层权重、混合模式、骨骼蒙版 |
| **LOD Settings** | 质量预设、LOD 偏差、调试颜色 |
| **Demo Menu** | 10 种演示场景一键生成 |

**新增状态结构:**
- `AdvancedPostProcessState` - SSAO/SSR/Volumetrics 设置
- `AdvancedShadowState` - CSM/PCSS 设置
- `LODState` - LOD 质量设置

**菜单更新:**
- Window 菜单重组 (Panels / Rendering / Animation 分组)
- Help 菜单添加 Demo Scenes 入口
- File 菜单完善 (New/Open/Save 回调连接)
- Edit 菜单完善 (Delete/Duplicate 回调连接)

**功能完善:**
- 截图功能 (F12 快捷键, 设置对话框)
- HDR 环境加载 (文件对话框)
- 快捷键帮助面板扩展 (新增 Cmd+Z/D/C/V, F12)
- SceneCallbacks 系统 (跨模块回调)

### 15.5 高级渲染系统连接 ✅

**UnifiedRenderer 新增 API (`engine/renderer/unified_renderer.h`):**

| API | 功能 |
|-----|------|
| `setSSAOEnabled/Settings` | SSAO 开关和参数 |
| `setSSREnabled/Settings` | SSR 开关和参数 |
| `setVolumetricFogEnabled/Settings` | 体积雾开关和参数 |
| `setGodRaysEnabled/Settings` | God Rays 开关和参数 |
| `setCSMEnabled/Settings` | CSM 级联阴影开关和参数 |
| `setPCSSEnabled/Settings` | PCSS 软阴影开关和参数 |
| `updateCSM` | 每帧更新 CSM 级联 |

**Metal Shader 实现 (`engine/renderer/shaders/advanced_post_process.metal`):**
- SSAO: 半球采样、深度重建、模糊
- SSR: 射线行进、二分搜索细化、边缘淡出
- Volumetric Fog: Henyey-Greenstein 相位函数、高度衰减
- God Rays: 屏幕空间光线、Poisson 采样
- CSM: 级联选择、PCF 采样、PCSS 软阴影

**Impl 状态 (`unified_renderer_metal.mm`):**
- `ssaoEnabled`, `ssaoSettings`, `ssaoTexture`, `ssaoPipeline` 等
- `ssrEnabled`, `ssrSettings`, `ssrTexture`, `ssrPipeline` 等
- `fogEnabled`, `fogSettings`, `volumetricTexture`, `fogPipeline` 等
- `csmEnabled`, `csmSettings`, `csmTextures[4]`, `csmViewProj[4]` 等
- `pcssEnabled`, `pcssBlockerSamples`, `pcssPCFSamples`, `pcssLightSize`

**LumaView 连接 (`LumaView.mm`):**
- 每帧同步 UI 设置到渲染器
- SSAO/SSR/Volumetrics/GodRays 设置
- CSM/PCSS 阴影设置

---

## Phase 16: 粒子系统 ✅

### 16.1 核心数据结构 (`engine/particles/particle.h`)

**Particle 结构:**
- position, velocity, color, size
- startColor/endColor, startSize/endSize
- rotation, angularVelocity
- life/maxLife/age

**EmissionShape 枚举:**
- Point, Sphere, Hemisphere, Cone, Box, Circle, Edge, Mesh

**ParticleEmitter:**
- 发射率控制 (emissionRate, maxParticles)
- 形状参数 (EmissionShapeParams)
- 初始值范围 (life, speed, size, color, rotation)
- 物理 (gravity, drag)
- 爆发 (ParticleBurst)
- 纹理动画 (textureRows/Cols)

**ParticleSystem:**
- 多发射器管理
- 位置/旋转
- 播放控制 (play/stop/pause)

**ParticleManager:**
- 全局单例管理
- 系统创建/销毁
- 每帧更新

### 16.2 模块系统 (`engine/particles/particle_modules.h`)

| 模块 | 功能 |
|------|------|
| ColorOverLifetimeModule | 颜色渐变曲线 |
| SizeOverLifetimeModule | 大小曲线 |
| VelocityOverLifetimeModule | 线性/轨道/径向速度 |
| ForceFieldModule | 方向/点/漩涡/湍流力场 |
| NoiseModule | 噪声扰动 |
| RotationOverLifetimeModule | 角速度曲线 |
| LimitVelocityModule | 速度限制 |
| CollisionModule | 地面碰撞/反弹 |
| SubEmitterModule | 子发射器触发 |
| TrailModule | 拖尾效果 |
| TextureSheetModule | 纹理动画 |

**辅助类:**
- ColorGradient - 颜色渐变
- FloatCurve - 浮点曲线
- GradientKey<T> - 关键帧

### 16.3 GPU 渲染 (`engine/renderer/shaders/particle.metal`)

**Vertex Shaders:**
- `particleVertex` - 公告板粒子
- `particleVertexStretched` - 速度拉伸粒子
- `trailVertex` - 拖尾渲染

**Fragment Shaders:**
- `particleFragment` - 纹理粒子 + 软粒子淡出
- `particleFragmentAdditive` - 加法混合
- `particleFragmentCircle` - 圆形程序化粒子
- `particleFragmentStar` - 星形/火花粒子
- `particleFragmentSmoke` - 烟雾粒子 (噪声)
- `particleFragmentFire` - 火焰粒子

**Uniform 结构:**
- ParticleUniforms (viewProj, camera, texture sheet, soft distance)
- ParticleData (position, size, color, rotation, frame)

### 16.4 预设效果 (`engine/particles/particle_presets.h`)

| 类别 | 预设 |
|------|------|
| **火焰** | fire, fireWithSparks |
| **烟雾** | smoke, campfireSmoke, steam |
| **爆炸** | explosion, explosionWithSmoke |
| **魔法** | magicSparkle, magicAura, magicOrb, portal |
| **天气** | rain, heavyRain, snow, blizzard |
| **特效** | sparks, weldingSparks, dust, waterSplash, bloodSplash |
| **环境** | fallingLeaves, fireflies, confetti |

**复合效果:**
- createFireWithSparks() - 火焰 + 火星
- createExplosion() - 爆炸 + 烟雾
- createMagicOrb() - 核心 + 环绕粒子
- createPortal() - 环形 + 漩涡

### 16.5 编辑器 UI (`engine/ui/editor_ui.h`)

**ParticleEditorState:**
- selectedSystem, selectedEmitterIndex
- previewPlaying, previewSpeed

**drawParticleEditorPanel:**
- 系统列表 (创建/删除/重命名)
- 从预设创建
- 预览控制 (Play/Pause/Restart/Stop)
- 发射器 Tab 切换
- 属性编辑:
  - Emission (rate, max, looping, bursts)
  - Shape (type, parameters)
  - Lifetime
  - Velocity (speed, gravity, drag)
  - Size (start/end)
  - Color (start/end, gradient)
  - Rotation
  - Rendering (billboard, stretch, sort, texture sheet)

**菜单集成:**
- Window → Rendering → Particle Editor

### 16.6 应用集成 (`LumaView.mm`)

- 添加 ParticleEditorState 实例变量
- 每帧调用 ParticleManager::update()
- 绘制 drawParticleEditorPanel()

---

## Phase 17: 物理系统 ✅

### 17.1 核心数据结构 (`engine/physics/physics_world.h`)

**PhysicsWorld:**
- 刚体管理 (createBody/destroyBody)
- 固定时间步进 (step/fixedStep)
- 重力和全局设置 (PhysicsSettings)
- 碰撞回调 (CollisionCallback, TriggerCallback)
- 查询接口 (raycast, queryAABB, querySphere)

**RigidBody:**
- 类型: Static, Dynamic, Kinematic
- 质量和惯性张量
- 位置/旋转/速度
- 力/扭矩/冲量
- 材质 (restitution, friction)
- 阻尼 (linear/angular damping)
- 休眠系统

**Collider:**
- 形状类型: Sphere, Box, Capsule, Plane, Mesh, Compound
- 本地偏移和旋转
- 触发器模式
- 碰撞层和蒙版

**AABB:**
- 边界盒结构
- expand/intersects/contains

### 17.2 碰撞检测 (`engine/physics/collision.h`)

**Broadphase:**
- AABB 相交测试
- 碰撞对生成 (broadphasePairs)

**Narrowphase:**
- Sphere vs Sphere
- Sphere vs Box
- Sphere vs Plane
- Box vs Box (SAT)
- Box vs Plane
- Capsule vs Sphere

**SAT 辅助:**
- projectOntoAxis
- axisOverlap

**GJK 支持函数:**
- supportSphere
- supportBox

### 17.3 碰撞响应

**冲量求解:**
- 相对速度计算
- 弹性系数 (restitution)
- 法向冲量
- 摩擦冲量 (Coulomb friction)

**位置校正:**
- 穿透深度校正
- Baumgarte stabilization

**迭代求解:**
- velocityIterations (默认 8)
- positionIterations (默认 3)

### 17.4 约束系统 (`engine/physics/constraints.h`)

| 约束类型 | 功能 |
|----------|------|
| DistanceConstraint | 固定距离 (绳索) |
| BallSocketConstraint | 球窝关节 (3DOF) |
| HingeConstraint | 铰链关节 (1DOF) + 限制 + 电机 |
| SliderConstraint | 滑块关节 (1DOF) + 限制 |
| FixedConstraint | 焊接约束 (0DOF) |
| SpringConstraint | 弹簧 (stiffness/damping) |

**ConstraintManager:**
- 约束创建/销毁
- 每帧求解
- 断裂检测 (breakForce)

### 17.5 编辑器 UI (`engine/ui/editor_ui.h`)

**PhysicsEditorState:**
- selectedBody, selectedConstraint
- 调试可视化选项
- 模拟控制 (pause/step/reset)

**drawPhysicsEditorPanel:**
- 模拟控制 (Pause/Resume/Step/Reset)
- 时间缩放
- 世界设置 (重力/迭代次数/休眠)
- 调试可视化开关
- 刚体创建 (类型/形状选择)
- 刚体列表 (选择/检查)
- 刚体属性编辑 (质量/速度/材质/阻尼)
- 碰撞器编辑 (大小/触发器)
- 约束创建和管理

**菜单集成:**
- Window → Rendering → Physics Editor

### 17.6 应用集成 (`LumaView.mm`)

- PhysicsEditorState 实例变量
- 每帧调用 PhysicsWorld::step()
- 每帧调用 ConstraintManager::solveConstraints()
- 绘制 drawPhysicsEditorPanel()

### 17.7 调试渲染 (`engine/physics/physics_debug.h`) ✅

**PhysicsDebugRenderer:**
- 碰撞器可视化 (Sphere/Box/Capsule/Plane)
- 颜色编码 (Static=灰, Dynamic=绿, Kinematic=蓝, Sleeping=紫, Trigger=黄)
- AABB 边界框显示
- 接触点和法向量显示
- 约束连接线和锚点显示
- 弹簧卷曲可视化
- 线性/角速度矢量显示

**DebugColors:**
- StaticCollider, DynamicCollider, KinematicCollider
- SleepingCollider, TriggerCollider
- AABB, ContactPoint, ContactNormal
- LinearVelocity, AngularVelocity
- ConstraintOK, ConstraintStressed, ConstraintBroken

### 17.8 精确射线检测 (`engine/physics/raycast.h`) ✅

**Ray 结构:**
- origin, direction (归一化)
- getPoint(t) 获取射线上点

**RaycastHit 结果:**
- hit, distance, point, normal
- body, collider 引用

**精确形状射线检测:**
| 形状 | 算法 |
|------|------|
| Sphere | 二次方程求解 |
| Box (OBB) | 本地空间 AABB slab 测试 |
| Capsule | 圆柱 + 两端半球组合 |
| Plane | 平面方程求解 |

**高级查询:**
- `raycast()` - 单射线最近命中
- `raycastAll()` - 返回所有命中
- `sphereCast()` - 球体扫掠
- `boxCast()` - 盒子扫掠

**便捷函数:**
- `physicsRaycast(origin, direction, maxDistance, layerMask)`
- `physicsRaycastAll(...)`
- `physicsSphereCast(...)`

**UI 集成:**
- 射线测试面板 (原点/方向/距离)
- 命中结果显示 (距离/点/法线/刚体ID)
- 射线可视化 (红=命中, 灰=未命中)
- 命中点标记和法向量显示

---

## Phase 18: 地形系统 ✅

### 18.1 地形核心 (`engine/terrain/terrain.h`)

**Heightmap:**
- 宽度/高度、数据存储
- `getHeight(x, y)`, `setHeight(x, y, h)`
- `sampleBilinear(u, v)` - 双线性插值
- `getNormal(x, y)` - 法线计算
- `normalize()` - 归一化

**Splatmap:**
- 4 层纹理权重 (MAX_LAYERS = 4)
- `getWeight/setWeight(layer, x, y)`
- `normalizeAt(x, y)` - 权重归一化

**TerrainLayer:**
- name, diffuseTexture, normalTexture
- tint, metallic, roughness, tileScale
- 高度混合 (minHeight, maxHeight, blendSharpness)
- 坡度混合 (minSlope, maxSlope)

**TerrainChunk:**
- 分块管理 (chunkX, chunkZ)
- LOD 级别控制
- `generateMesh()` - 从高度图生成网格
- TerrainMeshData (vertices, indices)

**Terrain:**
- TerrainSettings (分辨率、大小、高度缩放)
- `getHeightAt(worldX, worldZ)` - 世界坐标查询
- `getNormalAt(worldX, worldZ)`
- `updateLOD(cameraPos)` - LOD 更新
- `autoGenerateSplatmap()` - 自动材质分配

### 18.2 地形生成 (`engine/terrain/terrain_generator.h`)

**PerlinNoise:**
- 2D/3D Perlin 噪声实现
- fade/lerp/grad 辅助函数
- 512 元素置换表

**FractalNoiseSettings:**
- octaves, frequency, amplitude
- lacunarity, persistence
- exponent, ridged, ridgeOffset

**FractalNoise:**
- 多倍频叠加
- 脊状噪声支持
- 指数曲线映射

**HydraulicErosion:**
- 水滴侵蚀模拟
- ErosionSettings (iterations, lifetime, inertia 等)
- 沉积/侵蚀刷
- 高度梯度计算

**预设:**
| 预设 | 特点 |
|------|------|
| Flat | 平坦、低振幅 |
| Hills | 起伏丘陵 |
| Mountains | 高山、脊状噪声 |
| Islands | 岛屿、指数衰减 |
| Canyon | 峡谷、低指数 |

### 18.3 植被系统 (`engine/terrain/foliage.h`)

**FoliageInstance:**
- position, rotation, scale
- color (色彩变化)
- windPhase (风动画相位)

**FoliageLayerSettings:**
- density, densityVariation
- minScale, maxScale, rotation
- baseColor, colorVariation
- 高度/坡度/图层约束
- LOD 距离、剔除距离
- 风强度/频率
- 公告板/网格模式

**FoliagePatch:**
- 分块实例管理
- LOD 级别
- 可见性剔除

**FoliageLayer:**
- `generateInstances()` - 根据地形生成实例
- `updateLOD()` - 基于相机更新

**FoliageSystem:**
- 多图层管理
- `generateAll()`, `updateLOD()`

**预设:**
| 预设 | 密度 | 特点 |
|------|------|------|
| Grass | 20/m² | 基础草地 |
| TallGrass | 5/m² | 高草 |
| Flowers | 2/m² | 花朵、颜色变化大 |
| Rocks | 0.5/m² | 岩石、坡度区域 |
| Trees | 0.1/m² | 树木、大剔除距离 |

### 18.4 地形编辑器 UI

**TerrainEditorState:**
- noiseSettings, erosionSettings
- seed, presets
- brushMode/Radius/Strength
- selectedFoliageLayer

**drawTerrainEditorPanel:**
- 生成设置 (预设/噪声参数/侵蚀)
- 地形设置 (分辨率/大小/高度)
- 材质图层编辑
- 植被图层管理
- 笔刷工具 (抬升/降低/平滑/展平/绘制)

**菜单集成:**
- Window → Rendering → Terrain Editor

### 18.5 应用集成 (`LumaView.mm`)

- TerrainEditorState 实例变量
- 每帧调用 Terrain::updateLOD()
- 每帧调用 FoliageSystem::updateLOD()
- 绘制 drawTerrainEditorPanel()

---

## Phase 19: 音频系统 ✅

### 19.1 音频核心 (`engine/audio/audio.h`)

**AudioFormat:**
- Mono8, Mono16, Stereo8, Stereo16
- MonoFloat, StereoFloat

**AudioClip:**
- name, data, sampleRate, channels, bitsPerSample
- `getDuration()` - 获取时长
- `loadFromMemory()` - 从内存加载
- `generateSineWave()` - 生成正弦波测试音
- `generateWhiteNoise()` - 生成白噪声

**AudioSourceSettings:**
- 播放: volume, pitch, loop, playOnAwake, priority
- 3D: spatialize, minDistance, maxDistance
- 衰减: rolloff (Linear/Logarithmic/Custom), rolloffFactor
- 多普勒: dopplerLevel
- 空间: spread, reverbZoneMix

**AudioSource:**
- `setClip()`, `setPosition()`, `setVelocity()`
- `play()`, `pause()`, `unpause()`, `stop()`
- `getState()`, `getTime()`, `setTime()`
- computedVolume, computedPanL/R (3D 计算结果)

**AudioListener:**
- position, velocity
- forward, up (方向向量)
- `getRight()` - 计算右向量
- volume (监听器音量)

### 19.2 3D 空间音频

**距离衰减:**
- Linear: `1 - (d - minDist) / (maxDist - minDist)`
- Logarithmic: `minDist / (minDist + factor * (d - minDist))`

**立体声平移:**
- 计算声源相对监听器方向
- 与 right 向量点积得到左右平衡
- Constant power panning (等功率平移)

**多普勒效应:**
- `shift = (c + vL) / (c + vS)`
- c = 343 m/s (声速)
- 限制范围 0.5 - 2.0

### 19.3 音频混合器

**AudioMixerGroup:**
- name, volume, mute, solo
- parentIndex (层级关系)
- 效果: lowPassEnabled, lowPassCutoff, reverbEnabled, reverbMix

**AudioMixer:**
- 预设组: Master, Music, SFX, Ambient, UI
- `addGroup()`, `getGroup()`, `getGroupByName()`
- `getEffectiveVolume()` - 计算继承后的音量

### 19.4 音频系统 (AudioSystem)

**初始化:**
- `initialize(sampleRate, channels, bufferSize)`
- 默认: 44100Hz, 2 channels, 4096 samples

**资源管理:**
- `createClip()`, `getClip()`
- `createSource()`, `destroySource()`

**全局控制:**
- masterVolume, muted
- `pauseAll()`, `unpauseAll()`, `stopAll()`

**更新:**
- `update(dt)` - 更新所有 3D 计算
- `mixAudio(buffer, frameCount)` - 音频混合回调

**便捷函数:**
- `playOneShot(clip, position, volume)` - 一次性播放

### 19.5 音频编辑器 UI

**AudioEditorState:**
- selectedSourceIndex, selectedClipIndex, selectedMixerGroup
- testToneFrequency, testToneDuration
- showSourceGizmos, showListenerGizmo

**drawAudioEditorPanel:**
- 主控制 (Master Volume, Mute, Stop All)
- 监听器设置 (位置/方向/音量)
- 混合器面板 (分组/音量/Mute/Solo/效果)
- 音源列表 (创建/选择/删除)
- 音源详情 (播放控制/设置/3D 参数)
- 测试音生成 (正弦波/白噪声/位置播放)

**菜单集成:**
- Window → Rendering → Audio Editor

### 19.6 应用集成 (`LumaView.mm`)

- AudioEditorState 实例变量
- 每帧调用 AudioSystem::update(dt)
- 绘制 drawAudioEditorPanel()

---

## Phase 20: 全局光照系统 ✅

### 20.1 球谐函数 (`engine/renderer/gi/spherical_harmonics.h`)

**SHConstants:**
- L2 球谐函数常数 (9 系数)
- kC0-kC4: 归一化常数
- kA0-kA2: 辐照度转换常数
- kIrr*: 预计算辐照度系数

**SHCoefficients:**
- 9 个 RGB 系数存储
- `addSample(direction, radiance)` - 添加辐射样本
- `scale(s)`, `add(other)`, `lerp(a, b, t)`
- `evaluateIrradiance(normal)` - 计算法向辐照度
- `evaluateBasis(dir, basis)` - 计算基函数
- 静态构造: `fromDirectionalLight()`, `fromAmbient()`, `fromSkyGradient()`

**SHSampleGenerator:**
- `generateSamples(count)` - 球面斐波那契均匀分布
- 返回方向、基函数、立体角

**SHGPUData:**
- GPU 对齐数据格式 (4-float padding)
- `fromSHCoefficients()` - 转换为 GPU 格式

### 20.2 光照探针 (`engine/renderer/gi/light_probe.h`)

**LightProbe:**
- position, SHCoefficients
- `evaluateIrradiance(normal)` - 查询辐照度
- dirty/valid 状态标记

**LightProbeGroup:**
- 非规则探针组管理
- `addProbe()`, `removeProbe()`, `clear()`
- `findNearestProbes(position, maxCount)` - 最近探针查找
- 逆距离加权 (IDW) 插值
- `interpolateSH(position)` - SH 插值

**LightProbeGrid:**
- 规则 3D 网格探针存储
- `initialize(min, max, resX, resY, resZ)`
- `getProbe(x, y, z)`, `getCell(pos)`
- `sampleSH(position)` - 三线性插值

### 20.3 反射探针 (`engine/renderer/gi/reflection_probe.h`)

**ReflectionProbeSettings:**
- resolution, mipLevels, hdr
- nearClip, farClip, layerMask
- realtime/baked, refreshRate
- boxProjection, blendDistance

**ReflectionProbe:**
- name, position
- shape: Box/Sphere
- boxSize, boxOffset / sphereRadius
- influenceRadius, priority, intensity
- `containsPoint()`, `calculateBlendWeight()`
- `boxProjectReflection()` - 视差校正
- gpuCubemapHandle

**ReflectionProbeManager:**
- 预创建 Skybox 探针 (最低优先级)
- `createProbe()`, `removeProbe()`
- `findProbesForPoint(position, maxCount)` - 按优先级排序
- `getDirtyProbes()`, `markAllDirty()`

### 20.4 GI 系统 (`engine/renderer/gi/gi_system.h`)

**GISettings:**
- lightProbes/reflectionProbes 启用开关
- ambientSkyColor, ambientGroundColor, ambientIntensity
- bounces, raysPerSample, rayLength (烘焙)

**GISystem:**
- `initializeLightProbeGrid()` - 初始化探针网格
- `addLightProbeGroup()` - 添加探针组
- `sampleIndirectDiffuse(position, normal)` - 采样间接光
- `getAmbientSH()` - 获取环境 SH

**烘焙系统:**
- LightInfo (Directional/Point/Spot)
- RayTraceCallback - 自定义射线追踪
- `bakeLightProbe()` - 烘焙单个探针
- `bakeAllLightProbes()` - 烘焙网格
- `bakeAllLightProbeGroups()` - 烘焙所有组
- 半球余弦采样、多次弹射

**GPU 数据导出:**
- GPUProbeData 结构
- `exportGPUData()` - 导出所有探针 SH

### 20.5 GI 编辑器 UI

**GIEditorState:**
- gridMin/Max, gridResolution
- bakeProgress/Total
- selectedLightProbeGroup, selectedReflectionProbe
- visualization 选项

**drawGIEditorPanel:**
- GI 设置 (开关/强度/环境色)
- 光照探针网格 (初始化/分辨率)
- 光照探针组 (添加/管理)
- 反射探针列表 (创建/选择)
- 反射探针详情 (位置/形状/优先级/分辨率)
- 烘焙控制 (参数/进度/执行)
- 预览 (位置/法线/辐照度)
- 可视化选项

**菜单集成:**
- Window → Rendering → GI Editor

### 20.6 应用集成 (`LumaView.mm`)

- GIEditorState 实例变量
- 绘制 drawGIEditorPanel()

---

## Phase 21: 视频导出系统 ✅

### 21.1 核心结构 (`engine/video/video_export.h`)

**VideoFormat:**
- MP4_H264, MP4_H265, WebM_VP9, AVI_MJPEG
- GIF
- ImageSequence_PNG/JPG/TGA

**VideoQuality:**
- Low, Medium, High, Lossless

**FrameData:**
- pixels (RGBA/RGB), width, height, channels
- `convertToRGB()`, `flipVertical()`

**VideoExportSettings:**
- 输出: path, format, quality
- 分辨率: width, height, matchViewport
- 时间线: frameRate, startTime, endTime
- 编码: bitrate, keyframeInterval

### 21.2 编码器

**IVideoEncoder 接口:**
- `initialize()`, `encodeFrame()`, `finalize()`
- `getProgress()`, `getError()`

**ImageSequenceEncoder:**
- TGA 写入 (BGR/BGRA)
- PNG 占位 (降级到 TGA)

**FFmpegEncoder:**
- 管道到 FFmpeg 进程
- 自动构建命令行参数
- 支持多种编解码器

**GIFEncoder:**
- GIF89a 格式
- Netscape 循环扩展
- 简化 LZW (灰度)

### 21.3 录制管理器

**RecordingState:**
- Idle, Preparing, Recording, Paused, Finalizing, Complete, Error

**RecordingManager:**
- `startRecording()`, `stopRecording()`
- `pauseRecording()`, `resumeRecording()`
- `captureFrame()` - 渲染循环中调用
- 进度/完成回调
- 估算文件大小和剩余时间

### 21.4 编辑器 UI

**VideoExportState:**
- settings, formatIndex, qualityIndex
- resolutionPreset
- recordStartTime, avgFrameTime

**drawVideoExportPanel:**
- 输出设置 (格式/质量/路径)
- 分辨率 (预设/自定义)
- 时间线 (帧率/起止时间)
- 高级选项 (码率/关键帧)
- 录制控制 (开始/暂停/停止)
- 进度条和状态显示

---

## Phase 22: 网络系统 ✅

### 22.1 核心结构 (`engine/network/network.h`)

**NetworkRole:**
- None, Client, Server, Host

**NetworkMessageType:**
- Connect/Disconnect/Heartbeat
- StateUpdate/StateFull/StateRequest
- RPC/RPCResponse
- EntitySpawn/Destroy/Ownership
- ScriptRPC/ScriptStateSync

**NetworkMessage:**
- 写入: `writeByte/UInt16/UInt32/Float/String/Vec3/Bytes`
- 读取: `readByte/UInt16/UInt32/Float/String/Vec3/Bytes`
- `serialize()`, `deserialize()`

### 22.2 连接管理

**NetworkConnection:**
- id, state, address, port
- lastHeartbeat, roundTripTime
- bytesSent/Received, packets统计
- username, userId

**ConnectionState:**
- Disconnected, Connecting, Connected, Disconnecting

### 22.3 RPC 系统

**RPCDefinition:**
- name, id
- serverOnly, clientOnly, requiresOwnership
- Handler 回调

**NetworkPeer 基类:**
- `send()`, `broadcast()`
- `registerRPC()`, `callRPC()`
- 消息处理器注册
- 连接回调

### 22.4 服务器/客户端

**NetworkServer:**
- `start()`, `stop()`, `update()`
- `acceptConnection()`, `disconnectClient()`
- 心跳发送和超时检测

**NetworkClient:**
- `start()`, `stop()`, `update()`
- `isConnected()`, `getConnectionState()`
- 自动连接请求

**NetworkManager (单例):**
- `startServer()`, `startClient()`, `startHost()`
- `isServer()`, `isClient()`, `isHost()`
- 便捷 RPC 调用

---

## Phase 23: Lua 脚本系统 ✅

### 23.1 脚本值 (`engine/script/script_engine.h`)

**ScriptValueType:**
- Nil, Boolean, Number, String
- Table, Function, UserData
- Vec3, Quat

**ScriptValue:**
- 联合体存储各类型
- 类型检查: `isNil/Bool/Number/String/Table/Vec3/Quat`
- 网络序列化: `serialize()`, `deserialize()`

### 23.2 脚本属性和 RPC

**ScriptProperty:**
- name, value
- networked (网络同步)
- serverAuthority (服务器权威)
- dirty (已修改标记)

**ScriptRPCDef:**
- name
- serverOnly, clientOnly, ownerOnly
- luaFuncRef (Lua 函数引用)

### 23.3 脚本类和实例

**ScriptClass:**
- name, sourceFile, sourceCode
- properties[], rpcs[]
- Lua 回调引用: onStart, onUpdate, onDestroy
- 网络回调: onNetworkSpawn, onNetworkDespawn

**ScriptInstance:**
- scriptClass, entityId
- networkId, ownerConnection
- propertyValues
- instanceRef (Lua 表引用)
- `hasAuthority()` - 权威性检查

### 23.4 脚本引擎

**ScriptEngine:**
- `initialize()`, `shutdown()`
- `loadScript()`, `loadScriptString()`
- `registerClass()`, `getClass()`
- `createInstance()`, `destroyInstance()`
- `update(dt)` - 调用所有实例的 onUpdate

**函数调用:**
- `callFunction(name, args, results)`
- `callMethod(instance, method, args, results)`

**网络集成:**
- `setNetworkEnabled()`
- `callRPC()` - 发送网络 RPC
- `handleNetworkRPC()` - 处理传入 RPC
- `syncNetworkedProperties()` - 同步属性

**API 绑定:**
- `bindVec3()`, `bindQuat()`
- `bindInput()`, `bindEntity()`
- `bindNetwork()`, `bindDebug()`

### 23.5 网络+脚本整合设计

```lua
-- 示例脚本
PlayerController = {
    -- 网络同步属性
    networked = {
        health = { default = 100, authority = "server" },
        position = { default = Vec3(0,0,0), authority = "owner" }
    },
    
    -- 服务端 RPC
    ServerRPC = {
        takeDamage = function(self, amount)
            self.health = self.health - amount
        end
    },
    
    -- 客户端 RPC
    ClientRPC = {
        playEffect = function(self, effectName)
            -- 播放特效
        end
    },
    
    onUpdate = function(self, dt)
        if Network.hasAuthority(self) then
            -- 只有权威者更新
        end
    end
}
```

### 23.6 编辑器 UI

**NetworkPanelState:**
- serverAddress, serverPort
- selectedConnection, showStats

**drawNetworkPanel:**
- 角色状态显示
- 连接设置 (地址/端口)
- 启动按钮 (Server/Host/Client)
- 服务器: 客户端列表、踢人
- 客户端: 连接状态、RTT

**ScriptEditorState:**
- selectedClass/Instance
- newClassName, codeBuffer
- consoleLog, consoleInput

**drawScriptEditorPanel:**
- 脚本类列表
- 属性/RPC 编辑
- 网络集成开关
- Lua 控制台
- API 参考

### 23.7 应用集成 (`LumaView.mm`)

- NetworkPanelState, ScriptEditorState 实例
- 每帧: NetworkManager::update(), ScriptEngine::update()
- 绘制 Network 和 Script 面板

---

## Phase 24: AI/寻路系统 ✅

### 24.1 NavMesh (`engine/ai/navmesh.h`)

**NavPoly:**
- indices[] (最多6顶点)
- neighbors[] (相邻多边形)
- center, normal, area
- flags, areaType

**NavEdge:**
- polyA, polyB
- start, end, width

**NavMeshBuildSettings:**
- Agent: height, radius, maxClimb, maxSlope
- Voxelization: cellSize, cellHeight
- Region: minArea, mergeArea
- Polygon: maxEdgeLen, maxSimplificationError

**NavMesh:**
- `build(vertices, indices)` - 从几何体构建
- `buildFromHeightmap()` - 从高度图构建
- `addPolygon()`, `connectPolygons()`
- `findNearestPoly()` - 找最近多边形
- `getClosestPointOnPoly()` - 多边形上最近点
- `isPointInPoly()` - 点在多边形内测试
- `raycast()` - 射线检测

### 24.2 A* 寻路

**NavNode:**
- polyIndex, position
- gCost (起点代价), hCost (启发式)
- parentIndex, edgeIndex

**NavPath:**
- points[] (PathPoint)
- totalLength
- `getPositionAtDistance()` - 按距离获取位置

**NavPathfinder:**
- `findPath(start, end, outPath)` - A* 搜索
- `findPath(..., areaCosts)` - 带区域代价
- `smoothPath()` - 路径平滑 (视线检测)
- 启发式: 欧几里得距离
- 支持自定义迭代上限和权重

### 24.3 NavAgent (`engine/ai/nav_agent.h`)

**NavAgentState:**
- Idle, Moving, Waiting, Stuck, Arrived

**NavAgentSettings:**
- speed, acceleration, angularSpeed
- stoppingDistance, radius, height
- avoidObstacles, avoidancePriority
- pathUpdateInterval, autoRepath

**NavAgent:**
- `setDestination()` - 设置目标
- `stop()`, `resume()` - 控制
- `update(dt, navMesh)` - 每帧更新
- `move(direction, dt)` - 手动移动
- `getRemainingDistance()` - 剩余距离
- 回调: onPathComplete

**NavAgentManager:**
- `createAgent()`, `destroyAgent()`
- `update(dt, navMesh)` - 更新所有Agent
- `getAgentById()`, `getAgentCount()`

### 24.4 行为树 (`engine/ai/behavior_tree.h`)

**BTStatus:**
- Invalid, Success, Failure, Running

**复合节点:**
| 类型 | 说明 |
|------|------|
| Sequence | 顺序执行，失败即停 |
| Selector | 选择执行，成功即停 |
| Parallel | 并行执行 |
| RandomSelector | 随机选择子节点 |

**装饰节点:**
| 类型 | 说明 |
|------|------|
| Inverter | 反转结果 |
| Succeeder | 总是成功 |
| Repeater | 重复N次/-1无限 |
| Limiter | 限制执行次数 |

**叶节点:**
| 类型 | 说明 |
|------|------|
| Action | 自定义函数 |
| Condition | 条件检查 |
| Wait | 等待时间 |
| Log | 调试日志 |

**Blackboard:**
- `set<T>(key, value)`, `get<T>(key)`
- `has(key)`, `remove(key)`

**BTBuilder (Fluent API):**
```cpp
BTBuilder()
    .selector()
        .sequence("Attack")
            .condition(inRange("target", 2.0f))
            .action(attackTarget)
        .end()
        .sequence("Chase")
            .condition(hasTarget)
            .action(moveTo("target"))
        .end()
        .action(patrol)
    .end()
.build();
```

**预定义动作 (BTActions):**
- `moveTo(targetKey)` - 移动到目标
- `inRange(targetKey, range)` - 距离检查
- `checkBool(key, expected)` - 黑板值检查
- `setValue(key, value)` - 设置黑板值

### 24.5 AI 编辑器 UI

**AIEditorState:**
- navMeshSettings, navMeshBuilt
- selectedAgent, agentTestDestination
- pathStart/End, testPath

**drawAIEditorPanel:**
- NavMesh 设置和构建
- Agent 列表和详情
- 路径测试 (起点/终点/可视化)
- 行为树参考

**菜单集成:**
- Window → Rendering → AI Editor

### 24.6 应用集成 (`LumaView.mm`)

- AIEditorState 实例
- 每帧: NavAgentManager::update(dt, navMesh)
- 绘制 drawAIEditorPanel()

---

## Phase 25: 游戏UI系统 ✅

### 25.1 UI Core (`engine/game_ui/ui_core.h`)

**基础类型:**
- UIRect (x, y, width, height, contains, intersects)
- UIColor (r, g, b, a, 预定义颜色)
- UIMargin (left, right, top, bottom)
- UIPivot (x, y 归一化)
- UIAnchor (9点锚定 + Stretch)

**UIEvent:**
- PointerDown/Up/Move/Enter/Exit
- Click, DoubleClick
- DragStart/Drag/DragEnd
- Scroll, KeyDown/Up, TextInput
- Focus/Blur

**UIWidget:**
- 位置/大小/锚点/枢轴
- visible/enabled/interactive
- 层级 (addChild/removeChild/parent)
- 事件监听 (addEventListener, onClick, onHover)
- hitTest/hitTestRecursive
- updateLayout() 计算 worldRect

**UICanvas:**
- 根容器, screenSize
- handleEvent() 分发事件
- 焦点/悬停/按下状态管理

### 25.2 UI Widgets (`engine/game_ui/ui_widgets.h`)

| 控件 | 特性 |
|------|------|
| **UIPanel** | 背景色、边框、圆角 |
| **UILabel** | 文本、字体、对齐、阴影 |
| **UIImage** | 纹理、UV、9-slice、填充模式 |
| **UIButton** | 文本、图标、状态颜色(Normal/Hover/Pressed/Disabled) |
| **UICheckbox** | checked状态、toggle() |
| **UISlider** | value/min/max、step、方向 |
| **UIProgressBar** | value、动画、显示百分比 |
| **UIInputField** | 文本输入、placeholder、密码模式、maxLength |
| **UIDropdown** | 选项列表、展开/收起 |
| **UIScrollView** | 滚动内容、惯性、滚动条 |
| **UIListView** | 数据列表、选择、itemCreator |

### 25.3 UI Layout (`engine/game_ui/ui_layout.h`)

| 布局 | 说明 |
|------|------|
| **UIHorizontalLayout** | 水平排列, childAlign |
| **UIVerticalLayout** | 垂直排列, childAlign |
| **UIGridLayout** | 网格, columns, cellSize |
| **UIStackLayout** | 堆叠(重叠) |
| **UIFlowLayout** | 流式(自动换行) |
| **UIAnchorLayout** | 锚点布局 |

**通用属性:**
- padding, spacing
- childAlign (Start/Center/End/Stretch)
- fitContent (自动尺寸)

### 25.4 UI System (`engine/game_ui/ui_system.h`)

**UIRenderCommand:**
- Rect, RoundedRect, Text, Image, Line, Clip

**IUIRenderer:**
- drawRect, drawRoundedRect, drawText, drawImage
- pushClip/popClip

**UIWidgetDrawer:**
- draw(widget) 递归渲染所有控件

**UISystem:**
- createCanvas/getCanvas/removeCanvas
- update(dt) 更新所有画布
- handleEvent() 事件分发
- render(renderer) 渲染所有画布

**UIFactory:**
```cpp
auto panel = UIFactory::createPanel("MyPanel");
auto button = UIFactory::createButton("Click Me");
auto slider = UIFactory::createSlider();
auto vbox = UIFactory::createVBox();
```

### 25.5 Game UI 编辑器

**GameUIEditorState:**
- selectedCanvas, selectedWidgetId
- widgetTypeToCreate
- showPreview, previewScale

**drawGameUIEditorPanel:**
- Canvas 管理 (创建/删除/可见性)
- Widget 层级树
- Widget 创建 (所有类型)
- Widget 属性编辑 (位置/大小/锚点/颜色)
- 类型特定属性 (Label文本, Button颜色, Slider范围等)

**菜单入口:** Window → Rendering → Game UI Editor

### 25.6 应用集成 (`LumaView.mm`)

- GameUIEditorState 实例
- 每帧: UISystem::update(dt)
- 绘制 drawGameUIEditorPanel()

---

## Phase 26: 场景管理系统 ✅

### 26.1 场景管理 (`engine/scene/scene_manager.h`)

**SceneState:**
- Unloaded, Loading, Loaded, Active, Unloading

**SceneLoadMode:**
- Single (卸载其他场景)
- Additive (保留现有场景)

**SceneObject:**
- id, name, prefabPath
- position, rotation, scale
- active, parentId
- componentData (序列化数据)

**SceneData:**
- 对象列表, 环境设置 (ambient, skybox)
- 光照 (DirectionalLight)
- NavMesh引用, 依赖资源

**SceneManager:**
- `loadScene(path, mode)` - 同步加载
- `loadSceneAsync(path, mode, callbacks)` - 异步加载
- `preloadScene(path)` - 预加载
- `activatePreloadedScene(path)` - 激活预加载场景
- `createScene(name)` - 创建新场景
- `saveScene(scene, path)` - 保存场景
- `unloadScene()`, `unloadAllScenes()`

**SceneTransition:**
- 类型: None, Fade, Crossfade, SlideLeft/Right/Up/Down
- duration, color, progress
- `getFadeOpacity()` - 渐变不透明度

**SceneTransitionManager:**
- `transitionTo(path, type, duration)` - 带过渡切换场景
- 自动预加载 + 中点切换

---

## Phase 27: 数据驱动系统 ✅

### 27.1 配置表 (`engine/data/data_system.h`)

**ConfigValue:**
- 支持: bool, int64_t, double, string, string[], double[]

**ConfigTable:**
- `setBool/Int/Float/String()` - 设置值
- `getBool/Int/Float/String()` - 获取值 (带默认值)
- `parseFromString()` - 从 key=value 格式解析
- `serializeToString()` - 序列化

### 27.2 本地化

**Localization:**
- `setLanguage(code)` - 切换语言
- `loadStrings(language, strings)` - 加载字符串
- `get(key)` - 获取本地化字符串
- `format(key, args...)` - 格式化字符串
- 自动回退到英语

### 27.3 文件监控

**FileWatcher:**
- `addWatch(path, callback)` - 添加监控
- `update()` - 检查文件变化
- 基于修改时间检测

### 27.4 数据管理器

**DataManager:**
- `loadConfig(name)` - 加载配置表
- `reloadConfig(name)` - 重载配置
- `addConfigListener()` - 配置变化监听
- `loadLanguage(code)` - 加载语言文件
- `localize(key)` / `localizeFormat(key, args)`
- 热重载支持

**便捷宏:**
```cpp
LOC("key")                    // 获取本地化字符串
LOC_FMT("key", arg1, arg2)    // 格式化
CONFIG("name")->getInt("key") // 获取配置
```

---

## Phase 28: 打包/发布系统 ✅

### 28.1 资源打包 (`engine/build/build_system.h`)

**AssetBundleEntry:**
- sourcePath, bundlePath
- offset, size, originalSize
- crc32, compressed

**AssetBundler:**
- `addAsset(source, bundlePath)` - 添加资源
- `addDirectory(dir, filter)` - 添加目录
- `build(outputPath, compress)` - 构建包
- `buildManifest()` - 生成清单

### 28.2 构建配置

**BuildPlatform:**
- Windows, macOS, iOS, Android, Linux, WebGL

**BuildConfig:**
- Debug, Development, Release

**BuildSettings:**
- projectName, version, buildNumber
- platform, config
- outputDir, assetsDir
- compressAssets, stripDebugInfo
- bundleIdentifier (iOS/macOS/Android)
- teamId (iOS), keystorePath (Android)

### 28.3 构建管线

**BuildPipeline:**
- 可配置步骤列表
- 默认步骤: Validate → Clean → CreateDirectories → CopyAssets → BundleAssets → WriteMetadata
- `addStep()`, `insertStepBefore()`, `removeStep()`
- 进度回调支持

**BuildResult:**
- success, outputPath, errorMessage
- warnings, buildTimeMs, totalSize
- stepResults (每步结果)

**BuildManager:**
- `build(progressCallback)` - 执行构建
- `buildForPlatform(platform)` - 指定平台构建
- 预设: `useDebugPreset()`, `useDevelopmentPreset()`, `useReleasePreset()`

### 28.4 编辑器面板

**Scene Manager Panel:**
- 当前场景信息
- 已加载场景列表
- 加载/预加载/卸载操作
- 场景过渡设置

**Data Manager Panel:**
- 配置表加载/编辑/保存
- 本地化语言切换
- 热重载状态

**Build Settings Panel:**
- 项目信息编辑
- 平台/配置选择
- 路径设置
- 构建选项
- 一键构建 + 进度显示
- 构建结果显示

**菜单入口:** Window → Rendering → Scene Manager / Data Manager / Build Settings

---

## 📝 旧计划（已整合到路线图）

<details>
<summary>点击展开旧计划</summary>

#### 材质编辑器 UI
- [ ] **实时材质编辑**
  - Metallic、Roughness、Base Color 滑块
  - 纹理替换（拖拽或文件选择）
  - 实时预览更新

- [ ] **材质管理**
  - 材质预设保存/加载
  - 材质库（Material Library）
  - 材质实例化

#### 环境贴图支持
- [ ] .hdr 文件加载
  - 环境贴图切换
  - 环境强度调节

#### 5. 后处理效果
- [ ] **基础后处理**
  - Bloom（泛光效果）
  - 色调映射优化（多种算法可选）
  - 颜色分级（Color Grading）

- [ ] **高级后处理**
  - 屏幕空间环境光遮蔽（SSAO）
  - 抗锯齿（MSAA / TAA）
  - 景深（Depth of Field，可选）

### 优先级 3：动画与交互

#### 6. 动画支持
- [ ] **骨骼动画**
  - 骨骼层次结构解析
  - GPU Skinning（顶点着色器蒙皮）
  - 动画剪辑播放
  - 动画混合（Blending）

- [ ] **关键帧动画**
  - Transform 关键帧（位置、旋转、缩放）
  - 曲线编辑器（Bezier 曲线）
  - 动画时间轴

- [ ] **Timeline 编辑器**
  - 时间轴 UI（参考 GOLDEN_PATH）
  - 多轨道支持（Transform、Camera、Material、Animation）
  - 播放控制（播放/暂停/停止）
  - 关键帧编辑

#### 7. 交互系统
- [ ] **Action 系统**
  - Action 定义（ApplyLook、SetParameter、SwitchCamera 等）
  - Action 派发机制
  - Action 记录/回放

- [ ] **状态机**
  - 状态定义（State）
  - 状态转换（Transition）
  - 事件触发（Event Bus）

### 优先级 4：性能与优化

#### 8. 性能优化
- [ ] **渲染优化**
  - 视锥剔除（Frustum Culling）
  - 遮挡剔除（Occlusion Culling，可选）
  - LOD 系统（Level of Detail）
  - 批处理优化（Instancing）

- [ ] **资源优化**
  - 纹理压缩（BC1/BC3/BC7）
  - 纹理流式加载
  - 模型缓存

- [ ] **性能分析**
  - GPU 性能计数器
  - 帧时间分析
  - 性能面板（显示 Draw Calls、Triangles、Memory 等）

#### 9. 工具与工作流
- [ ] **打包工具增强**
  - 确定性构建（Deterministic Build）
  - Asset Registry（资产注册表）
  - 依赖分析

- [ ] **导出功能**
  - 场景导出（JSON/Binary）
  - 预览图生成
  - 移动端包生成（iOS/Android）

### 优先级 5：平台扩展

#### 10. 多平台支持
- [ ] **移动端运行时**
  - iOS Runtime（Metal 后端）
  - Android Runtime（Vulkan 后端）
  - 移动端 Look Fallback（简化渲染管线）

- [x] **跨平台渲染后端**
  - [ ] Vulkan 后端
  - [x] Metal 后端（macOS/iOS）
  - [x] 渲染抽象层（RHI）

#### 11. Look System
- [ ] **光照配置**
  - IBL/HDR 环境配置
  - 主光源设置
  - 光照预设

- [ ] **后处理配置**
  - 曝光、色调调节
  - 后处理链（Bloom、Tone Mapping 等）
  - Look 预设保存/加载

- [ ] **热切换**
  - 运行时切换 Look
  - Look 序列化

---

## 📝 开发笔记

### 技术决策
- **Pimpl 模式**：`UnifiedRenderer` 使用 Pimpl 隐藏平台实现细节
- **跨平台架构**：DX12 (Windows) 和 Metal (macOS) 共用相同接口
- **模块化设计**：将 UI、相机、视口分离，提高代码可维护性
- **动态裁剪平面**：根据相机距离自动调整，支持大场景缩放
- **异步加载**：纹理解码在后台线程，不阻塞主线程

### 已知问题
- 某些 FBX 模型的嵌入纹理路径可能不正确（已通过文件名匹配解决）

### 已完成优化
- [x] 常量缓冲区环形缓冲（避免每帧 Map/Unmap）
- [x] 异步纹理加载（后台线程解码）
- [x] 持久化映射（DX12）

---

## 📅 更新记录

- **2026-01-13**：**RHI 重构 & 代码统一 (阶段 3)**
  - 设计跨平台 RHI 抽象接口 (`engine/renderer/rhi/`)
    - `rhi_types.h` - 平台无关的渲染类型定义
    - `rhi_resources.h` - Buffer、Texture、Sampler、Shader、Pipeline 抽象
    - `rhi_device.h` - Device 和 CommandBuffer 抽象接口
    - `rhi.h` - 统一入口和辅助函数
  - 实现 Metal RHI 后端 (`metal_rhi.mm`)
    - MetalBuffer、MetalTexture、MetalSampler
    - MetalShader、MetalPipeline
    - MetalSwapchain、MetalCommandBuffer
    - MetalDevice（设备管理、资源创建）
  - 实现 DX12 RHI 后端 (`dx12_rhi.cpp`)
    - DX12Buffer、DX12Texture、DX12Sampler
    - DX12Shader、DX12Pipeline
    - DX12Swapchain、DX12CommandBuffer
    - DX12Device（设备管理、Root Signature）
  - **创建 UnifiedRenderer 统一渲染器**
    - `unified_renderer.h` - 跨平台渲染器接口
    - `unified_renderer_metal.mm` - Metal 实现
    - `unified_renderer_dx12.cpp` - DX12 实现
    - 支持 PBR 渲染、网格渲染、坐标轴渲染
    - 统一的模型加载和资源管理
  - **迁移应用到 UnifiedRenderer**
    - macOS `LumaView.mm` 使用 `UnifiedRenderer`
    - Windows 应用支持 (待完整测试)
  - **清理旧代码**
    - 旧 `PBRRenderer` 代码已完全删除
    - 统一使用 `UnifiedRenderer`

- **2026-01-23**：**异步纹理加载 & 代码清理**
  - 新增异步纹理加载系统 (`engine/asset/async_texture_loader.h/cpp`)
    - 2 个工作线程后台解码纹理
    - 几何体即时显示，纹理逐渐加载
    - UI 显示加载进度条
  - 代码清理
    - 删除 `_deprecated/` 文件夹（旧 PBR 渲染器）
    - 删除 `apps/creator_win/` Qt 版代码
    - 更新 README.md 反映当前架构
  - Windows Creator 迁移到 `UnifiedRenderer`
    - `apps/creator_imgui/main.cpp` 使用统一渲染器

- **2026-01-12**：**Metal 渲染后端支持 (阶段 1 & 2)**
  - 新增 Metal 着色器 (`engine/renderer/shaders/pbr.metal`)
    - 完整 PBR 管线（Cook-Torrance BRDF、ACES 色调映射）
    - Line shader（用于网格和坐标轴）
  - 新增 Metal 渲染器实现 (`unified_renderer_metal.mm`)
    - 与 DX12 版本功能对等
    - 支持纹理加载（Diffuse、Normal、Specular/Metallic）
    - 支持网格和坐标轴渲染
  - 新增 macOS 应用 (`apps/creator_macos/`)
    - `LumaView` Metal 视图
    - 完整 ImGui 集成
      - Model 面板（模型信息、打开文件）
      - Camera 面板（自动旋转、手动控制）
      - Material 面板（Metallic、Roughness、Base Color）
      - Viewport 面板（网格开关）
      - 帮助覆盖层（F1）
      - 状态栏（FPS）
    - Maya 风格相机控制（Option + 鼠标）
  - 更新 CMakeLists.txt 支持 macOS 构建
    - 条件编译 DX12/Metal
    - ImGui Metal 后端集成
    - 自动链接 Metal 框架
    - 着色器复制到 app bundle

- **2024-12-XX**：初始开发日志创建
  - 记录已完成功能（PBR 渲染、模型加载、相机控制、UI 等）
  - 制定下一步开发计划

---

## 🚀 Phase 16: 产品完善计划

### 16.1 优先级规划

#### P0 - 必须做（能用）
| 任务 | 状态 | 说明 |
|------|------|------|
| 整合测试 | ⏳ 待做 | 确保所有系统协同工作 |
| 基础示例项目 | ✅ 已完成 | A-D 四个示例项目 |
| 快速入门文档 | ✅ 已完成 | GOLDEN_PATH.md |

#### P1 - 应该做（好用）
| 任务 | 状态 | 说明 |
|------|------|------|
| 项目模板 | ⏳ 待做 | 空白/2D/3D 模板 |
| 自动保存 & 崩溃恢复 | ⏳ 待做 | 防止用户丢失工作 |
| Undo/Redo 完善 | ✅ 已完成 | Command 系统 |
| 快捷键系统 | ⏳ 待做 | 提高工作效率 |
| 资产导入向导 | ⏳ 待做 | 简化模型/纹理导入 |

#### P2 - 可以做（易用）
| 任务 | 状态 | 说明 |
|------|------|------|
| 欢迎页面/最近项目 | ⏳ 待做 | 更好的首次体验 |
| 偏好设置面板 | ⏳ 待做 | 自定义编辑器 |
| Console/日志面板 | ⏳ 待做 | 调试信息可视化 |
| 资产浏览器 | 🔄 进行中 | 管理项目资源 |

#### P3 - 未来考虑
| 任务 | 状态 | 说明 |
|------|------|------|
| 插件/扩展系统 | ⏳ 待做 | 社区贡献 |
| 可视化脚本 | 🔄 进行中 | 非程序员友好 |
| 协作编辑 | ⏳ 待做 | 团队工作流 |

---

### 16.2 资产浏览器（Asset Browser）

设计目标：提供直观的项目资源管理界面

#### 功能设计
1. **目录树视图**
   - 显示项目文件夹结构
   - 支持文件夹展开/折叠
   - 拖拽组织文件

2. **资产网格视图**
   - 缩略图预览（模型、纹理、材质）
   - 列表/网格切换
   - 搜索过滤

3. **资产类型支持**
   - 模型：.fbx, .obj, .gltf
   - 纹理：.png, .jpg, .hdr
   - 材质：.mat
   - 场景：.luma
   - 脚本：.lua
   - 音频：.wav, .mp3

4. **操作功能**
   - 双击打开/预览
   - 右键菜单（重命名、删除、复制）
   - 拖拽到场景
   - 导入新资产

---

### 16.3 角色创建系统（Character Creation System）

**日期**: 2026-01-23

设计目标：实现"照片 → 可编辑 3D 角色 → 可穿戴 → 可动画"的完整角色生产流水线。

#### 已完成模块

1. **BlendShape 系统** (`engine/character/blend_shape.h`)
   - BlendShapeTarget: 存储顶点偏移数据（位置、法线、切线）
   - BlendShapeChannel: 权重控制，支持多目标混合
   - BlendShapeMesh: 统一管理所有 BlendShape 数据
   - BlendShapePreset: 预设系统（表情、体型）
   - CPU 混合计算支持
   - 稀疏存储优化（只存储变化的顶点）

2. **BlendShape GPU 计算** (`engine/renderer/shaders/blend_shape.metal`)
   - 稀疏 Delta 应用 Kernel
   - 稠密 Per-Vertex 应用 Kernel
   - 预索引优化 Kernel（最高效）
   - BlendShape + 骨骼蒙皮联合计算 Kernel
   - 从 Base Mesh 初始化 Kernel

3. **身体参数系统** (`engine/character/character_body.h`)
   - Gender: 男/女/中性
   - AgeGroup: 幼年/青少年/青年/成年/老年
   - BodyPreset: 15+ 预设体型
   - BodyMeasurements: 20+ 细节参数（身高、体重、肌肉、肩宽...）
   - BlendShape 映射系统（参数 → 权重）
   - 序列化/反序列化支持

4. **脸部参数系统** (`engine/character/character_face.h`)
   - FaceShapeParams: 50+ 脸部形状参数
   - FaceExpressionParams: ARKit 52 兼容表情参数
   - FaceTextureParams: 肤色、眼色、唇色、化妆等
   - PhotoFaceResult: AI 照片重建结果数据结构
   - 脸部预设库支持

5. **角色整合系统** (`engine/character/character.h`)
   - Character: 统一角色类（Face + Body + Clothing + Animation）
   - CharacterClothing: 服装管理（装备、分层、颜色定制）
   - CharacterAnimationState: 动画状态管理
   - CharacterFactory: 角色创建工厂
   - CharacterManager: 多角色管理
   - 标准 Humanoid 骨骼（30+ 骨骼）
   - 导出支持（glTF, FBX, VRM 预留）

6. **角色创建 UI** (`engine/character/character_creator_ui.h`)
   - ImGui 集成界面
   - 分 Tab 设计（Body/Face/Clothing/Animation/Export）
   - 身体子面板（Preset/Overall/Torso/Arms/Legs/Skin）
   - 脸部子面板（Shape/Eyes/Nose/Mouth/Texture/Expression）
   - 滑块 + 重置按钮
   - 颜色选择器 + 预设
   - 预设网格展示

#### 产品规划文档

详细产品规划参见: `docs/CHARACTER_CREATION_SPEC.md`

7. **基础人体模型加载** (`engine/character/base_human_loader.h`)
   - BaseHumanModel: 统一模型数据结构
   - BaseHumanLoader: 支持 OBJ 和 MakeHuman target 文件
   - ProceduralHumanGenerator: 程序化人体生成（用于测试）
   - BaseHumanModelLibrary: 模型库管理

8. **AI 推理引擎** (`engine/character/ai/ai_inference.h`)
   - Tensor: 多维张量数据结构
   - InferenceSession: ONNX Runtime 封装
   - AIModelManager: 模型管理器
   - ImagePreprocess: 图像预处理工具（resize, normalize, NCHW转换）

9. **照片→脸部 Pipeline** (`engine/character/ai/face_reconstruction.h`)
   - FaceLandmarks: 468点 MediaPipe 兼容
   - FaceDetector: 人脸检测
   - FaceMeshEstimator: 人脸网格估计
   - FLAME3DMMParams: FLAME 模型参数
   - Face3DMMRegressor: 3DMM 参数回归
   - FaceParameterMapper: 3DMM→LUMA 参数映射
   - FaceTextureExtractor: 脸部贴图提取
   - PhotoToFacePipeline: 完整照片到脸部流程

10. **角色渲染器** (`engine/character/character_renderer.h`)
    - CharacterGPUData: 管理基础网格和变形后网格
    - CharacterRenderer: 渲染集成类
    - 自动更新 BlendShape 并同步到 GPU

11. **角色创建演示** (`examples/07_character_creator.h`)
    - CharacterCreatorDemo: 完整演示应用
    - ImGui 界面集成
    - 实时预览和调整
    - Body/Face/BlendShape/Export 标签页

12. **使用文档** (`docs/tutorials/07_character_creation.md`)
    - 完整的使用教程
    - API 参考
    - 代码示例
    - 常见问题解答

13. **编辑器 UI 集成** (`engine/ui/editor_ui.h`, `apps/luma_studio_macos/LumaView.mm`)
    - CharacterCreatorState: 角色创建 UI 状态
    - drawCharacterCreatorPanel: ImGui 角色创建面板
    - 菜单入口: Window → Tools → Character Creator
    - 回调系统: 随机化、预设、参数变化、照片导入、导出

14. **角色实时渲染预览**
    - initializeCharacter: 初始化角色和网格
    - updateAndRenderCharacter: 每帧更新和渲染角色
    - 参数变化自动同步到 BlendShape
    - 角色显示在场景右侧

15. **AI 模型管理** (`engine/character/ai/ai_model_manager.h`)
    - AIModelInfo: 模型信息结构
    - ModelRegistry: 已知模型注册表
    - AIModelManager: 管理模型下载、验证、加载
    - AI Model Setup 弹窗用于导入 ONNX 模型

16. **角色导出** (`engine/character/character_exporter.h`)
    - OBJExporter: Wavefront OBJ 格式导出
    - GLTFExporter: glTF 2.0 (.glb) 格式导出
    - CharacterExporter: 统一导出接口
    - 支持导出选项: 骨骼、BlendShape、缩放

17. **服装系统** (`engine/character/clothing_system.h`)
    - ClothingAsset: 服装资产定义
    - ClothingManager: 穿戴管理
    - ProceduralClothingGenerator: 程序化生成服装
    - 自动适配不同体型
    - UI 集成: Clothing 标签页

18. **动画重定向** (`engine/character/animation_retargeting.h`)
    - HumanoidBoneMapping: 标准人形骨骼映射
    - AnimationPose: 姿势数据
    - PoseLibrary: 姿势库
    - RetargetableClip: 可重定向动画片段
    - AnimationRetargeter: 重定向器
    - UI 集成: Animation 标签页

19. **风格化渲染** (`engine/character/stylized_rendering.h`)
    - CelShadingParams: 卡通着色参数
    - OutlineParams: 轮廓线参数
    - AnimeFeatures: 动漫特效
    - StylizedRenderingSettings: 完整设置
    - StylizedRenderingManager: 管理器
    - 支持: 写实、动漫、卡通、绘画、素描风格
    - UI 集成: Style 标签页

20. **MakeHuman 集成** (`engine/character/makehuman_integration.h`)
    - MakeHumanTargetLoader: 加载 .target 文件
    - MakeHumanSkeletonMapping: 骨骼映射
    - MakeHumanLoader: 模型加载器
    - MakeHumanAssetManager: 资产管理
    - 支持加载 MakeHuman 导出的模型和变形目标

#### 下一步计划

1. **实际模型部署**
   - 下载并部署 ONNX 格式的 AI 模型
   - 测试端到端照片处理流程

2. **服装系统**
   - 服装数据格式定义
   - 服装自动适配不同体型
   - 多层服装系统

3. **高级功能**
   - 动画重定向
   - 实时面部捕捉
   - 风格化渲染 (动漫/卡通)

---

### 21. 角色贴图系统（Character Texture System）

**文件**: `engine/character/texture_system.h`

#### 核心功能

1. **皮肤贴图生成**
   - Diffuse/Albedo 贴图
   - Normal Map（皮肤细节、毛孔、皱纹）
   - Roughness Map（粗糙度变化）
   - 多种族预设（Caucasian、Asian、African、Latino、Middle Eastern）

2. **眼睛贴图**
   - 虹膜纹理（放射状纤维图案）
   - 瞳孔大小可调
   - 眼白（巩膜）血丝效果
   - 多种眼睛颜色预设

3. **嘴唇贴图**
   - 颜色、光泽度控制
   - 龟裂效果

4. **参数系统**
   - SkinTextureParams: 皮肤色调、饱和度、毛孔、雀斑、皱纹、SSS
   - EyeTextureParams: 虹膜颜色/大小、瞳孔大小、细节、湿润度
   - LipTextureParams: 颜色、光泽、龟裂

5. **UI 集成**
   - 新增 "Texture" 标签页
   - 皮肤/眼睛/嘴唇预设选择
   - 实时参数调节
   - 分辨率选择（512/1024/2048）
   - 一键生成贴图

---

### 22. UV 映射优化（UV Mapping System）

**文件**: `engine/character/uv_mapping.h`

#### 核心功能

1. **UV 投影方法**
   - 圆柱形投影（身体）
   - 盒式投影（头部）
   - 球形投影（面部）
   - 平面投影（正面）

2. **切线计算**
   - 自动从 UV 计算切线向量
   - Gram-Schmidt 正交化
   - 支持法线贴图

3. **UV 修复**
   - 自动检测和修复 UV 接缝
   - 区域重映射

4. **布局预设**
   - Cylindrical: 简单圆柱投影
   - Atlas: 优化的分区布局

---

### 23. 发型库系统（Hair Style System）

**文件**: `engine/character/hair_system.h`

#### 核心功能

1. **发型资产**
   - HairStyleAsset: 发型定义
   - HairCategory: 短发/中发/长发/盘发/光头
   - 支持外部模型导入

2. **发型材质**
   - HairMaterial: 基色、高光、粗糙度、各向异性
   - 13种颜色预设（黑/棕/金/红/灰/白 + 幻想色）
   - 自定义颜色支持

3. **程序化发型生成**
   - generateBuzzCut(): 平头
   - generateMediumHair(): 中长发（发片法）
   - generatePonytail(): 马尾

4. **发型管理器**
   - HairManager: 角色发型管理
   - HairStyleLibrary: 发型库（单例）
   - 动态换发型/换发色

5. **UI 集成**
   - 新增 "Hair" 标签页
   - 发型下拉选择
   - 颜色预设 + 自定义颜色
   - 实时预览

---

### 24. 服装加载器（Clothing Loader）

**文件**: `engine/character/clothing_loader.h`

#### 核心功能

1. **模型格式支持**
   - OBJ, glTF/GLB, FBX, DAE, 3DS
   - 利用 Assimp 统一加载

2. **元数据系统**
   - `.meta` 边车文件支持
   - 简单 key=value 格式
   - 自动分类和槽位识别

3. **自动适配形状生成**
   - 基于网格分析自动生成 BlendShape
   - 身高/体重/胸围/臀围适配

4. **批量加载**
   - `loadDirectory()` 批量导入目录
   - `loadAndRegister()` 加载并注册到库

---

### 25. 布料物理系统（Cloth Simulation）

**文件**: `engine/character/cloth_simulation.h`

#### 核心功能

1. **质点-弹簧模型**
   - ClothParticle: 质点（位置/速度/质量）
   - ClothSpring: 弹簧约束（结构/剪切/弯曲）
   - Verlet 积分

2. **物理参数**
   - 重力、阻尼、空气阻力
   - 弹簧刚度（结构/剪切/弯曲分离）
   - 最大拉伸限制

3. **碰撞检测**
   - CollisionSphere: 球体碰撞
   - CollisionCapsule: 胶囊体碰撞
   - 碰撞摩擦

4. **身体碰撞生成器**
   - BodyCollisionGenerator
   - 自动从身体参数生成碰撞体

---

### 26. 服装贴图系统（Clothing Textures）

**文件**: `engine/character/clothing_textures.h`

#### 核心功能

1. **布料类型**
   - Cotton, Denim, Silk, Leather, Wool
   - Polyester, Velvet, Linen, Satin, Canvas

2. **程序化纹理生成**
   - generateDiffuse(): 基础颜色纹理
   - generateNormal(): 法线贴图
   - generateRoughness(): 粗糙度贴图

3. **布料特征**
   - 棉布编织纹理
   - 牛仔斜纹
   - 丝绸光泽
   - 皮革纹理
   - 羊毛纤维

4. **材质集管理**
   - ClothingMaterialSet: 完整材质包
   - ClothingTextureManager: 缓存和管理

---

### 27. 服装骨骼蒙皮（Clothing Skinning）

**文件**: `engine/character/clothing_skinning.h`

#### 核心功能

1. **自动权重生成**
   - 基于距离的权重计算
   - 热扩散权重平滑
   - 最多4骨骼影响

2. **蒙皮数据结构**
   - VertexSkinData: 顶点权重
   - ClothingSkinData: 整件服装蒙皮
   - 逆绑定矩阵

3. **蒙皮变形器**
   - ClothingSkinningDeformer
   - 实时骨骼矩阵变形
   - 法线同步变换

4. **管理器**
   - ClothingSkinningManager: 缓存蒙皮数据
   - 按需生成或加载

---

### 28. 角色模板系统（Character Template System）

**文件**: `engine/character/character_template.h`

#### 核心功能

1. **模板接口**
   - ICharacterTemplate: 抽象接口
   - CharacterType: Human/Anime/Cartoon/Mascot/Animal/Robot/Fantasy/Chibi
   - CharacterParams: 统一参数结构
   - BodyProportions: 身体比例预设

2. **模板注册表**
   - CharacterTemplateRegistry: 单例管理
   - 按类型注册/获取模板
   - createCharacter(): 统一创建接口

3. **基类实现**
   - BaseCharacterTemplate: 通用功能
   - 自动边界计算
   - 参数验证

---

### 29. 身体部件系统（Body Part System）

**文件**: `engine/character/body_part_system.h`

#### 核心功能

1. **部件类型**
   - BodyPartType: Head/Torso/Arms/Legs/Hands/Feet
   - 面部特征: Eyes/Nose/Mouth/Ears
   - 额外部件: Tail/Wings/Antenna
   - 配饰: Hat/Bow/Collar

2. **形状生成**
   - PartShape: Sphere/Ellipsoid/Capsule/Cylinder/Cone/Box/Custom
   - ProceduralPartGenerator: 程序化生成各种形状
   - 自动 UV/法线/切线

3. **部件组装**
   - BodyPartAssembly: 组装管理
   - 父子关系/附着点
   - combineMesh(): 合并为单一网格
   - createSkeleton(): 自动生成骨骼

---

### 30. 卡通特征系统（Cartoon Features）

**文件**: `engine/character/cartoon_features.h`

#### 核心功能

1. **眼睛样式**
   - CartoonEyeStyle: Circle/Oval/Dot/Button/Anime/Disney/Pie
   - 虹膜、瞳孔、高光、轮廓
   - 眼睑（用于表情）

2. **耳朵样式**
   - CartoonEarStyle: MouseRound/CatPointed/BunnyLong/BearRound/DogFloppy
   - 内外颜色、尖度、弯曲度

3. **鼻子样式**
   - CartoonNoseStyle: Dot/Triangle/Button/Animal/Beak/Snout
   - 大小、颜色、光泽

4. **嘴巴样式**
   - CartoonMouthStyle: None/Line/Smile/Open/Cat
   - Hello Kitty 无嘴支持！

5. **配饰**
   - Bow（蝴蝶结）、Collar（项圈）
   - 可自定义颜色和位置

---

### 31. 角色模板实现（Character Templates）

**文件**: `engine/character/character_templates.h`

#### 实现的模板

1. **HumanTemplate**
   - 写实人类角色
   - 7.5头身比例
   - 完整骨骼（21+骨骼）
   - 复用现有 ProceduralHumanGenerator

2. **MascotTemplate (Hello Kitty 风格)**
   - 大头小身（1.5头身）
   - 猫耳、蝴蝶结
   - 无嘴设计！
   - 简单骨骼（耳朵可动）
   - 黄色鼻子、黑色点眼

3. **CartoonTemplate (米奇风格)**
   - 卡通比例（4头身）
   - 大圆耳朵
   - 黑身体 + 浅色脸
   - 红色短裤、黄色大鞋
   - 白色手套
   - 尾巴支持

#### 使用方式

```cpp
// 注册模板
luma::registerDefaultTemplates();

// 创建 Hello Kitty 风格角色
auto& registry = luma::getTemplateRegistry();
luma::CharacterParams params;
params.type = luma::CharacterType::Mascot;
params.primaryColor = luma::Vec3(1, 1, 1);  // 白色
params.accentColor = luma::Vec3(1, 0.3f, 0.4f);  // 粉色蝴蝶结

auto result = registry.createCharacter(luma::CharacterType::Mascot, params);
```

---

### 16.4 可视化脚本（Visual Scripting）

设计目标：让非程序员也能创建游戏逻辑

#### 核心概念
1. **节点类型**
   - 事件节点（OnStart, OnUpdate, OnCollision）
   - 逻辑节点（If, Loop, Switch）
   - 数学节点（Add, Multiply, Lerp）
   - 变量节点（Get, Set）
   - 动作节点（Move, Rotate, PlaySound）

2. **连接系统**
   - 执行流（白色线）
   - 数据流（彩色线按类型）
   - 自动类型转换

3. **与 Lua 集成**
   - 可视化脚本编译为 Lua
   - 支持调用自定义 Lua 函数
   - 双向调试

---

### 32. 插件系统（Plugin System）

**文件**:
- `engine/plugin/plugin_system.h` - 插件接口定义
- `engine/plugin/plugin_manager.h` - 插件加载和管理
- `engine/plugin/plugin_examples.h` - 示例插件
- `docs/PLUGIN_DEVELOPMENT.md` - 插件开发指南

#### 支持的插件类型

| 类型 | 说明 | 接口 |
|------|------|------|
| CharacterTemplate | 角色模板 | `ICharacterTemplatePlugin` |
| Clothing | 服装 | `IClothingPlugin` |
| Hair | 发型 | `IHairPlugin` |
| Accessory | 配饰 | `IPlugin` |
| Animation | 动画 | `IPlugin` |
| Expression | 表情 | `IPlugin` |
| Material | 材质 | `IPlugin` |
| BodyPart | 身体部件 | `IPlugin` |

#### 插件包结构

```
my-plugin.lumapkg/
├── manifest.json      # 元数据
├── thumbnail.png      # 预览图
├── assets/
│   ├── meshes/       # 3D 模型
│   ├── textures/     # 贴图
│   └── configs/      # 配置
└── lib/              # 原生库 (可选)
```

#### 使用方式

```cpp
#include "engine/plugin/plugin_manager.h"

auto& pm = luma::getPluginManager();

// 添加插件目录
pm.addPluginDirectory("/path/to/plugins");

// 发现并加载插件
auto discovered = pm.discoverPlugins();
for (const auto& meta : discovered) {
    auto result = pm.loadPlugin(meta.id);
    if (result.success) {
        std::cout << "Loaded: " << meta.name << std::endl;
    }
}

// 获取所有服装插件
auto clothingPlugins = luma::getClothingPlugins();
for (auto& plugin : clothingPlugins) {
    for (const auto& item : plugin->getClothingItems()) {
        std::cout << "  - " << item.name << std::endl;
    }
}

// 搜索资源
auto results = pm.searchAssets("striped", luma::PluginType::Clothing);
```

#### 创建简单服装插件

1. 创建目录 `my-clothing/`
2. 添加 `manifest.json`:
```json
{
  "id": "com.artist.my-clothing",
  "name": "My Clothing Pack",
  "type": "clothing",
  "version": "1.0.0"
}
```
3. 添加模型到 `assets/meshes/`
4. 添加配置到 `assets/configs/item.json`

详见 `docs/PLUGIN_DEVELOPMENT.md`

---

### 33. 标准绑定系统（Standard Rig System）

**文件**:
- `engine/character/standard_rig.h` - 标准骨骼规范
- `engine/character/facial_rig.h` - 面部绑定系统
- `engine/character/auto_rig.h` - 自动绑定生成

#### 32.1 支持的骨骼标准

| 标准 | 用途 | 骨骼数 |
|------|------|--------|
| Luma | 内部标准（超集） | 65+ |
| Mixamo | Adobe 动画库 | 65 |
| Unity Humanoid | Unity Avatar 系统 | 55 |
| VRM | VTuber/虚拟角色 | 55 |
| Unreal Mannequin | UE5 骨骼 | 67 |

#### 32.2 骨骼命名规范

```cpp
namespace StandardBones {
    // 脊椎链
    Hips -> Spine -> Spine1 -> Spine2 -> Chest -> Neck -> Head
    
    // 手臂（左/右）
    Shoulder_L/R -> UpperArm_L/R -> LowerArm_L/R -> Hand_L/R
    
    // 腿（左/右）
    UpperLeg_L/R -> LowerLeg_L/R -> Foot_L/R -> Toes_L/R
    
    // 手指（每根3节）
    Thumb1/2/3, Index1/2/3, Middle1/2/3, Ring1/2/3, Pinky1/2/3
    
    // 面部
    Jaw, Eye_L/R, Eyebrow_L/R, EyelidUpper/Lower_L/R, Tongue1/2
}
```

#### 32.3 骨骼名称转换

```cpp
// 在不同标准间转换骨骼名称
auto& mapping = BoneMappingTable::getInstance();
std::string mixamoName = mapping.convertBoneName(
    "upperArm_L",           // Luma 名称
    RigStandard::Luma,      // 源标准
    RigStandard::Mixamo     // 目标标准
);
// 结果: "mixamorig:LeftArm"
```

#### 32.4 面部绑定（ARKit 52 BlendShapes）

**眼睛 (14)**
- eyeBlink/LookDown/LookIn/LookOut/LookUp/Squint/Wide (L/R)

**嘴巴 (28)**
- jaw: Forward, Left, Right, Open
- mouth: Close, Funnel, Pucker, Left, Right, Smile, Frown, Dimple, Stretch, Roll, Shrug, Press, LowerDown, UpperUp (L/R)

**眉毛 (5)**
- browDown (L/R), browInnerUp, browOuterUp (L/R)

**脸颊/鼻子 (5)**
- cheekPuff, cheekSquint (L/R), noseSneer (L/R)

**舌头 (1)**
- tongueOut

#### 32.5 表情预设

```cpp
// 使用预设表情
auto& library = ExpressionLibrary::getInstance();

// 可用预设
// - neutral, happy, sad, angry, surprised, fear, disgust
// - blink, wink_left, wink_right

FacialRigController controller;
controller.setExpression("happy", 0.8f);  // 80% 强度
controller.setExpression("angry", 0.3f, true);  // 叠加 30%
```

#### 32.6 Viseme 口型同步

```cpp
// 15 种口型
Visemes::sil   // 静默
Visemes::PP    // p, b, m
Visemes::FF    // f, v
Visemes::TH    // th
Visemes::DD    // t, d
Visemes::kk    // k, g
Visemes::CH    // ch, j, sh
Visemes::SS    // s, z
Visemes::nn    // n, l
Visemes::RR    // r
Visemes::aa    // A 元音
Visemes::E     // E 元音
Visemes::ih    // I 元音
Visemes::oh    // O 元音
Visemes::ou    // U 元音

VisemeController viseme;
viseme.setViseme(Visemes::aa, 0.8f);
viseme.applyToFacialRig(facialController);
```

#### 32.7 自动绑定生成

```cpp
// 为网格自动生成骨骼权重
AutoRigParams params;
params.method = AutoRigParams::WeightMethod::DistanceBased;
params.falloffDistance = 0.3f;
params.smoothIterations = 3;

MeshSkinData skinData = AutoRigGenerator::generateWeights(
    mesh, skeleton, params);

// 验证权重
if (AutoRigGenerator::validateWeights(skinData)) {
    skinData.applyToMesh(mesh);
}
```

#### 32.8 创建完整绑定角色

```cpp
// 一键创建标准绑定角色
auto result = CharacterRigManager::createRiggedCharacter(
    mesh,
    1.8f,                       // 身高
    RigStandard::Mixamo         // 输出标准
);

if (result.success) {
    Skeleton& skeleton = result.skeleton;
    // 骨骼可直接导入 Mixamo 动画
}
```

#### 32.9 导出兼容性

```cpp
// 检查骨骼兼容性
std::string report = RigExporter::getCompatibilityReport(
    skeleton, RigStandard::UnityHumanoid);

// 导出为 JSON（Unity/Blender 可用）
std::string json = RigExporter::exportToJson(skinData, skeleton);
```

---

### 34. 导出系统（Export System）

**文件**:
- `engine/export/gltf_exporter.h` - glTF/GLB 导出器
- `engine/export/model_exporter.h` - 统一导出接口

#### 支持的格式

| 格式 | 骨骼 | BlendShapes | 动画 | 推荐用途 |
|------|------|-------------|------|----------|
| GLB | ✅ | ✅ | ✅ | Unity, Unreal, Blender, Web |
| glTF | ✅ | ✅ | ✅ | 需要分离文件时 |
| FBX | ✅ | ✅ | ✅ | Maya, 3ds Max, Cinema 4D |
| OBJ | ❌ | ❌ | ❌ | 简单网格导出 |
| VRM | ✅ | ✅ | ❌ | VTuber, VRChat |

#### 使用方式

```cpp
#include "engine/export/model_exporter.h"

// 导出为 GLB
ExportOptions options;
options.format = ExportFormat::GLB;
options.exportSkeleton = true;
options.exportBlendShapes = true;
options.embedTextures = true;

auto result = ModelExporter::exportCharacter(characterData, "character.glb", options);
if (result.success) {
    std::cout << "Exported: " << result.outputPath << std::endl;
    std::cout << "Vertices: " << result.vertexCount << std::endl;
}
```

---

### 35. 项目文件系统（Project File System）

**文件**: `engine/project/project_file.h`

#### 项目文件格式 (.luma)

JSON 格式，包含：
- 角色参数（身体、面部）
- 贴图设置
- 发型和颜色
- 服装列表
- BlendShape 权重
- 视图设置

#### 使用方式

```cpp
#include "engine/project/project_file.h"

auto& pm = luma::getProjectManager();

// 保存项目
pm.currentProject.name = "MyCharacter";
pm.saveProjectAs("/path/to/character.luma");

// 加载项目
pm.loadProject("/path/to/character.luma");

// 最近项目
for (const auto& path : pm.recentProjects) {
    std::cout << path << std::endl;
}

// 自动保存
pm.enableAutoSave(true, 60);  // 每 60 秒
```

---

### 36. 动画预览系统（Animation Preview）

**文件**: `engine/animation/animation_preview.h`

#### 内置动画

| ID | 名称 | 类别 | 说明 |
|----|------|------|------|
| idle | Idle | Basic | 待机呼吸动画 |
| tpose | T-Pose | Reference | T 姿势参考 |
| apose | A-Pose | Reference | A 姿势参考 |
| wave | Wave | Gesture | 挥手 |
| walk | Walk | Locomotion | 行走循环 |
| run | Run | Locomotion | 跑步循环 |

#### 使用方式

```cpp
#include "engine/animation/animation_preview.h"

// 获取动画库
auto& lib = luma::getAnimationLibrary();

// 播放动画
AnimationPlayer player;
player.setSkeleton(&skeleton);
player.play("walk");

// 更新（每帧）
player.update(deltaTime);

// 控制
player.setPlaybackSpeed(1.5f);
player.pause();
player.resume();
player.stop();
```

---

### 37. 角色预设系统（Character Presets）

**文件**: `engine/character/character_presets.h`

#### 内置预设列表（游戏风格优先）

| 分类 | 预设 ID | 名称 | 说明 |
|------|---------|------|------|
| **西幻 Fantasy** | fantasy_elf | 精灵 | 尖耳优雅精灵 |
| | fantasy_paladin | 圣骑士 | 光明圣骑士 |
| | fantasy_dark_mage | 暗黑法师 | 神秘黑暗魔法师 |
| | fantasy_orc | 兽人战士 | 绿皮肌肉战士 |
| **武侠 Wuxia** | wuxia_swordsman | 剑客 | 仗剑江湖侠士 |
| | wuxia_female_knight | 女侠 | 飒爽英姿女侠 |
| | wuxia_monk | 武僧 | 少林武僧 |
| **古风 Gufeng** | gufeng_xianxia_hero | 仙侠少年 | 修仙少年英雄 |
| | gufeng_fairy | 仙子 | 飘逸出尘仙子 |
| | gufeng_emperor | 帝王 | 威严古代帝王 |
| | gufeng_princess | 公主 | 端庄典雅公主 |
| **动漫 Anime** | anime_girl | 动漫少女 | 大眼睛、粉色系 |
| | anime_boy | 动漫少年 | 经典少年漫风格 |
| | anime_chibi | Q版角色 | 超变形可爱风 |
| **卡通 Cartoon** | cartoon_western | 西方卡通 | 美式卡通风格 |
| | cartoon_pixar | 皮克斯风格 | 3D 动画电影风格 |
| **科幻 Sci-Fi** | scifi_cyborg | 赛博格 | 人机混合体 |
| | scifi_alien | 外星人 | 大眼无发外星生物 |
| **写实 Realistic** | realistic_athlete | 运动员 | 健美体型 |
| | realistic_child | 儿童 | 儿童比例 |
| | realistic_elderly | 老年人 | 带年龄特征 |
| | realistic_business_man | 商务男士 | 专业男性商务造型 |
| | realistic_business_woman | 商务女士 | 专业女性商务造型 |

#### 使用方式

```cpp
#include "engine/character/character_presets.h"

// 获取预设库
auto& lib = luma::getPresetLibrary();

// 应用预设
const auto* preset = lib.getPreset("anime_girl");
if (preset) {
    applyToCharacter(preset->data);
}

// 按分类筛选
auto animePresets = lib.getPresetsByCategory(PresetCategory::Anime);

// 随机生成
CharacterRandomizer randomizer;
auto randomData = randomizer.generateRandom();  // 完全随机
auto randomAnime = randomizer.generateRandomInStyle(PresetCategory::Anime);  // 动漫风格随机
```

#### UI 功能

- **分类筛选**: All / Realistic / Anime / Cartoon / Fantasy / Sci-Fi
- **预设网格**: 2 列布局，显示预设名称、中文名、皮肤/发色预览
- **分类徽章**: R(写实) A(动漫) C(卡通) F(奇幻) S(科幻)
- **随机按钮**: 一键随机生成角色

---

---

## Phase 29: 角色增强系统 ✅

### 29.1 配饰系统 (`engine/character/accessory_system.h`)

**配饰类型 (30+)**

| 部位 | 类型 |
|------|------|
| 头部 | Hat, Glasses, Sunglasses, Mask, Headband, Hairpin, Crown, Helmet |
| 耳部 | Earring, Earphone |
| 脸部 | Beard, Makeup, FacePaint |
| 颈部 | Necklace, Scarf, Tie, Bowtie, Choker |
| 手臂 | Watch, Bracelet, Gloves |
| 手部 | Ring |
| 背部 | Wings, Cape, Backpack, Weapon |
| 腰部 | Belt, Pouch |

**功能特性:**
- 骨骼挂载点自动绑定
- 成对配饰支持（如耳环左右自动镜像）
- 程序化配饰生成（眼镜、帽子、耳环、项链）
- 颜色和材质自定义

### 29.2 贴花系统 (`engine/character/decal_system.h`)

**贴花类型:**
- Tattoo (纹身), Scar (伤疤), Birthmark (胎记)
- Makeup (妆容), BodyPaint (彩绘), FacePaint (脸绘)
- Wound (伤口), Dirt (污渍), Blood (血迹)
- Freckles (雀斑), Wrinkles (皱纹)

**混合模式:**
- Normal, Multiply, Overlay, Additive

**程序化生成:**
- 部落图腾纹身
- 刀疤/抓伤
- 雀斑
- 腮红/口红
- 伤口

### 29.3 表情预设扩展 (`engine/character/expression_presets.h`)

**6 大类别, 30+ 预设表情:**

| 类别 | 表情 |
|------|------|
| Basic | happy, sad, angry, surprised, fear, disgust |
| Emotion | smirk, pout, crying, laughing, thinking, sleepy, determined, embarrassed, confused, proud |
| Communication | talking, whispering, shouting, kissing, whistling |
| Action | sneezing, yawning, eating, drinking, biting_lip |
| GameStyle | battle_cry, victory, defeat, concentration, pain, evil |
| AnimeStyle | anime_shock, anime_cute, anime_smug, anime_dead, anime_sparkling |

---

## Phase 30: 环境系统 ✅

### 30.1 天气系统 (`engine/environment/weather_system.h`)

**天气类型 (13种):**
- Clear, Cloudy, Overcast
- LightRain, HeavyRain, Thunderstorm
- LightSnow, HeavySnow, Blizzard
- Fog, DenseFog
- Hail, Sandstorm

**功能特性:**
- 降水粒子系统（雨滴/雪花）
- 风力和风向
- 雾效（距离雾、高度雾）
- 云层覆盖
- 闪电效果
- 光照调整（太阳强度、环境色、阴影）
- 平滑天气过渡

### 30.2 日夜循环 (`engine/environment/day_night_cycle.h`)

**时间系统:**
- 24 小时循环
- 7 种时段：Dawn, Morning, Noon, Afternoon, Dusk, Evening, Night
- 自动推进或手动控制

**太阳/月亮:**
- 基于经纬度的太阳位置计算
- 月相系统
- 日出/日落颜色变化

**天空:**
- 7 种时段天空渐变（天顶色、地平线色）
- 星星可见度（夜间）

### 30.3 水体系统增强

**基础水体 (`engine/environment/water_system.h`):**
- WaterType: Lake, River, Ocean, Pond, Stream
- Gerstner 波浪模拟
- 多八度波叠加
- 水面网格生成

**涟漪交互 (`engine/environment/water_interaction.h`):**
- RippleSimulation: 256x256 网格波动方程模拟
- AnalyticalRippleSystem: 点源扩散涟漪
- WaterInteractionManager: 入水/出水检测
- 角色/物体移动产生涟漪

**水下效果 (`engine/environment/underwater_effects.h`):**
- 颜色吸收（深度越深红光衰减越多）
- 散射雾
- 动态焦散
- 屏幕扭曲
- 气泡系统（自动上浮、摇摆）
- 浮游粒子
- 暗角效果
- 深度模糊

**浮力物理 (`engine/environment/water_physics.h`):**
- BuoyancySystem: 多采样点浮力计算
- 线性/角速度阻力
- SwimmingController: 角色游泳控制

**视觉特效 (`engine/environment/water_effects.h`):**
- SplashEffectSystem: 水花/水滴/喷雾粒子
- FoamSystem: 水面泡沫漂浮
- ShoreEffectSystem: 岸边波浪
- CausticsGenerator: 程序化焦散纹理
- WetSurfaceSystem: 湿润地面效果

---

## Phase 31: 编辑器增强系统 ✅

### 31.1 撤销/重做系统 (`engine/editor/undo_system.h`)

**命令模式实现:**
- ICommand 抽象接口
- 支持 execute/undo 操作
- 命令合并（连续操作合并为一个）
- 复合命令（多个命令打包）

**内置命令类型:**
```cpp
// 值变化命令
ValueChangeCommand<T>      // 任意类型值变化
FloatSliderCommand         // 滑块调整（自动合并）
ColorChangeCommand         // 颜色变化

// 变换命令
TransformCommand           // 位置/旋转/缩放
BoneRotationCommand        // 骨骼旋转
BlendShapeCommand          // 表情调整

// 复合命令
CompositeCommand           // 命令组
PresetApplyCommand         // 预设应用
LambdaCommand              // 简单 Lambda
```

**CommandHistory 管理器:**
- 无限撤销/重做（可配置上限）
- 内存限制管理
- 命令合并时间窗口
- 保存点追踪（脏标记）
- 变更通知回调

### 31.2 快捷键系统 (`engine/editor/hotkey_system.h`)

**按键定义:**
```cpp
enum class KeyCode {
    // 字母 A-Z
    // 数字 0-9
    // 功能键 F1-F12
    // 特殊键 Space, Enter, Escape 等
    // 小键盘
};

enum class KeyModifier {
    Ctrl, Shift, Alt, Super  // macOS Cmd / Windows Win
};
```

**预设快捷键:**
| 快捷键 | 操作 | 说明 |
|--------|------|------|
| Ctrl+Z | Undo | 撤销 |
| Ctrl+Shift+Z | Redo | 重做 |
| Ctrl+S | Save | 保存 |
| Ctrl+E | Export | 导出 |
| Q/W/E/R | 工具切换 | 选择/移动/旋转/缩放 |
| Space | Play/Pause | 播放/暂停动画 |
| K | Add Keyframe | 添加关键帧 |
| Numpad 1/3/7 | 视图切换 | 前/右/顶视图 |
| F11 | Fullscreen | 全屏 |

**特性:**
- 用户自定义绑定
- 冲突检测
- 配置文件保存/加载
- 上下文感知（不同面板不同快捷键）

### 31.3 渲染预设系统 (`engine/rendering/render_presets.h`)

**质量档位:**
```cpp
enum class RenderQuality {
    Preview,   // 预览（0.5x 分辨率，无 AO/SSR）
    Draft,     // 草稿（0.75x，基础效果）
    Standard,  // 标准（1x，TAA + SSAO + SSR）
    High,      // 高质量（GI + 体积雾）
    Ultra      // 超高（1.5x 超采样，全部特效）
};
```

**RenderSettings 完整参数:**
- 分辨率和渲染缩放
- 抗锯齿（FXAA/SMAA/TAA/MSAA）
- 阴影（分辨率/级联数/软阴影/PCF）
- 环境光遮蔽（SSAO/HBAO/GTAO）
- 屏幕空间反射
- 全局光照（光线追踪/采样/反弹）
- 体积效果
- 后处理（Bloom/DOF/动态模糊/色差/暗角/胶片颗粒）
- 色调映射（Reinhard/ACES/Filmic/AgX）
- 材质质量（各向异性/法线/视差/SSS）

**高质量输出设置:**
```cpp
struct OutputSettings {
    // 分辨率预设
    enum class Resolution {
        HD_720p, FullHD_1080p, QHD_1440p,
        UHD_4K, UHD_8K,
        Square_1K, Square_2K, Square_4K
    };
    
    ImageFormat format;     // PNG/JPG/EXR/TGA
    bool transparentBackground;
    int samples;            // 累积采样
    bool isSequence;        // 序列渲染
};
```

---

## Phase 32: 动画增强系统 ✅

### 32.1 Lip Sync 口型同步 (`engine/animation/lip_sync.h`)

**Viseme 定义（口型）:**
```cpp
enum class Viseme {
    Silence,        // 静音
    AA, AE, AH,     // 元音 a
    AO, AW, AY,     // 元音 o
    B_M_P,          // 双唇音
    CH_J_SH,        // 齿音
    D_T_N,          // 舌尖音
    EH, ER, EY,     // 元音 e
    F_V,            // 唇齿音
    G_K_NG,         // 舌根音
    IH, IY,         // 元音 i
    L, R,           // 流音
    OW, OY,         // 元音 o
    S_Z, TH,        // 齿擦音
    UH, UW,         // 元音 u
    W, Y            // 半元音
};
```

**Viseme → BlendShape 映射:**
- 基于 ARKit 52 BlendShapes
- 每个 Viseme 映射到多个 BlendShape 权重
- 例如 B_M_P → mouthClose(0.8) + mouthPressLeft(0.3)

**音频分析:**
```cpp
struct AudioFrame {
    float amplitude;           // RMS 振幅
    float pitch;               // 音高估计
    std::array<float, 8> spectrum;  // 8 段频谱
    float zeroCrossingRate;    // 过零率
    float spectralCentroid;    // 频谱质心
};
```

**LipSyncEngine:**
- 基于频谱特征分类 Viseme
- 平滑过渡
- 振幅自适应
- 可调节 jaw/lip 强调

**LipSyncTrack:**
- 预烘焙的口型时间轴
- 从音频生成
- 从音素时序生成（TTS）

### 32.2 场景布置系统 (`engine/scene/scene_layout.h`)

**SceneObject:**
```cpp
struct SceneObject {
    string id, name;
    SceneObjectType type;  // Character/Prop/Light/Camera/Environment
    
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    
    string parentId;       // 层级
    vector<string> childIds;
    
    bool visible, locked, selected;
    string layerName;
    
    // 渲染
    bool castShadow, receiveShadow;
    float opacity;
};
```

**场景图层:**
- Default / Characters / Props / Environment / Lights / Effects
- 图层可见性/锁定
- 图层颜色标记

**场景预设:**
| 预设 | 说明 |
|------|------|
| Simple Studio | 三点光源摄影棚 |
| Photo Studio | 专业摄影环境 |
| Park | 户外公园 |
| City Street | 城市街道 |
| Castle Hall | 城堡大厅（奇幻） |
| Spaceship Interior | 飞船内部（科幻） |

**场景操作:**
- 添加/删除/复制物体
- 层级管理（父子关系）
- 分组/取消分组
- 对齐（X/Y/Z 轴）
- 分布（均匀排列）
- 多选操作

### 32.3 相机动画系统 (`engine/animation/camera_animation.h`)

**CameraKeyframe:**
```cpp
struct CameraKeyframe {
    float time;
    Vec3 position, target, up;
    
    float fov;
    float nearPlane, farPlane;
    
    // 景深
    float focusDistance, aperture;
    bool dofEnabled;
    
    // 插值
    EaseType easeIn, easeOut;
    Vec3 inTangent, outTangent;  // Bezier
};
```

**CameraPath:**
- 关键帧序列
- 循环支持
- 时间轴编辑

**预设相机运动:**
| 运动 | 说明 |
|------|------|
| Orbit | 环绕目标旋转 |
| Dolly | 推拉（前后移动） |
| Truck | 横移（左右移动） |
| Crane | 升降 |
| Zoom | 变焦（改变 FOV） |
| Dolly Zoom | 希区柯克眩晕效果 |
| Arc | 弧形移动 |
| Focus Pull | 焦点转移 |
| Shake | 震动效果 |

**CameraInterpolator:**
- 线性/缓入/缓出/缓入缓出/Bezier 插值
- 位置：Bezier 曲线
- 方向：球面插值
- 属性：线性插值

**CameraAnimationPlayer:**
- 播放/暂停/停止
- 时间控制
- 速度调节
- 循环播放
- 完成回调

---

## Phase 33: 性能优化系统 ✅

### 33.1 LOD 系统 (`engine/renderer/lod_system.h`)

**LOD 层级定义:**
```cpp
struct LODLevel {
    int level;              // 0 = 最高质量
    float screenSize;       // 屏幕占比阈值
    float distance;         // 距离阈值
    
    shared_ptr<Mesh> mesh;
    shared_ptr<Mesh> shadowMesh;  // 阴影专用低模
    
    int vertexCount, triangleCount;
    float reductionPercent;
};
```

**LOD 生成器:**
- 基于边折叠的网格简化
- 可配置简化目标（50%/25%/10%）
- 可配置距离/屏幕尺寸阈值
- UV/法线/边界保持选项
- 阴影 LOD 额外简化

**LOD 实例管理:**
- 自动距离选择
- 平滑过渡（Cross-fade）
- LOD 偏移（全局质量调节）
- 强制 LOD 级别

### 33.2 GPU Instancing (`engine/renderer/gpu_instancing.h`)

**实例数据:**
```cpp
struct InstanceData {
    Mat4 transform;
    Vec4 color;
    Vec4 customData;      // 自定义着色器数据
    Vec3 boundingCenter;
    float boundingRadius;
};
```

**实例批次管理:**
- 按 Mesh + Material 分组
- 自动视锥剔除
- 实例缓冲区管理
- 批量添加/更新/移除

**植被实例化:**
```cpp
class VegetationInstancer {
    void addType(typeId, meshId, density, scaleRange);
    void generateForArea(typeId, min, max, heightmap, densityMap);
    vector<InstanceData> getInstanceData(typeId, time);  // 含风动画
};
```

**便捷功能:**
- `scatter()`: 随机散布
- `grid()`: 网格排列

---

## Phase 34: 编辑器面板系统 ✅

### 34.1 资源浏览器 (`engine/editor/asset_browser.h`)

**资源类型:**
- 3D: Model, Mesh, Skeleton, Animation
- 纹理: Texture, Cubemap
- 材质: Material, Shader
- 音频: Audio
- 数据: Prefab, Scene, Project, Config
- 角色: Character, Clothing, HairStyle
- 脚本: Script

**资源信息:**
```cpp
struct AssetInfo {
    string id, name, path;
    AssetType type;
    size_t fileSize;
    vector<string> tags;
    vector<string> dependencies;
    bool isLoaded, isModified;
};
```

**浏览器功能:**
- 文件夹导航（历史/前进/后退/上级）
- 视图模式（列表/网格/缩略图）
- 搜索和类型过滤
- 排序（名称/类型/大小/日期）
- 拖放导入
- 多选操作

**导入设置:**
- ModelImportSettings: 缩放、动画、材质、LOD
- TextureImportSettings: Mipmap、sRGB、压缩
- AudioImportSettings: 采样率、压缩、流式

### 34.2 历史记录面板 (`engine/editor/history_panel.h`)

**历史条目:**
```cpp
struct HistoryEntry {
    int index;
    string description;
    string type;
    bool isCurrent, isUndone;
    string icon;
    Vec3 color;
};
```

**面板功能:**
- 显示撤销/重做堆栈
- 当前状态标记
- 跳转到任意历史点
- 过滤和搜索
- 保存点标记（脏标志）

### 34.3 属性检查器 (`engine/editor/property_inspector.h`)

**属性类型:**
- 基础: Bool, Int, Float, String
- 向量: Vec2, Vec3, Vec4, Quat
- 颜色: Color3, Color4
- 复杂: Enum, Flags, Asset, Object, Array

**属性元数据:**
```cpp
struct PropertyMeta {
    string name, displayName, tooltip, category;
    PropertyType type;
    
    bool readOnly, hidden;
    float minValue, maxValue, step;
    bool slider, logarithmic;
    
    vector<string> enumValues;
    string assetType;
};
```

**检查器功能:**
- 分类分组显示
- 搜索过滤
- 修改追踪（高亮显示）
- 只读属性支持
- 嵌套对象展开
- 数组编辑

**IInspectable 接口:**
```cpp
class IInspectable {
    virtual vector<PropertyDef> getProperties() = 0;
    virtual string getDisplayName() const = 0;
    virtual string getTypeName() const = 0;
};
```

### 34.4 视口工具 (`engine/editor/viewport_tools.h`)

**测量工具:**
```cpp
struct Measurement {
    enum Type { Distance, Angle, Area, Volume };
    vector<Vec3> points;
    float value;
    string displayValue;  // 带单位格式化
};
```

- 两点距离测量（自动 mm/cm/m 单位）
- 三点角度测量
- 多边形面积计算
- 包围盒体积计算

**标注系统:**
```cpp
struct Annotation {
    enum Type { Note, Warning, Todo, Question };
    string text;
    Vec3 worldPosition;
    string attachedObjectId;
    bool pinned, resolved;
};
```

- 4 种标注类型（不同颜色图标）
- 附着到物体
- 可折叠/解决状态

**参考图:**
```cpp
struct ReferenceImage {
    string path;
    Vec3 position;
    Quat rotation;
    Vec2 size;
    float opacity;
    
    enum View { Front, Back, Left, Right, Top, Bottom };
    void setView(View v);  // 快速设置标准视图
};
```

**网格设置:**
- 可见性/尺寸/细分
- 主次网格颜色
- 轴线颜色
- 吸附到网格

---

## 🔗 相关文档

- [README.md](README.md) - 项目概览和快速开始
- [GOLDEN_PATH.md](GOLDEN_PATH.md) - 30 分钟上手指南
- [docs/PLUGIN_DEVELOPMENT.md](docs/PLUGIN_DEVELOPMENT.md) - 插件开发指南
