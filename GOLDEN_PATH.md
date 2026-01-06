# GOLDEN PATH — 30 分钟上手 LUMA (v1)

面向：Windows Creator 侧；目标：30 分钟内产出「好看 + 可交互 + 能在手机跑」的 Luma Take，并导出可在 iOS Runtime 运行的包。原则：Action 为唯一状态修改入口，Scene 仅引用 AssetID，构建 deterministic。

## 0. 前置准备
- 工具链：C++20、CMake ≥3.25、Ninja、Visual Studio Build Tools（MSVC）、Qt 6（用于 Creator UI/视口嵌入）、Python 3（如需脚本化打包）。
- 运行时：Windows 10/11 + DX12 级 GPU；iOS 设备（Metal）用于移动端验证。
- 依赖：glm、spdlog；后续引入 VFS、JobSystem、Profiler 等基础设施。

## 1. 获取与配置
- `git clone` 本仓库，初始化子模块（如有）。
- 配置构建（示例）：`cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo`
- 编译 Creator & Runtime：`cmake --build build -t luma_creator_win luma_runtime_ios`（目标名按实际 CMake 保持一致）

## 2. 启动 Creator
- 运行 `build/apps/luma_creator_win.exe`（或 IDE 直接启动）。
- 确保启动即完成 DX12 清屏 + Qt 视口嵌入（M0 骨架验证）。

## 3. 创建项目骨架
- 新建工程，生成默认 Scene（含一台 Camera、一盏 Key Light、示例网格）。
- 确认 Scene 节点仅存 AssetID 引用，无硬编码文件路径。

## 4. 导入与资产构建
- 在 Asset Import 面板导入 glTF（网格/材质/动画），写入 Asset Registry，触发 deterministic 构建：
  - 输入 glTF → 生成 `assets/*.bin`、`manifest.json`，记录 AssetID、type、deps。
  - 同输入应产生同 hash 输出；Runtime 不读取源 glTF。

## 5. 场景搭建
- 在层级面板放置模型，调整 Transform；添加/切换 Camera、Light。
- Renderable 组件绑定 Mesh + Material（通过 AssetID）；Animator 绑定 AnimationClip。

## 6. 配置 Look（Luma/Chroma）
- 在 Look 面板选择/编辑 Look：Lighting（IBL/HDR、主光）、Color（曝光、色调）、Post（Bloom/ToneMapping）。
- 桌面：延迟管线；移动：简化前向，自动降级（SSR→IBL、SSAO→Off/Weak、TAA→FXAA）。
- 保存 Look 可序列化、可热切换，并附带 Mobile Fallback。

## 7. 时间轴（Timeline-lite）
- 新建 Take 的 Timeline，添加 Track：
  - Transform Track：关键帧位置/旋转/缩放
  - Camera Track：FOV/位置
  - Param Track：材质参数或 Look 参数
  - Clip Track：挂载 glTF AnimationClip
- 播放/拖动时间指针，Evaluate(time) 驱动场景。

## 8. 动画与交互状态
- Animator：播放选定 AnimationClip，支持 GPU Skinning（播放级）。
- State Machine：定义 State + Transition（事件触发）；OnEnter/OnExit 绑定 Action 列表。
- Event Bus：InputClick / TimelineMarker → 触发 Action；禁止直接改世界状态。

## 9. Action 派发（唯一入口）
- 使用白名单 Action：
  - ApplyLook / SetParameter / SwitchCamera
  - PlayAnimation / SetState / SetMaterialVariant
- Creator UI、Graph、未来 Script 全部通过 Action 派发，便于记录/回放/跨端一致。

## 10. 预览与验证
- 桌面预览：在 Creator 播放 Take，观察帧率（30+ fps 优先稳定）；切换 Look，验证热更新。
- 交互测试：点击拾取/按钮触发状态切换，确认 Action 生效。

## 11. 打包导出
- 在 Creator 触发“一键导出”或命令行 packager：
  - 产物：`package/manifest.json`、`assets/*.bin`、`scene.bin`、可选 `preview.png`
  - 校验：同输入同输出 hash（deterministic），Scene 仅 AssetID。

## 12. 移动端运行（iOS First）
- 将 package 拷贝至 iOS Runtime 目录或通过调试部署。
- 启动 `luma_runtime_ios`，加载 package，播放 Take，验证：
  - 帧率 ≥30，移动 Look Fallback 生效
  - 基础交互可用，Action 驱动一致

## 13. 完成标准（Done）
- 30 分钟内：从空场景到可交互 Take，桌面预览正常，iOS 运行正常。
- 构建与打包 deterministic；无对源资产的运行时依赖；Action 是唯一状态入口；Look 可热切换且有移动降级。

