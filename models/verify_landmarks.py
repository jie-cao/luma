"""
验证 landmark_vertex_map.json 中的映射是否正确
输出一个带有标记点的 OBJ 文件，可以在 Blender 中查看
"""

import json
import os

def load_obj(path):
    """加载 OBJ 文件，返回顶点列表"""
    vertices = []
    with open(path, 'r') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                vertices.append((x, y, z))
    return vertices

def load_landmarks(json_path):
    """加载 landmark 映射"""
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    landmarks = {}
    for lm in data.get('landmarks', []):
        lm_id = lm['id']
        vertices = lm.get('vertices', [])
        if vertices:
            landmarks[lm_id] = {
                'name': lm.get('name', f'landmark_{lm_id}'),
                'vertex': vertices[0],
                'region': lm.get('region', 'unknown')
            }
    return landmarks

def export_landmark_spheres(vertices, landmarks, output_path):
    """导出一个 OBJ 文件，在每个 landmark 位置放置一个小球"""
    
    with open(output_path, 'w') as f:
        f.write("# Landmark verification mesh\n")
        f.write("# Each sphere marks a landmark position\n\n")
        
        vertex_offset = 0
        
        for lm_id in sorted(landmarks.keys()):
            lm = landmarks[lm_id]
            vertex_idx = lm['vertex']
            
            if vertex_idx >= len(vertices):
                print(f"Warning: landmark {lm_id} ({lm['name']}) has invalid vertex index {vertex_idx}")
                continue
            
            x, y, z = vertices[vertex_idx]
            
            # 创建一个小球 (8个顶点的简化球)
            radius = 0.003  # 3mm 半径
            
            # 球的顶点
            sphere_verts = [
                (x + radius, y, z),
                (x - radius, y, z),
                (x, y + radius, z),
                (x, y - radius, z),
                (x, y, z + radius),
                (x, y, z - radius),
            ]
            
            f.write(f"# Landmark {lm_id}: {lm['name']} (vertex {vertex_idx})\n")
            f.write(f"o landmark_{lm_id}_{lm['name']}\n")
            
            for vx, vy, vz in sphere_verts:
                f.write(f"v {vx:.6f} {vy:.6f} {vz:.6f}\n")
            
            # 简单的面 (八面体)
            base = vertex_offset + 1
            f.write(f"f {base} {base+2} {base+4}\n")
            f.write(f"f {base} {base+4} {base+3}\n")
            f.write(f"f {base} {base+3} {base+5}\n")
            f.write(f"f {base} {base+5} {base+2}\n")
            f.write(f"f {base+1} {base+4} {base+2}\n")
            f.write(f"f {base+1} {base+3} {base+4}\n")
            f.write(f"f {base+1} {base+5} {base+3}\n")
            f.write(f"f {base+1} {base+2} {base+5}\n")
            f.write("\n")
            
            vertex_offset += 6
    
    print(f"Exported {len(landmarks)} landmark markers to {output_path}")

def print_landmark_positions(vertices, landmarks):
    """打印每个 landmark 的位置，用于检查"""
    
    print("\n=== Landmark Positions ===")
    print(f"{'ID':>3} {'Name':<25} {'Vertex':>6} {'X':>10} {'Y':>10} {'Z':>10} {'Region':<10}")
    print("-" * 85)
    
    # 按区域分组
    regions = {}
    for lm_id, lm in landmarks.items():
        region = lm['region']
        if region not in regions:
            regions[region] = []
        regions[region].append((lm_id, lm))
    
    for region in ['jaw', 'chin', 'left_eyebrow', 'right_eyebrow', 'nose', 'left_eye', 'right_eye', 'mouth_outer', 'mouth_inner']:
        if region not in regions:
            continue
        
        print(f"\n--- {region.upper()} ---")
        for lm_id, lm in sorted(regions[region]):
            vertex_idx = lm['vertex']
            if vertex_idx < len(vertices):
                x, y, z = vertices[vertex_idx]
                print(f"{lm_id:>3} {lm['name']:<25} {vertex_idx:>6} {x:>10.4f} {y:>10.4f} {z:>10.4f} {region:<10}")
            else:
                print(f"{lm_id:>3} {lm['name']:<25} {vertex_idx:>6} INVALID")

def check_landmark_consistency(vertices, landmarks):
    """检查 landmark 位置是否合理"""
    
    print("\n=== Consistency Checks ===")
    
    # 检查左右对称性
    # iBUG 68: 0-16 是下巴轮廓，应该左右对称
    # 17-21 左眉毛, 22-26 右眉毛
    # 36-41 左眼, 42-47 右眼
    # 48-59 外嘴唇, 60-67 内嘴唇
    
    def get_pos(lm_id):
        if lm_id in landmarks:
            v = landmarks[lm_id]['vertex']
            if v < len(vertices):
                return vertices[v]
        return None
    
    # 检查下巴中点 (landmark 8) 是否在 X=0 附近
    chin = get_pos(8)
    if chin:
        print(f"Chin center (8): X = {chin[0]:.4f} (should be ~0)")
    
    # 检查鼻尖 (landmark 30) 是否在 X=0 附近
    nose_tip = get_pos(30)
    if nose_tip:
        print(f"Nose tip (30): X = {nose_tip[0]:.4f} (should be ~0)")
    
    # 检查左右眼中心是否对称
    left_eye_center = get_pos(36)
    right_eye_center = get_pos(45)
    if left_eye_center and right_eye_center:
        print(f"Left eye (36): X = {left_eye_center[0]:.4f}")
        print(f"Right eye (45): X = {right_eye_center[0]:.4f}")
        print(f"Symmetry: {abs(left_eye_center[0] + right_eye_center[0]):.4f} (should be ~0)")
    
    # 检查眉毛是否在眼睛上方
    left_brow = get_pos(19)
    left_eye = get_pos(37)
    if left_brow and left_eye:
        print(f"Left brow Y (19): {left_brow[1]:.4f}, Left eye Y (37): {left_eye[1]:.4f}")
        print(f"Brow above eye: {left_brow[1] > left_eye[1]} (should be True)")
    
    # 检查嘴巴是否在鼻子下方
    mouth_top = get_pos(51)
    nose_bottom = get_pos(33)
    if mouth_top and nose_bottom:
        print(f"Mouth top Y (51): {mouth_top[1]:.4f}, Nose bottom Y (33): {nose_bottom[1]:.4f}")
        print(f"Mouth below nose: {mouth_top[1] < nose_bottom[1]} (should be True)")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    obj_path = os.path.join(script_dir, "Super Average Head.obj")
    json_path = os.path.join(script_dir, "landmark_vertex_map.json")
    output_path = os.path.join(script_dir, "landmark_markers.obj")
    
    print(f"Loading mesh from: {obj_path}")
    vertices = load_obj(obj_path)
    print(f"Loaded {len(vertices)} vertices")
    
    print(f"Loading landmarks from: {json_path}")
    landmarks = load_landmarks(json_path)
    print(f"Loaded {len(landmarks)} landmarks")
    
    # 打印位置
    print_landmark_positions(vertices, landmarks)
    
    # 一致性检查
    check_landmark_consistency(vertices, landmarks)
    
    # 导出可视化
    export_landmark_spheres(vertices, landmarks, output_path)
    print(f"\n>>> 在 Blender 中打开 {output_path} 和 Super Average Head.obj 来验证 landmark 位置")

if __name__ == "__main__":
    main()
