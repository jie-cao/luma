"""
Convert OBJ head mesh to binary format for C++ loading.

Usage:
    python convert_head.py head_base.obj

This script reads an OBJ file and outputs:
    - head_base.bin: Binary mesh data (vertices, indices, normals, UVs)
    - head_base_info.txt: Mesh statistics

Binary format:
    - uint32: vertex count
    - uint32: index count
    - uint32: neck boundary start index
    - uint32: neck boundary count
    - float[vertexCount * 3]: positions (x, y, z)
    - float[vertexCount * 3]: normals (nx, ny, nz)
    - float[vertexCount * 2]: UVs (u, v)
    - uint32[indexCount]: triangle indices
"""

import sys
import os
import struct
import numpy as np

def load_obj(filepath):
    """Load OBJ file and return vertices, normals, UVs, and faces."""
    positions = []
    normals = []
    uvs = []
    faces = []  # Each face is a list of (pos_idx, uv_idx, norm_idx)
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            parts = line.split()
            if not parts:
                continue
            
            if parts[0] == 'v':
                # Vertex position
                positions.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif parts[0] == 'vn':
                # Vertex normal
                normals.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif parts[0] == 'vt':
                # Texture coordinate
                uvs.append([float(parts[1]), float(parts[2])])
            elif parts[0] == 'f':
                # Face (can be triangle or quad)
                face_verts = []
                for vert in parts[1:]:
                    indices = vert.split('/')
                    pos_idx = int(indices[0]) - 1  # OBJ is 1-indexed
                    uv_idx = int(indices[1]) - 1 if len(indices) > 1 and indices[1] else -1
                    norm_idx = int(indices[2]) - 1 if len(indices) > 2 and indices[2] else -1
                    face_verts.append((pos_idx, uv_idx, norm_idx))
                
                # Triangulate if quad
                if len(face_verts) == 3:
                    faces.append(face_verts)
                elif len(face_verts) == 4:
                    # Split quad into two triangles
                    faces.append([face_verts[0], face_verts[1], face_verts[2]])
                    faces.append([face_verts[0], face_verts[2], face_verts[3]])
                elif len(face_verts) > 4:
                    # Fan triangulation for n-gons
                    for i in range(1, len(face_verts) - 1):
                        faces.append([face_verts[0], face_verts[i], face_verts[i+1]])
    
    return positions, normals, uvs, faces

def calculate_normals(positions, faces):
    """Calculate smooth vertex normals from face data."""
    # Initialize normals to zero
    normals = [[0.0, 0.0, 0.0] for _ in positions]
    
    # Accumulate face normals to vertices
    for face in faces:
        # Get vertex positions for this face
        p0 = np.array(positions[face[0][0]])
        p1 = np.array(positions[face[1][0]])
        p2 = np.array(positions[face[2][0]])
        
        # Calculate face normal
        edge1 = p1 - p0
        edge2 = p2 - p0
        face_normal = np.cross(edge1, edge2)
        
        # Normalize (avoid division by zero)
        length = np.linalg.norm(face_normal)
        if length > 1e-8:
            face_normal = face_normal / length
        
        # Add to each vertex's normal
        for pos_idx, _, _ in face:
            normals[pos_idx][0] += face_normal[0]
            normals[pos_idx][1] += face_normal[1]
            normals[pos_idx][2] += face_normal[2]
    
    # Normalize all vertex normals
    for i in range(len(normals)):
        n = np.array(normals[i])
        length = np.linalg.norm(n)
        if length > 1e-8:
            normals[i] = (n / length).tolist()
        else:
            normals[i] = [0.0, 1.0, 0.0]  # Default up
    
    return normals

def build_mesh(positions, normals, uvs, faces):
    """Build indexed mesh with unique vertices."""
    # If no normals in OBJ, calculate them
    if not normals:
        print("No normals in OBJ file, calculating smooth normals...")
        normals = calculate_normals(positions, faces)
        # Update faces to use calculated normals (same index as position)
        new_faces = []
        for face in faces:
            new_face = []
            for pos_idx, uv_idx, _ in face:
                new_face.append((pos_idx, uv_idx, pos_idx))  # norm_idx = pos_idx
            new_faces.append(new_face)
        faces = new_faces
    
    vertex_map = {}  # (pos_idx, uv_idx, norm_idx) -> new_vertex_idx
    vertices = []  # List of (pos, norm, uv)
    indices = []
    
    for face in faces:
        for pos_idx, uv_idx, norm_idx in face:
            key = (pos_idx, uv_idx, norm_idx)
            
            if key not in vertex_map:
                # Create new vertex
                pos = positions[pos_idx]
                norm = normals[norm_idx] if norm_idx >= 0 and norm_idx < len(normals) else [0, 1, 0]
                uv = uvs[uv_idx] if uv_idx >= 0 and uv_idx < len(uvs) else [0, 0]
                
                vertex_map[key] = len(vertices)
                vertices.append((pos, norm, uv))
            
            indices.append(vertex_map[key])
    
    return vertices, indices

def find_neck_boundary(vertices, indices):
    """Find neck boundary vertices (lowest Y vertices that form a ring)."""
    # Find minimum Y
    min_y = min(v[0][1] for v in vertices)
    
    # Find vertices near minimum Y (within 5% of height range)
    y_values = [v[0][1] for v in vertices]
    y_range = max(y_values) - min(y_values)
    threshold = min_y + y_range * 0.05
    
    neck_indices = []
    for i, v in enumerate(vertices):
        if v[0][1] <= threshold:
            neck_indices.append(i)
    
    # Sort by angle around Y axis for proper ordering
    if neck_indices:
        center_x = sum(vertices[i][0][0] for i in neck_indices) / len(neck_indices)
        center_z = sum(vertices[i][0][2] for i in neck_indices) / len(neck_indices)
        
        def angle_key(idx):
            x = vertices[idx][0][0] - center_x
            z = vertices[idx][0][2] - center_z
            return np.arctan2(z, x)
        
        neck_indices.sort(key=angle_key)
    
    return neck_indices

def normalize_mesh(vertices):
    """Center and scale mesh to fit in unit cube."""
    positions = np.array([v[0] for v in vertices])
    
    # Find bounds
    min_pos = positions.min(axis=0)
    max_pos = positions.max(axis=0)
    center = (min_pos + max_pos) / 2
    size = max_pos - min_pos
    scale = 0.23 / max(size)  # Scale to ~23cm head height
    
    print(f"Original bounds: min={min_pos}, max={max_pos}")
    print(f"Center: {center}, Size: {size}, Scale: {scale}")
    
    # Apply transformation
    normalized = []
    for pos, norm, uv in vertices:
        new_pos = [(pos[0] - center[0]) * scale,
                   (pos[1] - center[1]) * scale,
                   (pos[2] - center[2]) * scale]
        normalized.append((new_pos, norm, uv))
    
    return normalized

def save_binary(filepath, vertices, indices, neck_start, neck_count):
    """Save mesh in binary format."""
    with open(filepath, 'wb') as f:
        # Header
        f.write(struct.pack('I', len(vertices)))
        f.write(struct.pack('I', len(indices)))
        f.write(struct.pack('I', neck_start))
        f.write(struct.pack('I', neck_count))
        
        # Positions
        for pos, norm, uv in vertices:
            f.write(struct.pack('fff', pos[0], pos[1], pos[2]))
        
        # Normals
        for pos, norm, uv in vertices:
            f.write(struct.pack('fff', norm[0], norm[1], norm[2]))
        
        # UVs
        for pos, norm, uv in vertices:
            f.write(struct.pack('ff', uv[0], uv[1]))
        
        # Indices
        for idx in indices:
            f.write(struct.pack('I', idx))

def main():
    if len(sys.argv) < 2:
        print("Usage: python convert_head.py <input.obj> [output_name]")
        print("Example: python convert_head.py head_base.obj")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_name = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(input_path)[0]
    
    if not os.path.exists(input_path):
        print(f"Error: File not found: {input_path}")
        sys.exit(1)
    
    print(f"Loading OBJ: {input_path}")
    positions, normals, uvs, faces = load_obj(input_path)
    
    print(f"Raw data: {len(positions)} positions, {len(normals)} normals, {len(uvs)} UVs, {len(faces)} faces")
    
    print("Building indexed mesh...")
    vertices, indices = build_mesh(positions, normals, uvs, faces)
    
    print(f"Indexed mesh: {len(vertices)} vertices, {len(indices)} indices ({len(indices)//3} triangles)")
    
    print("Normalizing mesh...")
    vertices = normalize_mesh(vertices)
    
    print("Finding neck boundary...")
    neck_indices = find_neck_boundary(vertices, indices)
    neck_start = min(neck_indices) if neck_indices else 0
    neck_count = len(neck_indices)
    print(f"Neck boundary: {neck_count} vertices starting at index {neck_start}")
    
    # Save binary
    bin_path = output_name + ".bin"
    print(f"Saving binary: {bin_path}")
    save_binary(bin_path, vertices, indices, neck_start, neck_count)
    
    # Save info
    info_path = output_name + "_info.txt"
    with open(info_path, 'w') as f:
        f.write(f"Head Mesh Info\n")
        f.write(f"==============\n")
        f.write(f"Source: {input_path}\n")
        f.write(f"Vertices: {len(vertices)}\n")
        f.write(f"Triangles: {len(indices) // 3}\n")
        f.write(f"Neck boundary: {neck_count} vertices at index {neck_start}\n")
        
        # Bounds
        positions = [v[0] for v in vertices]
        min_x = min(p[0] for p in positions)
        max_x = max(p[0] for p in positions)
        min_y = min(p[1] for p in positions)
        max_y = max(p[1] for p in positions)
        min_z = min(p[2] for p in positions)
        max_z = max(p[2] for p in positions)
        
        f.write(f"\nBounds:\n")
        f.write(f"  X: [{min_x:.4f}, {max_x:.4f}]\n")
        f.write(f"  Y: [{min_y:.4f}, {max_y:.4f}]\n")
        f.write(f"  Z: [{min_z:.4f}, {max_z:.4f}]\n")
    
    print(f"Saved info: {info_path}")
    print("\nDone!")
    print(f"\nTo use in LUMA Studio:")
    print(f"  1. Place {bin_path} in the models/ directory")
    print(f"  2. Rebuild and run the application")

if __name__ == '__main__':
    main()
