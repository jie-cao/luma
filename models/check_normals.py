import struct
import numpy as np

with open('head_base.bin', 'rb') as f:
    vertex_count = struct.unpack('I', f.read(4))[0]
    index_count = struct.unpack('I', f.read(4))[0]
    f.read(8)  # skip neck info
    
    positions = []
    for i in range(vertex_count):
        x, y, z = struct.unpack('fff', f.read(12))
        positions.append([x, y, z])
    
    normals = []
    for i in range(vertex_count):
        nx, ny, nz = struct.unpack('fff', f.read(12))
        normals.append([nx, ny, nz])
    
    f.read(vertex_count * 8)  # skip UVs
    
    indices = []
    for i in range(index_count):
        idx = struct.unpack('I', f.read(4))[0]
        indices.append(idx)

# Check winding order of triangles in shoulder area
print('Checking triangle winding in shoulder area...')
problem_tris = []
for i in range(0, len(indices), 3):
    i0, i1, i2 = indices[i], indices[i+1], indices[i+2]
    p0 = np.array(positions[i0])
    p1 = np.array(positions[i1])
    p2 = np.array(positions[i2])
    
    # Check if in shoulder area
    center = (p0 + p1 + p2) / 3
    if center[0] > 0.03 and center[1] < -0.05 and center[2] < 0.02:
        # Calculate face normal from winding
        edge1 = p1 - p0
        edge2 = p2 - p0
        face_normal = np.cross(edge1, edge2)
        length = np.linalg.norm(face_normal)
        if length > 1e-8:
            face_normal = face_normal / length
        
        # Get average vertex normal
        avg_normal = (np.array(normals[i0]) + np.array(normals[i1]) + np.array(normals[i2])) / 3
        length = np.linalg.norm(avg_normal)
        if length > 1e-8:
            avg_normal = avg_normal / length
        
        # Check if they point same direction
        dot = np.dot(face_normal, avg_normal)
        problem_tris.append((i//3, dot, center, face_normal, avg_normal))

print(f'Found {len(problem_tris)} triangles in shoulder area')
inverted = [t for t in problem_tris if t[1] < 0]
print(f'Inverted normals: {len(inverted)}')

print('\nAll shoulder triangles (dot product):')
for tri_idx, dot, center, fn, vn in problem_tris[:20]:
    status = 'INVERTED!' if dot < 0 else 'ok'
    print(f'  Tri {tri_idx}: dot={dot:.2f} center=({center[0]:.3f},{center[1]:.3f},{center[2]:.3f}) {status}')
