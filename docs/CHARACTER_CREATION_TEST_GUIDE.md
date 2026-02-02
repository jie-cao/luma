# LUMA 角色创建系统 - 端到端测试指南

本文档提供完整的产品测试流程，帮助您验证角色创建系统的所有功能。

---

## 目录

1. [环境准备](#1-环境准备)
2. [启动测试](#2-启动测试)
3. [功能测试清单](#3-功能测试清单)
4. [测试场景](#4-测试场景)
5. [问题排查](#5-问题排查)

---

## 1. 环境准备

### 1.1 编译项目

```bash
# macOS
cd /Users/jcao4/code/luma
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# 或使用 Xcode
cmake .. -G Xcode
open LUMA.xcodeproj
```

### 1.2 运行应用

```bash
# 运行 LUMA Studio (macOS)
./build/apps/luma_studio_macos/LumaStudio.app/Contents/MacOS/LumaStudio
```

### 1.3 可选：准备 AI 模型

如需测试照片导入功能，需准备 ONNX 模型：

```
models/ai/
├── face_detector.onnx    # 人脸检测模型
└── face_mesh.onnx        # 人脸网格模型
```

### 1.4 可选：准备 MakeHuman 资源

如需使用高质量模型：

```
assets/makehuman/
├── base.obj              # 基础模型
└── targets/              # 变形目标
    ├── macrodetails/
    ├── body/
    └── face/
```

---

## 2. 启动测试

### 2.1 打开角色创建器

1. 启动 LUMA Studio
2. 菜单栏：**Window** → **Tools** → **Character Creator**
3. 点击 **"Initialize Character Creator"** 按钮

### 2.2 验证初始化成功

- [ ] Character Creator 窗口显示
- [ ] 3D 视口右侧出现程序化人体模型
- [ ] 控制台显示初始化日志
- [ ] Statistics 显示顶点/三角形/骨骼数量

---

## 3. 功能测试清单

### 3.1 Body 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 性别切换 | 切换 Gender 下拉框 | 模型体型变化 | ☐ |
| 体型预设 | 点击预设按钮 (Athletic/Average 等) | 参数自动调整 | ☐ |
| 身高调节 | 拖动 Height 滑块 | 模型高度变化 | ☐ |
| 体重调节 | 拖动 Weight 滑块 | 模型胖瘦变化 | ☐ |
| 肌肉度 | 拖动 Muscularity 滑块 | 肌肉轮廓变化 | ☐ |
| 肩宽 | 拖动 Shoulder Width 滑块 | 肩部宽度变化 | ☐ |
| 肤色 | 使用颜色选择器 | 皮肤颜色变化 | ☐ |
| 随机化 | 点击 Randomize 按钮 | 随机生成参数 | ☐ |

### 3.2 Face 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 脸宽 | 拖动 Face Width 滑块 | 脸部宽度变化 | ☐ |
| 眼睛大小 | 拖动 Eye Size 滑块 | 眼睛大小变化 | ☐ |
| 眼距 | 拖动 Eye Spacing 滑块 | 眼睛间距变化 | ☐ |
| 鼻子大小 | 拖动 Nose Size 滑块 | 鼻子比例变化 | ☐ |
| 嘴巴宽度 | 拖动 Mouth Width 滑块 | 嘴部宽度变化 | ☐ |
| 下巴长度 | 拖动 Chin Length 滑块 | 下巴形状变化 | ☐ |
| 眼睛颜色 | 使用颜色选择器 | 眼睛颜色变化 | ☐ |

### 3.3 AI 照片导入测试（需要 AI 模型）

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 模型状态检查 | 查看 AI Models 状态 | 显示 Ready/Not Configured | ☐ |
| AI 模型导入 | 点击 AI Model Setup | 打开设置窗口 | ☐ |
| 导入照片 | 点击 Import from Photo | 打开文件选择器 | ☐ |
| 照片处理 | 选择人脸照片 | 脸部参数自动调整 | ☐ |

### 3.4 BlendShapes 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| BlendShape 列表 | 展开 BlendShapes 区域 | 显示可用变形列表 | ☐ |
| 权重调节 | 拖动任意 BlendShape 滑块 | 模型形状变化 | ☐ |
| 重置所有 | 点击 Reset All | 所有权重归零 | ☐ |

### 3.5 Clothing 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 分类切换 | 切换 Category 下拉框 | 显示对应类别服装 | ☐ |
| 查看 Tops | 选择 Tops 分类 | 显示 T恤选项 | ☐ |
| 查看 Bottoms | 选择 Bottoms 分类 | 显示裤子/裙子选项 | ☐ |
| 穿戴服装 | 选择服装并点击 Equip | 模型显示服装 | ☐ |
| 脱下服装 | 点击 Unequip 或 X | 服装移除 | ☐ |
| 颜色修改 | 修改 Color 选择器 | 服装颜色变化 | ☐ |
| 已装备列表 | 查看 Currently Equipped | 显示当前穿戴 | ☐ |

### 3.6 Animation 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 分类切换 | 切换 Pose Category | 显示对应类别姿势 | ☐ |
| 应用姿势 | 点击姿势名称 | 模型姿势变化 | ☐ |
| T-Pose | 选择 Bind → T-Pose | 双臂水平展开 | ☐ |
| Idle 姿势 | 选择 Idle → Idle Stand | 放松站立姿势 | ☐ |
| 挥手姿势 | 选择 Action → Wave | 挥手姿势 | ☐ |
| 播放动画 | 点击 Play (Idle Breathing) | 角色微微呼吸 | ☐ |
| 停止动画 | 点击 Stop | 动画停止 | ☐ |
| 调节速度 | 拖动 Speed 滑块 | 动画速度变化 | ☐ |

### 3.7 Style 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 风格切换 | 切换 Rendering Style | 参数自动调整 | ☐ |
| 写实风格 | 选择 Realistic | 关闭轮廓线和分级着色 | ☐ |
| 动漫风格 | 选择 Anime | 启用轮廓线、3级着色 | ☐ |
| 卡通风格 | 选择 Cartoon | 粗轮廓线、2级着色 | ☐ |
| 轮廓线开关 | 切换 Enable Outline | 控制轮廓线显示 | ☐ |
| 轮廓线粗细 | 拖动 Thickness 滑块 | 轮廓线宽度变化 | ☐ |
| 轮廓线颜色 | 修改颜色选择器 | 轮廓线颜色变化 | ☐ |
| 着色级数 | 拖动 Shading Bands | 光影层次变化 | ☐ |
| 边缘光 | 切换 Enable Rim Light | 边缘高光开关 | ☐ |
| 色彩鲜艳度 | 拖动 Vibrancy 滑块 | 颜色饱和度变化 | ☐ |

### 3.8 Export 标签页测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 格式选择 | 切换 Format 下拉框 | glTF/FBX/OBJ 选项 | ☐ |
| 骨骼选项 | 切换 Include Skeleton | 勾选框状态切换 | ☐ |
| BlendShape 选项 | 切换 Include BlendShapes | 勾选框状态切换 | ☐ |
| 导出 glTF | 选择 glTF 并导出 | 生成 .glb 文件 | ☐ |
| 导出 OBJ | 选择 OBJ 并导出 | 生成 .obj 文件 | ☐ |
| 验证文件 | 用其他软件打开导出文件 | 模型正确显示 | ☐ |

### 3.9 通用功能测试

| 测试项 | 操作 | 预期结果 | 通过 |
|--------|------|----------|------|
| 自动旋转 | 切换 Auto Rotate | 模型自动旋转 | ☐ |
| 手动旋转 | 关闭 Auto Rotate，拖动滑块 | 手动控制角度 | ☐ |
| 实时预览 | 调整任意参数 | 3D 预览即时更新 | ☐ |
| 控制台日志 | 查看控制台窗口 | 显示操作日志 | ☐ |

---

## 4. 测试场景

### 场景 1：快速创建男性角色

```
目标：创建一个运动型男性角色

步骤：
1. 初始化 Character Creator
2. Body 标签：
   - Gender: Male
   - Preset: Athletic
   - Height: 0.7
   - Muscularity: 0.8
3. Face 标签：
   - 调整眼睛和下巴
4. Clothing 标签：
   - Equip: tshirt_black
   - Equip: pants_jeans
   - Equip: shoes_black
5. Animation 标签：
   - Apply: Idle Stand 姿势
6. Export：
   - 导出为 glTF 格式
   
预期结果：
- 角色外观符合设定
- 服装正确显示
- 导出文件可在其他软件打开
```

### 场景 2：创建动漫风格女性角色

```
目标：创建一个动漫风格的女性角色

步骤：
1. 初始化 Character Creator
2. Body 标签：
   - Gender: Female
   - Preset: Slim
   - Height: 0.45
3. Face 标签：
   - Eye Size: 0.7 (大眼睛)
   - Nose Size: 0.3 (小鼻子)
4. Style 标签：
   - Rendering Style: Anime
   - Shading Bands: 3
   - Enable Outline: Yes
   - Vibrancy: 1.3
5. Clothing 标签：
   - Equip: skirt_red
6. Animation 标签：
   - Apply: Wave 姿势
   
预期结果：
- 动漫风格视觉效果
- 大眼睛小鼻子的特征
- 轮廓线清晰可见
```

### 场景 3：照片生成角色（需要 AI 模型）

```
目标：从照片生成角色脸部

前置条件：
- 已导入 AI 模型 (face_detector.onnx, face_mesh.onnx)

步骤：
1. 初始化 Character Creator
2. Face 标签：
   - 确认 AI Models: Ready
   - 点击 "Import from Photo..."
   - 选择包含清晰人脸的照片
3. 等待处理完成
4. 微调生成的脸部参数
5. 添加服装和姿势
6. 导出

预期结果：
- 脸部参数自动调整
- 生成的脸部与照片相似
```

### 场景 4：服装换装测试

```
目标：测试服装系统的完整功能

步骤：
1. 创建基础角色
2. Clothing 标签：
   - Tops: 依次尝试所有 T恤
   - 观察颜色变化
   - 修改服装颜色
   - Bottoms: 尝试裤子和裙子
   - 验证裙子只对女性显示
   - Footwear: 添加鞋子
3. 移除所有服装
4. 重新穿戴组合

预期结果：
- 服装正确显示在角色身上
- 自动适配体型
- 颜色修改即时生效
```

### 场景 5：动画测试

```
目标：测试动画系统

步骤：
1. 创建角色并穿上服装
2. Animation 标签：
   - 依次应用所有姿势
   - 验证服装跟随角色变形
3. 播放 Idle Breathing 动画
   - 调整速度到 0.5x
   - 调整速度到 2.0x
4. 播放 Wave 动画
   - 观察动画完成后自动停止
5. 切换不同姿势
   - 从 Wave 切换到 Sitting

预期结果：
- 姿势切换流畅
- 动画播放正常
- 服装跟随角色动作
```

---

## 5. 问题排查

### 5.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| 角色不显示 | 初始化失败 | 检查控制台日志，确认模型加载 |
| 参数调整无反应 | BlendShape 数据缺失 | 确认程序化模型正确生成 |
| AI 导入失败 | 模型文件缺失 | 按 AI Model Setup 说明导入模型 |
| 导出文件为空 | 角色数据无效 | 确认角色已正确初始化 |
| 服装不显示 | 渲染问题 | 检查服装是否已 Equip |
| 动画不播放 | 骨骼未初始化 | 确认角色骨骼已设置 |

### 5.2 日志检查

控制台应显示以下关键日志：

```
[INFO] Character initialized with XXX vertices
[INFO] Clothing library initialized
[INFO] Animation library initialized
[INFO] Applied pose: idle
[INFO] Equipped: tshirt_white
[INFO] Export successful!
```

### 5.3 性能指标

| 指标 | 正常范围 |
|------|----------|
| 初始化时间 | < 2 秒 |
| 参数调整响应 | < 100ms |
| BlendShape 更新 | < 50ms |
| 导出时间 | < 5 秒 |

---

## 附录：测试结果记录表

```
测试日期: ____________
测试人员: ____________
系统版本: ____________

总测试项: 50+
通过项数: ____
失败项数: ____
通过率:   ____%

主要问题：
1. 
2. 
3. 

建议改进：
1. 
2. 
3. 
```

---

## 更新记录

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-01-23 | 1.0 | 初始版本，包含完整功能测试清单 |
