# LUMA 角色创建系统 - 产品规划文档

> 版本: 1.0  
> 最后更新: 2026-01-23  
> 状态: 开发中

---

## 一、产品愿景

构建一个 **"照片 → 可编辑 3D 角色 → 可穿戴 → 可动画"** 的完整角色生产流水线。

### 核心价值主张
- **照片驱动**: 一张照片即可生成 3D 脸部
- **高度可定制**: 生成后可继续调整（捏脸捏身）
- **标准输出**: 符合游戏引擎规范的模型、骨骼、绑定
- **商业扩展**: 服装库、动画库可持续更新和变现

---

## 二、功能模块

### 2.1 脸部系统 (优先级: P0)

#### 2.1.1 照片→3D脸部 Pipeline

```
照片输入
    │
    ▼
┌──────────────────┐     ┌──────────────────┐
│ 人脸检测 & 对齐  │────▶│ 特征点提取      │
│ (MediaPipe/MTCNN)│     │ (468个3D点)     │
└──────────────────┘     └────────┬─────────┘
                                  │
                                  ▼
                        ┌──────────────────┐
                        │ 3DMM 参数回归    │
                        │ (DECA/EMOCA)     │
                        │ ↓                │
                        │ • 形状参数       │
                        │ • 表情参数       │
                        │ • 姿态参数       │
                        │ • 贴图参数       │
                        └────────┬─────────┘
                                  │
    ┌─────────────────────────────┼─────────────────────────────┐
    │                             │                             │
    ▼                             ▼                             ▼
┌─────────┐                ┌─────────────┐              ┌──────────────┐
│生成脸部 │                │生成UV贴图   │              │ 多照片融合   │
│BlendShape│               │(从照片投射) │              │ (可选)       │
│ 权重    │                └─────────────┘              └──────────────┘
└────┬────┘
     │
     ▼
┌─────────────────────────────────────────────┐
│           参数化的 3D 脸部模型              │
│  • BlendShape 驱动 (可继续调整)            │
│  • 标准骨骼绑定                             │
│  • UV 贴图                                  │
└─────────────────────────────────────────────┘
```

#### 2.1.2 脸部调整参数

| 区域 | 参数 | 数量 |
|------|------|------|
| 整体脸型 | 圆/方/长/瓜子/心形 | 5 |
| 眼睛 | 大小/间距/高度/角度/眼皮/眼窝 | 10 |
| 眉毛 | 高度/角度/粗细/间距 | 6 |
| 鼻子 | 高度/宽度/长度/鼻翼/鼻梁/鼻尖 | 8 |
| 嘴巴 | 大小/厚度/上唇/下唇/嘴角 | 8 |
| 下巴 | 长度/宽度/角度/凸出 | 5 |
| 耳朵 | 大小/位置/角度 | 4 |
| 脸颊 | 颧骨/脂肪/凹陷 | 4 |
| **总计** | | **~50** |

---

### 2.2 身体系统 (优先级: P0)

#### 2.2.1 基础参数

| 类别 | 参数 | 范围 |
|------|------|------|
| 性别 | 男/女/中性 | 枚举 |
| 年龄 | 幼年/青年/中年/老年 | 0.0 ~ 1.0 |
| 身高 | 矮/中/高 | 0.0 ~ 1.0 |
| 体型 | 瘦/中/胖 | 0.0 ~ 1.0 |

#### 2.2.2 细节参数

| 区域 | 参数 |
|------|------|
| 上身 | 肩宽、胸围、腰围、手臂粗细、肌肉量 |
| 下身 | 臀围、大腿粗细、小腿粗细、腿长比例 |
| 四肢 | 手掌大小、脚掌大小、关节粗细 |

#### 2.2.3 预设体型

```
男性预设:
├── 瘦弱型 (Slim)
├── 标准型 (Average)
├── 肌肉型 (Muscular)
├── 壮硕型 (Heavy)
└── 老年型 (Elderly)

女性预设:
├── 纤细型 (Slim)
├── 标准型 (Average)
├── 丰满型 (Curvy)
├── 运动型 (Athletic)
└── 老年型 (Elderly)

儿童预设:
├── 幼儿 (Toddler)
├── 儿童 (Child)
└── 青少年 (Teen)
```

---

### 2.3 脸身整合 (优先级: P0)

#### 技术方案: 统一参数化模型

```
┌──────────┐     ┌──────────┐     ┌──────────────────────────┐
│ 脸部参数 │     │ 身体参数 │     │                          │
│ (来自AI) │────▶│ (预设)   │────▶│    完整角色模型          │
└──────────┘     └──────────┘     │  • 一个连续 Mesh         │
                                  │  • 脖子区域平滑过渡      │
                                  │  • 肤色一致性            │
                                  │  • 统一骨骼              │
                                  └──────────────────────────┘
```

#### 关键技术点:
1. **Neck Ring**: 脸部和身体共享顶点环，保证几何连续
2. **肤色混合**: 自动匹配照片肤色到身体
3. **LOD 系统**: 不同距离使用不同精度模型
4. **统一骨骼**: 标准 Humanoid Rig (65 根骨骼)

---

### 2.4 服装系统 (优先级: P1)

#### 2.4.1 服装库结构

```
服装分类:
├── 按风格
│   ├── 西幻 (Fantasy)
│   │   ├── 骑士
│   │   ├── 法师
│   │   └── 刺客
│   ├── 中式 (Chinese)
│   │   ├── 汉服
│   │   ├── 武侠
│   │   └── 现代中式
│   ├── 现代 (Modern)
│   │   ├── 休闲
│   │   ├── 正装
│   │   └── 运动
│   └── 科幻 (Sci-Fi)
│       ├── 赛博朋克
│       └── 太空服
│
└── 按部位
    ├── 头部 (帽子/头盔/发饰)
    ├── 上身
    │   ├── 内衣层
    │   ├── 衬衫层
    │   ├── 外套层
    │   └── 披风层
    ├── 下身 (裤子/裙子)
    ├── 鞋子
    └── 配饰 (眼镜/耳环/项链/手表)
```

#### 2.4.2 服装适配

- 每件服装绑定到标准骨骼
- 自动适应不同体型 (通过 BlendShape)
- 颜色/材质可调整
- 支持多层穿戴

#### 2.4.3 照片→服装 (P2, 进阶功能)

**阶段 A**: 颜色/图案提取
```
服装照片 → AI分割提取服装 → 提取颜色/图案 → 应用到现有3D服装模板
```

**阶段 B**: 服装类型识别
- AI 识别服装类型 (T恤/衬衫/夹克...)
- 自动匹配最相似的 3D 模板
- 应用颜色/图案

**阶段 C**: 3D 服装生成 (研究性质)
- 从照片直接生成 3D 网格

---

### 2.5 动画系统 (优先级: P1)

#### 2.5.1 姿势 & 动画库

| 类别 | 内容 |
|------|------|
| 基础姿势 | T-Pose, A-Pose, 站立, 坐姿 |
| 移动动画 | 行走, 跑步, 跳跃, 蹲下 |
| 交互动画 | 挥手, 点头, 握手 |
| 战斗动画 | 攻击, 格挡, 受击 |
| 情绪动画 | 高兴, 悲伤, 愤怒 |
| 舞蹈动画 | 各种舞蹈 |

#### 2.5.2 动画重定向

```
源骨骼 ──▶ 骨骼映射 ──▶ 目标骨骼

• 支持不同骨骼结构的动画复用
• 自动适应不同身材比例
• IK 修正 (脚不穿地)
```

#### 2.5.3 视频→动作捕捉 (P2)

```
视频 ──▶ AI姿态估计 ──▶ 骨骼动画 ──▶ 重定向到角色
        (MediaPipe/OpenPose)   (BVH/FBX)
```

---

## 三、技术架构

### 3.1 核心数据结构

#### BlendShape 系统

```cpp
// BlendShape 目标 - 存储形变数据
struct BlendShapeTarget {
    std::string name;                     // 如 "smile", "eyeWide"
    std::vector<BlendShapeDelta> deltas;  // 顶点偏移
};

struct BlendShapeDelta {
    uint32_t vertexIndex;                 // 受影响的顶点索引
    Vec3 positionDelta;                   // 位置偏移
    Vec3 normalDelta;                     // 法线偏移
    Vec3 tangentDelta;                    // 切线偏移 (可选)
};

// BlendShape 通道 - 控制权重
struct BlendShapeChannel {
    std::string name;
    float weight = 0.0f;                  // 当前权重 [0, 1]
    std::vector<int> targetIndices;       // 关联的 target 索引
    std::vector<float> targetWeights;     // 各 target 的混合权重
};
```

#### 角色参数

```cpp
struct CharacterParams {
    // 身体基础参数
    Gender gender = Gender::Male;
    float age = 0.5f;           // 0=幼年, 1=老年
    float height = 0.5f;        // 0=矮, 1=高
    float weight = 0.5f;        // 0=瘦, 1=胖
    
    // 身体细节
    BodyDetailParams bodyDetails;
    
    // 脸部参数 (来自 AI 或手动调整)
    FaceParams faceParams;
    
    // 肤色
    Vec3 skinColor = {0.8f, 0.6f, 0.5f};
};
```

### 3.2 GPU 加速

BlendShape 混合在 GPU 上通过 Compute Shader 实现:

```metal
// BlendShape Compute Shader (Metal)
kernel void applyBlendShapes(
    device const float3* basePositions [[buffer(0)]],
    device const float3* baseNormals   [[buffer(1)]],
    device float3* outPositions        [[buffer(2)]],
    device float3* outNormals          [[buffer(3)]],
    device const BlendShapeData* blendData [[buffer(4)]],
    constant BlendShapeUniforms& uniforms [[buffer(5)]],
    uint vid [[thread_position_in_grid]])
{
    float3 pos = basePositions[vid];
    float3 nor = baseNormals[vid];
    
    // 遍历所有激活的 BlendShape
    for (uint i = 0; i < uniforms.activeCount; i++) {
        float weight = blendData[i].weight;
        if (weight > 0.001f) {
            pos += blendData[i].deltas[vid].position * weight;
            nor += blendData[i].deltas[vid].normal * weight;
        }
    }
    
    outPositions[vid] = pos;
    outNormals[vid] = normalize(nor);
}
```

### 3.3 AI 模型集成

| 功能 | 模型 | 部署方式 |
|------|------|---------|
| 人脸检测 | MediaPipe Face | ONNX Runtime |
| 特征点提取 | MediaPipe Face Mesh | ONNX Runtime |
| 3D 重建 | DECA / EMOCA | ONNX Runtime |
| 姿态估计 | MediaPipe Pose | ONNX Runtime |
| 服装分割 | SAM / Segment Anything | ONNX Runtime |

---

## 四、实施路线图

### Phase 1: MVP (最小可用产品)

```
阶段 1.1: 基础框架 ✅
├── [x] BlendShape 系统核心 (blend_shape.h)
├── [x] BlendShape GPU 计算 (blend_shape.metal)
├── [x] 身体参数系统 (character_body.h)
├── [x] 脸部参数系统 (character_face.h)
├── [x] 角色整合系统 (character.h)
├── [x] 角色创建 UI (character_creator_ui.h)
├── [ ] 基础人体模型 (待集成开源资产)
└── [ ] 预设保存/加载测试

阶段 1.2: 照片→脸部 ✅
├── [x] 集成 ONNX Runtime (ai_inference.h)
├── [x] 集成人脸检测 (FaceDetector)
├── [x] 集成 Face Mesh (FaceMeshEstimator - 468点)
├── [x] 集成 3DMM 模型 (Face3DMMRegressor - FLAME兼容)
├── [x] 参数→BlendShape 映射 (FaceParameterMapper)
├── [x] 脸部贴图提取 (FaceTextureExtractor)
└── [x] 完整 Pipeline (PhotoToFacePipeline)

阶段 1.3: 整合
├── [ ] 脸部-身体无缝连接
├── [ ] 肤色统一
└── [ ] 完整角色导出 (glTF)

✓ MVP 完成: 照片生成可编辑的 3D 人物
```

### Phase 2: V1.0 (完整产品)

```
阶段 2.1: 服装库系统
├── [ ] 服装数据格式定义
├── [ ] 服装适配引擎
├── [ ] 多层穿戴系统
├── [ ] 颜色/材质编辑器
└── [ ] 首批服装资产

阶段 2.2: 基础动画
├── [ ] 姿势预设库
├── [ ] 基础动画包
└── [ ] 动画播放器

✓ V1.0 完成: 可创建、穿衣、摆姿势的角色
```

### Phase 3: V2.0 (增值功能)

```
阶段 3.1: 高级动画
├── [ ] 动画重定向系统
├── [ ] 视频→动作捕捉
└── [ ] 更多动画包

阶段 3.2: 照片→服装
├── [ ] 服装分割 AI
├── [ ] 颜色/图案提取
└── [ ] 服装类型匹配

阶段 3.3: 资产商店
├── [ ] 资产打包系统
├── [ ] 资产商店 UI
└── [ ] 付费/订阅系统

✓ V2.0 完成: 完整商业产品
```

---

## 五、商业模式

### 5.1 免费功能
- 基础角色创建
- 身体预设
- 基础服装 (5-10 件)
- 基础动画 (5-10 个)

### 5.2 付费内容

| 内容类型 | 定价策略 |
|----------|----------|
| 服装包 | $5-15 / 包 |
| 配饰包 | $3-8 / 包 |
| 动画包 | $5-20 / 包 |
| 发型包 | $5-10 / 包 |
| 高级功能 | 订阅 $10-30 / 月 |

### 5.3 目标用户
- 独立游戏开发者
- 影视预览 / 分镜
- VTuber / 虚拟形象
- 3D 打印爱好者
- 教育培训

---

## 六、技术依赖

### 6.1 第三方库

| 库 | 用途 | 协议 |
|----|------|------|
| ONNX Runtime | AI 模型推理 | MIT |
| MediaPipe | 人脸/姿态检测 | Apache 2.0 |
| stb_image | 图像加载 | Public Domain |
| tinygltf | glTF 导出 | MIT |

### 6.2 开源资产 (候选)

| 资产 | 来源 | 协议 |
|------|------|------|
| 人体基础模型 | MakeHuman | CC0 |
| BlendShape 数据 | MakeHuman | AGPL (需评估) |
| 面部重建模型 | DECA/FLAME | 学术协议 |

---

## 七、更新日志

| 日期 | 版本 | 内容 |
|------|------|------|
| 2026-01-23 | 1.0 | 初始文档创建 |
| 2026-01-23 | 1.1 | 完成基础框架实现 (BlendShape系统, 身体/脸部参数, 角色整合, UI) |
| 2026-01-23 | 1.2 | 完成人体模型加载器和程序化生成, AI推理引擎, 照片→脸部完整Pipeline |

---

## 八、附录

### A. 标准骨骼结构 (Humanoid Rig)

```
Root
├── Hips
│   ├── Spine
│   │   ├── Spine1
│   │   │   ├── Spine2
│   │   │   │   ├── Neck
│   │   │   │   │   └── Head
│   │   │   │   │       ├── LeftEye
│   │   │   │   │       ├── RightEye
│   │   │   │   │       └── Jaw
│   │   │   │   ├── LeftShoulder
│   │   │   │   │   └── LeftArm
│   │   │   │   │       └── LeftForeArm
│   │   │   │   │           └── LeftHand (+ fingers)
│   │   │   │   └── RightShoulder
│   │   │   │       └── RightArm
│   │   │   │           └── RightForeArm
│   │   │   │               └── RightHand (+ fingers)
│   ├── LeftUpLeg
│   │   └── LeftLeg
│   │       └── LeftFoot
│   │           └── LeftToeBase
│   └── RightUpLeg
│       └── RightLeg
│           └── RightFoot
│               └── RightToeBase
```

### B. ARKit 52 面部 BlendShape 列表

```
eyeBlinkLeft, eyeLookDownLeft, eyeLookInLeft, eyeLookOutLeft,
eyeLookUpLeft, eyeSquintLeft, eyeWideLeft, eyeBlinkRight,
eyeLookDownRight, eyeLookInRight, eyeLookOutRight, eyeLookUpRight,
eyeSquintRight, eyeWideRight, jawForward, jawLeft, jawRight,
jawOpen, mouthClose, mouthFunnel, mouthPucker, mouthLeft,
mouthRight, mouthSmileLeft, mouthSmileRight, mouthFrownLeft,
mouthFrownRight, mouthDimpleLeft, mouthDimpleRight, mouthStretchLeft,
mouthStretchRight, mouthRollLower, mouthRollUpper, mouthShrugLower,
mouthShrugUpper, mouthPressLeft, mouthPressRight, mouthLowerDownLeft,
mouthLowerDownRight, mouthUpperUpLeft, mouthUpperUpRight,
browDownLeft, browDownRight, browInnerUp, browOuterUpLeft,
browOuterUpRight, cheekPuff, cheekSquintLeft, cheekSquintRight,
noseSneerLeft, noseSneerRight, tongueOut
```

### C. 参考竞品

| 产品 | 特点 | 定价 |
|------|------|------|
| MetaHuman | 高质量写实人物 | 免费 (UE 生态) |
| Character Creator 4 | 全功能角色创建 | $199-399 |
| DAZ3D | 丰富资产生态 | 免费 + 资产付费 |
| VRoid Studio | 动漫风格 | 免费 |
| Ready Player Me | Web 快速创建 | API 付费 |
