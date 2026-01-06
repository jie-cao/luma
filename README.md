# LUMA Skeleton

最小可运行骨架，包含：
- Engine 核心：Scene/Action/Timeline/Look，Material 参数与 Variant 存储（Action 为唯一状态入口）。
- 渲染抽象：RenderGraph + RHI，DX12 后端可清屏、处理 backbuffer 状态，上传材质参数到 CB；Metal/Vulkan 占位。
- 工具与应用：
  - `luma_dx12_clear`：DX12 清屏示例。
  - `luma_demo`：控制台 Action 流示例。
- `luma_creator_imgui`：ImGui Creator（视口 + 资产/Inspector/Timeline 面板，参数/Variant 通过 Action 写回并上传 RHI）。
  - `luma_packager`：生成 deterministic manifest + assets bin，支持 glTF 解析（可选 tinygltf）。

## 环境要求
- Windows：MSVC 2019/2022（x64）+ CMake ≥ 3.25 + Ninja + DX12 GPU。
- ImGui（默认开启，CMake FetchContent 自动拉取）。
- tinygltf（可选，启用 `LUMA_WITH_TINYGLTF` 时需提供头文件目录 `third_party/tinygltf`）。

## 目录概览
- `engine/`：foundation、scene、actions、renderer（rhi/render_graph）、asset pipeline。
- `apps/`：`demo`、`dx12_clear`、`creator_imgui`。
- `tools/packager`：打包生成 manifest/assets。

## 配置与编译
### 常规（不含 Qt、不含 tinygltf）
```pwsh
cd C:\code\luma
cmake -S . -B build -G Ninja
cmake --build build
```

### 启用 ImGui Creator（默认 ON）
```pwsh
cd C:\code\luma
cmake -S . -B build -G Ninja -DLUMA_USE_IMGUI=ON
cmake --build build --target luma_creator_imgui
```
> 会通过 FetchContent 拉取 imgui v1.89.9（需可访问 GitHub）。

### 启用 tinygltf 解析 glTF 元数据
```pwsh
cmake -S . -B build -G Ninja -DLUMA_WITH_TINYGLTF=ON
cmake --build build
```
> 需在 `third_party/tinygltf` 放置 tinygltf 头文件。

## 运行示例
- DX12 清屏：`build\luma_dx12_clear.exe`
- 控制台 Action：`build\luma_demo.exe`
- ImGui Creator（默认）：`build\luma_creator_imgui.exe`
  - 资产/Inspector/Timeline 在 ImGui 面板，参数与 Variant 通过 Action 写回并上传至 DX12 CB。
- Packager：`build\luma_packager.exe package_out [path\to\model.gltf]`
  - 无 glTF 时生成 demo manifest/bin；有 glTF 时解析生成资产清单，bin 写入原始内容或 stub。

## 当前限制与后续方向
- Vulkan 未实现，Metal 为占位。
- DX12 仅清屏示例，材质参数已上传到 CB，但未绑定实际 PSO/绘制。
- glTF 解析为元数据，网格/纹理加载与绘制未接通。
- 参数删除/重命名、真实材质应用需后续完善。

