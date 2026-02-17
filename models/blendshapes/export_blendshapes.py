"""
Blender 脚本：导出 Shape Keys 为引擎可用的二进制格式

使用方法（命令行）：
    blender head_with_shapes.blend --background --python export_blendshapes.py

或在 Blender 中直接运行此脚本。

输出格式 (.blendshapes):
    Header:
        uint32: magic (0x42534850 = "BSHP")
        uint32: version (1)
        uint32: vertex_count
        uint32: shape_count
    
    For each shape:
        uint32: name_length
        char[name_length]: name (UTF-8)
        float: min_weight
        float: max_weight
        float: default_weight
        uint32: delta_count
        For each delta:
            uint32: vertex_index
            float[3]: position_delta
            float[3]: normal_delta
"""

import bpy
import struct
import os
import sys
import numpy as np
from mathlib import Vector

MAGIC = 0x42534850  # "BSHP"
VERSION = 1
DELTA_THRESHOLD = 0.00001  # 忽略小于此值的变形


def calculate_vertex_normals(mesh, shape_key_index):
    """计算指定 Shape Key 状态下的顶点法线"""
    # 这是一个简化版本，实际应该重新计算法线
    # 但对于小变形，法线变化通常很小
    return None


def export_blendshapes(obj, output_path):
    """导出对象的所有 Shape Keys"""
    
    if obj.data.shape_keys is None:
        print("ERROR: Object has no shape keys!")
        return False
    
    mesh = obj.data
    key_blocks = obj.data.shape_keys.key_blocks
    
    # 获取 Basis（基础形状）
    if "Basis" not in key_blocks:
        print("ERROR: No 'Basis' shape key found!")
        return False
    
    basis = key_blocks["Basis"]
    basis_coords = np.array([v.co[:] for v in basis.data])
    
    vertex_count = len(basis_coords)
    
    # 收集所有非 Basis 的 Shape Keys
    shapes_to_export = []
    for kb in key_blocks:
        if kb.name == "Basis":
            continue
        
        # 计算与 Basis 的差异
        shape_coords = np.array([v.co[:] for v in kb.data])
        deltas = shape_coords - basis_coords
        
        # 找出有显著变化的顶点
        delta_magnitudes = np.linalg.norm(deltas, axis=1)
        significant_indices = np.where(delta_magnitudes > DELTA_THRESHOLD)[0]
        
        if len(significant_indices) == 0:
            print(f"  SKIP: {kb.name} (no significant deltas)")
            continue
        
        shapes_to_export.append({
            "name": kb.name,
            "min_weight": kb.slider_min,
            "max_weight": kb.slider_max,
            "default_weight": 0.0,
            "deltas": [
                {
                    "vertex_index": int(idx),
                    "position_delta": deltas[idx].tolist(),
                    "normal_delta": [0.0, 0.0, 0.0]  # 简化：不计算法线变化
                }
                for idx in significant_indices
            ]
        })
        
        print(f"  + {kb.name}: {len(significant_indices)} deltas")
    
    if len(shapes_to_export) == 0:
        print("ERROR: No shapes with significant deltas to export!")
        return False
    
    # 写入二进制文件
    with open(output_path, 'wb') as f:
        # Header
        f.write(struct.pack('<I', MAGIC))
        f.write(struct.pack('<I', VERSION))
        f.write(struct.pack('<I', vertex_count))
        f.write(struct.pack('<I', len(shapes_to_export)))
        
        # Shapes
        for shape in shapes_to_export:
            # Name
            name_bytes = shape["name"].encode('utf-8')
            f.write(struct.pack('<I', len(name_bytes)))
            f.write(name_bytes)
            
            # Weight range
            f.write(struct.pack('<f', shape["min_weight"]))
            f.write(struct.pack('<f', shape["max_weight"]))
            f.write(struct.pack('<f', shape["default_weight"]))
            
            # Deltas
            f.write(struct.pack('<I', len(shape["deltas"])))
            for delta in shape["deltas"]:
                f.write(struct.pack('<I', delta["vertex_index"]))
                f.write(struct.pack('<3f', *delta["position_delta"]))
                f.write(struct.pack('<3f', *delta["normal_delta"]))
    
    print(f"\nExported {len(shapes_to_export)} shapes to: {output_path}")
    print(f"File size: {os.path.getsize(output_path)} bytes")
    
    return True


def export_blendshapes_json(obj, output_path):
    """导出为 JSON 格式（用于调试和查看）"""
    import json
    
    if obj.data.shape_keys is None:
        return False
    
    key_blocks = obj.data.shape_keys.key_blocks
    basis = key_blocks["Basis"]
    basis_coords = np.array([v.co[:] for v in basis.data])
    
    data = {
        "vertex_count": len(basis_coords),
        "shapes": []
    }
    
    for kb in key_blocks:
        if kb.name == "Basis":
            continue
        
        shape_coords = np.array([v.co[:] for v in kb.data])
        deltas = shape_coords - basis_coords
        delta_magnitudes = np.linalg.norm(deltas, axis=1)
        significant_indices = np.where(delta_magnitudes > DELTA_THRESHOLD)[0]
        
        if len(significant_indices) == 0:
            continue
        
        # 只保存前几个 delta 用于预览
        preview_deltas = [
            {
                "vertex": int(idx),
                "delta": deltas[idx].tolist(),
                "magnitude": float(delta_magnitudes[idx])
            }
            for idx in significant_indices[:10]
        ]
        
        data["shapes"].append({
            "name": kb.name,
            "delta_count": len(significant_indices),
            "max_delta": float(np.max(delta_magnitudes[significant_indices])),
            "preview_deltas": preview_deltas
        })
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)
    
    print(f"Exported JSON preview to: {output_path}")
    return True


def main():
    # 获取输出路径
    if "--" in sys.argv:
        # 命令行模式
        argv = sys.argv[sys.argv.index("--") + 1:]
        output_dir = argv[0] if argv else os.path.dirname(bpy.data.filepath)
    else:
        # 交互模式
        output_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else "/tmp"
    
    # 获取当前选中的对象，或者找第一个有 Shape Keys 的网格
    obj = bpy.context.active_object
    
    if obj is None or obj.type != 'MESH' or obj.data.shape_keys is None:
        # 尝试找一个有 shape keys 的网格
        for o in bpy.data.objects:
            if o.type == 'MESH' and o.data.shape_keys is not None:
                obj = o
                break
    
    if obj is None:
        print("ERROR: No mesh with shape keys found!")
        return
    
    print(f"Exporting BlendShapes from: {obj.name}")
    print(f"Output directory: {output_dir}")
    
    # 导出二进制格式
    bin_path = os.path.join(output_dir, "blendshapes.bin")
    export_blendshapes(obj, bin_path)
    
    # 导出 JSON 预览
    json_path = os.path.join(output_dir, "blendshapes_preview.json")
    export_blendshapes_json(obj, json_path)


if __name__ == "__main__":
    main()
