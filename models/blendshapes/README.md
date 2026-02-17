# BlendShape 工作流 (MetaHuman 风格)

## 概述

本系统采用与 MetaHuman 类似的分层架构：

```
用户界面参数 (如 "鼻子长度")
        ↓
   参数映射层
        ↓
底层 BlendShape targets (在 Blender 中雕刻)
        ↓
   最终网格变形
```

## BlendShape 命名规范

### Identity (身份特征) - 永久性面部结构

| 类别 | BlendShape 名称 | 描述 |
|------|----------------|------|
| **Face** | `id_face_width` | 脸宽 |
| | `id_face_length` | 脸长 |
| | `id_face_round` | 脸圆润度 |
| **Forehead** | `id_forehead_height` | 额头高度 |
| | `id_forehead_width` | 额头宽度 |
| | `id_forehead_slope` | 额头倾斜度 |
| **Eyes** | `id_eye_size` | 眼睛大小 |
| | `id_eye_spacing` | 眼距 |
| | `id_eye_height` | 眼睛高度位置 |
| | `id_eye_depth` | 眼窝深度 |
| | `id_eye_angle` | 眼角倾斜 |
| | `id_eye_lid_upper` | 上眼睑 |
| | `id_eye_lid_lower` | 下眼睑 |
| **Eyebrows** | `id_brow_height` | 眉毛高度 |
| | `id_brow_angle` | 眉毛角度 |
| | `id_brow_thickness` | 眉毛粗细区域 |
| **Nose** | `id_nose_length` | 鼻子长度 |
| | `id_nose_width` | 鼻子宽度 |
| | `id_nose_height` | 鼻梁高度 |
| | `id_nose_bridge` | 鼻梁形状 |
| | `id_nose_tip` | 鼻尖形状 |
| | `id_nostril_width` | 鼻翼宽度 |
| | `id_nostril_flare` | 鼻翼外扩 |
| **Mouth** | `id_mouth_width` | 嘴宽 |
| | `id_lip_upper_thick` | 上唇厚度 |
| | `id_lip_lower_thick` | 下唇厚度 |
| | `id_lip_protrusion` | 嘴唇突出 |
| | `id_philtrum` | 人中长度 |
| **Chin** | `id_chin_length` | 下巴长度 |
| | `id_chin_width` | 下巴宽度 |
| | `id_chin_protrusion` | 下巴突出 |
| | `id_chin_cleft` | 下巴沟 |
| **Jaw** | `id_jaw_width` | 下颌宽度 |
| | `id_jaw_angle` | 下颌角度 |
| **Cheeks** | `id_cheekbone_height` | 颧骨高度 |
| | `id_cheekbone_width` | 颧骨宽度 |
| | `id_cheek_fullness` | 脸颊丰满度 |
| **Ears** | `id_ear_size` | 耳朵大小 |
| | `id_ear_angle` | 耳朵角度 |

### Expression (表情) - ARKit 52 BlendShapes

参见 `arkit_blendshapes.md`

## 工作流程

### 1. 在 Blender 中创建 BlendShape

```bash
# 打开基础头部模型
blender models/head_base.obj

# 运行脚本创建 Shape Key 模板
# 脚本会为每个 BlendShape 创建一个空的 Shape Key
```

### 2. 雕刻每个 BlendShape

对于每个 Shape Key：
1. 选择该 Shape Key
2. 进入 Sculpt 模式
3. 雕刻变形（只修改相关区域的顶点）
4. 保持对称性（使用 X 镜像）

### 3. 导出

```bash
python models/blendshapes/export_blendshapes.py models/head_with_shapes.blend
```

### 4. 在引擎中加载

引擎会自动加载 `models/blendshapes/identity_shapes.bin`

## 变形幅度参考 (基于人体测量学)

| 特征 | 平均值 | 变化范围 (±2σ) |
|------|--------|----------------|
| 脸宽 | 140mm | ±15mm |
| 脸长 | 120mm | ±12mm |
| 眼距 | 62mm | ±5mm |
| 鼻长 | 50mm | ±6mm |
| 鼻宽 | 35mm | ±4mm |
| 嘴宽 | 50mm | ±5mm |
| 下巴长 | 30mm | ±5mm |

这些数据来自人体测量学研究，确保变形在解剖学上合理。
