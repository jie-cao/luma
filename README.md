# LUMA Creator

跨平台实时 3D 创作平台，支持 **FBX/OBJ/glTF** 等 40+ 格式模型加载和 **PBR 渲染**。

## 功能概览

- **跨平台渲染**：统一的 UnifiedRenderer，支持 DX12 (Windows) 和 Metal (macOS)
- **完整 PBR 管线**：Cook-Torrance BRDF、法线贴图、ORM 纹理、ACES 色调映射
- **异步纹理加载**：模型几何体即时显示，纹理后台加载
- **模型加载**：通过 Assimp 支持 **FBX、OBJ、glTF、DAE、3DS** 等 40+ 格式
- **ImGui 界面**：
  - Model 面板：打开模型、显示加载进度
  - Camera 面板：Maya 风格轨道相机控制
  - Material 面板：调节 Metallic/Roughness/Base Color

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
│   ├── foundation/           # 日志、类型
│   ├── scene/                # Scene/Node
│   ├── asset/
│   │   ├── model_loader      # Assimp 模型加载
│   │   └── async_texture_loader  # 异步纹理加载
│   ├── renderer/
│   │   ├── unified_renderer.h     # 统一渲染器接口
│   │   ├── unified_renderer_dx12.cpp   # DX12 实现
│   │   ├── unified_renderer_metal.mm   # Metal 实现
│   │   ├── mesh.h                 # Mesh/Vertex 结构
│   │   ├── shaders/
│   │   │   └── pbr.metal          # Metal PBR 着色器
│   │   └── rhi/                   # RHI 抽象层 (预留)
│   └── viewport/             # 相机控制
├── apps/
│   ├── creator_imgui/        # Windows ImGui 主程序
│   ├── creator_macos/        # macOS Metal 主程序
│   └── demo/                 # 控制台示例
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

## 后续方向

- [ ] Shader 热重载
- [ ] 骨骼动画 (GPU Skinning)
- [ ] 多物体场景编辑
- [ ] IBL 环境光照
- [ ] iOS/Android Runtime
- [ ] Vulkan 后端
