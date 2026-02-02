# LUMA Creator

跨平台实时 3D 创作平台，支持 **FBX/OBJ/glTF** 等 40+ 格式模型加载和 **PBR 渲染**。

## 功能概览

### 核心渲染
- **跨平台渲染**：统一的 UnifiedRenderer，支持 DX12 (Windows) 和 Metal (macOS)
- **完整 PBR 管线**：Cook-Torrance BRDF、法线贴图、ORM 纹理、ACES 色调映射
- **风格化渲染**：卡通着色、描边、Cel Shading
- **异步纹理加载**：模型几何体即时显示，纹理后台加载
- **模型加载**：通过 Assimp 支持 **FBX、OBJ、glTF、DAE、3DS** 等 40+ 格式

### 角色创建系统 (NEW!)
- **角色模板**：支持多种角色类型
  - Human - 写实人类（7.5 头身）
  - Cartoon - 卡通角色（米奇风格，4 头身）
  - Mascot - 吉祥物（Hello Kitty 风格，1.5 头身）
  - Anime/Chibi/Animal/Robot/Fantasy
- **身体部件系统**：可组合的头、躯干、四肢、尾巴、翅膀等
- **卡通特征**：多种眼睛/耳朵/鼻子/嘴巴样式（支持无嘴设计）
- **BlendShape 变形**：面部表情和身体调整
- **程序化贴图**：皮肤、眼睛、嘴唇纹理自动生成
- **发型库**：多种发型和颜色预设
- **服装系统**：
  - 服装加载（OBJ/glTF/FBX）
  - 布料物理模拟
  - 程序化布料纹理（棉、牛仔、丝绸、皮革等）
  - 自动骨骼蒙皮

### 插件系统
- **第三方扩展**：支持用户和开发者添加内容
  - 角色模板插件（新角色类型）
  - 服装插件（衣服、鞋子、配饰）
  - 发型插件
  - 动画插件
- **插件格式**：`.lumapkg` 包含 manifest.json + 资源
- **热加载**：无需重启应用即可加载新插件

### 配饰系统 (NEW!)
- **配饰类型**：眼镜、帽子、耳环、项链、手表、翅膀、披风等 30+ 种
- **骨骼挂载**：自动绑定到角色骨骼
- **成对配饰**：耳环等自动镜像

### 贴花系统 (NEW!)
- **贴花类型**：纹身、伤疤、雀斑、妆容、伤口等
- **混合模式**：Normal、Multiply、Overlay、Additive
- **程序化生成**：部落图腾、刀疤、雀斑等

### 表情系统 (NEW!)
- **30+ 预设表情**：分 6 大类
  - 基本情感、扩展情感、交流表情
  - 动作表情、游戏风格、动漫风格
- **ARKit 兼容**：52 BlendShapes 标准

### 环境系统 (NEW!)
- **天气系统**：
  - 13 种天气类型（晴/多云/雨/雪/雾/沙尘暴等）
  - 粒子效果、闪电、风力
  - 平滑天气过渡
- **日夜循环**：
  - 24 小时太阳/月亮轨迹
  - 7 种时段天空渐变
  - 星星可见度
- **水体系统**：
  - Gerstner 波浪模拟
  - 涟漪交互系统
  - 水下效果（散射、气泡、焦散）
  - 浮力物理
  - 水花/泡沫/岸边效果

### 编辑器界面
- **ImGui 界面**：
  - Model 面板：打开模型、显示加载进度
  - Camera 面板：Maya 风格轨道相机控制
  - Material 面板：调节 Metallic/Roughness/Base Color
  - Character Creator：身体/面部/服装/发型/贴图/动画
  - Plugin Browser：浏览和管理已安装插件

## 环境要求

### Windows
- Windows 10/11 + MSVC 2019/2022（x64）
- CMake ≥ 3.25 + Ninja
- DX12 兼容 GPU

### macOS
- macOS 11.0 (Big Sur) 或更高
- Xcode Command Line Tools
- CMake ≥ 3.25
- Metal 兼容 GPU（所有 Apple Silicon 和大部分 Intel Mac）

## 快速开始

### Windows（DX12）

```powershell
cd C:\code\luma
cmake -S . -B build -G Ninja
cmake --build build --target luma_creator_imgui
.\build\luma_creator_imgui.exe
```

### macOS（Metal）

```bash
cd ~/code/luma
cmake -S . -B build -G Ninja
cmake --build build --target luma_creator_macos

# 运行应用
open build/luma_creator_macos.app
```

## 控制说明

| 操作 | Windows | macOS |
|------|---------|-------|
| 轨道旋转 | Alt + 左键 | Option + 左键 |
| 平移 | Alt + 中键 | Option + 中键 |
| 缩放 | Alt + 右键 / 滚轮 | Option + 右键 / 滚轮 |
| 重置相机 | F | F |
| 切换网格 | G | G |
| 自动旋转 | - | R |
| 打开模型 | 菜单/按钮 | O |

## 支持的格式

| 格式 | 扩展名 | 说明 |
|------|--------|------|
| FBX | `.fbx` | Autodesk 格式，游戏/影视常用 |
| OBJ | `.obj` | 通用静态网格格式 |
| glTF | `.gltf`, `.glb` | Khronos 开放标准 |
| Collada | `.dae` | XML 格式 |
| 3DS | `.3ds` | 3ds Max 格式 |
| Blender | `.blend` | Blender 原生格式 |
| STL | `.stl` | 3D 打印常用 |
| PLY | `.ply` | 点云/扫描数据 |

## 架构概览

```
luma/
├── engine/
│   ├── foundation/           # 日志、数学类型 (Vec3, Mat4, Quat)
│   ├── scene/                # Scene/Node
│   ├── asset/
│   │   ├── model_loader      # Assimp 模型加载
│   │   └── async_texture_loader  # 异步纹理加载
│   ├── renderer/
│   │   ├── unified_renderer.h     # 统一渲染器接口
│   │   ├── unified_renderer_dx12.cpp   # DX12 实现
│   │   ├── unified_renderer_metal.mm   # Metal 实现
│   │   ├── mesh.h                 # Mesh/Vertex 结构
│   │   └── shaders/               # 着色器
│   ├── animation/            # 骨骼动画系统
│   │   ├── skeleton.h        # 骨骼层级
│   │   ├── animation.h       # 动画剪辑
│   │   └── animator.h        # 动画播放器
│   ├── character/            # 角色创建系统
│   │   ├── character_template.h   # 角色模板接口
│   │   ├── character_templates.h  # Human/Cartoon/Mascot 模板
│   │   ├── body_part_system.h     # 身体部件组装
│   │   ├── cartoon_features.h     # 卡通特征生成
│   │   ├── blend_shape.h          # BlendShape 变形
│   │   ├── character_body.h       # 身体参数
│   │   ├── character_face.h       # 面部参数
│   │   ├── texture_system.h       # 程序化贴图
│   │   ├── hair_system.h          # 发型库
│   │   ├── clothing_system.h      # 服装系统
│   │   ├── clothing_loader.h      # 服装加载器
│   │   ├── cloth_simulation.h     # 布料物理
│   │   ├── clothing_textures.h    # 布料纹理
│   │   ├── clothing_skinning.h    # 服装蒙皮
│   │   ├── accessory_system.h     # 配饰系统
│   │   ├── decal_system.h         # 贴花系统
│   │   ├── expression_presets.h   # 表情预设库
│   │   └── stylized_rendering.h   # 卡通渲染
│   ├── environment/          # 环境系统
│   │   ├── weather_system.h       # 天气系统
│   │   ├── day_night_cycle.h      # 日夜循环
│   │   ├── water_system.h         # 水体系统
│   │   ├── water_interaction.h    # 涟漪交互
│   │   ├── underwater_effects.h   # 水下效果
│   │   ├── water_physics.h        # 浮力物理
│   │   └── water_effects.h        # 水花/泡沫
│   ├── ui/                   # 编辑器 UI
│   │   └── editor_ui.h       # ImGui 界面
│   └── viewport/             # 相机控制
├── apps/
│   ├── luma_studio_macos/    # macOS 完整编辑器
│   ├── luma_viewer_ios/      # iOS 查看器
│   ├── luma_viewer_android/  # Android 查看器
│   └── luma_creator_imgui/   # Windows ImGui 程序
└── tools/packager/           # 打包工具
```

## 渲染特性

### PBR 管线
- **Cook-Torrance BRDF**：GGX 法线分布、Smith 几何遮蔽、Fresnel-Schlick
- **纹理支持**：Diffuse/Albedo、Normal Map、Specular/ORM
- **色调映射**：ACES Filmic
- **环境光**：半球环境光

### 性能优化
- **异步纹理加载**：2 个工作线程后台解码纹理
- **常量缓冲区环形缓冲**：避免每帧 Map/Unmap 开销 (DX12)
- **持久化映射**：减少 CPU-GPU 同步

## 角色创建示例

### 创建 Hello Kitty 风格角色

```cpp
#include "engine/character/character_templates.h"

// 注册模板
luma::registerDefaultTemplates();

// 配置参数
luma::CharacterParams params;
params.type = luma::CharacterType::Mascot;
params.primaryColor = luma::Vec3(1, 1, 1);      // 白色身体
params.accentColor = luma::Vec3(1, 0.3f, 0.4f); // 粉色蝴蝶结
params.hasMouth = false;                         // Hello Kitty 无嘴！
params.earStyle = 1;                             // 猫耳

// 创建角色
auto result = luma::getTemplateRegistry()
    .createCharacter(luma::CharacterType::Mascot, params);

// 使用生成的数据
luma::Mesh mesh = result.baseMesh;
luma::Skeleton skeleton = result.skeleton;
```

### 创建米奇风格角色

```cpp
luma::CharacterParams params;
params.type = luma::CharacterType::Cartoon;
params.primaryColor = luma::Vec3(0.1f, 0.1f, 0.1f);  // 黑色身体
params.secondaryColor = luma::Vec3(0.95f, 0.85f, 0.75f); // 脸部肤色
params.accentColor = luma::Vec3(1.0f, 0.2f, 0.2f);   // 红色短裤
params.hasTail = true;
params.earStyle = 0;  // 圆形鼠耳

auto result = luma::getTemplateRegistry()
    .createCharacter(luma::CharacterType::Cartoon, params);
```

## 技术栈

- C++20
- DirectX 12 / Metal
- ImGui v1.89.9
- Assimp v5.3.1
- stb_image
- CMake + Ninja

## 获取测试模型

- [Khronos glTF Sample Models](https://github.com/KhronosGroup/glTF-Sample-Models)
- [Sketchfab](https://sketchfab.com)
- [Mixamo](https://www.mixamo.com)
- [Polyhaven](https://polyhaven.com/models)

## 已完成功能

- [x] Shader 热重载
- [x] 骨骼动画 (GPU Skinning)
- [x] 多物体场景编辑
- [x] IBL 环境光照
- [x] iOS/Android 查看器
- [x] 角色创建系统
- [x] 多角色模板（Human/Cartoon/Mascot）
- [x] 服装系统（加载/物理/贴图/蒙皮）
- [x] 发型库系统
- [x] 程序化贴图生成

## 后续方向

- [ ] Vulkan 后端
- [ ] 更多角色模板（Animal/Robot/Fantasy）
- [ ] 动作捕捉导入
- [ ] 实时语音驱动面部表情
- [ ] 多角色场景协作
