# LUMA Creator

实时 3D 创作平台骨架，支持 **FBX/OBJ/glTF** 等 40+ 格式模型加载和 PBR 渲染。

## 功能概览

- **3D 渲染**：DX12 PBR 渲染管线（Blinn-Phong 光照、深度测试）
- **模型加载**：通过 Assimp 支持 **FBX、OBJ、glTF、DAE、3DS** 等 40+ 格式
- **ImGui 界面**：
  - Model 面板：打开模型文件（支持多格式）
  - Camera 面板：自动/手动旋转、距离调节
  - Material 面板：调节 Metallic/Roughness/Base Color
- **Engine 核心**：Scene/Action/Timeline/Look，材质参数通过 Action 驱动
- **工具**：Packager 生成 deterministic manifest + assets bin

## 环境要求

- Windows 10/11 + MSVC 2019/2022（x64）
- CMake ≥ 3.25 + Ninja
- DX12 兼容 GPU
- 网络访问（CMake FetchContent 拉取 imgui、assimp）

## 快速开始

### 1. 配置与编译

在 **Visual Studio Developer Command Prompt** 中：

```powershell
cd C:\code\luma
cmake -S . -B build -G Ninja
cmake --build build --target luma_creator_imgui
```

### 2. 运行

```powershell
.\build\luma_creator_imgui.exe
```

### 3. 加载模型

1. 启动后显示默认彩色立方体
2. 点击 **Model** 面板的 **"Open Model..."** 按钮
3. 选择任意支持的 3D 文件（FBX、OBJ、glTF 等）
4. 模型自动居中并调整相机

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

## 获取测试模型

- [Khronos glTF Sample Models](https://github.com/KhronosGroup/glTF-Sample-Models) - glTF 测试模型
- [Sketchfab](https://sketchfab.com) - 可下载 FBX/glTF
- [Mixamo](https://www.mixamo.com) - 免费角色和动画（FBX）
- [TurboSquid](https://www.turbosquid.com/Search/3D-Models/free) - 免费 FBX/OBJ 模型
- [Polyhaven](https://polyhaven.com/models) - CC0 高质量模型

## 目录结构

```
luma/
├── engine/
│   ├── foundation/     # 日志、类型
│   ├── scene/          # Scene/Node
│   ├── actions/        # Action 系统
│   ├── asset/          # glTF 加载器、打包管线
│   ├── renderer/
│   │   ├── mesh.h      # Mesh 结构
│   │   ├── rhi/        # DX12/Metal/Vulkan 后端
│   │   └── render_graph/
│   └── engine_facade.h
├── apps/
│   ├── creator_imgui/  # ImGui 主程序
│   ├── demo/           # 控制台 Action 示例
│   └── dx12_clear/     # DX12 清屏示例
└── tools/packager/     # 打包工具
```

## 其他 Target

```powershell
# 全部编译
cmake --build build

# DX12 清屏示例
.\build\luma_dx12_clear.exe

# 控制台 Action 示例
.\build\luma_demo.exe

# 打包工具（生成 manifest + assets）
.\build\luma_packager.exe package_out [path\to\model.gltf]
```

## 界面说明

| 面板 | 功能 |
|------|------|
| Model | 加载 glTF、显示当前模型名和网格数 |
| Camera | 自动/手动旋转、相机距离、模型边界信息 |
| Material | Metallic / Roughness / Base Color 调节 |

## 技术栈

- C++20
- DirectX 12
- ImGui v1.89.9
- Assimp v5.3.1 (支持 FBX/OBJ/glTF 等 40+ 格式)
- CMake + Ninja

## 后续方向

- [ ] 纹理加载（Base Color / Normal / Metallic-Roughness）
- [ ] 骨骼动画 (GPU Skinning)
- [ ] 多物体场景编辑
- [ ] Look System（光照/后处理/调色）
- [ ] Mobile Runtime (iOS/Android)
- [ ] Vulkan/Metal 后端
