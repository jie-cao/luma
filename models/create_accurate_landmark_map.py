"""
创建准确的 iBUG 68 landmark 到网格顶点的映射
基于网格的几何结构和解剖学位置来定位

iBUG 68 landmark 布局:
- 0-16: 下巴轮廓 (从右耳到左耳)
- 17-21: 左眉毛 (从内到外)
- 22-26: 右眉毛 (从内到外)
- 27-30: 鼻梁
- 31-35: 鼻子底部 (左翼、左鼻孔、鼻尖、右鼻孔、右翼)
- 36-41: 左眼 (顺时针从外角开始)
- 42-47: 右眼 (顺时针从内角开始)
- 48-59: 外嘴唇 (顺时针从左角开始)
- 60-67: 内嘴唇 (顺时针从左角开始)
"""

import json
import os
import numpy as np

def load_obj(path):
    """加载 OBJ 文件"""
    vertices = []
    with open(path, 'r') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                vertices.append([x, y, z])
    return np.array(vertices)

def find_vertex_at_position(vertices, target_x, target_y, target_z, 
                            x_tolerance=0.1, y_tolerance=0.02, z_tolerance=0.1):
    """在指定位置附近找到最近的顶点"""
    # 首先按 Y 坐标筛选
    y_mask = np.abs(vertices[:, 1] - target_y) < y_tolerance
    candidates = np.where(y_mask)[0]
    
    if len(candidates) == 0:
        # 放宽 Y 容差
        y_mask = np.abs(vertices[:, 1] - target_y) < y_tolerance * 2
        candidates = np.where(y_mask)[0]
    
    if len(candidates) == 0:
        return None
    
    # 在候选中找最近的
    candidate_verts = vertices[candidates]
    target = np.array([target_x, target_y, target_z])
    distances = np.linalg.norm(candidate_verts - target, axis=1)
    
    best_idx = candidates[np.argmin(distances)]
    return int(best_idx)

def find_vertex_by_criteria(vertices, x_range=None, y_range=None, z_range=None, 
                            prefer='closest_to_center', center_x=0.0):
    """根据范围条件找顶点"""
    mask = np.ones(len(vertices), dtype=bool)
    
    if x_range:
        mask &= (vertices[:, 0] >= x_range[0]) & (vertices[:, 0] <= x_range[1])
    if y_range:
        mask &= (vertices[:, 1] >= y_range[0]) & (vertices[:, 1] <= y_range[1])
    if z_range:
        mask &= (vertices[:, 2] >= z_range[0]) & (vertices[:, 2] <= z_range[1])
    
    candidates = np.where(mask)[0]
    if len(candidates) == 0:
        return None
    
    if prefer == 'closest_to_center':
        # 找 X 最接近 center_x 的
        x_dists = np.abs(vertices[candidates, 0] - center_x)
        return int(candidates[np.argmin(x_dists)])
    elif prefer == 'max_z':
        # 找 Z 最大的 (最前面)
        return int(candidates[np.argmax(vertices[candidates, 2])])
    elif prefer == 'min_y':
        # 找 Y 最小的 (最下面)
        return int(candidates[np.argmin(vertices[candidates, 1])])
    elif prefer == 'max_y':
        # 找 Y 最大的 (最上面)
        return int(candidates[np.argmax(vertices[candidates, 1])])
    
    return int(candidates[0])

def analyze_mesh(vertices):
    """分析网格的基本几何特征"""
    print("=== Mesh Analysis ===")
    print(f"Vertex count: {len(vertices)}")
    print(f"X range: {vertices[:, 0].min():.4f} to {vertices[:, 0].max():.4f}")
    print(f"Y range: {vertices[:, 1].min():.4f} to {vertices[:, 1].max():.4f}")
    print(f"Z range: {vertices[:, 2].min():.4f} to {vertices[:, 2].max():.4f}")
    
    # 找中心线上的顶点 (X ≈ 0)
    center_mask = np.abs(vertices[:, 0]) < 0.005
    center_verts = vertices[center_mask]
    print(f"\nCenter line vertices (|X| < 0.005): {len(center_verts)}")
    if len(center_verts) > 0:
        print(f"  Y range: {center_verts[:, 1].min():.4f} to {center_verts[:, 1].max():.4f}")
        print(f"  Z range: {center_verts[:, 2].min():.4f} to {center_verts[:, 2].max():.4f}")
    
    return {
        'x_min': vertices[:, 0].min(),
        'x_max': vertices[:, 0].max(),
        'y_min': vertices[:, 1].min(),
        'y_max': vertices[:, 1].max(),
        'z_min': vertices[:, 2].min(),
        'z_max': vertices[:, 2].max(),
        'width': vertices[:, 0].max() - vertices[:, 0].min(),
        'height': vertices[:, 1].max() - vertices[:, 1].min(),
    }

def create_landmark_mapping(vertices):
    """创建 68 个 landmark 的映射"""
    
    stats = analyze_mesh(vertices)
    
    # 估算关键 Y 坐标 (基于头部比例)
    head_height = stats['height']
    y_base = stats['y_min']
    
    # 典型人脸比例 (从下巴到头顶)
    chin_y = y_base + head_height * 0.08      # 下巴
    mouth_y = y_base + head_height * 0.18     # 嘴巴
    nose_tip_y = y_base + head_height * 0.30  # 鼻尖
    nose_bridge_y = y_base + head_height * 0.45  # 鼻梁
    eye_y = y_base + head_height * 0.50       # 眼睛
    brow_y = y_base + head_height * 0.58      # 眉毛
    
    # 估算关键 X 坐标
    face_width = stats['width'] * 0.8  # 脸宽约为头宽的 80%
    eye_x = face_width * 0.18          # 眼睛到中心的距离
    brow_outer_x = face_width * 0.28   # 眉毛外侧
    nose_wing_x = face_width * 0.10    # 鼻翼
    mouth_corner_x = face_width * 0.15 # 嘴角
    jaw_x = face_width * 0.5           # 下巴轮廓最宽处
    
    print(f"\n=== Estimated Positions ===")
    print(f"Chin Y: {chin_y:.4f}")
    print(f"Mouth Y: {mouth_y:.4f}")
    print(f"Nose tip Y: {nose_tip_y:.4f}")
    print(f"Eye Y: {eye_y:.4f}")
    print(f"Brow Y: {brow_y:.4f}")
    print(f"Eye X offset: {eye_x:.4f}")
    
    landmarks = {}
    
    # === 下巴轮廓 (0-16) ===
    # 从右耳到左耳，沿着下巴轮廓
    jaw_points = 17
    for i in range(jaw_points):
        t = i / (jaw_points - 1)  # 0 到 1
        
        # X: 从右 (-) 到左 (+)
        if t < 0.5:
            # 右半边
            x = -jaw_x * (1 - t * 2)
        else:
            # 左半边
            x = jaw_x * ((t - 0.5) * 2)
        
        # Y: 在下巴附近，中间最低
        y_offset = abs(t - 0.5) * 2  # 0 在中间，1 在两端
        y = chin_y + y_offset * head_height * 0.15
        
        # Z: 中间最前
        z_offset = 1 - abs(t - 0.5) * 2
        z = stats['z_max'] * 0.7 + z_offset * 0.05
        
        name = ['jaw_right_ear', 'jaw_right_1', 'jaw_right_2', 'jaw_right_3', 
                'jaw_right_4', 'jaw_right_chin', 'jaw_chin_right', 'jaw_chin_center',
                'jaw_chin_bottom', 'jaw_chin_left', 'jaw_left_chin', 'jaw_left_4',
                'jaw_left_3', 'jaw_left_2', 'jaw_left_1', 'jaw_left_ear_low', 'jaw_left_ear'][i]
        
        vertex = find_vertex_at_position(vertices, x, y, z, 
                                         x_tolerance=0.02, y_tolerance=0.015, z_tolerance=0.05)
        if vertex is None:
            vertex = find_vertex_by_criteria(vertices, 
                                             x_range=(x-0.03, x+0.03),
                                             y_range=(y-0.02, y+0.02),
                                             prefer='max_z')
        landmarks[i] = {'name': name, 'vertex': vertex, 'region': 'jaw' if i < 5 or i > 11 else 'chin'}
    
    # === 左眉毛 (17-21) ===
    for i, (name, x_mult) in enumerate([
        ('left_brow_inner', 0.3),
        ('left_brow_1', 0.5),
        ('left_brow_center', 0.7),
        ('left_brow_2', 0.9),
        ('left_brow_outer', 1.1),
    ]):
        x = brow_outer_x * x_mult
        y = brow_y + (0.5 - abs(x_mult - 0.7)) * 0.01  # 中间略高
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.01, x+0.01),
                                         y_range=(y-0.015, y+0.015),
                                         prefer='max_z')
        landmarks[17 + i] = {'name': name, 'vertex': vertex, 'region': 'left_eyebrow'}
    
    # === 右眉毛 (22-26) ===
    for i, (name, x_mult) in enumerate([
        ('right_brow_inner', 0.3),
        ('right_brow_1', 0.5),
        ('right_brow_center', 0.7),
        ('right_brow_2', 0.9),
        ('right_brow_outer', 1.1),
    ]):
        x = -brow_outer_x * x_mult  # 负 X 是右边
        y = brow_y + (0.5 - abs(x_mult - 0.7)) * 0.01
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.01, x+0.01),
                                         y_range=(y-0.015, y+0.015),
                                         prefer='max_z')
        landmarks[22 + i] = {'name': name, 'vertex': vertex, 'region': 'right_eyebrow'}
    
    # === 鼻梁 (27-30) ===
    nose_bridge_positions = [
        ('nose_bridge_top', nose_bridge_y + 0.02),
        ('nose_bridge_1', nose_bridge_y),
        ('nose_bridge_2', nose_bridge_y - 0.015),
        ('nose_bridge_bottom', nose_tip_y + 0.015),
    ]
    for i, (name, y) in enumerate(nose_bridge_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(-0.005, 0.005),
                                         y_range=(y-0.01, y+0.01),
                                         prefer='max_z')
        landmarks[27 + i] = {'name': name, 'vertex': vertex, 'region': 'nose'}
    
    # === 鼻子底部 (31-35) ===
    nose_bottom_positions = [
        ('nose_left_wing', nose_wing_x, nose_tip_y - 0.005),
        ('nose_left_nostril', nose_wing_x * 0.5, nose_tip_y - 0.008),
        ('nose_tip', 0.0, nose_tip_y),
        ('nose_right_nostril', -nose_wing_x * 0.5, nose_tip_y - 0.008),
        ('nose_right_wing', -nose_wing_x, nose_tip_y - 0.005),
    ]
    for i, (name, x, y) in enumerate(nose_bottom_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.008, x+0.008),
                                         y_range=(y-0.01, y+0.01),
                                         prefer='max_z')
        landmarks[31 + i] = {'name': name, 'vertex': vertex, 'region': 'nose'}
    
    # === 左眼 (36-41) ===
    # 顺时针从外角开始
    left_eye_positions = [
        ('left_eye_outer', eye_x + 0.012, eye_y),
        ('left_eye_top_outer', eye_x + 0.006, eye_y + 0.008),
        ('left_eye_top_inner', eye_x - 0.006, eye_y + 0.008),
        ('left_eye_inner', eye_x - 0.012, eye_y),
        ('left_eye_bottom_inner', eye_x - 0.006, eye_y - 0.006),
        ('left_eye_bottom_outer', eye_x + 0.006, eye_y - 0.006),
    ]
    for i, (name, x, y) in enumerate(left_eye_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.008, x+0.008),
                                         y_range=(y-0.008, y+0.008),
                                         prefer='max_z')
        landmarks[36 + i] = {'name': name, 'vertex': vertex, 'region': 'left_eye'}
    
    # === 右眼 (42-47) ===
    right_eye_positions = [
        ('right_eye_inner', -eye_x + 0.012, eye_y),
        ('right_eye_top_inner', -eye_x + 0.006, eye_y + 0.008),
        ('right_eye_top_outer', -eye_x - 0.006, eye_y + 0.008),
        ('right_eye_outer', -eye_x - 0.012, eye_y),
        ('right_eye_bottom_outer', -eye_x - 0.006, eye_y - 0.006),
        ('right_eye_bottom_inner', -eye_x + 0.006, eye_y - 0.006),
    ]
    for i, (name, x, y) in enumerate(right_eye_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.008, x+0.008),
                                         y_range=(y-0.008, y+0.008),
                                         prefer='max_z')
        landmarks[42 + i] = {'name': name, 'vertex': vertex, 'region': 'right_eye'}
    
    # === 外嘴唇 (48-59) ===
    mouth_y_top = mouth_y + 0.008
    mouth_y_bottom = mouth_y - 0.012
    outer_mouth_positions = [
        ('mouth_left', mouth_corner_x, mouth_y),
        ('mouth_top_left_1', mouth_corner_x * 0.6, mouth_y_top - 0.002),
        ('mouth_top_left_2', mouth_corner_x * 0.3, mouth_y_top),
        ('mouth_top_center', 0.0, mouth_y_top + 0.002),
        ('mouth_top_right_2', -mouth_corner_x * 0.3, mouth_y_top),
        ('mouth_top_right_1', -mouth_corner_x * 0.6, mouth_y_top - 0.002),
        ('mouth_right', -mouth_corner_x, mouth_y),
        ('mouth_bottom_right_1', -mouth_corner_x * 0.6, mouth_y_bottom + 0.002),
        ('mouth_bottom_right_2', -mouth_corner_x * 0.3, mouth_y_bottom),
        ('mouth_bottom_center', 0.0, mouth_y_bottom),
        ('mouth_bottom_left_2', mouth_corner_x * 0.3, mouth_y_bottom),
        ('mouth_bottom_left_1', mouth_corner_x * 0.6, mouth_y_bottom + 0.002),
    ]
    for i, (name, x, y) in enumerate(outer_mouth_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.008, x+0.008),
                                         y_range=(y-0.008, y+0.008),
                                         prefer='max_z')
        landmarks[48 + i] = {'name': name, 'vertex': vertex, 'region': 'mouth_outer'}
    
    # === 内嘴唇 (60-67) ===
    inner_mouth_y_top = mouth_y + 0.004
    inner_mouth_y_bottom = mouth_y - 0.006
    inner_mouth_positions = [
        ('inner_mouth_left', mouth_corner_x * 0.7, mouth_y),
        ('inner_mouth_top_left', mouth_corner_x * 0.35, inner_mouth_y_top),
        ('inner_mouth_top_center', 0.0, inner_mouth_y_top + 0.002),
        ('inner_mouth_top_right', -mouth_corner_x * 0.35, inner_mouth_y_top),
        ('inner_mouth_right', -mouth_corner_x * 0.7, mouth_y),
        ('inner_mouth_bottom_right', -mouth_corner_x * 0.35, inner_mouth_y_bottom),
        ('inner_mouth_bottom_center', 0.0, inner_mouth_y_bottom),
        ('inner_mouth_bottom_left', mouth_corner_x * 0.35, inner_mouth_y_bottom),
    ]
    for i, (name, x, y) in enumerate(inner_mouth_positions):
        vertex = find_vertex_by_criteria(vertices,
                                         x_range=(x-0.008, x+0.008),
                                         y_range=(y-0.006, y+0.006),
                                         prefer='max_z')
        landmarks[60 + i] = {'name': name, 'vertex': vertex, 'region': 'mouth_inner'}
    
    return landmarks

def save_landmark_map(landmarks, output_path):
    """保存 landmark 映射到 JSON"""
    data = {
        "version": "1.0",
        "meshFile": "Super Average Head.obj",
        "landmarkStandard": "ibug68",
        "landmarkCount": 68,
        "markedCount": sum(1 for lm in landmarks.values() if lm['vertex'] is not None),
        "auto_estimated": False,
        "note": "由 create_accurate_landmark_map.py 基于几何分析生成",
        "landmarks": []
    }
    
    for lm_id in range(68):
        if lm_id in landmarks:
            lm = landmarks[lm_id]
            entry = {
                "id": lm_id,
                "name": lm['name'],
                "region": lm['region'],
                "vertices": [lm['vertex']] if lm['vertex'] is not None else [],
                "weights": [1.0] if lm['vertex'] is not None else [],
                "auto_estimated": False
            }
        else:
            entry = {
                "id": lm_id,
                "name": f"landmark_{lm_id}",
                "region": "unknown",
                "vertices": [],
                "weights": [],
                "auto_estimated": True
            }
        data["landmarks"].append(entry)
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    
    print(f"\nSaved to {output_path}")
    print(f"Marked: {data['markedCount']}/68 landmarks")

def verify_landmarks(vertices, landmarks):
    """验证 landmark 位置"""
    print("\n=== Landmark Verification ===")
    
    # 检查是否有 None
    missing = [lm_id for lm_id, lm in landmarks.items() if lm['vertex'] is None]
    if missing:
        print(f"WARNING: Missing vertices for landmarks: {missing}")
    
    # 检查对称性
    def get_pos(lm_id):
        if lm_id in landmarks and landmarks[lm_id]['vertex'] is not None:
            return vertices[landmarks[lm_id]['vertex']]
        return None
    
    print("\nSymmetry check:")
    pairs = [
        (36, 45, "Eye outer corners"),
        (39, 42, "Eye inner corners"),
        (17, 26, "Brow outer"),
        (21, 22, "Brow inner"),
        (48, 54, "Mouth corners"),
        (31, 35, "Nose wings"),
    ]
    for left, right, name in pairs:
        l_pos = get_pos(left)
        r_pos = get_pos(right)
        if l_pos is not None and r_pos is not None:
            sym_error = abs(l_pos[0] + r_pos[0])
            y_diff = abs(l_pos[1] - r_pos[1])
            print(f"  {name}: X symmetry error = {sym_error:.4f}, Y diff = {y_diff:.4f}")
    
    # 检查中心线
    print("\nCenter line check:")
    center_landmarks = [8, 27, 28, 29, 30, 33, 51, 57, 62, 66]
    for lm_id in center_landmarks:
        pos = get_pos(lm_id)
        if pos is not None:
            name = landmarks[lm_id]['name']
            print(f"  {lm_id} ({name}): X = {pos[0]:.4f}")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    obj_path = os.path.join(script_dir, "Super Average Head.obj")
    output_path = os.path.join(script_dir, "landmark_vertex_map.json")
    
    print(f"Loading mesh from: {obj_path}")
    vertices = load_obj(obj_path)
    
    landmarks = create_landmark_mapping(vertices)
    verify_landmarks(vertices, landmarks)
    save_landmark_map(landmarks, output_path)
    
    # 也运行验证脚本
    print("\n" + "="*50)
    print("Running verification...")
    import subprocess
    subprocess.run(["python", os.path.join(script_dir, "verify_landmarks.py")])

if __name__ == "__main__":
    main()
