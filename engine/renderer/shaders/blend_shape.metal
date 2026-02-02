// BlendShape Compute Shader
// GPU-accelerated morph target blending for character customization
// Part of LUMA Character Creation System

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// Constants
// ============================================================================

constant uint MAX_ACTIVE_BLEND_SHAPES = 64;
constant uint THREADS_PER_GROUP = 256;

// ============================================================================
// Data Structures
// ============================================================================

// Per-vertex base data
struct BaseVertex {
    float3 position;
    float3 normal;
    float3 tangent;
};

// Blend shape delta for a single vertex
struct BlendShapeDelta {
    uint vertexIndex;
    float3 positionDelta;
    float3 normalDelta;
    float3 tangentDelta;
};

// Blend shape target header (stored separately from deltas)
struct BlendShapeTargetHeader {
    uint deltaOffset;       // Offset into delta buffer
    uint deltaCount;        // Number of deltas
    float weight;           // Current weight for this target
    float _padding;
};

// Uniforms for the compute pass
struct BlendShapeUniforms {
    uint vertexCount;           // Total vertices in mesh
    uint activeTargetCount;     // Number of active blend shape targets
    uint _padding1;
    uint _padding2;
};

// Output vertex (position + normal + tangent)
struct OutputVertex {
    float3 position;
    float3 normal;
    float3 tangent;
};

// ============================================================================
// Sparse Delta Application (per-delta kernel)
// ============================================================================

// This kernel processes blend shape deltas sparsely
// Each thread handles one delta entry
kernel void applyBlendShapeDeltasSparse(
    device const BaseVertex* baseVertices [[buffer(0)]],
    device float3* outPositions [[buffer(1)]],
    device float3* outNormals [[buffer(2)]],
    device const BlendShapeDelta* deltas [[buffer(3)]],
    device const BlendShapeTargetHeader* targets [[buffer(4)]],
    constant BlendShapeUniforms& uniforms [[buffer(5)]],
    uint tid [[thread_position_in_grid]])
{
    // First, initialize output from base (done in a separate pass or first dispatch)
    // This kernel focuses on applying deltas
    
    // Find which target and delta this thread handles
    uint currentDelta = 0;
    uint targetIdx = 0;
    
    for (uint t = 0; t < uniforms.activeTargetCount; t++) {
        uint targetDeltaCount = targets[t].deltaCount;
        if (tid >= currentDelta && tid < currentDelta + targetDeltaCount) {
            targetIdx = t;
            break;
        }
        currentDelta += targetDeltaCount;
    }
    
    // Check bounds
    if (targetIdx >= uniforms.activeTargetCount) return;
    
    BlendShapeTargetHeader target = targets[targetIdx];
    uint localDeltaIdx = tid - currentDelta;
    
    if (localDeltaIdx >= target.deltaCount) return;
    if (abs(target.weight) < 0.001f) return;
    
    // Read delta
    BlendShapeDelta delta = deltas[target.deltaOffset + localDeltaIdx];
    
    if (delta.vertexIndex >= uniforms.vertexCount) return;
    
    // Apply weighted delta atomically
    // Note: Metal doesn't have native float atomics, so we use atomic_fetch_add on int
    // For production, consider using a different approach (see below)
    float weight = target.weight;
    
    // Simple non-atomic addition (works if each vertex is only touched by one thread per target)
    outPositions[delta.vertexIndex] += delta.positionDelta * weight;
    outNormals[delta.vertexIndex] += delta.normalDelta * weight;
}

// ============================================================================
// Dense Per-Vertex Application (simpler, may be faster for many blend shapes)
// ============================================================================

// This kernel processes one vertex at a time
// More efficient when many blend shapes affect most vertices
kernel void applyBlendShapesDense(
    device const float3* basePositions [[buffer(0)]],
    device const float3* baseNormals [[buffer(1)]],
    device float3* outPositions [[buffer(2)]],
    device float3* outNormals [[buffer(3)]],
    device const BlendShapeTargetHeader* targets [[buffer(4)]],
    device const BlendShapeDelta* allDeltas [[buffer(5)]],
    constant BlendShapeUniforms& uniforms [[buffer(6)]],
    uint vid [[thread_position_in_grid]])
{
    if (vid >= uniforms.vertexCount) return;
    
    // Start with base position and normal
    float3 position = basePositions[vid];
    float3 normal = baseNormals[vid];
    
    // Apply all active blend shapes
    for (uint t = 0; t < uniforms.activeTargetCount; t++) {
        BlendShapeTargetHeader target = targets[t];
        
        if (abs(target.weight) < 0.001f) continue;
        
        // Search for this vertex in the delta list (binary search would be better)
        // For production, use a vertex-indexed structure instead
        for (uint d = 0; d < target.deltaCount; d++) {
            BlendShapeDelta delta = allDeltas[target.deltaOffset + d];
            
            if (delta.vertexIndex == vid) {
                position += delta.positionDelta * target.weight;
                normal += delta.normalDelta * target.weight;
                break;  // Assuming one delta per vertex per target
            }
        }
    }
    
    // Normalize the normal
    normal = normalize(normal);
    
    // Write output
    outPositions[vid] = position;
    outNormals[vid] = normal;
}

// ============================================================================
// Optimized: Pre-indexed Blend Shape Application
// ============================================================================

// Structure for pre-indexed blend shape data
// Each vertex has a fixed-size list of blend shape contributions
struct VertexBlendShapeEntry {
    uint targetIndex;       // Which blend shape target
    float3 positionDelta;
    float3 normalDelta;
};

struct VertexBlendShapeInfo {
    uint entryOffset;       // Offset into entry buffer
    uint entryCount;        // Number of blend shapes affecting this vertex
};

// Most efficient kernel: pre-indexed per-vertex data
kernel void applyBlendShapesIndexed(
    device const float3* basePositions [[buffer(0)]],
    device const float3* baseNormals [[buffer(1)]],
    device float3* outPositions [[buffer(2)]],
    device float3* outNormals [[buffer(3)]],
    device const VertexBlendShapeInfo* vertexInfo [[buffer(4)]],
    device const VertexBlendShapeEntry* entries [[buffer(5)]],
    device const float* targetWeights [[buffer(6)]],  // Array of weights indexed by targetIndex
    constant uint& vertexCount [[buffer(7)]],
    uint vid [[thread_position_in_grid]])
{
    if (vid >= vertexCount) return;
    
    // Start with base
    float3 position = basePositions[vid];
    float3 normal = baseNormals[vid];
    
    // Get this vertex's blend shape info
    VertexBlendShapeInfo info = vertexInfo[vid];
    
    // Apply all blend shapes that affect this vertex
    for (uint i = 0; i < info.entryCount; i++) {
        VertexBlendShapeEntry entry = entries[info.entryOffset + i];
        float weight = targetWeights[entry.targetIndex];
        
        if (abs(weight) > 0.001f) {
            position += entry.positionDelta * weight;
            normal += entry.normalDelta * weight;
        }
    }
    
    // Normalize and write
    outPositions[vid] = position;
    outNormals[vid] = normalize(normal);
}

// ============================================================================
// Initialize Output from Base
// ============================================================================

kernel void initializeFromBase(
    device const float3* basePositions [[buffer(0)]],
    device const float3* baseNormals [[buffer(1)]],
    device float3* outPositions [[buffer(2)]],
    device float3* outNormals [[buffer(3)]],
    constant uint& vertexCount [[buffer(4)]],
    uint vid [[thread_position_in_grid]])
{
    if (vid >= vertexCount) return;
    
    outPositions[vid] = basePositions[vid];
    outNormals[vid] = baseNormals[vid];
}

// ============================================================================
// Skinning with Blend Shapes Combined
// ============================================================================

// Bone matrices
struct BoneData {
    float4x4 matrix;
};

// Combined blend shape + skinning in one pass
kernel void applyBlendShapesAndSkinning(
    device const float3* basePositions [[buffer(0)]],
    device const float3* baseNormals [[buffer(1)]],
    device const uint4* boneIndices [[buffer(2)]],      // 4 bone indices per vertex
    device const float4* boneWeights [[buffer(3)]],     // 4 bone weights per vertex
    device float3* outPositions [[buffer(4)]],
    device float3* outNormals [[buffer(5)]],
    device const VertexBlendShapeInfo* vertexInfo [[buffer(6)]],
    device const VertexBlendShapeEntry* entries [[buffer(7)]],
    device const float* targetWeights [[buffer(8)]],
    device const BoneData* bones [[buffer(9)]],
    constant uint& vertexCount [[buffer(10)]],
    uint vid [[thread_position_in_grid]])
{
    if (vid >= vertexCount) return;
    
    // Step 1: Apply blend shapes
    float3 position = basePositions[vid];
    float3 normal = baseNormals[vid];
    
    VertexBlendShapeInfo info = vertexInfo[vid];
    for (uint i = 0; i < info.entryCount; i++) {
        VertexBlendShapeEntry entry = entries[info.entryOffset + i];
        float weight = targetWeights[entry.targetIndex];
        
        if (abs(weight) > 0.001f) {
            position += entry.positionDelta * weight;
            normal += entry.normalDelta * weight;
        }
    }
    normal = normalize(normal);
    
    // Step 2: Apply skinning
    uint4 indices = boneIndices[vid];
    float4 weights = boneWeights[vid];
    
    float3 skinnedPos = float3(0.0);
    float3 skinnedNor = float3(0.0);
    
    // Bone 0
    if (weights.x > 0.0) {
        float4x4 m = bones[indices.x].matrix;
        skinnedPos += (m * float4(position, 1.0)).xyz * weights.x;
        skinnedNor += (m * float4(normal, 0.0)).xyz * weights.x;
    }
    
    // Bone 1
    if (weights.y > 0.0) {
        float4x4 m = bones[indices.y].matrix;
        skinnedPos += (m * float4(position, 1.0)).xyz * weights.y;
        skinnedNor += (m * float4(normal, 0.0)).xyz * weights.y;
    }
    
    // Bone 2
    if (weights.z > 0.0) {
        float4x4 m = bones[indices.z].matrix;
        skinnedPos += (m * float4(position, 1.0)).xyz * weights.z;
        skinnedNor += (m * float4(normal, 0.0)).xyz * weights.z;
    }
    
    // Bone 3
    if (weights.w > 0.0) {
        float4x4 m = bones[indices.w].matrix;
        skinnedPos += (m * float4(position, 1.0)).xyz * weights.w;
        skinnedNor += (m * float4(normal, 0.0)).xyz * weights.w;
    }
    
    // Write output
    outPositions[vid] = skinnedPos;
    outNormals[vid] = normalize(skinnedNor);
}

// ============================================================================
// Utility: Copy positions/normals back to interleaved vertex buffer
// ============================================================================

struct InterleavedVertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
    float3 color;
};

kernel void copyToInterleavedBuffer(
    device const float3* positions [[buffer(0)]],
    device const float3* normals [[buffer(1)]],
    device InterleavedVertex* vertices [[buffer(2)]],
    constant uint& vertexCount [[buffer(3)]],
    uint vid [[thread_position_in_grid]])
{
    if (vid >= vertexCount) return;
    
    vertices[vid].position = positions[vid];
    vertices[vid].normal = normals[vid];
}
