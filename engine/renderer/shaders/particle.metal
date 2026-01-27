// Particle Rendering Shaders for Metal
// Billboard particles with soft blending
#include <metal_stdlib>
using namespace metal;

// ===== Particle Vertex Data (CPU -> GPU) =====
struct ParticleData {
    float3 position;
    float size;
    float4 color;
    float rotation;
    float textureFrame;
    float2 padding;
};

// ===== Uniforms =====
struct ParticleUniforms {
    float4x4 viewProj;
    float4x4 view;
    float3 cameraPos;
    float3 cameraRight;
    float3 cameraUp;
    float2 screenSize;
    float nearPlane;
    float farPlane;
    
    // Texture sheet
    float2 textureTiles;  // x = tilesX, y = tilesY
    
    // Soft particles
    float softParticleDistance;
    
    // Stretch
    float stretchFactor;
    float3 padding;
};

// ===== Vertex Output =====
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
    float depth;
};

// ===== Billboard Particle Vertex Shader =====
vertex VertexOut particleVertex(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant ParticleData* particles [[buffer(0)]],
    constant ParticleUniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    ParticleData p = particles[instanceID];
    
    // Quad vertices (2 triangles, 6 vertices per particle)
    // Could use instancing with index buffer for better efficiency
    float2 quadVerts[6] = {
        float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.5, 0.5),
        float2(-0.5, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5)
    };
    
    float2 texCoords[6] = {
        float2(0, 1), float2(1, 1), float2(1, 0),
        float2(0, 1), float2(1, 0), float2(0, 0)
    };
    
    float2 quadPos = quadVerts[vertexID % 6];
    float2 texCoord = texCoords[vertexID % 6];
    
    // Apply rotation
    float cosR = cos(p.rotation);
    float sinR = sin(p.rotation);
    float2 rotatedPos = float2(
        quadPos.x * cosR - quadPos.y * sinR,
        quadPos.x * sinR + quadPos.y * cosR
    );
    
    // Billboard - face camera
    float3 worldPos = p.position + 
                     uniforms.cameraRight * rotatedPos.x * p.size +
                     uniforms.cameraUp * rotatedPos.y * p.size;
    
    out.position = uniforms.viewProj * float4(worldPos, 1.0);
    out.color = p.color;
    out.depth = length(worldPos - uniforms.cameraPos);
    
    // Texture sheet animation
    if (uniforms.textureTiles.x > 1 || uniforms.textureTiles.y > 1) {
        int frame = int(p.textureFrame);
        int tilesX = int(uniforms.textureTiles.x);
        int tilesY = int(uniforms.textureTiles.y);
        
        int col = frame % tilesX;
        int row = frame / tilesX;
        
        float uScale = 1.0 / uniforms.textureTiles.x;
        float vScale = 1.0 / uniforms.textureTiles.y;
        
        texCoord.x = (col + texCoord.x) * uScale;
        texCoord.y = (row + texCoord.y) * vScale;
    }
    
    out.texCoord = texCoord;
    
    return out;
}

// ===== Velocity-Stretched Particle Vertex Shader =====
struct ParticleDataStretched {
    float3 position;
    float size;
    float4 color;
    float rotation;
    float textureFrame;
    float3 velocity;
    float padding;
};

vertex VertexOut particleVertexStretched(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant ParticleDataStretched* particles [[buffer(0)]],
    constant ParticleUniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    ParticleDataStretched p = particles[instanceID];
    
    float2 quadVerts[6] = {
        float2(-0.5, -0.5), float2(0.5, -0.5), float2(0.5, 0.5),
        float2(-0.5, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5)
    };
    
    float2 texCoords[6] = {
        float2(0, 1), float2(1, 1), float2(1, 0),
        float2(0, 1), float2(1, 0), float2(0, 0)
    };
    
    float2 quadPos = quadVerts[vertexID % 6];
    float2 texCoord = texCoords[vertexID % 6];
    
    // Calculate stretch direction from velocity
    float3 viewVel = (uniforms.view * float4(p.velocity, 0.0)).xyz;
    float speed = length(viewVel);
    
    float stretch = 1.0 + speed * uniforms.stretchFactor;
    float3 stretchDir = speed > 0.001 ? normalize(p.velocity) : uniforms.cameraUp;
    float3 perpDir = normalize(cross(stretchDir, uniforms.cameraPos - p.position));
    
    // Build billboard with stretch
    float3 worldPos = p.position + 
                     perpDir * quadPos.x * p.size +
                     stretchDir * quadPos.y * p.size * stretch;
    
    out.position = uniforms.viewProj * float4(worldPos, 1.0);
    out.color = p.color;
    out.texCoord = texCoord;
    out.depth = length(worldPos - uniforms.cameraPos);
    
    return out;
}

// ===== Particle Fragment Shader =====
fragment float4 particleFragment(
    VertexOut in [[stage_in]],
    texture2d<float> particleTexture [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]],  // Scene depth for soft particles
    constant ParticleUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler texSampler(filter::linear, address::clamp_to_edge);
    
    float4 texColor = particleTexture.sample(texSampler, in.texCoord);
    float4 color = in.color * texColor;
    
    // Soft particle fade (when close to scene geometry)
    if (uniforms.softParticleDistance > 0.0) {
        float2 screenUV = in.position.xy / uniforms.screenSize;
        float sceneDepth = depthTexture.sample(texSampler, screenUV).r;
        
        // Linearize depths
        float linearParticleDepth = in.depth;
        float linearSceneDepth = uniforms.nearPlane * uniforms.farPlane / 
                                 (uniforms.farPlane - sceneDepth * (uniforms.farPlane - uniforms.nearPlane));
        
        float depthDiff = linearSceneDepth - linearParticleDepth;
        float softFade = saturate(depthDiff / uniforms.softParticleDistance);
        
        color.a *= softFade;
    }
    
    return color;
}

// ===== Additive Blending Fragment =====
fragment float4 particleFragmentAdditive(
    VertexOut in [[stage_in]],
    texture2d<float> particleTexture [[texture(0)]]
) {
    constexpr sampler texSampler(filter::linear, address::clamp_to_edge);
    
    float4 texColor = particleTexture.sample(texSampler, in.texCoord);
    float4 color = in.color * texColor;
    
    // Pre-multiply alpha for additive
    color.rgb *= color.a;
    
    return color;
}

// ===== Trail Vertex Shader =====
struct TrailVertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float4 color [[attribute(2)]];
    float width [[attribute(3)]];
};

struct TrailUniforms {
    float4x4 viewProj;
    float3 cameraPos;
};

vertex VertexOut trailVertex(
    TrailVertexIn in [[stage_in]],
    constant TrailUniforms& uniforms [[buffer(0)]]
) {
    VertexOut out;
    
    out.position = uniforms.viewProj * float4(in.position, 1.0);
    out.texCoord = in.texCoord;
    out.color = in.color;
    out.depth = length(in.position - uniforms.cameraPos);
    
    return out;
}

// ===== Simple Circle Particle (no texture) =====
fragment float4 particleFragmentCircle(
    VertexOut in [[stage_in]]
) {
    // Distance from center
    float2 center = float2(0.5, 0.5);
    float dist = length(in.texCoord - center) * 2.0;
    
    // Soft circle
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    
    float4 color = in.color;
    color.a *= alpha;
    
    return color;
}

// ===== Spark/Star Particle =====
fragment float4 particleFragmentStar(
    VertexOut in [[stage_in]]
) {
    float2 uv = in.texCoord - 0.5;
    
    // Star shape (4 points)
    float angle = atan2(uv.y, uv.x);
    float radius = length(uv) * 2.0;
    
    float star = abs(cos(angle * 2.0));
    star = pow(star, 0.5);
    
    float alpha = (1.0 - radius) * star;
    alpha = max(0.0, alpha);
    
    // Add glow
    float glow = exp(-radius * 3.0);
    alpha = max(alpha, glow * 0.5);
    
    float4 color = in.color;
    color.a *= alpha;
    
    return color;
}

// ===== Smoke/Cloud Particle =====
fragment float4 particleFragmentSmoke(
    VertexOut in [[stage_in]],
    texture2d<float> noiseTexture [[texture(0)]]
) {
    constexpr sampler texSampler(filter::linear, address::repeat);
    
    // Sample noise
    float noise = noiseTexture.sample(texSampler, in.texCoord).r;
    
    // Distance from center
    float2 center = float2(0.5, 0.5);
    float dist = length(in.texCoord - center) * 2.0;
    
    // Soft cloud shape with noise
    float alpha = 1.0 - smoothstep(0.3, 1.0, dist + noise * 0.3);
    
    float4 color = in.color;
    color.a *= alpha;
    
    return color;
}

// ===== Fire Particle =====
fragment float4 particleFragmentFire(
    VertexOut in [[stage_in]]
) {
    float2 uv = in.texCoord;
    float2 center = float2(0.5, 0.5);
    
    // Vertical gradient (hotter at bottom)
    float heat = 1.0 - uv.y;
    
    // Distance from center
    float dist = length(uv - center) * 2.0;
    
    // Fire shape
    float flame = 1.0 - smoothstep(0.0, 0.8 + heat * 0.2, dist);
    
    // Color based on heat
    float4 color;
    color.r = in.color.r * (0.5 + heat * 0.5);
    color.g = in.color.g * heat * heat;
    color.b = in.color.b * heat * heat * heat;
    color.a = in.color.a * flame;
    
    return color;
}
