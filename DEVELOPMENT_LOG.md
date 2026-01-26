# LUMA Creator 开发日志

本文档记录 LUMA Creator 的开发进度和计划，持续更新。

---

## 📋 已完成功能

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

## 阶段 14：平台扩展 📋

> 目标：将 LUMA Studio 扩展到多个平台

### 14.1 iOS 查看器
- [ ] iOS Metal 渲染器
- [ ] 触摸手势 (旋转/缩放/平移)
- [ ] ARKit 支持 (可选)
- [ ] 移动端 UI 适配
- [ ] 性能优化 (LOD/剔除)

### 14.2 Android 查看器
- [ ] Vulkan 渲染器
- [ ] 触摸交互
- [ ] 多设备兼容
- [ ] 内存优化

### 14.3 Web 版本
- [ ] WebGPU 渲染器
- [ ] WebGL 2.0 回退
- [ ] 资源流式加载
- [ ] Web Worker 多线程

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

## 🔗 相关文档

- [README.md](README.md) - 项目概览和快速开始
- [GOLDEN_PATH.md](GOLDEN_PATH.md) - 30 分钟上手指南
