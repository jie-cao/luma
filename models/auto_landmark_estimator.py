"""
Auto Landmark Estimator
=======================
基于网格几何自动估算68个关键点的顶点索引。

这个工具会根据头部网格的几何特征（位置、法线方向等）
自动推断每个关键点最可能对应的顶点。

生成的结果是初始估计，建议在Blender中手动验证和调整。

使用方法：
    python auto_landmark_estimator.py head_base.obj
    
输出：
    landmark_vertex_map.json
"""

import sys
import os
import json
import numpy as np
from collections import defaultdict

def load_obj(filepath):
    """加载OBJ文件"""
    positions = []
    normals = []
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            parts = line.split()
            if not parts:
                continue
            
            if parts[0] == 'v':
                positions.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif parts[0] == 'vn':
                normals.append([float(parts[1]), float(parts[2]), float(parts[3])])
    
    return np.array(positions), np.array(normals) if normals else None

def compute_vertex_normals(positions, faces=None):
    """计算顶点法线（简化版：使用位置到中心的方向）"""
    center = positions.mean(axis=0)
    normals = positions - center
    norms = np.linalg.norm(normals, axis=1, keepdims=True)
    normals = normals / (norms + 1e-8)
    return normals

def normalize_mesh(positions):
    """归一化网格到单位立方体"""
    min_pos = positions.min(axis=0)
    max_pos = positions.max(axis=0)
    center = (min_pos + max_pos) / 2
    size = max_pos - min_pos
    scale = 1.0 / max(size)
    
    normalized = (positions - center) * scale
    return normalized, center, scale

class LandmarkEstimator:
    """基于几何的关键点估算器"""
    
    def __init__(self, positions, normals=None):
        self.positions = positions
        self.normals = normals if normals is not None else compute_vertex_normals(positions)
        
        # 归一化坐标
        self.norm_pos, self.center, self.scale = normalize_mesh(positions)
        
        # 计算边界
        self.min_pos = self.norm_pos.min(axis=0)
        self.max_pos = self.norm_pos.max(axis=0)
        
        # 预计算一些有用的值
        self.front_mask = self.norm_pos[:, 2] > 0  # Z > 0 是正面
        
    def find_vertex_near(self, target_pos, mask=None, n=1):
        """找到最接近目标位置的顶点"""
        if mask is None:
            mask = np.ones(len(self.norm_pos), dtype=bool)
        
        valid_indices = np.where(mask)[0]
        if len(valid_indices) == 0:
            return [] if n > 1 else -1
        
        valid_pos = self.norm_pos[valid_indices]
        distances = np.linalg.norm(valid_pos - target_pos, axis=1)
        
        if n == 1:
            best_idx = np.argmin(distances)
            return int(valid_indices[best_idx])
        else:
            sorted_indices = np.argsort(distances)[:n]
            return [int(valid_indices[i]) for i in sorted_indices]
    
    def find_frontmost_at_height(self, y_ratio, x_ratio=0.5):
        """在指定高度找到最前面的顶点"""
        y_target = self.min_pos[1] + (self.max_pos[1] - self.min_pos[1]) * y_ratio
        x_target = self.min_pos[0] + (self.max_pos[0] - self.min_pos[0]) * x_ratio
        
        # 在目标高度附近的顶点
        y_tolerance = (self.max_pos[1] - self.min_pos[1]) * 0.05
        height_mask = np.abs(self.norm_pos[:, 1] - y_target) < y_tolerance
        
        # 在目标X位置附近
        x_tolerance = (self.max_pos[0] - self.min_pos[0]) * 0.1
        x_mask = np.abs(self.norm_pos[:, 0] - x_target) < x_tolerance
        
        mask = height_mask & x_mask & self.front_mask
        
        if not mask.any():
            mask = height_mask & self.front_mask
        
        if not mask.any():
            return self.find_vertex_near([x_target, y_target, 0.5])
        
        # 找最前面的
        valid_indices = np.where(mask)[0]
        frontmost_idx = valid_indices[np.argmax(self.norm_pos[valid_indices, 2])]
        return int(frontmost_idx)
    
    def estimate_jaw_contour(self):
        """估算下颌轮廓（0-16）"""
        landmarks = []
        
        # 下颌轮廓从右耳到左耳，经过下巴
        # Y坐标大约在 0.1-0.4 范围（下半脸）
        
        for i in range(17):
            # 从右到左
            angle = np.pi * i / 16  # 0 到 π
            
            # X位置：从右(-0.5)到左(0.5)
            x_ratio = 0.5 - 0.5 * np.cos(angle)
            
            # Y位置：中间最低（下巴），两边较高（耳朵下方）
            y_base = 0.25  # 基准高度
            y_offset = 0.15 * np.sin(angle)  # 中间低，两边高
            y_ratio = y_base - y_offset + 0.1
            
            # 找到对应顶点
            target = [
                self.min_pos[0] + (self.max_pos[0] - self.min_pos[0]) * x_ratio,
                self.min_pos[1] + (self.max_pos[1] - self.min_pos[1]) * y_ratio,
                0.3  # 前面
            ]
            
            # 下巴区域要更前面
            if 5 <= i <= 11:
                target[2] = 0.5
            
            idx = self.find_vertex_near(target, self.front_mask)
            landmarks.append(idx)
        
        return landmarks
    
    def estimate_eyebrows(self):
        """估算眉毛（17-26）"""
        landmarks = []
        
        # 左眉毛 (17-21)
        for i in range(5):
            x_ratio = 0.55 + i * 0.08  # 从内到外
            y_ratio = 0.72 - i * 0.01  # 略微下降
            idx = self.find_frontmost_at_height(y_ratio, x_ratio)
            landmarks.append(idx)
        
        # 右眉毛 (22-26)
        for i in range(5):
            x_ratio = 0.45 - i * 0.08  # 从内到外
            y_ratio = 0.72 - i * 0.01
            idx = self.find_frontmost_at_height(y_ratio, x_ratio)
            landmarks.append(idx)
        
        return landmarks
    
    def estimate_nose(self):
        """估算鼻子（27-35）"""
        landmarks = []
        
        # 鼻梁 (27-30)
        for i in range(4):
            y_ratio = 0.68 - i * 0.07
            idx = self.find_frontmost_at_height(y_ratio, 0.5)
            landmarks.append(idx)
        
        # 鼻底 (31-35)
        nose_y = 0.42
        nose_x_positions = [0.58, 0.54, 0.50, 0.46, 0.42]  # 左翼到右翼
        for x_ratio in nose_x_positions:
            idx = self.find_frontmost_at_height(nose_y, x_ratio)
            landmarks.append(idx)
        
        return landmarks
    
    def estimate_eyes(self):
        """估算眼睛（36-47）"""
        landmarks = []
        
        eye_y = 0.62
        
        # 左眼 (36-41)
        left_eye_x = [0.58, 0.60, 0.64, 0.66, 0.64, 0.60]
        left_eye_y = [eye_y, eye_y + 0.02, eye_y + 0.02, eye_y, eye_y - 0.02, eye_y - 0.02]
        for x, y in zip(left_eye_x, left_eye_y):
            idx = self.find_frontmost_at_height(y, x)
            landmarks.append(idx)
        
        # 右眼 (42-47)
        right_eye_x = [0.42, 0.40, 0.36, 0.34, 0.36, 0.40]
        right_eye_y = [eye_y, eye_y + 0.02, eye_y + 0.02, eye_y, eye_y - 0.02, eye_y - 0.02]
        for x, y in zip(right_eye_x, right_eye_y):
            idx = self.find_frontmost_at_height(y, x)
            landmarks.append(idx)
        
        return landmarks
    
    def estimate_mouth(self):
        """估算嘴巴（48-67）"""
        landmarks = []
        
        mouth_y = 0.32
        
        # 外嘴唇 (48-59)
        outer_x = [0.58, 0.56, 0.53, 0.50, 0.47, 0.44, 0.42, 0.44, 0.47, 0.50, 0.53, 0.56]
        outer_y = [mouth_y, mouth_y + 0.02, mouth_y + 0.03, mouth_y + 0.035, 
                   mouth_y + 0.03, mouth_y + 0.02, mouth_y,
                   mouth_y - 0.02, mouth_y - 0.03, mouth_y - 0.035,
                   mouth_y - 0.03, mouth_y - 0.02]
        
        for x, y in zip(outer_x, outer_y):
            idx = self.find_frontmost_at_height(y, x)
            landmarks.append(idx)
        
        # 内嘴唇 (60-67)
        inner_x = [0.56, 0.53, 0.50, 0.47, 0.44, 0.47, 0.50, 0.53]
        inner_y = [mouth_y, mouth_y + 0.015, mouth_y + 0.02, mouth_y + 0.015,
                   mouth_y, mouth_y - 0.015, mouth_y - 0.02, mouth_y - 0.015]
        
        for x, y in zip(inner_x, inner_y):
            idx = self.find_frontmost_at_height(y, x)
            landmarks.append(idx)
        
        return landmarks
    
    def estimate_all(self):
        """估算所有68个关键点"""
        landmarks = []
        
        # 下颌 0-16
        landmarks.extend(self.estimate_jaw_contour())
        
        # 眉毛 17-26
        landmarks.extend(self.estimate_eyebrows())
        
        # 鼻子 27-35
        landmarks.extend(self.estimate_nose())
        
        # 眼睛 36-47
        landmarks.extend(self.estimate_eyes())
        
        # 嘴巴 48-67
        landmarks.extend(self.estimate_mouth())
        
        return landmarks


# 关键点定义
LANDMARK_DEFINITIONS = [
    {"id": 0,  "name": "jaw_right_ear",      "region": "jaw"},
    {"id": 1,  "name": "jaw_right_1",        "region": "jaw"},
    {"id": 2,  "name": "jaw_right_2",        "region": "jaw"},
    {"id": 3,  "name": "jaw_right_3",        "region": "jaw"},
    {"id": 4,  "name": "jaw_right_4",        "region": "jaw"},
    {"id": 5,  "name": "jaw_right_chin",     "region": "jaw"},
    {"id": 6,  "name": "jaw_chin_right",     "region": "chin"},
    {"id": 7,  "name": "jaw_chin_center",    "region": "chin"},
    {"id": 8,  "name": "jaw_chin_bottom",    "region": "chin"},
    {"id": 9,  "name": "jaw_chin_left",      "region": "chin"},
    {"id": 10, "name": "jaw_left_chin",      "region": "jaw"},
    {"id": 11, "name": "jaw_left_4",         "region": "jaw"},
    {"id": 12, "name": "jaw_left_3",         "region": "jaw"},
    {"id": 13, "name": "jaw_left_2",         "region": "jaw"},
    {"id": 14, "name": "jaw_left_1",         "region": "jaw"},
    {"id": 15, "name": "jaw_left_ear_low",   "region": "jaw"},
    {"id": 16, "name": "jaw_left_ear",       "region": "jaw"},
    {"id": 17, "name": "left_brow_inner",    "region": "brow"},
    {"id": 18, "name": "left_brow_1",        "region": "brow"},
    {"id": 19, "name": "left_brow_center",   "region": "brow"},
    {"id": 20, "name": "left_brow_2",        "region": "brow"},
    {"id": 21, "name": "left_brow_outer",    "region": "brow"},
    {"id": 22, "name": "right_brow_inner",   "region": "brow"},
    {"id": 23, "name": "right_brow_1",       "region": "brow"},
    {"id": 24, "name": "right_brow_center",  "region": "brow"},
    {"id": 25, "name": "right_brow_2",       "region": "brow"},
    {"id": 26, "name": "right_brow_outer",   "region": "brow"},
    {"id": 27, "name": "nose_bridge_top",    "region": "nose"},
    {"id": 28, "name": "nose_bridge_1",      "region": "nose"},
    {"id": 29, "name": "nose_bridge_2",      "region": "nose"},
    {"id": 30, "name": "nose_bridge_bottom", "region": "nose"},
    {"id": 31, "name": "nose_left_wing",     "region": "nose"},
    {"id": 32, "name": "nose_left_nostril",  "region": "nose"},
    {"id": 33, "name": "nose_tip",           "region": "nose"},
    {"id": 34, "name": "nose_right_nostril", "region": "nose"},
    {"id": 35, "name": "nose_right_wing",    "region": "nose"},
    {"id": 36, "name": "left_eye_inner",     "region": "eye"},
    {"id": 37, "name": "left_eye_top_inner", "region": "eye"},
    {"id": 38, "name": "left_eye_top_outer", "region": "eye"},
    {"id": 39, "name": "left_eye_outer",     "region": "eye"},
    {"id": 40, "name": "left_eye_bot_outer", "region": "eye"},
    {"id": 41, "name": "left_eye_bot_inner", "region": "eye"},
    {"id": 42, "name": "right_eye_inner",    "region": "eye"},
    {"id": 43, "name": "right_eye_top_inner","region": "eye"},
    {"id": 44, "name": "right_eye_top_outer","region": "eye"},
    {"id": 45, "name": "right_eye_outer",    "region": "eye"},
    {"id": 46, "name": "right_eye_bot_outer","region": "eye"},
    {"id": 47, "name": "right_eye_bot_inner","region": "eye"},
    {"id": 48, "name": "mouth_left_corner",  "region": "mouth"},
    {"id": 49, "name": "mouth_top_left_1",   "region": "mouth"},
    {"id": 50, "name": "mouth_top_left_2",   "region": "mouth"},
    {"id": 51, "name": "mouth_top_center",   "region": "mouth"},
    {"id": 52, "name": "mouth_top_right_2",  "region": "mouth"},
    {"id": 53, "name": "mouth_top_right_1",  "region": "mouth"},
    {"id": 54, "name": "mouth_right_corner", "region": "mouth"},
    {"id": 55, "name": "mouth_bot_right_1",  "region": "mouth"},
    {"id": 56, "name": "mouth_bot_right_2",  "region": "mouth"},
    {"id": 57, "name": "mouth_bot_center",   "region": "mouth"},
    {"id": 58, "name": "mouth_bot_left_2",   "region": "mouth"},
    {"id": 59, "name": "mouth_bot_left_1",   "region": "mouth"},
    {"id": 60, "name": "mouth_inner_left",   "region": "mouth"},
    {"id": 61, "name": "mouth_inner_top_l",  "region": "mouth"},
    {"id": 62, "name": "mouth_inner_top_c",  "region": "mouth"},
    {"id": 63, "name": "mouth_inner_top_r",  "region": "mouth"},
    {"id": 64, "name": "mouth_inner_right",  "region": "mouth"},
    {"id": 65, "name": "mouth_inner_bot_r",  "region": "mouth"},
    {"id": 66, "name": "mouth_inner_bot_c",  "region": "mouth"},
    {"id": 67, "name": "mouth_inner_bot_l",  "region": "mouth"},
]


def main():
    if len(sys.argv) < 2:
        print("使用方法: python auto_landmark_estimator.py <head.obj>")
        print("示例: python auto_landmark_estimator.py head_base.obj")
        sys.exit(1)
    
    obj_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "landmark_vertex_map.json"
    
    if not os.path.exists(obj_path):
        print(f"错误: 文件不存在: {obj_path}")
        sys.exit(1)
    
    print(f"加载网格: {obj_path}")
    positions, normals = load_obj(obj_path)
    print(f"顶点数: {len(positions)}")
    
    print("估算关键点...")
    estimator = LandmarkEstimator(positions, normals)
    vertex_indices = estimator.estimate_all()
    
    print(f"生成 {len(vertex_indices)} 个关键点映射")
    
    # 构建输出数据
    landmarks = []
    for i, vi in enumerate(vertex_indices):
        lm_def = LANDMARK_DEFINITIONS[i]
        landmarks.append({
            "id": i,
            "name": lm_def["name"],
            "region": lm_def["region"],
            "vertices": [vi],
            "weights": [1.0],
            "auto_estimated": True
        })
    
    data = {
        "version": "1.0",
        "meshFile": os.path.basename(obj_path),
        "landmarkStandard": "ibug68",
        "landmarkCount": 68,
        "markedCount": 68,
        "auto_estimated": True,
        "note": "此文件由 auto_landmark_estimator.py 自动生成，建议在Blender中验证和调整",
        "landmarks": landmarks
    }
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    
    print(f"\n已保存到: {output_path}")
    print("\n注意: 自动估算的关键点可能不够准确，建议：")
    print("  1. 在Blender中打开网格和此JSON文件")
    print("  2. 验证每个关键点位置")
    print("  3. 使用 blender_landmark_marker.py 手动调整不准确的点")


if __name__ == '__main__':
    main()
