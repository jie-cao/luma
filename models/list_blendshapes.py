import struct

with open('blendshapes.bin', 'rb') as f:
    magic, ver, verts, shapes = struct.unpack('<4I', f.read(16))
    print(f"BlendShapes in file: {shapes}")
    print(f"Vertex count: {verts}")
    print()
    
    for i in range(shapes):
        name_len = struct.unpack('<I', f.read(4))[0]
        name = f.read(name_len).decode('utf-8')
        min_w, max_w, def_w = struct.unpack('<3f', f.read(12))
        delta_count = struct.unpack('<I', f.read(4))[0]
        
        print(f"  {i+1}. {name}: {delta_count} deltas (range: {min_w} to {max_w})")
        
        # Skip delta data
        f.seek(f.tell() + delta_count * 28)  # 28 bytes per delta
