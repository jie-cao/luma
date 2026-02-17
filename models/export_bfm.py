"""
Export BFM (Basel Face Model) from 3DDFA_V2 to binary format for C++ loading.

Usage:
1. Clone 3DDFA_V2: git clone https://github.com/cleardusk/3DDFA_V2.git
2. Run: python export_bfm.py path/to/3DDFA_V2

Output files:
- bfm_mean_face.bin: Mean face vertices (N*3 floats)
- bfm_shape_basis.bin: Shape basis vectors (N*3 x 40 floats)
- bfm_exp_basis.bin: Expression basis vectors (N*3 x 10 floats)
- bfm_triangles.bin: Triangle indices (M*3 uint32s)
- bfm_info.txt: Model info (vertex count, triangle count, etc.)
"""

import sys
import os
import numpy as np
import struct

def load_pkl(filepath):
    """Load pickle file"""
    import pickle
    with open(filepath, 'rb') as f:
        return pickle.load(f, encoding='latin1')

def main():
    if len(sys.argv) < 2:
        print("Usage: python export_bfm.py <path_to_3DDFA_V2>")
        print("Example: python export_bfm.py C:/code/3DDFA_V2")
        sys.exit(1)
    
    ddfa_path = sys.argv[1]
    bfm_path = os.path.join(ddfa_path, 'configs', 'bfm_noneck_v3.pkl')
    tri_path = os.path.join(ddfa_path, 'configs', 'tri.pkl')
    
    if not os.path.exists(bfm_path):
        print(f"Error: BFM file not found at {bfm_path}")
        print("Please provide the correct path to 3DDFA_V2 repository")
        sys.exit(1)
    
    print(f"Loading BFM from {bfm_path}...")
    bfm = load_pkl(bfm_path)
    
    # Extract data
    u = bfm['u'].astype(np.float32)  # Mean face (N*3,)
    w_shp = bfm['w_shp'].astype(np.float32)[:, :40]  # Shape basis (N*3, 40)
    w_exp = bfm['w_exp'].astype(np.float32)[:, :10]  # Expression basis (N*3, 10)
    keypoints = bfm['keypoints'].astype(np.int32)  # 68 landmark indices
    
    # Load triangles
    print(f"Loading triangles from {tri_path}...")
    tri = load_pkl(tri_path)
    tri = tri.T.astype(np.int32)  # (M, 3)
    
    num_vertices = len(u) // 3
    num_triangles = len(tri)
    
    print(f"BFM Model Info:")
    print(f"  Vertices: {num_vertices}")
    print(f"  Triangles: {num_triangles}")
    print(f"  Shape dims: {w_shp.shape[1]}")
    print(f"  Expression dims: {w_exp.shape[1]}")
    print(f"  Keypoints: {len(keypoints)}")
    
    # Reshape mean face to (N, 3)
    u_reshaped = u.reshape(-1, 3)
    
    # Output directory
    out_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Save mean face vertices
    mean_path = os.path.join(out_dir, 'bfm_mean_face.bin')
    print(f"Saving mean face to {mean_path}...")
    with open(mean_path, 'wb') as f:
        f.write(struct.pack('I', num_vertices))
        f.write(u_reshaped.astype(np.float32).tobytes())
    
    # Save shape basis
    shape_path = os.path.join(out_dir, 'bfm_shape_basis.bin')
    print(f"Saving shape basis to {shape_path}...")
    with open(shape_path, 'wb') as f:
        f.write(struct.pack('II', num_vertices * 3, 40))
        f.write(w_shp.astype(np.float32).tobytes())
    
    # Save expression basis
    exp_path = os.path.join(out_dir, 'bfm_exp_basis.bin')
    print(f"Saving expression basis to {exp_path}...")
    with open(exp_path, 'wb') as f:
        f.write(struct.pack('II', num_vertices * 3, 10))
        f.write(w_exp.astype(np.float32).tobytes())
    
    # Save triangles
    tri_path_out = os.path.join(out_dir, 'bfm_triangles.bin')
    print(f"Saving triangles to {tri_path_out}...")
    with open(tri_path_out, 'wb') as f:
        f.write(struct.pack('I', num_triangles))
        f.write(tri.astype(np.uint32).tobytes())
    
    # Save keypoints
    kp_path = os.path.join(out_dir, 'bfm_keypoints.bin')
    print(f"Saving keypoints to {kp_path}...")
    with open(kp_path, 'wb') as f:
        f.write(struct.pack('I', len(keypoints)))
        f.write(keypoints.astype(np.int32).tobytes())
    
    # Save info file
    info_path = os.path.join(out_dir, 'bfm_info.txt')
    with open(info_path, 'w') as f:
        f.write(f"BFM Model exported from 3DDFA_V2\n")
        f.write(f"Vertices: {num_vertices}\n")
        f.write(f"Triangles: {num_triangles}\n")
        f.write(f"Shape dimensions: 40\n")
        f.write(f"Expression dimensions: 10\n")
        f.write(f"Keypoints: {len(keypoints)}\n")
        
        # Print vertex bounds
        print(f"\nVertex bounds:")
        print(f"  X: [{u_reshaped[:, 0].min():.2f}, {u_reshaped[:, 0].max():.2f}]")
        print(f"  Y: [{u_reshaped[:, 1].min():.2f}, {u_reshaped[:, 1].max():.2f}]")
        print(f"  Z: [{u_reshaped[:, 2].min():.2f}, {u_reshaped[:, 2].max():.2f}]")
        
        f.write(f"\nVertex bounds:\n")
        f.write(f"  X: [{u_reshaped[:, 0].min():.2f}, {u_reshaped[:, 0].max():.2f}]\n")
        f.write(f"  Y: [{u_reshaped[:, 1].min():.2f}, {u_reshaped[:, 1].max():.2f}]\n")
        f.write(f"  Z: [{u_reshaped[:, 2].min():.2f}, {u_reshaped[:, 2].max():.2f}]\n")
    
    print(f"\nDone! Files saved to {out_dir}")
    print("\nTo use in C++:")
    print("1. Load bfm_mean_face.bin as base vertices")
    print("2. Load bfm_shape_basis.bin and bfm_exp_basis.bin")
    print("3. Reconstruct face: vertices = mean + shape_basis @ shape_coeffs + exp_basis @ exp_coeffs")
    print("4. Use bfm_triangles.bin for rendering")

if __name__ == '__main__':
    main()
