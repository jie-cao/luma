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
  - `PBRRenderer`：渲染逻辑封装（Pimpl 模式）
  - `OrbitCamera`：相机状态管理
  - `Viewport`：视口控制器（输入处理、渲染编排）
  - `UIPanels`：UI 面板模块化
  - `main.cpp`：精简的应用层（窗口、消息循环）

- [x] **资源管理**
  - GPU 缓冲区管理（顶点、索引）
  - 纹理资源管理（SRV 索引分配）
  - 命令列表同步（Fence、等待 GPU）

---

## 🎯 下一步计划

### 优先级 1：核心功能增强

#### 1. 场景管理（多对象支持）
- [ ] **场景图系统**
  - 场景节点层次结构
  - 多模型同时加载和显示
  - 对象变换（位置、旋转、缩放）
  - 场景序列化/反序列化

- [ ] **对象选择与操作**
  - 鼠标拾取（射线检测）
  - 对象高亮显示
  - 变换 Gizmo（移动、旋转、缩放手柄）
  - 多选支持

- [ ] **场景编辑器 UI**
  - 场景层次面板（Scene Hierarchy）
  - 属性面板（Transform、Material）
  - 对象列表（添加/删除/重命名）

#### 2. 阴影渲染
- [ ] **方向光阴影**
  - Shadow Mapping（深度贴图）
  - 级联阴影贴图（CSM，可选）
  - 软阴影（PCF、PCSS）

- [ ] **阴影优化**
  - 阴影贴图分辨率可调
  - 阴影距离裁剪
  - 阴影偏置（Bias）调整

#### 3. 材质编辑器 UI
- [ ] **实时材质编辑**
  - Metallic、Roughness、Base Color 滑块
  - 纹理替换（拖拽或文件选择）
  - 实时预览更新

- [ ] **材质管理**
  - 材质预设保存/加载
  - 材质库（Material Library）
  - 材质实例化

### 优先级 2：渲染质量提升

#### 4. 环境光照增强
- [ ] **Image-Based Lighting (IBL)**
  - HDR 环境贴图加载
  - 预过滤环境贴图（Prefiltered Environment Map）
  - 辐照度贴图（Irradiance Map）
  - 镜面反射 IBL

- [ ] **环境贴图支持**
  - .hdr 文件加载
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

- [ ] **跨平台渲染后端**
  - Vulkan 后端
  - Metal 后端（macOS/iOS）
  - 渲染抽象层（RHI）

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
- **Pimpl 模式**：`PBRRenderer` 使用 Pimpl 隐藏 DX12 实现细节，便于未来支持多后端
- **模块化设计**：将 UI、相机、视口分离，提高代码可维护性
- **动态裁剪平面**：根据相机距离自动调整，支持大场景缩放

### 已知问题
- 某些 FBX 模型的嵌入纹理路径可能不正确（已通过文件名匹配解决）
- 默认立方体在纹理系统改动后变为白色（需要检查默认材质）

### 待优化项
- 常量缓冲区更新可以优化（避免每帧 Map/Unmap）
- 纹理加载可以异步化
- 网格和轴线的渲染可以合并到同一管线

---

## 📅 更新记录

- **2024-12-XX**：初始开发日志创建
  - 记录已完成功能（PBR 渲染、模型加载、相机控制、UI 等）
  - 制定下一步开发计划

---

## 🔗 相关文档

- [README.md](README.md) - 项目概览和快速开始
- [GOLDEN_PATH.md](GOLDEN_PATH.md) - 30 分钟上手指南
