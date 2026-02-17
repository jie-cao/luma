"""
Blender 脚本：为标准头部模型创建 BlendShape (Shape Key) 模板

使用方法：
1. 在 Blender 中打开 head_base.obj
2. 选中头部网格对象
3. 运行此脚本 (Text Editor -> Run Script)

脚本会创建所有需要的 Shape Keys，艺术家只需雕刻每个形状。
"""

import bpy
import json
import os

# ============================================================================
# BlendShape 定义 - MetaHuman 风格
# ============================================================================

IDENTITY_BLENDSHAPES = {
    # 整体脸型
    "face": [
        ("id_face_width", "脸宽 - 向两侧扩展脸颊和下颌"),
        ("id_face_length", "脸长 - 向下拉伸下巴区域"),
        ("id_face_round", "脸圆润 - 增加脸颊丰满度"),
    ],
    
    # 额头
    "forehead": [
        ("id_forehead_height", "额头高 - 向上移动发际线区域"),
        ("id_forehead_width", "额头宽 - 向两侧扩展太阳穴"),
        ("id_forehead_slope", "额头斜 - 向后倾斜额头"),
        ("id_forehead_bossing", "眉弓突出 - 眉骨区域向前突出"),
    ],
    
    # 眼睛
    "eyes": [
        ("id_eye_size", "眼睛大小 - 整体放大眼眶区域"),
        ("id_eye_spacing", "眼距 - 向外移动整个眼睛区域"),
        ("id_eye_height", "眼睛高度 - 向上移动眼睛位置"),
        ("id_eye_depth", "眼窝深度 - 眼球区域向内凹陷"),
        ("id_eye_angle_up", "眼角上扬 - 外眼角向上"),
        ("id_eye_angle_down", "眼角下垂 - 外眼角向下"),
        ("id_eye_lid_upper_crease", "双眼皮深度"),
        ("id_eye_lid_upper_fat", "上眼睑脂肪"),
        ("id_eye_lid_lower_bag", "下眼袋"),
        ("id_eye_inner_corner", "内眼角形状"),
        ("id_eye_outer_corner", "外眼角形状"),
    ],
    
    # 眉毛区域 (不是眉毛本身，而是眉骨和周围皮肤)
    "brow": [
        ("id_brow_height", "眉毛高度 - 整个眉弓区域上移"),
        ("id_brow_inner_up", "眉头上扬"),
        ("id_brow_outer_up", "眉尾上扬"),
        ("id_brow_spacing", "眉间距 - 眉头间距"),
        ("id_brow_protrusion", "眉骨突出程度"),
    ],
    
    # 鼻子
    "nose": [
        ("id_nose_length", "鼻长 - 鼻尖向下延伸"),
        ("id_nose_width", "鼻宽 - 鼻翼向两侧扩展"),
        ("id_nose_height", "鼻高 - 整个鼻梁向前突出"),
        ("id_nose_bridge_width", "鼻梁宽度"),
        ("id_nose_bridge_curve", "鼻梁曲线 - 驼峰或凹陷"),
        ("id_nose_tip_up", "鼻尖上翘"),
        ("id_nose_tip_down", "鼻尖下垂"),
        ("id_nose_tip_width", "鼻尖宽度"),
        ("id_nose_tip_shape", "鼻尖形状 - 圆润或尖锐"),
        ("id_nostril_width", "鼻翼宽度"),
        ("id_nostril_flare", "鼻翼外扩程度"),
        ("id_nostril_height", "鼻孔高度"),
    ],
    
    # 嘴巴
    "mouth": [
        ("id_mouth_width", "嘴宽"),
        ("id_mouth_height", "嘴的垂直位置"),
        ("id_lip_upper_thick", "上唇厚度"),
        ("id_lip_lower_thick", "下唇厚度"),
        ("id_lip_upper_curve", "上唇曲线 - 唇峰形状"),
        ("id_lip_lower_curve", "下唇曲线"),
        ("id_lip_protrusion", "嘴唇整体突出"),
        ("id_lip_corner_up", "嘴角上扬"),
        ("id_lip_corner_down", "嘴角下垂"),
        ("id_philtrum_length", "人中长度"),
        ("id_philtrum_width", "人中宽度"),
        ("id_philtrum_depth", "人中深度"),
    ],
    
    # 下巴
    "chin": [
        ("id_chin_length", "下巴长度 - 向下延伸"),
        ("id_chin_width", "下巴宽度"),
        ("id_chin_protrusion", "下巴突出 - 向前突出"),
        ("id_chin_retrusion", "下巴后缩"),
        ("id_chin_cleft", "下巴沟/美人沟"),
        ("id_chin_shape", "下巴形状 - 方形或圆形"),
    ],
    
    # 下颌
    "jaw": [
        ("id_jaw_width", "下颌宽度 - 咬肌区域"),
        ("id_jaw_angle", "下颌角 - 角度锐利程度"),
        ("id_jaw_height", "下颌高度"),
        ("id_jaw_line", "下颌线清晰度"),
    ],
    
    # 颧骨和脸颊
    "cheeks": [
        ("id_cheekbone_height", "颧骨高度位置"),
        ("id_cheekbone_width", "颧骨宽度"),
        ("id_cheekbone_protrusion", "颧骨突出程度"),
        ("id_cheek_fullness", "脸颊丰满度"),
        ("id_cheek_hollow", "脸颊凹陷"),
        ("id_nasolabial", "法令纹区域"),
    ],
    
    # 耳朵
    "ears": [
        ("id_ear_size", "耳朵大小"),
        ("id_ear_angle", "耳朵角度 - 贴头或招风"),
        ("id_ear_lobe_size", "耳垂大小"),
        ("id_ear_lobe_attached", "耳垂连接方式"),
        ("id_ear_point", "耳尖形状"),
    ],
    
    # 颈部 (影响头颈连接)
    "neck": [
        ("id_neck_width", "颈部宽度"),
        ("id_neck_length", "颈部长度"),
        ("id_adam_apple", "喉结突出程度"),
    ],
}

# ARKit 52 表情 BlendShapes
EXPRESSION_BLENDSHAPES = [
    # 眉毛
    "browDownLeft", "browDownRight",
    "browInnerUp",
    "browOuterUpLeft", "browOuterUpRight",
    
    # 眼睛
    "eyeBlinkLeft", "eyeBlinkRight",
    "eyeLookDownLeft", "eyeLookDownRight",
    "eyeLookInLeft", "eyeLookInRight",
    "eyeLookOutLeft", "eyeLookOutRight",
    "eyeLookUpLeft", "eyeLookUpRight",
    "eyeSquintLeft", "eyeSquintRight",
    "eyeWideLeft", "eyeWideRight",
    
    # 脸颊
    "cheekPuff",
    "cheekSquintLeft", "cheekSquintRight",
    
    # 鼻子
    "noseSneerLeft", "noseSneerRight",
    
    # 下颌
    "jawForward", "jawLeft", "jawRight", "jawOpen",
    
    # 嘴巴
    "mouthClose",
    "mouthFunnel", "mouthPucker",
    "mouthLeft", "mouthRight",
    "mouthSmileLeft", "mouthSmileRight",
    "mouthFrownLeft", "mouthFrownRight",
    "mouthDimpleLeft", "mouthDimpleRight",
    "mouthStretchLeft", "mouthStretchRight",
    "mouthRollLower", "mouthRollUpper",
    "mouthShrugLower", "mouthShrugUpper",
    "mouthPressLeft", "mouthPressRight",
    "mouthLowerDownLeft", "mouthLowerDownRight",
    "mouthUpperUpLeft", "mouthUpperUpRight",
    
    # 舌头
    "tongueOut",
]


def create_shape_keys(obj):
    """为对象创建所有 Shape Keys"""
    
    # 确保有 Basis shape key
    if obj.data.shape_keys is None:
        obj.shape_key_add(name="Basis", from_mix=False)
    
    created_count = 0
    
    # 创建 Identity BlendShapes
    print("\n=== Creating Identity BlendShapes ===")
    for category, shapes in IDENTITY_BLENDSHAPES.items():
        print(f"\n[{category}]")
        for shape_name, description in shapes:
            if shape_name not in obj.data.shape_keys.key_blocks:
                sk = obj.shape_key_add(name=shape_name, from_mix=False)
                sk.slider_min = -1.0  # 允许负值（反向变形）
                sk.slider_max = 1.0
                sk.value = 0.0
                print(f"  + {shape_name}: {description}")
                created_count += 1
            else:
                print(f"  - {shape_name} (already exists)")
    
    # 创建 Expression BlendShapes
    print("\n=== Creating Expression BlendShapes (ARKit 52) ===")
    for shape_name in EXPRESSION_BLENDSHAPES:
        full_name = f"expr_{shape_name}"
        if full_name not in obj.data.shape_keys.key_blocks:
            sk = obj.shape_key_add(name=full_name, from_mix=False)
            sk.slider_min = 0.0  # 表情通常是 0-1
            sk.slider_max = 1.0
            sk.value = 0.0
            print(f"  + {full_name}")
            created_count += 1
        else:
            print(f"  - {full_name} (already exists)")
    
    return created_count


def export_shape_key_list(obj, filepath):
    """导出 Shape Key 列表为 JSON（用于文档和验证）"""
    
    if obj.data.shape_keys is None:
        print("No shape keys to export")
        return
    
    data = {
        "identity": {},
        "expression": []
    }
    
    for category, shapes in IDENTITY_BLENDSHAPES.items():
        data["identity"][category] = [
            {"name": name, "description": desc} 
            for name, desc in shapes
        ]
    
    data["expression"] = EXPRESSION_BLENDSHAPES
    
    with open(filepath, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    
    print(f"Exported shape key list to: {filepath}")


def main():
    # 获取当前选中的对象
    obj = bpy.context.active_object
    
    if obj is None:
        print("ERROR: No object selected!")
        print("Please select the head mesh object first.")
        return
    
    if obj.type != 'MESH':
        print(f"ERROR: Selected object '{obj.name}' is not a mesh!")
        return
    
    print(f"Creating BlendShape template for: {obj.name}")
    print(f"Vertex count: {len(obj.data.vertices)}")
    
    # 创建 Shape Keys
    count = create_shape_keys(obj)
    
    print(f"\n=== Summary ===")
    print(f"Created {count} new Shape Keys")
    print(f"Total Shape Keys: {len(obj.data.shape_keys.key_blocks)}")
    
    # 导出列表
    blend_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else "/tmp"
    json_path = os.path.join(blend_dir, "blendshape_list.json")
    export_shape_key_list(obj, json_path)
    
    print("\n=== Next Steps ===")
    print("1. For each Shape Key in the list:")
    print("   - Select it in the Shape Keys panel")
    print("   - Set its value to 1.0")
    print("   - Enter Sculpt mode and sculpt the deformation")
    print("   - Use X-Mirror for symmetric shapes")
    print("2. Save the .blend file")
    print("3. Run export_blendshapes.py to export to engine format")


# 运行
if __name__ == "__main__":
    main()
