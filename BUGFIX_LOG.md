# LUMA Bugfix 日志

本文档记录所有已修复的 bug 和问题，按日期组织。

---

## 2024-12-19 - Gizmo 交互和渲染修复

### 🐛 Bug #1: Gizmo 移动方向反转
**问题描述：**
- Gizmo 拖拽移动时，鼠标向右移动，物体向左移动（方向相反）
- 影响用户体验，不符合直觉

**根本原因：**
- `updateDrag` 函数中计算 `axisDeltaScalar` 时，坐标系或投影矩阵的方向问题
- 需要反转方向以匹配用户期望

**修复方案：**
- 在 `engine/editor/gizmo.h` 的 `updateDrag` 函数中，添加方向反转：
  ```cpp
  axisDeltaScalar = -axisDeltaScalar;
  ```
- 文件：`engine/editor/gizmo.h` (line ~408)

**状态：** ✅ 已修复

---

### 🐛 Bug #2: Gizmo 线条太细，难以看清
**问题描述：**
- Gizmo 线条太细，在屏幕上难以看清
- 不符合主流 3D 软件（Blender/Maya/Unreal）的视觉标准

**根本原因：**
- 使用单像素线条渲染，没有实现粗线条效果

**修复方案：**
- 在 `generateRenderData` 中实现"粗线条"效果：
  - 为每个轴生成多条稍微偏移的线条（5条）
  - 线条粗细为轴长度的 2%
  - 提高颜色亮度（红色 1.0,0.2,0.2，绿色 0.2,1.0,0.2，蓝色 0.2,0.4,1.0）
- 文件：`engine/editor/gizmo.h` (line ~493-520)

**状态：** ✅ 已修复

---

### 🐛 Bug #3: Gizmo 被物体覆盖，不可见
**问题描述：**
- Gizmo 渲染在物体后面时会被遮挡，无法看到
- 不符合主流软件的设计（Gizmo 应该始终可见）

**根本原因：**
- 使用普通的 line pipeline，深度测试为 `LESS`，会被物体深度遮挡

**修复方案：**
- 创建专用的 Gizmo pipeline：
  - 深度测试函数：`D3D12_COMPARISON_FUNC_ALWAYS`（始终通过深度测试）
  - 深度写入：`D3D12_DEPTH_WRITE_MASK_ZERO`（不写入深度缓冲区）
  - 深度偏移：`DepthBias = -1000`（将 Gizmo 稍微拉近相机）
- 在 `renderGizmoLines` 中使用 `gizmoPipelineState` 而不是 `linePipelineState`
- 文件：
  - `engine/renderer/unified_renderer_dx12.cpp` (line ~497, ~932-937, ~2702)

**状态：** ✅ 已修复

---

### 🐛 Bug #4: 旋转和缩放模式不工作
**问题描述：**
- 切换到旋转模式（E）或缩放模式（R）后，拖拽 Gizmo 没有反应
- 只有移动模式（W）正常工作

**根本原因：**
1. **旋转模式：**
   - `beginDrag` 和 `updateDrag` 使用轴线投影，但旋转应该使用平面投影
   - 角度计算不正确

2. **缩放模式：**
   - 使用了未声明的 `screenScale` 变量
   - 缩放计算逻辑有问题

**修复方案：**

1. **旋转模式修复：**
   - 在 `beginDrag` 中，旋转模式使用平面投影（垂直于旋转轴）：
     ```cpp
     if (mode_ == GizmoMode::Rotate) {
         Vec3 planeNormal = axisX;  // 使用轴作为平面法线
         dragStartPos_ = projectOntoPlane(ray, pos, planeNormal);
     }
     ```
   - 在 `updateDrag` 中，旋转模式也使用平面投影
   - 改进角度计算：将点投影到旋转平面，使用平面内的向量计算角度
   - 文件：`engine/editor/gizmo.h` (line ~343-365, ~425-450)

2. **缩放模式修复：**
   - 添加 `dragScreenScale_` 成员变量存储拖拽开始时的 `screenScale`
   - 在 `beginDrag` 中保存 `screenScale` 值
   - 在 `updateDrag` 中使用存储的 `dragScreenScale_` 而不是未声明的 `screenScale`
   - 修复缩放计算逻辑，使用正确的 `axisDeltaScalar`
   - 文件：`engine/editor/gizmo.h` (line ~186, ~333, ~418-422)

**状态：** ✅ 已修复

---

### 🐛 Bug #5: 编译错误 - screenScale 未声明
**问题描述：**
- 编译时错误：`error C2065: "screenScale": 未声明的标识符`
- 位置：`engine/editor/gizmo.h(434)`

**根本原因：**
- `updateDrag` 函数中使用了 `screenScale` 变量，但该函数没有这个参数
- 缩放计算需要 `screenScale` 来计算轴长度

**修复方案：**
- 添加 `dragScreenScale_` 成员变量
- 在 `beginDrag` 时保存 `screenScale` 值
- 在 `updateDrag` 中使用 `dragScreenScale_`
- 文件：`engine/editor/gizmo.h` (line ~186, ~333, ~434)

**状态：** ✅ 已修复

---

## 修复文件清单

### 修改的文件：
1. `engine/editor/gizmo.h`
   - 修复移动方向反转
   - 实现粗线条渲染
   - 修复旋转和缩放模式
   - 添加 `dragScreenScale_` 成员变量

2. `engine/renderer/unified_renderer_dx12.cpp`
   - 添加 `gizmoPipelineState` 成员变量
   - 创建专用的 Gizmo pipeline（始终可见）
   - 修改 `renderGizmoLines` 使用 Gizmo pipeline

3. `engine/renderer/mesh.h`
   - 添加 `create_cylinder` 函数（为将来使用）

---

## 测试建议

1. **移动模式（W）：**
   - 选择物体，按 W 切换到移动模式
   - 拖拽红色/绿色/蓝色轴线，物体应该沿对应方向移动
   - 鼠标向右移动，物体应该向右移动（方向正确）

2. **旋转模式（E）：**
   - 选择物体，按 E 切换到旋转模式
   - 拖拽旋转圆圈，物体应该围绕对应轴旋转

3. **缩放模式（R）：**
   - 选择物体，按 R 切换到缩放模式
   - 拖拽轴线，物体应该沿对应轴缩放

4. **可见性测试：**
   - 将相机移动到物体后面
   - Gizmo 应该仍然可见，不被物体遮挡

---

## 相关参考

- Blender Gizmo 实现参考
- Maya Transform Gizmo 设计
- Unreal Engine 编辑器 Gizmo 实现

---

## 备注

- 所有修复都经过编译测试
- Gizmo 现在使用 screen-space sizing，在任何距离都保持恒定的屏幕大小（100像素）
- 未来可以考虑使用几何体（圆柱体）而不是线条来渲染 Gizmo，以获得更好的视觉效果
