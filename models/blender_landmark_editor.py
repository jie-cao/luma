"""
Blender Landmark Editor for LUMA
在 Blender 中可视化和编辑 68 个 iBUG landmark 点

使用方法:
1. 在 Blender 中打开 Super Average Head.obj
2. 运行此脚本 (Text Editor -> Run Script)
3. 在 3D 视图中会显示 68 个球体标记当前的 landmark 位置
4. 选择球体并移动到正确的顶点位置
5. 运行 export_landmarks() 导出新的映射文件

iBUG 68 点布局:
- 0-16: 下巴轮廓 (右耳到左耳)
- 17-21: 左眉毛
- 22-26: 右眉毛  
- 27-30: 鼻梁
- 31-35: 鼻子底部
- 36-41: 左眼
- 42-47: 右眼
- 48-59: 外嘴唇
- 60-67: 内嘴唇
"""

import bpy
import bmesh
import json
import os
from mathutils import Vector

# Landmark 名称和区域
LANDMARK_INFO = {
    # 下巴轮廓 (0-16)
    0: ("jaw_right_ear", "jaw"),
    1: ("jaw_right_1", "jaw"),
    2: ("jaw_right_2", "jaw"),
    3: ("jaw_right_3", "jaw"),
    4: ("jaw_right_4", "jaw"),
    5: ("jaw_right_chin", "jaw"),
    6: ("jaw_chin_right", "chin"),
    7: ("jaw_chin_center", "chin"),
    8: ("jaw_chin_bottom", "chin"),
    9: ("jaw_chin_left", "chin"),
    10: ("jaw_left_chin", "jaw"),
    11: ("jaw_left_4", "jaw"),
    12: ("jaw_left_3", "jaw"),
    13: ("jaw_left_2", "jaw"),
    14: ("jaw_left_1", "jaw"),
    15: ("jaw_left_ear_low", "jaw"),
    16: ("jaw_left_ear", "jaw"),
    
    # 左眉毛 (17-21)
    17: ("left_brow_inner", "left_eyebrow"),
    18: ("left_brow_1", "left_eyebrow"),
    19: ("left_brow_center", "left_eyebrow"),
    20: ("left_brow_2", "left_eyebrow"),
    21: ("left_brow_outer", "left_eyebrow"),
    
    # 右眉毛 (22-26)
    22: ("right_brow_inner", "right_eyebrow"),
    23: ("right_brow_1", "right_eyebrow"),
    24: ("right_brow_center", "right_eyebrow"),
    25: ("right_brow_2", "right_eyebrow"),
    26: ("right_brow_outer", "right_eyebrow"),
    
    # 鼻梁 (27-30)
    27: ("nose_bridge_top", "nose"),
    28: ("nose_bridge_1", "nose"),
    29: ("nose_bridge_2", "nose"),
    30: ("nose_bridge_bottom", "nose"),
    
    # 鼻子底部 (31-35)
    31: ("nose_left_wing", "nose"),
    32: ("nose_left_nostril", "nose"),
    33: ("nose_tip", "nose"),
    34: ("nose_right_nostril", "nose"),
    35: ("nose_right_wing", "nose"),
    
    # 左眼 (36-41) - 顺时针从外角
    36: ("left_eye_outer", "left_eye"),
    37: ("left_eye_top_outer", "left_eye"),
    38: ("left_eye_top_inner", "left_eye"),
    39: ("left_eye_inner", "left_eye"),
    40: ("left_eye_bottom_inner", "left_eye"),
    41: ("left_eye_bottom_outer", "left_eye"),
    
    # 右眼 (42-47) - 顺时针从内角
    42: ("right_eye_inner", "right_eye"),
    43: ("right_eye_top_inner", "right_eye"),
    44: ("right_eye_top_outer", "right_eye"),
    45: ("right_eye_outer", "right_eye"),
    46: ("right_eye_bottom_outer", "right_eye"),
    47: ("right_eye_bottom_inner", "right_eye"),
    
    # 外嘴唇 (48-59)
    48: ("mouth_left", "mouth_outer"),
    49: ("mouth_top_left_1", "mouth_outer"),
    50: ("mouth_top_left_2", "mouth_outer"),
    51: ("mouth_top_center", "mouth_outer"),
    52: ("mouth_top_right_2", "mouth_outer"),
    53: ("mouth_top_right_1", "mouth_outer"),
    54: ("mouth_right", "mouth_outer"),
    55: ("mouth_bottom_right_1", "mouth_outer"),
    56: ("mouth_bottom_right_2", "mouth_outer"),
    57: ("mouth_bottom_center", "mouth_outer"),
    58: ("mouth_bottom_left_2", "mouth_outer"),
    59: ("mouth_bottom_left_1", "mouth_outer"),
    
    # 内嘴唇 (60-67)
    60: ("inner_mouth_left", "mouth_inner"),
    61: ("inner_mouth_top_left", "mouth_inner"),
    62: ("inner_mouth_top_center", "mouth_inner"),
    63: ("inner_mouth_top_right", "mouth_inner"),
    64: ("inner_mouth_right", "mouth_inner"),
    65: ("inner_mouth_bottom_right", "mouth_inner"),
    66: ("inner_mouth_bottom_center", "mouth_inner"),
    67: ("inner_mouth_bottom_left", "mouth_inner"),
}

# 区域颜色
REGION_COLORS = {
    "jaw": (0.8, 0.4, 0.2, 1.0),      # 橙色
    "chin": (0.9, 0.5, 0.3, 1.0),     # 浅橙
    "left_eyebrow": (0.2, 0.6, 0.8, 1.0),  # 蓝色
    "right_eyebrow": (0.2, 0.6, 0.8, 1.0),
    "nose": (0.2, 0.8, 0.4, 1.0),     # 绿色
    "left_eye": (0.8, 0.2, 0.8, 1.0), # 紫色
    "right_eye": (0.8, 0.2, 0.8, 1.0),
    "mouth_outer": (0.8, 0.2, 0.2, 1.0),  # 红色
    "mouth_inner": (0.9, 0.4, 0.4, 1.0),  # 浅红
}

def get_head_mesh():
    """获取场景中的头部网格"""
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH' and 'head' in obj.name.lower():
            return obj
    # 如果没找到，返回选中的网格
    if bpy.context.active_object and bpy.context.active_object.type == 'MESH':
        return bpy.context.active_object
    return None

def find_closest_vertex(mesh_obj, position):
    """找到最近的顶点"""
    mesh = mesh_obj.data
    min_dist = float('inf')
    closest_idx = -1
    
    # 转换到世界坐标
    world_matrix = mesh_obj.matrix_world
    
    for i, vert in enumerate(mesh.vertices):
        world_pos = world_matrix @ vert.co
        dist = (world_pos - position).length
        if dist < min_dist:
            min_dist = dist
            closest_idx = i
    
    return closest_idx, min_dist

def create_landmark_markers():
    """创建 68 个 landmark 标记球体"""
    
    # 删除旧的标记
    for obj in list(bpy.context.scene.objects):
        if obj.name.startswith("LM_"):
            bpy.data.objects.remove(obj, do_unlink=True)
    
    # 获取头部网格
    head = get_head_mesh()
    if not head:
        print("错误: 找不到头部网格，请先导入 Super Average Head.obj")
        return
    
    print(f"使用网格: {head.name}, {len(head.data.vertices)} 顶点")
    
    # 尝试加载现有的 landmark 映射
    script_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else os.getcwd()
    json_path = os.path.join(script_dir, "landmark_vertex_map.json")
    
    existing_mapping = {}
    if os.path.exists(json_path):
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
            for lm in data.get('landmarks', []):
                lm_id = lm['id']
                verts = lm.get('vertices', [])
                if verts:
                    existing_mapping[lm_id] = verts[0]
        print(f"加载了 {len(existing_mapping)} 个现有 landmark 映射")
    
    # 创建标记集合
    if "Landmarks" not in bpy.data.collections:
        collection = bpy.data.collections.new("Landmarks")
        bpy.context.scene.collection.children.link(collection)
    else:
        collection = bpy.data.collections["Landmarks"]
    
    # 创建材质
    materials = {}
    for region, color in REGION_COLORS.items():
        mat_name = f"LM_Mat_{region}"
        if mat_name not in bpy.data.materials:
            mat = bpy.data.materials.new(name=mat_name)
            mat.use_nodes = True
            bsdf = mat.node_tree.nodes.get("Principled BSDF")
            if bsdf:
                bsdf.inputs["Base Color"].default_value = color
                bsdf.inputs["Emission"].default_value = color[:3] + (1.0,)
                bsdf.inputs["Emission Strength"].default_value = 0.5
        materials[region] = bpy.data.materials[mat_name]
    
    # 创建 68 个标记
    world_matrix = head.matrix_world
    
    for lm_id in range(68):
        name, region = LANDMARK_INFO.get(lm_id, (f"landmark_{lm_id}", "unknown"))
        
        # 确定位置
        if lm_id in existing_mapping:
            vert_idx = existing_mapping[lm_id]
            if vert_idx < len(head.data.vertices):
                pos = world_matrix @ head.data.vertices[vert_idx].co
            else:
                pos = Vector((0, 0, 0))
        else:
            # 默认位置 (需要手动调整)
            pos = Vector((0, 0, 0))
        
        # 创建球体
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.003, location=pos)
        marker = bpy.context.active_object
        marker.name = f"LM_{lm_id:02d}_{name}"
        
        # 设置材质
        if region in materials:
            marker.data.materials.append(materials[region])
        
        # 移动到集合
        for coll in marker.users_collection:
            coll.objects.unlink(marker)
        collection.objects.link(marker)
    
    print(f"创建了 68 个 landmark 标记")
    print("提示: 选择标记球体，按 G 移动到正确的顶点位置")
    print("完成后运行 export_landmarks() 导出映射")

def snap_markers_to_vertices():
    """将所有标记吸附到最近的顶点"""
    head = get_head_mesh()
    if not head:
        print("错误: 找不到头部网格")
        return
    
    for obj in bpy.context.scene.objects:
        if obj.name.startswith("LM_"):
            vert_idx, dist = find_closest_vertex(head, obj.location)
            if vert_idx >= 0:
                world_pos = head.matrix_world @ head.data.vertices[vert_idx].co
                obj.location = world_pos
                print(f"{obj.name}: 吸附到顶点 {vert_idx} (距离 {dist:.4f})")

def export_landmarks():
    """导出 landmark 映射到 JSON"""
    head = get_head_mesh()
    if not head:
        print("错误: 找不到头部网格")
        return
    
    landmarks = []
    
    for lm_id in range(68):
        name, region = LANDMARK_INFO.get(lm_id, (f"landmark_{lm_id}", "unknown"))
        marker_name = f"LM_{lm_id:02d}_{name}"
        
        marker = bpy.data.objects.get(marker_name)
        if not marker:
            print(f"警告: 找不到标记 {marker_name}")
            landmarks.append({
                "id": lm_id,
                "name": name,
                "region": region,
                "vertices": [],
                "weights": [],
                "auto_estimated": True
            })
            continue
        
        # 找最近的顶点
        vert_idx, dist = find_closest_vertex(head, marker.location)
        
        landmarks.append({
            "id": lm_id,
            "name": name,
            "region": region,
            "vertices": [vert_idx],
            "weights": [1.0],
            "auto_estimated": False,
            "snap_distance": dist
        })
    
    # 保存
    data = {
        "version": "1.0",
        "meshFile": head.name + ".obj",
        "landmarkStandard": "ibug68",
        "landmarkCount": 68,
        "markedCount": sum(1 for lm in landmarks if lm["vertices"]),
        "auto_estimated": False,
        "note": "由 Blender landmark editor 手动标记",
        "landmarks": landmarks
    }
    
    script_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else os.getcwd()
    output_path = os.path.join(script_dir, "landmark_vertex_map.json")
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    
    print(f"导出到: {output_path}")
    print(f"标记了 {data['markedCount']}/68 个 landmark")

def select_landmark_by_id(lm_id):
    """选择指定 ID 的 landmark 标记"""
    name, _ = LANDMARK_INFO.get(lm_id, (f"landmark_{lm_id}", "unknown"))
    marker_name = f"LM_{lm_id:02d}_{name}"
    
    bpy.ops.object.select_all(action='DESELECT')
    marker = bpy.data.objects.get(marker_name)
    if marker:
        marker.select_set(True)
        bpy.context.view_layer.objects.active = marker
        print(f"选中: {marker_name}")
    else:
        print(f"找不到: {marker_name}")

# 注册为 Blender 操作符
class LUMA_OT_CreateLandmarks(bpy.types.Operator):
    bl_idname = "luma.create_landmarks"
    bl_label = "Create Landmark Markers"
    bl_description = "创建 68 个 landmark 标记球体"
    
    def execute(self, context):
        create_landmark_markers()
        return {'FINISHED'}

class LUMA_OT_SnapLandmarks(bpy.types.Operator):
    bl_idname = "luma.snap_landmarks"
    bl_label = "Snap to Vertices"
    bl_description = "将所有标记吸附到最近的顶点"
    
    def execute(self, context):
        snap_markers_to_vertices()
        return {'FINISHED'}

class LUMA_OT_ExportLandmarks(bpy.types.Operator):
    bl_idname = "luma.export_landmarks"
    bl_label = "Export Landmarks"
    bl_description = "导出 landmark 映射到 JSON"
    
    def execute(self, context):
        export_landmarks()
        return {'FINISHED'}

class LUMA_PT_LandmarkPanel(bpy.types.Panel):
    bl_label = "LUMA Landmarks"
    bl_idname = "LUMA_PT_landmarks"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'LUMA'
    
    def draw(self, context):
        layout = self.layout
        layout.operator("luma.create_landmarks")
        layout.operator("luma.snap_landmarks")
        layout.operator("luma.export_landmarks")

classes = [
    LUMA_OT_CreateLandmarks,
    LUMA_OT_SnapLandmarks,
    LUMA_OT_ExportLandmarks,
    LUMA_PT_LandmarkPanel,
]

def register():
    for cls in classes:
        bpy.utils.register_class(cls)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register()
    print("\n=== LUMA Landmark Editor ===")
    print("在 3D 视图右侧面板找到 'LUMA' 标签")
    print("或直接调用:")
    print("  create_landmark_markers()  - 创建标记")
    print("  snap_markers_to_vertices() - 吸附到顶点")
    print("  export_landmarks()         - 导出映射")
