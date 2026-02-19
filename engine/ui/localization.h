// LUMA Localization System
// Multi-language support for UI text
#pragma once

#include <string>
#include <unordered_map>

namespace luma {
namespace ui {

// Supported languages
enum class Language {
    Chinese,    // 简体中文
    English,    // English
};

// Localization manager - singleton
class Localization {
public:
    static Localization& instance() {
        static Localization inst;
        return inst;
    }
    
    // Get/Set current language
    Language getLanguage() const { return currentLanguage_; }
    void setLanguage(Language lang) { currentLanguage_ = lang; }
    
    // Get localized string by key
    const char* get(const char* key) const {
        auto& strings = (currentLanguage_ == Language::Chinese) ? chinese_ : english_;
        auto it = strings.find(key);
        if (it != strings.end()) {
            return it->second.c_str();
        }
        return key;  // Fallback to key itself
    }
    
    // Shorthand
    const char* operator()(const char* key) const { return get(key); }
    
private:
    Localization() {
        initStrings();
    }
    
    void initStrings() {
        // ========== Menu ==========
        chinese_["File"] = "文件";
        chinese_["Edit"] = "编辑";
        chinese_["View"] = "视图";
        chinese_["Window"] = "窗口";
        chinese_["Help"] = "帮助";
        
        chinese_["New Scene"] = "新建场景";
        chinese_["Open..."] = "打开...";
        chinese_["Save"] = "保存";
        chinese_["Save As..."] = "另存为...";
        chinese_["Import Model..."] = "导入模型...";
        chinese_["Export..."] = "导出...";
        chinese_["Exit"] = "退出";
        
        chinese_["Undo"] = "撤销";
        chinese_["Redo"] = "重做";
        chinese_["Cut"] = "剪切";
        chinese_["Copy"] = "复制";
        chinese_["Paste"] = "粘贴";
        chinese_["Delete"] = "删除";
        chinese_["Duplicate"] = "复制对象";
        chinese_["Select All"] = "全选";
        
        // ========== Toolbar ==========
        chinese_["Move"] = "移动";
        chinese_["Rotate"] = "旋转";
        chinese_["Scale"] = "缩放";
        chinese_["Local"] = "本地";
        chinese_["World"] = "世界";
        chinese_["Snap"] = "吸附";
        
        // ========== Mode Bar ==========
        chinese_["Mode:"] = "模式:";
        chinese_["Scene"] = "场景";
        chinese_["Character"] = "角色";
        chinese_["Edit"] = "编辑";
        chinese_["Animation"] = "动画";
        chinese_["Play"] = "互动";
        chinese_["Project:"] = "项目:";
        
        // ========== Panels ==========
        chinese_["Hierarchy"] = "层级";
        chinese_["Inspector"] = "检查器";
        chinese_["Assets"] = "资源";
        chinese_["Console"] = "控制台";
        chinese_["Browser"] = "浏览";
        chinese_["Cache"] = "缓存";
        
        // ========== Inspector ==========
        chinese_["Transform"] = "变换";
        chinese_["Position"] = "位置";
        chinese_["Rotation"] = "旋转";
        chinese_["Scale"] = "缩放";
        chinese_["Reset"] = "重置";
        chinese_["Model"] = "模型";
        chinese_["Name"] = "名称";
        chinese_["Meshes"] = "网格数";
        chinese_["Vertices"] = "顶点数";
        chinese_["Triangles"] = "三角形";
        chinese_["Textures"] = "贴图数";
        chinese_["Material"] = "材质";
        
        // ========== Material Editor ==========
        chinese_["Base Color"] = "基础颜色";
        chinese_["Metallic"] = "金属度";
        chinese_["Roughness"] = "粗糙度";
        chinese_["Emission"] = "自发光";
        chinese_["Diffuse Map"] = "漫反射贴图";
        chinese_["Normal Map"] = "法线贴图";
        chinese_["Specular Map"] = "高光贴图";
        chinese_["Loaded"] = "已加载";
        chinese_["None"] = "无";
        
        // ========== Welcome Screen ==========
        chinese_["LUMA Studio"] = "LUMA Studio";
        chinese_["Real-time 3D Creation Platform"] = "实时3D创作平台";
        chinese_["New Scene"] = "新建场景";
        chinese_["Create empty scene"] = "创建空白场景";
        chinese_["Open Project"] = "打开项目";
        chinese_["Open .luma file"] = "打开 .luma 文件";
        chinese_["Quick Start"] = "快速开始";
        chinese_["From preset"] = "从预设开始";
        chinese_["Recent Projects"] = "最近项目";
        chinese_["Don't show on startup"] = "启动时不再显示";
        chinese_["Skip"] = "跳过";
        
        // ========== Scene Presets ==========
        chinese_["Select Preset"] = "选择场景预设";
        chinese_["Studio"] = "摄影棚";
        chinese_["Outdoor Park"] = "户外公园";
        chinese_["Medieval Castle"] = "中世纪城堡";
        chinese_["Sci-Fi Spaceship"] = "科幻太空船";
        
        // ========== Empty Scene Guide ==========
        chinese_["Start Creating"] = "开始创作";
        chinese_["Create Character"] = "创建角色";
        chinese_["Add Object"] = "添加物体";
        chinese_["Import Model"] = "导入模型";
        chinese_["Or"] = "或者";
        chinese_["Start from preset:"] = "从场景预设开始:";
        chinese_["Tip: Right-click viewport or click [+ Add] to add objects"] = "提示: 右键视口或点击 [+ 添加] 开始添加对象";
        
        // ========== Edit Mode ==========
        chinese_["Edit Mode - Inspector"] = "编辑模式 - 检查器";
        chinese_["Exit"] = "退出";
        chinese_["Mesh List"] = "网格列表";
        chinese_["Material Properties"] = "材质属性";
        chinese_["PBR Material Parameters"] = "PBR 材质参数";
        chinese_["No editable mesh"] = "无可编辑的网格";
        chinese_["Select a mesh to edit material"] = "选择一个网格以编辑材质";
        chinese_["indices"] = "索引";
        
        // Selection Mode
        chinese_["Selection Mode"] = "选择模式";
        chinese_["Vertex"] = "顶点";
        chinese_["Edge"] = "边";
        chinese_["Face"] = "面";
        
        // Edit Tools
        chinese_["Edit Tools"] = "编辑工具";
        chinese_["Select"] = "选择";
        chinese_["Move"] = "移动";
        chinese_["Rotate"] = "旋转";
        chinese_["Extrude"] = "挤出";
        
        // Wireframe Display
        chinese_["Wireframe Display"] = "线框显示";
        chinese_["Display Options"] = "显示选项";
        chinese_["Show Quad Edges"] = "显示四边面边";
        chinese_["Show All Edges"] = "显示所有边";
        chinese_["Show Vertices"] = "显示顶点";
        chinese_["Show original quad/ngon edges (hide triangulation)"] = "显示原始四边面/多边面边（隐藏三角化边）";
        chinese_["Show all edges including triangulation"] = "显示所有边包括三角化边";
        chinese_["Show vertex points in vertex mode"] = "在顶点模式下显示顶点点";
        
        // Select Tool
        chinese_["Select Tool"] = "选择工具";
        chinese_["Click"] = "点选";
        chinese_["Box"] = "框选";
        chinese_["Circle"] = "圆选";
        chinese_["Lasso"] = "套索";
        chinese_["Click to select (W)"] = "点击选择 (W)";
        chinese_["Box select (B)"] = "框选 (B)";
        chinese_["Circle select (C)"] = "圆形选择 (C)";
        chinese_["Lasso select (L)"] = "套索选择 (L)";
        chinese_["Click Select (W)"] = "点选 (W)";
        chinese_["Box Select (B)"] = "框选 (B)";
        chinese_["Circle Select (C)"] = "圆选 (C)";
        chinese_["Lasso Select (L)"] = "套索 (L)";
        chinese_["Mat"] = "材质";
        
        // Mesh Statistics
        chinese_["Mesh Statistics"] = "网格统计";
        chinese_["Faces"] = "面数";
        chinese_["Quads"] = "四边面";
        chinese_["N-gons"] = "多边面";
        
        // Save/Cancel
        chinese_["Save & Exit"] = "保存并退出";
        chinese_["Cancel"] = "取消";
        chinese_["Save changes and return to Scene mode"] = "保存修改并返回场景模式";
        chinese_["Discard changes and return to Scene mode"] = "放弃修改并返回场景模式";
        chinese_["* Unsaved changes"] = "* 有未保存的修改";
        
        // ========== UV Editor ==========
        chinese_["UV Editor"] = "UV 编辑器";
        chinese_["UV"] = "UV";
        chinese_["Select All"] = "全选";
        chinese_["Select None"] = "取消选择";
        chinese_["Pack Islands"] = "打包 UV 岛";
        chinese_["Projection"] = "投影";
        chinese_["Planar (Top)"] = "平面投影（顶部）";
        chinese_["Planar (Front)"] = "平面投影（正面）";
        chinese_["Planar (Side)"] = "平面投影（侧面）";
        chinese_["Box Projection"] = "立方体投影";
        chinese_["Cylindrical"] = "圆柱投影";
        chinese_["Spherical"] = "球形投影";
        chinese_["View"] = "视图";
        chinese_["Show Texture"] = "显示纹理";
        chinese_["Show Grid"] = "显示网格";
        chinese_["Show UV Bounds"] = "显示 UV 边界";
        chinese_["Highlight Stretch"] = "高亮拉伸";
        chinese_["Highlight Overlap"] = "高亮重叠";
        chinese_["Reset View"] = "重置视图";
        chinese_["Box"] = "立方体";
        chinese_["Planar"] = "平面";
        chinese_["Unwrap"] = "展开";
        chinese_["Flip H"] = "水平翻转";
        chinese_["Flip V"] = "垂直翻转";
        chinese_["Rotate 90"] = "旋转90°";
        chinese_["Fit"] = "适应";
        chinese_["Selected"] = "已选择";
        chinese_["Undo"] = "撤销";
        chinese_["Redo"] = "重做";
        chinese_["Live Preview"] = "实时预览";
        chinese_["Color:"] = "颜色:";
        
        // ========== Tooltips ==========
        chinese_["Please select a character object"] = "请先选择角色对象";
        chinese_["Please select an object"] = "请先选择对象";
        chinese_["Selected object has no skeleton"] = "选中对象没有骨骼";
        chinese_["0 = Non-metal (plastic, wood)\n1 = Pure metal (gold, silver, copper)"] = "0 = 非金属 (塑料、木材)\n1 = 纯金属 (金、银、铜)";
        chinese_["0 = Smooth (specular reflection)\n1 = Rough (diffuse reflection)"] = "0 = 光滑 (镜面反射)\n1 = 粗糙 (漫反射)";
        
        // ========== Status ==========
        chinese_["Selected:"] = "已选择:";
        chinese_["Objects:"] = "对象:";
        chinese_["Visible:"] = "可见:";
        chinese_["FPS:"] = "帧率:";
        chinese_["Frame:"] = "帧时间:";
        
        // ========== Misc ==========
        chinese_["Search..."] = "搜索...";
        chinese_["Drop assets or entities here"] = "拖放资源或实体到这里";
        chinese_["Refresh"] = "刷新";
        chinese_["Path:"] = "路径:";
        chinese_["Cannot read directory"] = "无法读取目录";
        chinese_["Unnamed Scene"] = "未命名场景";
        chinese_["Loading textures..."] = "加载贴图中...";
        
        // ========== Language ==========
        chinese_["Language"] = "语言";
        chinese_["Chinese"] = "中文";
        chinese_["English"] = "English";
        
        // ========== Additional UI ==========
        chinese_["Screenshot"] = "截图";
        chinese_["Screenshot Settings..."] = "截图设置...";
        chinese_["History"] = "历史记录";
        chinese_["Panels"] = "面板";
        chinese_["Statistics"] = "统计";
        chinese_["Demo Scenes..."] = "演示场景...";
        chinese_["Shortcuts"] = "快捷键";
        chinese_["About"] = "关于";
        chinese_["No entity selected"] = "未选择对象";
        chinese_["Clear"] = "清除";
        chinese_["Auto-scroll"] = "自动滚动";
        
        // ========== Window Menu ==========
        chinese_["Rendering"] = "渲染";
        chinese_["Post-Processing"] = "后期处理";
        chinese_["Advanced Post-Process"] = "高级后期处理";
        chinese_["Advanced Shadows"] = "高级阴影";
        chinese_["Environment / IBL"] = "环境 / IBL";
        chinese_["Lighting"] = "光照";
        chinese_["LOD Settings"] = "LOD 设置";
        chinese_["Particle Editor"] = "粒子编辑器";
        chinese_["Physics Editor"] = "物理编辑器";
        chinese_["Terrain Editor"] = "地形编辑器";
        chinese_["Audio Editor"] = "音频编辑器";
        chinese_["GI Editor"] = "全局光照编辑器";
        chinese_["Video Export"] = "视频导出";
        chinese_["Network"] = "网络";
        chinese_["AI Editor"] = "AI 编辑器";
        chinese_["Game UI Editor"] = "游戏 UI 编辑器";
        chinese_["Scene Manager"] = "场景管理器";
        chinese_["Data Manager"] = "数据管理器";
        chinese_["Build Settings"] = "构建设置";
        chinese_["Tools"] = "工具";
        chinese_["Character Creator"] = "角色创建器";
        chinese_["Scripting"] = "脚本";
        chinese_["Visual Script"] = "可视化脚本";
        chinese_["Script Editor"] = "脚本编辑器";
        chinese_["Timeline"] = "时间线";
        chinese_["State Machine Editor"] = "状态机编辑器";
        chinese_["Blend Tree Editor"] = "混合树编辑器";
        chinese_["Animation Layers"] = "动画层";
        chinese_["IK Settings"] = "IK 设置";
        chinese_["Reset Layout"] = "重置布局";
        chinese_["Animation Timeline"] = "动画时间线";
        chinese_["Render Settings"] = "渲染设置";
        chinese_["Post Processing"] = "后期处理";
        
        // ========== Edit Mode View ==========
        chinese_["View Mode"] = "视图模式";
        chinese_["Solid"] = "实体";
        chinese_["Wire+"] = "线框+";
        chinese_["Wire"] = "线框";
        chinese_["Full PBR material rendering"] = "完整 PBR 材质渲染";
        chinese_["Solid gray shading (clay)"] = "实体灰色着色（黏土）";
        chinese_["Wireframe overlay"] = "线框叠加";
        chinese_["Wireframe only"] = "仅线框";
        chinese_["Material + wireframe overlay"] = "材质 + 线框叠加";
        chinese_["Wireframe only (no solid)"] = "仅线框（无实体）";
        chinese_["Show Wireframe Overlay"] = "显示线框叠加";
        chinese_["Selected Mesh"] = "选中网格";
        chinese_["Click mesh in list to highlight"] = "点击列表中的网格以高亮显示";
        
        // ========== English (same as keys for most) ==========
        english_["File"] = "File";
        english_["Edit"] = "Edit";
        english_["View"] = "View";
        english_["Window"] = "Window";
        english_["Help"] = "Help";
        english_["New Scene"] = "New Scene";
        english_["Open..."] = "Open...";
        english_["Save"] = "Save";
        english_["Save As..."] = "Save As...";
        english_["Import Model..."] = "Import Model...";
        english_["Export..."] = "Export...";
        english_["Exit"] = "Exit";
        english_["Undo"] = "Undo";
        english_["Redo"] = "Redo";
        english_["Cut"] = "Cut";
        english_["Copy"] = "Copy";
        english_["Paste"] = "Paste";
        english_["Delete"] = "Delete";
        english_["Duplicate"] = "Duplicate";
        english_["Select All"] = "Select All";
        english_["Move"] = "Move";
        english_["Rotate"] = "Rotate";
        english_["Scale"] = "Scale";
        english_["Local"] = "Local";
        english_["World"] = "World";
        english_["Snap"] = "Snap";
        english_["Mode:"] = "Mode:";
        english_["Scene"] = "Scene";
        english_["Character"] = "Character";
        english_["Animation"] = "Animation";
        english_["Play"] = "Play";
        english_["Project:"] = "Project:";
        english_["Hierarchy"] = "Hierarchy";
        english_["Inspector"] = "Inspector";
        english_["Assets"] = "Assets";
        english_["Console"] = "Console";
        english_["Browser"] = "Browser";
        english_["Cache"] = "Cache";
        english_["Transform"] = "Transform";
        english_["Position"] = "Position";
        english_["Rotation"] = "Rotation";
        english_["Reset"] = "Reset";
        english_["Model"] = "Model";
        english_["Name"] = "Name";
        english_["Meshes"] = "Meshes";
        english_["Vertices"] = "Vertices";
        english_["Triangles"] = "Triangles";
        english_["Textures"] = "Textures";
        english_["Material"] = "Material";
        english_["Base Color"] = "Base Color";
        english_["Metallic"] = "Metallic";
        english_["Roughness"] = "Roughness";
        english_["Emission"] = "Emission";
        english_["Diffuse Map"] = "Diffuse Map";
        english_["Normal Map"] = "Normal Map";
        english_["Specular Map"] = "Specular Map";
        english_["Loaded"] = "Loaded";
        english_["None"] = "None";
        english_["Real-time 3D Creation Platform"] = "Real-time 3D Creation Platform";
        english_["Create empty scene"] = "Create empty scene";
        english_["Open Project"] = "Open Project";
        english_["Open .luma file"] = "Open .luma file";
        english_["Quick Start"] = "Quick Start";
        english_["From preset"] = "From preset";
        english_["Recent Projects"] = "Recent Projects";
        english_["Don't show on startup"] = "Don't show on startup";
        english_["Skip"] = "Skip";
        english_["Select Preset"] = "Select Preset";
        english_["Studio"] = "Studio";
        english_["Outdoor Park"] = "Outdoor Park";
        english_["Medieval Castle"] = "Medieval Castle";
        english_["Sci-Fi Spaceship"] = "Sci-Fi Spaceship";
        english_["Start Creating"] = "Start Creating";
        english_["Create Character"] = "Create Character";
        english_["Add Object"] = "Add Object";
        english_["Import Model"] = "Import Model";
        english_["Or"] = "Or";
        english_["Start from preset:"] = "Start from preset:";
        english_["Tip: Right-click viewport or click [+ Add] to add objects"] = "Tip: Right-click viewport or click [+ Add] to add objects";
        english_["Edit Mode - Inspector"] = "Edit Mode - Inspector";
        english_["Mesh List"] = "Mesh List";
        english_["Material Properties"] = "Material Properties";
        english_["PBR Material Parameters"] = "PBR Material Parameters";
        english_["No editable mesh"] = "No editable mesh";
        english_["Select a mesh to edit material"] = "Select a mesh to edit material";
        english_["indices"] = "indices";
        english_["Live Preview"] = "Live Preview";
        english_["Color:"] = "Color:";
        english_["Please select a character object"] = "Please select a character object";
        english_["Please select an object"] = "Please select an object";
        english_["Selected object has no skeleton"] = "Selected object has no skeleton";
        english_["0 = Non-metal (plastic, wood)\n1 = Pure metal (gold, silver, copper)"] = "0 = Non-metal (plastic, wood)\n1 = Pure metal (gold, silver, copper)";
        english_["0 = Smooth (specular reflection)\n1 = Rough (diffuse reflection)"] = "0 = Smooth (specular reflection)\n1 = Rough (diffuse reflection)";
        english_["Selected:"] = "Selected:";
        english_["Objects:"] = "Objects:";
        english_["Visible:"] = "Visible:";
        english_["FPS:"] = "FPS:";
        english_["Frame:"] = "Frame:";
        english_["Search..."] = "Search...";
        english_["Drop assets or entities here"] = "Drop assets or entities here";
        english_["Refresh"] = "Refresh";
        english_["Path:"] = "Path:";
        english_["Cannot read directory"] = "Cannot read directory";
        english_["Unnamed Scene"] = "Unnamed Scene";
        english_["Loading textures..."] = "Loading textures...";
        english_["Language"] = "Language";
        english_["Chinese"] = "中文";
        english_["English"] = "English";
        
        // ========== Additional UI ==========
        english_["Screenshot"] = "Screenshot";
        english_["Screenshot Settings..."] = "Screenshot Settings...";
        english_["History"] = "History";
        english_["Panels"] = "Panels";
        english_["Statistics"] = "Statistics";
        english_["Demo Scenes..."] = "Demo Scenes...";
        english_["Shortcuts"] = "Shortcuts";
        english_["About"] = "About";
        english_["No entity selected"] = "No entity selected";
        english_["Clear"] = "Clear";
        english_["Auto-scroll"] = "Auto-scroll";
        
        // ========== Window Menu ==========
        english_["Rendering"] = "Rendering";
        english_["Post-Processing"] = "Post-Processing";
        english_["Advanced Post-Process"] = "Advanced Post-Process";
        english_["Advanced Shadows"] = "Advanced Shadows";
        english_["Environment / IBL"] = "Environment / IBL";
        english_["Lighting"] = "Lighting";
        english_["LOD Settings"] = "LOD Settings";
        english_["Particle Editor"] = "Particle Editor";
        english_["Physics Editor"] = "Physics Editor";
        english_["Terrain Editor"] = "Terrain Editor";
        english_["Audio Editor"] = "Audio Editor";
        english_["GI Editor"] = "GI Editor";
        english_["Video Export"] = "Video Export";
        english_["Network"] = "Network";
        english_["AI Editor"] = "AI Editor";
        english_["Game UI Editor"] = "Game UI Editor";
        english_["Scene Manager"] = "Scene Manager";
        english_["Data Manager"] = "Data Manager";
        english_["Build Settings"] = "Build Settings";
        english_["Tools"] = "Tools";
        english_["Character Creator"] = "Character Creator";
        english_["Scripting"] = "Scripting";
        english_["Visual Script"] = "Visual Script";
        english_["Script Editor"] = "Script Editor";
        english_["Timeline"] = "Timeline";
        english_["State Machine Editor"] = "State Machine Editor";
        english_["Blend Tree Editor"] = "Blend Tree Editor";
        english_["Animation Layers"] = "Animation Layers";
        english_["IK Settings"] = "IK Settings";
        english_["Reset Layout"] = "Reset Layout";
        english_["Animation Timeline"] = "Animation Timeline";
        english_["Render Settings"] = "Render Settings";
        english_["Post Processing"] = "Post Processing";
        
        // ========== Edit Mode View ==========
        english_["View Mode"] = "View Mode";
        english_["Solid"] = "Solid";
        english_["Wire+"] = "Wire+";
        english_["Wire"] = "Wire";
        english_["Full PBR material rendering"] = "Full PBR material rendering";
        english_["Solid gray shading (clay)"] = "Solid gray shading (clay)";
        english_["Wireframe overlay"] = "Wireframe overlay";
        english_["Wireframe only"] = "Wireframe only";
        english_["Material + wireframe overlay"] = "Material + wireframe overlay";
        english_["Wireframe only (no solid)"] = "Wireframe only (no solid)";
        english_["Show Wireframe Overlay"] = "Show Wireframe Overlay";
        english_["Selected Mesh"] = "Selected Mesh";
        english_["Click mesh in list to highlight"] = "Click mesh in list to highlight";
        
        // ========== Character Mode ==========
        english_["Character"] = "Character";
        english_["Character Mode - Inspector"] = "Character Mode - Inspector";
        english_["Create a character to begin"] = "Create a character to begin";
        english_["Create Character"] = "Create Character";
        english_["Style"] = "Style";
        english_["Realistic"] = "Realistic";
        english_["Stylized"] = "Stylized";
        english_["Anime"] = "Anime";
        english_["Cartoon"] = "Cartoon";
        english_["Chibi"] = "Chibi";
        english_["Creation Method"] = "Creation Method";
        english_["From Template"] = "From Template";
        english_["Create from a built-in human template"] = "Create from a built-in human template";
        english_["From Photo (AI)"] = "From Photo (AI)";
        english_["Upload a photo to auto-generate face"] = "Upload a photo to auto-generate face";
        english_["Random Generate"] = "Random Generate";
        english_["Generate a random character"] = "Generate a random character";
        english_["Blank Character"] = "Blank Character";
        english_["Start with default parameters"] = "Start with default parameters";
        english_["Face Presets"] = "Face Presets";
        english_["Overview"] = "Overview";
        english_["Body"] = "Body";
        english_["Face"] = "Face";
        english_["Hair"] = "Hair";
        english_["Clothing"] = "Clothing";
        english_["Expression"] = "Expression";
        english_["Export"] = "Export";
        english_["Back to Scene"] = "Back to Scene";
        english_["Randomize"] = "Randomize";
        english_["Reset to Default"] = "Reset to Default";
        english_["Reset All"] = "Reset All";
        english_["Body Customization"] = "Body Customization";
        english_["Gender"] = "Gender";
        english_["Male"] = "Male";
        english_["Female"] = "Female";
        english_["Proportions"] = "Proportions";
        english_["Height"] = "Height";
        english_["Weight"] = "Weight";
        english_["Muscularity"] = "Muscularity";
        english_["Upper Body"] = "Upper Body";
        english_["Shoulder Width"] = "Shoulder Width";
        english_["Chest"] = "Chest";
        english_["Waist"] = "Waist";
        english_["Arm Length"] = "Arm Length";
        english_["Lower Body"] = "Lower Body";
        english_["Hip Width"] = "Hip Width";
        english_["Leg Length"] = "Leg Length";
        english_["Skin Color"] = "Skin Color";
        english_["Skin Tone"] = "Skin Tone";
        english_["Body Presets"] = "Body Presets";
        english_["Athletic"] = "Athletic";
        english_["Slim"] = "Slim";
        english_["Average"] = "Average";
        english_["Heavy"] = "Heavy";
        english_["Muscular"] = "Muscular";
        english_["Face Sculpting"] = "Face Sculpting";
        english_["Overall"] = "Overall";
        english_["Forehead"] = "Forehead";
        english_["Eyes"] = "Eyes";
        english_["Brows"] = "Brows";
        english_["Nose"] = "Nose";
        english_["Mouth"] = "Mouth";
        english_["Chin"] = "Chin";
        english_["Jaw"] = "Jaw";
        english_["Cheeks"] = "Cheeks";
        english_["Ears"] = "Ears";
        english_["Appearance"] = "Appearance";
        english_["Eye Color"] = "Eye Color";
        english_["Lip Color"] = "Lip Color";
        english_["Wrinkles"] = "Wrinkles";
        english_["Freckles"] = "Freckles";
        english_["Skin Roughness"] = "Skin Roughness";
        english_["Hair Style"] = "Hair Style";
        english_["Hair Color"] = "Hair Color";
        english_["Short"] = "Short";
        english_["Medium"] = "Medium";
        english_["Long"] = "Long";
        english_["Updo"] = "Updo";
        english_["Bald"] = "Bald";
        english_["Top"] = "Top";
        english_["Bottom"] = "Bottom";
        english_["Shoes"] = "Shoes";
        english_["Accessory"] = "Accessory";
        english_["No items available"] = "No items available";
        english_["Intensity"] = "Intensity";
        english_["Preset Expressions"] = "Preset Expressions";
        english_["Neutral"] = "Neutral";
        english_["Smile"] = "Smile";
        english_["Frown"] = "Frown";
        english_["Surprise"] = "Surprise";
        english_["Angry"] = "Angry";
        english_["Advanced Blend Shapes"] = "Advanced Blend Shapes";
        english_["Export Character"] = "Export Character";
        english_["Export Format"] = "Export Format";
        english_["Apply to Scene"] = "Apply to Scene";
        english_["Update the character in the scene with current settings"] = "Update the character in the scene with current settings";
        english_["Vertices"] = "Vertices";
        english_["Triangles"] = "Triangles";
        english_["Name"] = "Name";
        english_["Rename"] = "Rename";
        english_["Import Photo for Face Generation"] = "Import Photo for Face Generation";
        english_["Processing"] = "Processing";
        english_["Face generation complete!"] = "Face generation complete!";
        english_["OK"] = "OK";
        english_["Select a photo file:"] = "Select a photo file:";
        english_["Generate"] = "Generate";
        english_["Cancel"] = "Cancel";
        
        // Chinese translations for character mode
        chinese_["Character"] = "角色";
        chinese_["Character Mode - Inspector"] = "角色模式 - 属性面板";
        chinese_["Create a character to begin"] = "创建一个角色以开始";
        chinese_["Create Character"] = "创建角色";
        chinese_["Style"] = "风格";
        chinese_["Realistic"] = "写实";
        chinese_["Stylized"] = "风格化";
        chinese_["Anime"] = "动漫";
        chinese_["Cartoon"] = "卡通";
        chinese_["Chibi"] = "Q版";
        chinese_["Creation Method"] = "创建方式";
        chinese_["From Template"] = "从模板创建";
        chinese_["Create from a built-in human template"] = "使用内置人体模板创建";
        chinese_["From Photo (AI)"] = "从照片生成 (AI)";
        chinese_["Upload a photo to auto-generate face"] = "上传照片自动生成面部";
        chinese_["Random Generate"] = "随机生成";
        chinese_["Generate a random character"] = "生成随机角色";
        chinese_["Blank Character"] = "空白角色";
        chinese_["Start with default parameters"] = "使用默认参数开始";
        chinese_["Face Presets"] = "面部预设";
        chinese_["Overview"] = "总览";
        chinese_["Body"] = "体型";
        chinese_["Face"] = "面部";
        chinese_["Hair"] = "发型";
        chinese_["Clothing"] = "服装";
        chinese_["Expression"] = "表情";
        chinese_["Export"] = "导出";
        chinese_["Back to Scene"] = "返回场景";
        chinese_["Randomize"] = "随机化";
        chinese_["Reset to Default"] = "恢复默认";
        chinese_["Reset All"] = "重置全部";
        chinese_["Body Customization"] = "体型定制";
        chinese_["Gender"] = "性别";
        chinese_["Male"] = "男性";
        chinese_["Female"] = "女性";
        chinese_["Proportions"] = "身材比例";
        chinese_["Height"] = "身高";
        chinese_["Weight"] = "体重";
        chinese_["Muscularity"] = "肌肉";
        chinese_["Upper Body"] = "上半身";
        chinese_["Shoulder Width"] = "肩宽";
        chinese_["Chest"] = "胸围";
        chinese_["Waist"] = "腰围";
        chinese_["Arm Length"] = "臂长";
        chinese_["Lower Body"] = "下半身";
        chinese_["Hip Width"] = "臀宽";
        chinese_["Leg Length"] = "腿长";
        chinese_["Skin Color"] = "肤色";
        chinese_["Skin Tone"] = "肤色";
        chinese_["Body Presets"] = "体型预设";
        chinese_["Athletic"] = "运动型";
        chinese_["Slim"] = "纤细";
        chinese_["Average"] = "普通";
        chinese_["Heavy"] = "壮实";
        chinese_["Muscular"] = "肌肉型";
        chinese_["Face Sculpting"] = "面部雕塑";
        chinese_["Overall"] = "整体";
        chinese_["Forehead"] = "额头";
        chinese_["Eyes"] = "眼睛";
        chinese_["Brows"] = "眉毛";
        chinese_["Nose"] = "鼻子";
        chinese_["Mouth"] = "嘴巴";
        chinese_["Chin"] = "下巴";
        chinese_["Jaw"] = "下颌";
        chinese_["Cheeks"] = "脸颊";
        chinese_["Ears"] = "耳朵";
        chinese_["Appearance"] = "外观";
        chinese_["Eye Color"] = "眼睛颜色";
        chinese_["Lip Color"] = "嘴唇颜色";
        chinese_["Wrinkles"] = "皱纹";
        chinese_["Freckles"] = "雀斑";
        chinese_["Skin Roughness"] = "皮肤粗糙度";
        chinese_["Hair Style"] = "发型";
        chinese_["Hair Color"] = "发色";
        chinese_["Short"] = "短发";
        chinese_["Medium"] = "中发";
        chinese_["Long"] = "长发";
        chinese_["Updo"] = "盘发";
        chinese_["Bald"] = "光头";
        chinese_["Top"] = "上装";
        chinese_["Bottom"] = "下装";
        chinese_["Shoes"] = "鞋";
        chinese_["Accessory"] = "配饰";
        chinese_["No items available"] = "暂无可用项目";
        chinese_["Intensity"] = "强度";
        chinese_["Preset Expressions"] = "预设表情";
        chinese_["Neutral"] = "中性";
        chinese_["Smile"] = "微笑";
        chinese_["Frown"] = "皱眉";
        chinese_["Surprise"] = "惊讶";
        chinese_["Angry"] = "愤怒";
        chinese_["Advanced Blend Shapes"] = "高级变形控制";
        chinese_["Export Character"] = "导出角色";
        chinese_["Export Format"] = "导出格式";
        chinese_["Apply to Scene"] = "应用到场景";
        chinese_["Update the character in the scene with current settings"] = "将当前设置更新到场景中的角色";
        chinese_["Vertices"] = "顶点";
        chinese_["Triangles"] = "三角形";
        chinese_["Name"] = "名称";
        chinese_["Rename"] = "重命名";
        chinese_["Import Photo for Face Generation"] = "导入照片生成面部";
        chinese_["Processing"] = "处理中";
        chinese_["Face generation complete!"] = "面部生成完成！";
        chinese_["OK"] = "确定";
        chinese_["Select a photo file:"] = "选择照片文件:";
        chinese_["Generate"] = "生成";
        chinese_["Cancel"] = "取消";
        
        // Photo deformation comparison
        chinese_["Photo Deformation Settings"] = "照片变形设置";
        chinese_["Compare"] = "对比";
        chinese_["Generated"] = "生成结果";
        chinese_["Standard"] = "标准头";
        chinese_["Deform Strength"] = "变形强度";
        chinese_["Influence Radius"] = "影响半径";
        chinese_["Re-apply Deformation"] = "重新应用变形";
        chinese_["Reset to Base Mesh"] = "重置为标准网格";
        chinese_["How much the photo shape affects the mesh (1.0 = 100%)"] = "照片形状对网格的影响程度 (1.0 = 100%)";
        chinese_["How far each landmark affects nearby vertices"] = "每个特征点影响周围顶点的范围";
        chinese_["Adjust these parameters and re-import to change deformation strength"] = "调整参数后点击重新应用来改变变形效果";
        
        // Landmark visualization
        chinese_["Show Landmarks"] = "显示特征点";
        chinese_["Display 68 facial landmark points on the mesh for debugging"] = "在网格上显示68个人脸特征点用于调试";
    }
    
    Language currentLanguage_ = Language::Chinese;
    std::unordered_map<std::string, std::string> chinese_;
    std::unordered_map<std::string, std::string> english_;
};

// Convenience macro for localized strings
#define L(key) luma::ui::Localization::instance().get(key)

// Convenience function
inline const char* loc(const char* key) {
    return Localization::instance().get(key);
}

} // namespace ui
} // namespace luma
