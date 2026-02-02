# LUMA 插件开发指南

本文档介绍如何为 LUMA 创建第三方插件，包括角色模板、服装、发型等内容扩展。

## 目录

- [插件类型](#插件类型)
- [插件包结构](#插件包结构)
- [Manifest 配置](#manifest-配置)
- [创建服装插件](#创建服装插件)
- [创建发型插件](#创建发型插件)
- [创建角色模板插件](#创建角色模板插件)
- [资源规范](#资源规范)
- [发布插件](#发布插件)

---

## 插件类型

LUMA 支持以下类型的插件：

| 类型 | 说明 | 难度 |
|------|------|------|
| `clothing` | 服装（上衣、裤子、鞋子等） | ⭐ 简单 |
| `hair` | 发型 | ⭐ 简单 |
| `accessory` | 配饰（眼镜、帽子等） | ⭐ 简单 |
| `character_template` | 角色模板（如机器人、精灵） | ⭐⭐⭐ 高级 |
| `animation` | 动画剪辑 | ⭐⭐ 中等 |
| `expression` | 面部表情预设 | ⭐ 简单 |
| `material` | 自定义材质 | ⭐⭐ 中等 |
| `body_part` | 自定义身体部件 | ⭐⭐ 中等 |

---

## 插件包结构

插件以文件夹形式组织，放置在插件目录下：

```
my-awesome-plugin/
├── manifest.json          # 必需：插件元数据
├── thumbnail.png          # 推荐：预览图 (512x512)
├── README.md              # 推荐：说明文档
├── assets/
│   ├── meshes/           # 3D 模型文件
│   │   ├── item1.obj
│   │   ├── item2.fbx
│   │   └── item3.gltf
│   ├── textures/         # 贴图文件
│   │   ├── item1_diffuse.png
│   │   ├── item1_normal.png
│   │   └── item1_roughness.png
│   ├── materials/        # 材质定义 (可选)
│   │   └── item1.json
│   └── configs/          # 资源配置
│       ├── item1.json
│       └── item2.json
├── scripts/              # Lua 脚本 (可选，高级)
│   └── main.lua
└── lib/                  # 原生库 (可选，高级)
    ├── windows/
    │   └── plugin.dll
    ├── macos/
    │   └── plugin.dylib
    └── linux/
        └── plugin.so
```

### 插件目录位置

| 平台 | 路径 |
|------|------|
| Windows | `%APPDATA%/LUMA/plugins/` |
| macOS | `~/Library/Application Support/LUMA/plugins/` |
| Linux | `~/.local/share/luma/plugins/` |

---

## Manifest 配置

`manifest.json` 是插件的核心配置文件：

```json
{
  "id": "com.yourname.plugin-name",
  "name": "My Awesome Plugin",
  "description": "A collection of cool items",
  "author": "Your Name",
  "website": "https://yourwebsite.com",
  "license": "CC-BY-4.0",
  "version": "1.0.0",
  "minEngineVersion": "1.0.0",
  "type": "clothing",
  "thumbnail": "thumbnail.png",
  "tags": ["clothing", "modern", "casual"],
  "dependencies": []
}
```

### 字段说明

| 字段 | 必需 | 说明 |
|------|------|------|
| `id` | ✅ | 唯一标识符，建议使用反向域名格式 |
| `name` | ✅ | 显示名称 |
| `description` | ✅ | 简短描述 |
| `author` | ✅ | 作者名称 |
| `version` | ✅ | 版本号 (语义化版本) |
| `type` | ✅ | 插件类型 |
| `website` | ❌ | 作者网站 |
| `license` | ❌ | 许可证 |
| `minEngineVersion` | ❌ | 最低 LUMA 版本要求 |
| `thumbnail` | ❌ | 预览图路径 |
| `tags` | ❌ | 搜索标签 |
| `dependencies` | ❌ | 依赖的其他插件 ID |

---

## 创建服装插件

### 1. 准备 3D 模型

- **格式**: OBJ, FBX, glTF (推荐 glTF)
- **朝向**: Y-up, 面向 +Z
- **单位**: 米
- **原点**: 模型中心或挂载点
- **UV**: 0-1 范围内

### 2. 准备贴图

- **Diffuse/Albedo**: `item_diffuse.png`
- **Normal Map**: `item_normal.png`
- **Roughness**: `item_roughness.png` (或 ORM 贴图)
- **分辨率**: 1024x1024 或 2048x2048

### 3. 创建资源配置

`assets/configs/tshirt.json`:

```json
{
  "id": "tshirt_striped",
  "name": "Striped T-Shirt",
  "category": "tops",
  "description": "A casual striped t-shirt",
  "mesh": "assets/meshes/tshirt.obj",
  "texture": "assets/textures/tshirt_diffuse.png",
  "normalMap": "assets/textures/tshirt_normal.png",
  "thumbnail": "assets/thumbnails/tshirt.png",
  "slot": "chest",
  "conflictingSlots": [],
  "supportedBodyTypes": ["male", "female"],
  "hasSkinning": true,
  "hasPhysics": false,
  "tags": ["casual", "striped", "cotton"]
}
```

### 4. 服装插槽说明

| Slot | 说明 | 冲突 |
|------|------|------|
| `head` | 帽子、头盔 | - |
| `face` | 面具、眼镜 | - |
| `chest` | 上衣、外套 | - |
| `legs` | 裤子、裙子 | - |
| `feet` | 鞋子、靴子 | - |
| `hands` | 手套 | - |
| `full_body` | 连体衣、裙子 | chest, legs |

---

## 创建发型插件

### 1. 发型模型规范

- **多边形数**: 建议 5000-20000 面
- **发丝**: 使用 Hair Cards 或 Hair Strips
- **原点**: 头顶中心
- **UV**: 用于头发纹理和渐变

### 2. 发型配置

`assets/configs/ponytail.json`:

```json
{
  "id": "ponytail_high",
  "name": "High Ponytail",
  "category": "long",
  "description": "A high ponytail hairstyle",
  "mesh": "assets/meshes/ponytail.obj",
  "texture": "assets/textures/hair_texture.png",
  "thumbnail": "assets/thumbnails/ponytail.png",
  "defaultColor": [0.2, 0.15, 0.1],
  "supportsColorChange": true,
  "hasPhysics": true,
  "physicsSettings": {
    "stiffness": 0.5,
    "damping": 0.3,
    "gravity": 1.0
  },
  "attachBone": "head",
  "offset": [0, 0.1, -0.05],
  "tags": ["long", "ponytail", "feminine"]
}
```

### 3. 发型类别

| Category | 说明 |
|----------|------|
| `bald` | 光头/平头 |
| `short` | 短发 |
| `medium` | 中长发 |
| `long` | 长发 |
| `updo` | 盘发 |

---

## 创建角色模板插件

角色模板插件较复杂，需要定义骨骼、网格生成逻辑和参数。

### 方式 1: 纯资源模板

适合基于现有模型的模板：

```json
{
  "id": "com.artist.elf-template",
  "name": "Elf Character",
  "type": "character_template",
  ...
}
```

`assets/configs/elf_template.json`:

```json
{
  "id": "elf",
  "name": "Elf",
  "description": "Fantasy elf character with pointed ears",
  "baseMesh": "assets/meshes/elf_base.fbx",
  "skeleton": "assets/meshes/elf_base.fbx",
  "blendShapes": "assets/meshes/elf_blendshapes.fbx",
  "defaultParams": {
    "height": 1.85,
    "primaryColor": [0.95, 0.9, 0.85],
    "earPointiness": 0.8
  },
  "customizableParams": [
    {
      "name": "earPointiness",
      "displayName": "Ear Pointiness",
      "min": 0.0,
      "max": 1.0,
      "default": 0.8
    },
    {
      "name": "eyeSize",
      "displayName": "Eye Size",
      "min": 0.8,
      "max": 1.2,
      "default": 1.1
    }
  ]
}
```

### 方式 2: 代码模板 (高级)

对于需要程序化生成的模板，使用 C++ 插件：

```cpp
// my_plugin.cpp
#include <luma/plugin/plugin_system.h>

class ElfTemplatePlugin : public luma::ICharacterTemplatePlugin {
public:
    // 实现接口方法...
    
    CharacterTemplatePluginResult createCharacter(
        const CharacterTemplatePluginParams& params) override 
    {
        // 程序化生成精灵角色
    }
};

LUMA_PLUGIN_EXPORT(ElfTemplatePlugin)
```

编译为动态库放入 `lib/` 目录。

---

## 资源规范

### 3D 模型

| 项目 | 规范 |
|------|------|
| 格式 | glTF 2.0 (推荐), FBX, OBJ |
| 坐标系 | Y-up, 面向 +Z |
| 单位 | 米 |
| 面数 | 服装 < 10K, 角色 < 50K |
| UV | 0-1 范围，无重叠 |

### 贴图

| 类型 | 格式 | 分辨率 | 说明 |
|------|------|--------|------|
| Diffuse | PNG/JPG | 1024-2048 | sRGB 颜色空间 |
| Normal | PNG | 1024-2048 | 切线空间, OpenGL 格式 |
| Roughness | PNG | 512-1024 | 线性, 灰度图 |
| Metallic | PNG | 512-1024 | 线性, 灰度图 |
| ORM | PNG | 1024-2048 | R=AO, G=Roughness, B=Metallic |

### 缩略图

- **格式**: PNG
- **分辨率**: 512x512
- **背景**: 透明或纯色

---

## 发布插件

### 1. 打包

将插件文件夹压缩为 `.zip` 并重命名为 `.lumapkg`：

```bash
cd my-plugin
zip -r ../my-plugin.lumapkg .
```

### 2. 测试

将插件放入本地插件目录测试：

```
~/Library/Application Support/LUMA/plugins/my-plugin/
```

启动 LUMA，检查插件是否正确加载。

### 3. 发布渠道

- **LUMA 插件商店** (即将推出)
- **GitHub Releases**
- **个人网站**

### 4. 许可证建议

| 许可证 | 适用场景 |
|--------|----------|
| CC-BY | 允许商用，需署名 |
| CC-BY-NC | 非商用，需署名 |
| CC0 | 公有领域，无限制 |
| MIT | 代码插件 |

---

## 调试技巧

### 查看加载日志

```cpp
// 在应用启动时
auto& pm = luma::getPluginManager();
pm.addListener(std::make_shared<DebugListener>());

// 发现所有插件
auto plugins = pm.discoverPlugins();
for (const auto& meta : plugins) {
    std::cout << "Found: " << meta.name << " v" << meta.version.toString() << std::endl;
}
```

### 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| Plugin not found | manifest.json 缺失或格式错误 | 检查 JSON 语法 |
| Missing dependency | 依赖插件未安装 | 安装依赖 |
| Asset load failed | 模型路径错误 | 检查相对路径 |
| Invalid version | 版本号格式错误 | 使用 X.Y.Z 格式 |

---

## API 参考

### PluginManager

```cpp
auto& pm = luma::getPluginManager();

// 添加插件目录
pm.addPluginDirectory("/path/to/plugins");

// 发现插件
auto discovered = pm.discoverPlugins();

// 加载插件
auto result = pm.loadPlugin("com.example.plugin");
if (result.success) {
    auto plugin = result.plugin;
}

// 按类型获取
auto hairPlugins = pm.getPluginsByType(luma::PluginType::Hair);

// 搜索资源
auto results = pm.searchAssets("striped", luma::PluginType::Clothing);
```

### IPlugin 接口

```cpp
class IPlugin {
    virtual const PluginMetadata& getMetadata() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual std::vector<PluginAsset> getAssets() const = 0;
    virtual const PluginAsset* getAsset(const std::string& id) const = 0;
};
```

---

## 示例插件

完整示例请参考 `engine/plugin/plugin_examples.h`：

- `ExampleClothingPlugin` - 服装插件示例
- `ExampleHairPlugin` - 发型插件示例
- `ExampleRobotTemplatePlugin` - 角色模板示例

---

## 联系与支持

- **文档**: https://luma.dev/docs/plugins
- **GitHub**: https://github.com/luma/luma
- **Discord**: https://discord.gg/luma

---

*Happy Creating!* 🎨
