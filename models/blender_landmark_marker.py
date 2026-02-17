"""
Blender Landmark Marker Tool
============================
在Blender中标记68个iBUG标准关键点并导出为JSON。

使用方法：
1. 在Blender中打开你的标准头OBJ文件
2. 切换到Scripting工作区
3. 打开此脚本并运行
4. 在Python控制台中使用以下命令：
   - mark()   : 将当前选中的顶点标记为下一个关键点
   - skip()   : 跳过当前关键点（如果网格上没有对应位置）
   - undo()   : 撤销上一个标记
   - status() : 查看当前进度
   - export() : 导出JSON文件
   - reset()  : 重新开始

关键点顺序（iBUG 68点标准）：
- 0-16:  下颌轮廓（从右耳到左耳，17点）
- 17-21: 左眉毛（5点）
- 22-26: 右眉毛（5点）
- 27-30: 鼻梁（4点）
- 31-35: 鼻底（5点）
- 36-41: 左眼（6点）
- 42-47: 右眼（6点）
- 48-59: 外嘴唇（12点）
- 60-67: 内嘴唇（8点）
"""

import bpy
import json
import os
from mathutils import Vector

# iBUG 68点关键点定义
LANDMARK_DEFINITIONS = [
    # 下颌轮廓 0-16 (17点) - 从右耳下方开始，沿下巴到左耳下方
    {"id": 0,  "name": "jaw_right_ear",      "region": "jaw",   "desc": "右耳下方"},
    {"id": 1,  "name": "jaw_right_1",        "region": "jaw",   "desc": "右下颌1"},
    {"id": 2,  "name": "jaw_right_2",        "region": "jaw",   "desc": "右下颌2"},
    {"id": 3,  "name": "jaw_right_3",        "region": "jaw",   "desc": "右下颌3"},
    {"id": 4,  "name": "jaw_right_4",        "region": "jaw",   "desc": "右下颌角"},
    {"id": 5,  "name": "jaw_right_chin",     "region": "jaw",   "desc": "右下巴"},
    {"id": 6,  "name": "jaw_chin_right",     "region": "chin",  "desc": "下巴右侧"},
    {"id": 7,  "name": "jaw_chin_center",    "region": "chin",  "desc": "下巴中心"},
    {"id": 8,  "name": "jaw_chin_bottom",    "region": "chin",  "desc": "下巴最低点"},
    {"id": 9,  "name": "jaw_chin_left",      "region": "chin",  "desc": "下巴左侧"},
    {"id": 10, "name": "jaw_left_chin",      "region": "jaw",   "desc": "左下巴"},
    {"id": 11, "name": "jaw_left_4",         "region": "jaw",   "desc": "左下颌角"},
    {"id": 12, "name": "jaw_left_3",         "region": "jaw",   "desc": "左下颌3"},
    {"id": 13, "name": "jaw_left_2",         "region": "jaw",   "desc": "左下颌2"},
    {"id": 14, "name": "jaw_left_1",         "region": "jaw",   "desc": "左下颌1"},
    {"id": 15, "name": "jaw_left_ear_low",   "region": "jaw",   "desc": "左耳下方低"},
    {"id": 16, "name": "jaw_left_ear",       "region": "jaw",   "desc": "左耳下方"},
    
    # 左眉毛 17-21 (5点) - 从内侧到外侧
    {"id": 17, "name": "left_brow_inner",    "region": "brow",  "desc": "左眉内侧"},
    {"id": 18, "name": "left_brow_1",        "region": "brow",  "desc": "左眉1"},
    {"id": 19, "name": "left_brow_center",   "region": "brow",  "desc": "左眉中心"},
    {"id": 20, "name": "left_brow_2",        "region": "brow",  "desc": "左眉2"},
    {"id": 21, "name": "left_brow_outer",    "region": "brow",  "desc": "左眉外侧"},
    
    # 右眉毛 22-26 (5点) - 从内侧到外侧
    {"id": 22, "name": "right_brow_inner",   "region": "brow",  "desc": "右眉内侧"},
    {"id": 23, "name": "right_brow_1",       "region": "brow",  "desc": "右眉1"},
    {"id": 24, "name": "right_brow_center",  "region": "brow",  "desc": "右眉中心"},
    {"id": 25, "name": "right_brow_2",       "region": "brow",  "desc": "右眉2"},
    {"id": 26, "name": "right_brow_outer",   "region": "brow",  "desc": "右眉外侧"},
    
    # 鼻梁 27-30 (4点) - 从眉心到鼻尖上方
    {"id": 27, "name": "nose_bridge_top",    "region": "nose",  "desc": "鼻梁顶部（眉心）"},
    {"id": 28, "name": "nose_bridge_1",      "region": "nose",  "desc": "鼻梁上部"},
    {"id": 29, "name": "nose_bridge_2",      "region": "nose",  "desc": "鼻梁中部"},
    {"id": 30, "name": "nose_bridge_bottom", "region": "nose",  "desc": "鼻梁底部"},
    
    # 鼻底 31-35 (5点) - 鼻翼和鼻尖
    {"id": 31, "name": "nose_left_wing",     "region": "nose",  "desc": "左鼻翼外侧"},
    {"id": 32, "name": "nose_left_nostril",  "region": "nose",  "desc": "左鼻孔"},
    {"id": 33, "name": "nose_tip",           "region": "nose",  "desc": "鼻尖"},
    {"id": 34, "name": "nose_right_nostril", "region": "nose",  "desc": "右鼻孔"},
    {"id": 35, "name": "nose_right_wing",    "region": "nose",  "desc": "右鼻翼外侧"},
    
    # 左眼 36-41 (6点) - 从内眼角顺时针
    {"id": 36, "name": "left_eye_inner",     "region": "eye",   "desc": "左眼内眼角"},
    {"id": 37, "name": "left_eye_top_inner", "region": "eye",   "desc": "左眼上眼睑内"},
    {"id": 38, "name": "left_eye_top_outer", "region": "eye",   "desc": "左眼上眼睑外"},
    {"id": 39, "name": "left_eye_outer",     "region": "eye",   "desc": "左眼外眼角"},
    {"id": 40, "name": "left_eye_bot_outer", "region": "eye",   "desc": "左眼下眼睑外"},
    {"id": 41, "name": "left_eye_bot_inner", "region": "eye",   "desc": "左眼下眼睑内"},
    
    # 右眼 42-47 (6点) - 从内眼角顺时针
    {"id": 42, "name": "right_eye_inner",    "region": "eye",   "desc": "右眼内眼角"},
    {"id": 43, "name": "right_eye_top_inner","region": "eye",   "desc": "右眼上眼睑内"},
    {"id": 44, "name": "right_eye_top_outer","region": "eye",   "desc": "右眼上眼睑外"},
    {"id": 45, "name": "right_eye_outer",    "region": "eye",   "desc": "右眼外眼角"},
    {"id": 46, "name": "right_eye_bot_outer","region": "eye",   "desc": "右眼下眼睑外"},
    {"id": 47, "name": "right_eye_bot_inner","region": "eye",   "desc": "右眼下眼睑内"},
    
    # 外嘴唇 48-59 (12点) - 从左嘴角顺时针
    {"id": 48, "name": "mouth_left_corner",  "region": "mouth", "desc": "左嘴角"},
    {"id": 49, "name": "mouth_top_left_1",   "region": "mouth", "desc": "上唇左1"},
    {"id": 50, "name": "mouth_top_left_2",   "region": "mouth", "desc": "上唇左2"},
    {"id": 51, "name": "mouth_top_center",   "region": "mouth", "desc": "上唇中心（唇峰）"},
    {"id": 52, "name": "mouth_top_right_2",  "region": "mouth", "desc": "上唇右2"},
    {"id": 53, "name": "mouth_top_right_1",  "region": "mouth", "desc": "上唇右1"},
    {"id": 54, "name": "mouth_right_corner", "region": "mouth", "desc": "右嘴角"},
    {"id": 55, "name": "mouth_bot_right_1",  "region": "mouth", "desc": "下唇右1"},
    {"id": 56, "name": "mouth_bot_right_2",  "region": "mouth", "desc": "下唇右2"},
    {"id": 57, "name": "mouth_bot_center",   "region": "mouth", "desc": "下唇中心"},
    {"id": 58, "name": "mouth_bot_left_2",   "region": "mouth", "desc": "下唇左2"},
    {"id": 59, "name": "mouth_bot_left_1",   "region": "mouth", "desc": "下唇左1"},
    
    # 内嘴唇 60-67 (8点) - 从左内角顺时针
    {"id": 60, "name": "mouth_inner_left",   "region": "mouth", "desc": "内唇左"},
    {"id": 61, "name": "mouth_inner_top_l",  "region": "mouth", "desc": "内唇上左"},
    {"id": 62, "name": "mouth_inner_top_c",  "region": "mouth", "desc": "内唇上中"},
    {"id": 63, "name": "mouth_inner_top_r",  "region": "mouth", "desc": "内唇上右"},
    {"id": 64, "name": "mouth_inner_right",  "region": "mouth", "desc": "内唇右"},
    {"id": 65, "name": "mouth_inner_bot_r",  "region": "mouth", "desc": "内唇下右"},
    {"id": 66, "name": "mouth_inner_bot_c",  "region": "mouth", "desc": "内唇下中"},
    {"id": 67, "name": "mouth_inner_bot_l",  "region": "mouth", "desc": "内唇下左"},
]

class LandmarkMarker:
    """关键点标记器"""
    
    def __init__(self):
        self.landmarks = {}
        self.current_index = 0
        self.history = []  # 用于撤销
        
    def get_current_landmark(self):
        """获取当前要标记的关键点信息"""
        if self.current_index >= 68:
            return None
        return LANDMARK_DEFINITIONS[self.current_index]
    
    def mark_selected_vertex(self):
        """将当前选中的顶点标记为下一个关键点"""
        obj = bpy.context.active_object
        if not obj or obj.type != 'MESH':
            print("错误：请先选择一个网格对象")
            return False
        
        # 切换到Object模式以读取选择状态
        current_mode = obj.mode
        if current_mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
        
        mesh = obj.data
        selected_verts = [v.index for v in mesh.vertices if v.select]
        
        if not selected_verts:
            print("错误：请先选择至少一个顶点")
            if current_mode != 'OBJECT':
                bpy.ops.object.mode_set(mode=current_mode)
            return False
        
        if self.current_index >= 68:
            print("已标记完所有68个关键点！使用 export() 导出。")
            if current_mode != 'OBJECT':
                bpy.ops.object.mode_set(mode=current_mode)
            return False
        
        lm_def = LANDMARK_DEFINITIONS[self.current_index]
        
        # 计算权重（如果选了多个顶点，平均分配）
        weights = [1.0 / len(selected_verts)] * len(selected_verts)
        
        # 获取顶点位置（用于验证）
        positions = []
        for vi in selected_verts:
            v = mesh.vertices[vi]
            positions.append([v.co.x, v.co.y, v.co.z])
        
        self.landmarks[self.current_index] = {
            "id": self.current_index,
            "name": lm_def["name"],
            "region": lm_def["region"],
            "vertices": selected_verts,
            "weights": weights,
            "positions": positions  # 用于验证
        }
        
        # 保存历史
        self.history.append(self.current_index)
        
        print(f"✓ [{self.current_index:2d}/67] {lm_def['name']:25s} -> 顶点 {selected_verts}")
        self.current_index += 1
        
        # 显示下一个
        if self.current_index < 68:
            next_lm = LANDMARK_DEFINITIONS[self.current_index]
            print(f"  下一个: [{self.current_index:2d}] {next_lm['name']} ({next_lm['desc']})")
        else:
            print("\n🎉 所有68个关键点已标记完成！")
            print("   使用 export() 导出JSON文件")
        
        # 切换回Edit模式并取消选择
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='DESELECT')
        
        return True
    
    def skip_current(self):
        """跳过当前关键点（标记为无效）"""
        if self.current_index >= 68:
            print("已完成所有关键点")
            return
        
        lm_def = LANDMARK_DEFINITIONS[self.current_index]
        
        self.landmarks[self.current_index] = {
            "id": self.current_index,
            "name": lm_def["name"],
            "region": lm_def["region"],
            "vertices": [],
            "weights": [],
            "skipped": True
        }
        
        self.history.append(self.current_index)
        
        print(f"⊘ [{self.current_index:2d}/67] {lm_def['name']:25s} -> 已跳过")
        self.current_index += 1
        
        if self.current_index < 68:
            next_lm = LANDMARK_DEFINITIONS[self.current_index]
            print(f"  下一个: [{self.current_index:2d}] {next_lm['name']} ({next_lm['desc']})")
    
    def undo_last(self):
        """撤销上一个标记"""
        if not self.history:
            print("没有可撤销的操作")
            return
        
        last_index = self.history.pop()
        if last_index in self.landmarks:
            del self.landmarks[last_index]
        
        self.current_index = last_index
        lm_def = LANDMARK_DEFINITIONS[self.current_index]
        print(f"↩ 已撤销，回到: [{self.current_index:2d}] {lm_def['name']} ({lm_def['desc']})")
    
    def show_status(self):
        """显示当前进度"""
        marked = len([l for l in self.landmarks.values() if not l.get('skipped', False)])
        skipped = len([l for l in self.landmarks.values() if l.get('skipped', False)])
        
        print(f"\n{'='*50}")
        print(f"进度: {self.current_index}/68")
        print(f"已标记: {marked}, 已跳过: {skipped}")
        print(f"{'='*50}")
        
        if self.current_index < 68:
            lm_def = LANDMARK_DEFINITIONS[self.current_index]
            print(f"当前: [{self.current_index:2d}] {lm_def['name']}")
            print(f"描述: {lm_def['desc']}")
            print(f"区域: {lm_def['region']}")
        else:
            print("所有关键点已处理完毕！")
        print(f"{'='*50}\n")
    
    def export_json(self, filepath=None):
        """导出为JSON文件"""
        if filepath is None:
            # 默认导出到当前blend文件同目录
            blend_path = bpy.data.filepath
            if blend_path:
                dir_path = os.path.dirname(blend_path)
                filepath = os.path.join(dir_path, "landmark_vertex_map.json")
            else:
                filepath = "C:/code/luma/models/landmark_vertex_map.json"
        
        obj = bpy.context.active_object
        mesh_name = obj.name if obj else "unknown"
        
        # 构建导出数据
        landmarks_list = []
        for i in range(68):
            if i in self.landmarks:
                lm = self.landmarks[i]
                landmarks_list.append({
                    "id": lm["id"],
                    "name": lm["name"],
                    "region": lm["region"],
                    "vertices": lm["vertices"],
                    "weights": lm["weights"]
                })
            else:
                # 未标记的关键点
                lm_def = LANDMARK_DEFINITIONS[i]
                landmarks_list.append({
                    "id": i,
                    "name": lm_def["name"],
                    "region": lm_def["region"],
                    "vertices": [],
                    "weights": [],
                    "missing": True
                })
        
        data = {
            "version": "1.0",
            "meshFile": mesh_name + ".obj",
            "landmarkStandard": "ibug68",
            "landmarkCount": 68,
            "markedCount": len([l for l in landmarks_list if l["vertices"]]),
            "landmarks": landmarks_list
        }
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        
        print(f"\n{'='*50}")
        print(f"✓ 已导出到: {filepath}")
        print(f"  总关键点: 68")
        print(f"  已标记: {data['markedCount']}")
        print(f"  缺失: {68 - data['markedCount']}")
        print(f"{'='*50}\n")
        
        return filepath


# 全局实例
_marker = None

def _get_marker():
    global _marker
    if _marker is None:
        _marker = LandmarkMarker()
    return _marker

# 快捷函数
def mark():
    """标记当前选中顶点为下一个关键点"""
    _get_marker().mark_selected_vertex()

def skip():
    """跳过当前关键点"""
    _get_marker().skip_current()

def undo():
    """撤销上一个标记"""
    _get_marker().undo_last()

def status():
    """查看当前进度"""
    _get_marker().show_status()

def export(path=None):
    """导出JSON文件"""
    return _get_marker().export_json(path)

def reset():
    """重置，从头开始"""
    global _marker
    _marker = LandmarkMarker()
    print("已重置，从第0个点开始")
    status()


# 启动时显示帮助
print("\n" + "="*60)
print("  LUMA 关键点标记工具 v1.0")
print("="*60)
print("""
使用方法：
  1. 确保已选中你的头部网格对象
  2. 按 Tab 进入 Edit Mode
  3. 选择模式改为 Vertex（顶点）
  4. 按照提示选择顶点，然后在控制台输入命令

命令列表：
  mark()   - 将选中顶点标记为当前关键点
  skip()   - 跳过当前关键点（如果网格上没有）
  undo()   - 撤销上一个标记
  status() - 查看当前进度
  export() - 导出JSON文件
  reset()  - 重新开始

导出路径：
  export()                    -> 默认路径
  export("C:/path/to/file.json") -> 指定路径
""")
print("="*60 + "\n")

# 显示初始状态
status()
