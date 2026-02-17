"""
自动生成初始 BlendShapes - 基于 Landmark 和解剖学知识

这个脚本会为标准头部模型自动生成一组初始的 BlendShape targets。
生成的结果可以作为艺术家进一步调整的起点。

使用方法：
    python auto_generate_blendshapes.py head_base.obj landmark_vertex_map.json

输出：
    head_with_shapes.blend - 带有 Shape Keys 的 Blender 文件
"""

import numpy as np
import json
import sys
import os

# 尝试导入 Blender 模块（如果在 Blender 中运行）
try:
    import bpy
    import bmesh
    IN_BLENDER = True
except ImportError:
    IN_BLENDER = False
    print("Not running in Blender - will generate data only")


# ============================================================================
# 人体测量学数据 (Anthropometric Data)
# 来源: 各种人体测量学研究的综合数据
# ============================================================================

# 变形范围（单位：相对于头高的比例）
# 例如：0.05 表示头高的 5%，对于 23cm 的头就是 11.5mm
DEFORMATION_RANGES = {
    # 整体脸型
    "id_face_width": 0.08,        # ±8% 头高 ≈ ±18mm
    "id_face_length": 0.06,       # ±6% ≈ ±14mm
    "id_face_round": 0.05,        # ±5% ≈ ±11mm
    
    # 额头
    "id_forehead_height": 0.05,   # ±5% ≈ ±11mm
    "id_forehead_width": 0.06,    # ±6%
    "id_forehead_slope": 0.04,    # ±4%
    "id_forehead_bossing": 0.03,  # ±3%
    
    # 眼睛
    "id_eye_size": 0.02,          # ±2% (眼睛变化范围较小)
    "id_eye_spacing": 0.025,      # ±2.5% ≈ ±6mm
    "id_eye_height": 0.03,        # ±3%
    "id_eye_depth": 0.025,        # ±2.5%
    "id_eye_angle_up": 0.015,     # ±1.5%
    "id_eye_angle_down": 0.015,
    
    # 眉毛
    "id_brow_height": 0.025,      # ±2.5%
    "id_brow_inner_up": 0.015,
    "id_brow_outer_up": 0.015,
    "id_brow_spacing": 0.02,
    
    # 鼻子
    "id_nose_length": 0.035,      # ±3.5% ≈ ±8mm
    "id_nose_width": 0.025,       # ±2.5% ≈ ±6mm
    "id_nose_height": 0.04,       # ±4% ≈ ±9mm
    "id_nose_bridge_width": 0.015,
    "id_nose_bridge_curve": 0.02,
    "id_nose_tip_up": 0.02,
    "id_nose_tip_down": 0.02,
    "id_nose_tip_width": 0.015,
    "id_nostril_width": 0.02,
    "id_nostril_flare": 0.015,
    
    # 嘴巴
    "id_mouth_width": 0.03,       # ±3% ≈ ±7mm
    "id_lip_upper_thick": 0.015,  # ±1.5%
    "id_lip_lower_thick": 0.02,   # ±2%
    "id_lip_protrusion": 0.025,
    "id_philtrum_length": 0.015,
    
    # 下巴
    "id_chin_length": 0.035,      # ±3.5% ≈ ±8mm
    "id_chin_width": 0.03,
    "id_chin_protrusion": 0.03,
    
    # 下颌
    "id_jaw_width": 0.05,         # ±5% ≈ ±11mm
    "id_jaw_angle": 0.03,
    
    # 颧骨
    "id_cheekbone_height": 0.02,
    "id_cheekbone_width": 0.035,
    "id_cheekbone_protrusion": 0.025,
    "id_cheek_fullness": 0.03,
    
    # 耳朵
    "id_ear_size": 0.025,
    "id_ear_angle": 0.03,
}


# ============================================================================
# Landmark 到区域的映射 (iBUG 68 点)
# ============================================================================

# iBUG 68 landmark 索引定义
LANDMARK_REGIONS = {
    "jaw_line": list(range(0, 17)),           # 0-16: 下颌轮廓
    "right_eyebrow": list(range(17, 22)),     # 17-21: 右眉
    "left_eyebrow": list(range(22, 27)),      # 22-26: 左眉
    "nose_bridge": list(range(27, 31)),       # 27-30: 鼻梁
    "nose_bottom": list(range(31, 36)),       # 31-35: 鼻底
    "right_eye": list(range(36, 42)),         # 36-41: 右眼
    "left_eye": list(range(42, 48)),          # 42-47: 左眼
    "outer_mouth": list(range(48, 60)),       # 48-59: 外嘴唇
    "inner_mouth": list(range(60, 68)),       # 60-67: 内嘴唇
}

# 特殊点
SPECIAL_LANDMARKS = {
    "chin": 8,                    # 下巴尖
    "nose_tip": 30,               # 鼻尖
    "left_mouth_corner": 48,      # 左嘴角
    "right_mouth_corner": 54,     # 右嘴角
    "upper_lip_center": 51,       # 上唇中心
    "lower_lip_center": 57,       # 下唇中心
    "left_eye_inner": 39,         # 左眼内角
    "left_eye_outer": 36,         # 左眼外角
    "right_eye_inner": 42,        # 右眼内角
    "right_eye_outer": 45,        # 右眼外角
    "nose_left": 31,              # 鼻翼左
    "nose_right": 35,             # 鼻翼右
}


def load_obj(filepath):
    """加载 OBJ 文件"""
    vertices = []
    faces = []
    
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            
            if parts[0] == 'v':
                vertices.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif parts[0] == 'f':
                # 解析面（可能是 v, v/vt, v/vt/vn 格式）
                face = []
                for p in parts[1:]:
                    idx = int(p.split('/')[0]) - 1  # OBJ 索引从 1 开始
                    face.append(idx)
                faces.append(face)
    
    return np.array(vertices), faces


def load_landmarks(filepath):
    """加载 landmark 到顶点的映射"""
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    # 转换为 {landmark_idx: vertex_idx} 格式
    mapping = {}
    for item in data.get("landmarks", []):
        lm_idx = item.get("id", item.get("landmark_index"))
        # 如果有多个顶点，取第一个
        vertices = item.get("vertices", [])
        if vertices:
            # 支持两种格式：[vertex_idx, ...] 或 [{"vertex_index": idx}, ...]
            if isinstance(vertices[0], dict):
                mapping[lm_idx] = vertices[0]["vertex_index"]
            else:
                mapping[lm_idx] = vertices[0]
    
    return mapping


def compute_region_weights(vertices, landmark_mapping, region_landmarks, falloff_radius):
    """
    计算每个顶点属于某个区域的权重
    基于到区域 landmark 的距离
    """
    weights = np.zeros(len(vertices))
    
    # 获取区域中心点
    region_vertices = []
    for lm_idx in region_landmarks:
        if lm_idx in landmark_mapping:
            v_idx = landmark_mapping[lm_idx]
            region_vertices.append(vertices[v_idx])
    
    if not region_vertices:
        return weights
    
    region_center = np.mean(region_vertices, axis=0)
    
    # 计算每个顶点到区域中心的距离
    distances = np.linalg.norm(vertices - region_center, axis=1)
    
    # 使用高斯衰减
    weights = np.exp(-(distances ** 2) / (2 * falloff_radius ** 2))
    
    return weights


def compute_directional_weights(vertices, landmark_mapping, center_landmarks, 
                                 direction, falloff_radius):
    """
    计算方向性权重（例如：外眼角向上）
    direction: 'left', 'right', 'up', 'down', 'front', 'back'
    """
    weights = np.zeros(len(vertices))
    
    # 获取中心点
    center_vertices = []
    for lm_idx in center_landmarks:
        if lm_idx in landmark_mapping:
            v_idx = landmark_mapping[lm_idx]
            center_vertices.append(vertices[v_idx])
    
    if not center_vertices:
        return weights
    
    center = np.mean(center_vertices, axis=0)
    
    # 计算距离权重
    distances = np.linalg.norm(vertices - center, axis=1)
    dist_weights = np.exp(-(distances ** 2) / (2 * falloff_radius ** 2))
    
    # 计算方向权重
    relative_pos = vertices - center
    
    if direction == 'left':
        dir_weights = np.clip(-relative_pos[:, 0], 0, None)
    elif direction == 'right':
        dir_weights = np.clip(relative_pos[:, 0], 0, None)
    elif direction == 'up':
        dir_weights = np.clip(relative_pos[:, 1], 0, None)
    elif direction == 'down':
        dir_weights = np.clip(-relative_pos[:, 1], 0, None)
    elif direction == 'front':
        dir_weights = np.clip(relative_pos[:, 2], 0, None)
    elif direction == 'back':
        dir_weights = np.clip(-relative_pos[:, 2], 0, None)
    else:
        dir_weights = np.ones(len(vertices))
    
    # 归一化方向权重
    max_dir = np.max(dir_weights)
    if max_dir > 0:
        dir_weights /= max_dir
    
    weights = dist_weights * dir_weights
    
    return weights


class BlendShapeGenerator:
    """BlendShape 生成器"""
    
    def __init__(self, vertices, faces, landmark_mapping):
        self.vertices = vertices
        self.faces = faces
        self.landmark_mapping = landmark_mapping
        
        # 计算网格边界
        self.min_bounds = np.min(vertices, axis=0)
        self.max_bounds = np.max(vertices, axis=0)
        self.center = (self.min_bounds + self.max_bounds) / 2
        self.size = self.max_bounds - self.min_bounds
        self.head_height = self.size[1]
        
        print(f"Mesh: {len(vertices)} vertices")
        print(f"Bounds: {self.min_bounds} to {self.max_bounds}")
        print(f"Head height: {self.head_height:.4f}")
        
        # 预计算区域权重
        self._precompute_regions()
    
    def _precompute_regions(self):
        """预计算各个面部区域的权重"""
        self.region_weights = {}
        
        # 基于 Y 坐标的粗略区域划分
        normalized_y = (self.vertices[:, 1] - self.min_bounds[1]) / self.size[1]
        normalized_x = (self.vertices[:, 0] - self.center[0]) / (self.size[0] / 2)
        normalized_z = (self.vertices[:, 2] - self.min_bounds[2]) / self.size[2]
        
        # 前脸 vs 后脑
        front_weight = np.clip((normalized_z - 0.3) / 0.4, 0, 1)
        
        # 各区域（结合 landmark 和坐标）
        falloff = self.head_height * 0.15
        
        # 下颌线
        self.region_weights["jaw"] = compute_region_weights(
            self.vertices, self.landmark_mapping, 
            LANDMARK_REGIONS["jaw_line"], falloff) * front_weight
        
        # 眉毛
        brow_landmarks = LANDMARK_REGIONS["right_eyebrow"] + LANDMARK_REGIONS["left_eyebrow"]
        self.region_weights["brow"] = compute_region_weights(
            self.vertices, self.landmark_mapping, brow_landmarks, falloff * 0.7)
        
        # 眼睛
        eye_landmarks = LANDMARK_REGIONS["right_eye"] + LANDMARK_REGIONS["left_eye"]
        self.region_weights["eyes"] = compute_region_weights(
            self.vertices, self.landmark_mapping, eye_landmarks, falloff * 0.6)
        
        # 鼻子
        nose_landmarks = LANDMARK_REGIONS["nose_bridge"] + LANDMARK_REGIONS["nose_bottom"]
        self.region_weights["nose"] = compute_region_weights(
            self.vertices, self.landmark_mapping, nose_landmarks, falloff * 0.8)
        
        # 嘴巴
        mouth_landmarks = LANDMARK_REGIONS["outer_mouth"]
        self.region_weights["mouth"] = compute_region_weights(
            self.vertices, self.landmark_mapping, mouth_landmarks, falloff * 0.7)
        
        # 下巴
        chin_landmarks = [SPECIAL_LANDMARKS["chin"]]
        self.region_weights["chin"] = compute_region_weights(
            self.vertices, self.landmark_mapping, chin_landmarks, falloff * 0.8)
        
        # 颧骨（基于坐标，因为没有直接的 landmark）
        cheek_y = (normalized_y > 0.35) & (normalized_y < 0.55)
        cheek_x = np.abs(normalized_x) > 0.3
        self.region_weights["cheeks"] = (cheek_y & cheek_x).astype(float) * front_weight
        
        # 额头
        forehead_mask = (normalized_y > 0.6) & (normalized_z > 0.3)
        self.region_weights["forehead"] = forehead_mask.astype(float)
        
        # 耳朵（侧面）
        ear_mask = (np.abs(normalized_x) > 0.7) & (normalized_y > 0.3) & (normalized_y < 0.65)
        self.region_weights["ears"] = ear_mask.astype(float)
    
    def generate_shape(self, name, deformation_func):
        """
        生成一个 BlendShape
        
        deformation_func: 函数，接受 (vertices, weights, magnitude) 返回 deltas
        """
        magnitude = DEFORMATION_RANGES.get(name, 0.03) * self.head_height
        
        # 根据名称确定区域权重
        if "face_" in name:
            weights = self.region_weights.get("jaw", np.ones(len(self.vertices)))
        elif "forehead" in name:
            weights = self.region_weights["forehead"]
        elif "eye" in name and "brow" not in name:
            weights = self.region_weights["eyes"]
        elif "brow" in name:
            weights = self.region_weights["brow"]
        elif "nose" in name or "nostril" in name:
            weights = self.region_weights["nose"]
        elif "mouth" in name or "lip" in name or "philtrum" in name:
            weights = self.region_weights["mouth"]
        elif "chin" in name:
            weights = self.region_weights["chin"]
        elif "jaw" in name:
            weights = self.region_weights["jaw"]
        elif "cheek" in name:
            weights = self.region_weights["cheeks"]
        elif "ear" in name:
            weights = self.region_weights["ears"]
        else:
            weights = np.ones(len(self.vertices))
        
        # 生成变形
        deltas = deformation_func(self.vertices, weights, magnitude)
        
        return deltas
    
    def generate_all_shapes(self):
        """生成所有 BlendShapes"""
        shapes = {}
        
        # === 整体脸型 ===
        shapes["id_face_width"] = self.generate_shape(
            "id_face_width",
            lambda v, w, m: np.column_stack([
                v[:, 0] * 0.15 * w,  # X 方向扩展
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_face_length"] = self.generate_shape(
            "id_face_length",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                -m * w * (1 - (v[:, 1] - self.min_bounds[1]) / self.size[1]),  # 下部向下
                np.zeros(len(v))
            ])
        )
        
        # === 鼻子 ===
        shapes["id_nose_length"] = self.generate_shape(
            "id_nose_length",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                -m * w,  # 向下延伸
                np.zeros(len(v))
            ])
        )
        
        shapes["id_nose_width"] = self.generate_shape(
            "id_nose_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,  # 向两侧扩展
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_nose_height"] = self.generate_shape(
            "id_nose_height",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                np.zeros(len(v)),
                m * w  # 向前突出
            ])
        )
        
        # === 嘴巴 ===
        shapes["id_mouth_width"] = self.generate_shape(
            "id_mouth_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_lip_upper_thick"] = self.generate_shape(
            "id_lip_upper_thick",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                m * w * 0.3,  # 略微向上
                m * w  # 向前突出
            ])
        )
        
        shapes["id_lip_lower_thick"] = self.generate_shape(
            "id_lip_lower_thick",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                -m * w * 0.3,  # 略微向下
                m * w  # 向前突出
            ])
        )
        
        # === 下巴 ===
        shapes["id_chin_length"] = self.generate_shape(
            "id_chin_length",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                -m * w,  # 向下延伸
                np.zeros(len(v))
            ])
        )
        
        shapes["id_chin_width"] = self.generate_shape(
            "id_chin_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_chin_protrusion"] = self.generate_shape(
            "id_chin_protrusion",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                np.zeros(len(v)),
                m * w
            ])
        )
        
        # === 下颌 ===
        shapes["id_jaw_width"] = self.generate_shape(
            "id_jaw_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        # === 眼睛 ===
        shapes["id_eye_spacing"] = self.generate_shape(
            "id_eye_spacing",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_eye_height"] = self.generate_shape(
            "id_eye_height",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                m * w,
                np.zeros(len(v))
            ])
        )
        
        shapes["id_eye_depth"] = self.generate_shape(
            "id_eye_depth",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                np.zeros(len(v)),
                -m * w  # 向内凹陷
            ])
        )
        
        # === 眉毛 ===
        shapes["id_brow_height"] = self.generate_shape(
            "id_brow_height",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                m * w,
                np.zeros(len(v))
            ])
        )
        
        # === 颧骨 ===
        shapes["id_cheekbone_width"] = self.generate_shape(
            "id_cheekbone_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        shapes["id_cheekbone_protrusion"] = self.generate_shape(
            "id_cheekbone_protrusion",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                np.zeros(len(v)),
                m * w
            ])
        )
        
        # === 额头 ===
        shapes["id_forehead_height"] = self.generate_shape(
            "id_forehead_height",
            lambda v, w, m: np.column_stack([
                np.zeros(len(v)),
                m * w,
                np.zeros(len(v))
            ])
        )
        
        shapes["id_forehead_width"] = self.generate_shape(
            "id_forehead_width",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                np.zeros(len(v))
            ])
        )
        
        # === 耳朵 ===
        shapes["id_ear_size"] = self.generate_shape(
            "id_ear_size",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                m * w * 0.5,
                np.zeros(len(v))
            ])
        )
        
        shapes["id_ear_angle"] = self.generate_shape(
            "id_ear_angle",
            lambda v, w, m: np.column_stack([
                np.sign(v[:, 0] - self.center[0]) * m * w,
                np.zeros(len(v)),
                -m * w * 0.5
            ])
        )
        
        return shapes


def export_to_blendshapes_bin(shapes, vertices, output_path):
    """导出为引擎二进制格式"""
    import struct
    
    MAGIC = 0x42534850
    VERSION = 1
    
    with open(output_path, 'wb') as f:
        # Header
        f.write(struct.pack('<I', MAGIC))
        f.write(struct.pack('<I', VERSION))
        f.write(struct.pack('<I', len(vertices)))
        f.write(struct.pack('<I', len(shapes)))
        
        for name, deltas in shapes.items():
            # 找出有效的 delta
            magnitudes = np.linalg.norm(deltas, axis=1)
            valid_indices = np.where(magnitudes > 0.00001)[0]
            
            # Name
            name_bytes = name.encode('utf-8')
            f.write(struct.pack('<I', len(name_bytes)))
            f.write(name_bytes)
            
            # Weight range
            f.write(struct.pack('<f', -1.0))  # min
            f.write(struct.pack('<f', 1.0))   # max
            f.write(struct.pack('<f', 0.0))   # default
            
            # Deltas
            f.write(struct.pack('<I', len(valid_indices)))
            for idx in valid_indices:
                f.write(struct.pack('<I', int(idx)))
                f.write(struct.pack('<3f', *deltas[idx]))
                f.write(struct.pack('<3f', 0, 0, 0))  # normal delta
            
            print(f"  {name}: {len(valid_indices)} deltas")
    
    print(f"\nExported to: {output_path}")


def main():
    if len(sys.argv) < 3:
        print("Usage: python auto_generate_blendshapes.py <head.obj> <landmark_vertex_map.json>")
        print("       python auto_generate_blendshapes.py head_base.obj ../landmark_vertex_map.json")
        sys.exit(1)
    
    obj_path = sys.argv[1]
    landmark_path = sys.argv[2]
    
    print(f"Loading mesh: {obj_path}")
    vertices, faces = load_obj(obj_path)
    
    print(f"Loading landmarks: {landmark_path}")
    landmark_mapping = load_landmarks(landmark_path)
    print(f"  Found {len(landmark_mapping)} landmarks")
    
    # 生成 BlendShapes
    generator = BlendShapeGenerator(vertices, faces, landmark_mapping)
    shapes = generator.generate_all_shapes()
    
    print(f"\nGenerated {len(shapes)} BlendShapes")
    
    # 导出
    output_dir = os.path.dirname(obj_path) or "."
    output_path = os.path.join(output_dir, "blendshapes.bin")
    export_to_blendshapes_bin(shapes, vertices, output_path)


if __name__ == "__main__":
    main()
