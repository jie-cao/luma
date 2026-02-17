// LUMA Procedural Texture Functions
// HLSL functions for procedural texture generation in node-based materials
// Includes: Hash, Noise (Perlin/Simplex), Voronoi, fBM, Patterns

#ifndef PROCEDURAL_HLSLI
#define PROCEDURAL_HLSLI

// ===== Hash Functions =====
// Quality hash functions for procedural generation

float hash11(float p) {
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float hash21(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float2 hash22(float2 p) {
    float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float3 hash33(float3 p3) {
    p3 = frac(p3 * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

float hash31(float3 p) {
    p = frac(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return frac((p.x + p.y) * p.z);
}

// ===== Perlin Noise 2D =====

float2 perlinGrad2D(float2 p) {
    float angle = hash21(p) * 6.283185307;
    return float2(cos(angle), sin(angle));
}

float perlinNoise2D(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    
    // Quintic interpolation
    float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    float2 g00 = perlinGrad2D(i + float2(0, 0));
    float2 g10 = perlinGrad2D(i + float2(1, 0));
    float2 g01 = perlinGrad2D(i + float2(0, 1));
    float2 g11 = perlinGrad2D(i + float2(1, 1));
    
    float n00 = dot(g00, f - float2(0, 0));
    float n10 = dot(g10, f - float2(1, 0));
    float n01 = dot(g01, f - float2(0, 1));
    float n11 = dot(g11, f - float2(1, 1));
    
    float nx0 = lerp(n00, n10, u.x);
    float nx1 = lerp(n01, n11, u.x);
    
    return lerp(nx0, nx1, u.y);
}

// ===== Perlin Noise 3D =====

float3 perlinGrad3D(float3 p) {
    float3 h = hash33(p);
    return normalize(h * 2.0 - 1.0);
}

float perlinNoise3D(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    float n000 = dot(perlinGrad3D(i + float3(0,0,0)), f - float3(0,0,0));
    float n100 = dot(perlinGrad3D(i + float3(1,0,0)), f - float3(1,0,0));
    float n010 = dot(perlinGrad3D(i + float3(0,1,0)), f - float3(0,1,0));
    float n110 = dot(perlinGrad3D(i + float3(1,1,0)), f - float3(1,1,0));
    float n001 = dot(perlinGrad3D(i + float3(0,0,1)), f - float3(0,0,1));
    float n101 = dot(perlinGrad3D(i + float3(1,0,1)), f - float3(1,0,1));
    float n011 = dot(perlinGrad3D(i + float3(0,1,1)), f - float3(0,1,1));
    float n111 = dot(perlinGrad3D(i + float3(1,1,1)), f - float3(1,1,1));
    
    float nx00 = lerp(n000, n100, u.x);
    float nx10 = lerp(n010, n110, u.x);
    float nx01 = lerp(n001, n101, u.x);
    float nx11 = lerp(n011, n111, u.x);
    float nxy0 = lerp(nx00, nx10, u.y);
    float nxy1 = lerp(nx01, nx11, u.y);
    
    return lerp(nxy0, nxy1, u.z);
}

// ===== Simplex Noise 2D =====

float simplexNoise2D(float2 p) {
    const float K1 = 0.366025404;  // (sqrt(3)-1)/2
    const float K2 = 0.211324865;  // (3-sqrt(3))/6
    
    float2 i = floor(p + (p.x + p.y) * K1);
    float2 a = p - i + (i.x + i.y) * K2;
    float2 o = (a.x > a.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
    float2 b = a - o + K2;
    float2 c = a - 1.0 + 2.0 * K2;
    
    float3 h = max(0.5 - float3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    float3 n = h * h * h * h * float3(
        dot(a, hash22(i) * 2.0 - 1.0),
        dot(b, hash22(i + o) * 2.0 - 1.0),
        dot(c, hash22(i + 1.0) * 2.0 - 1.0)
    );
    
    return dot(n, float3(70.0, 70.0, 70.0));
}

// ===== Value Noise 2D =====

float valueNoise2D(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);
    
    float a = hash21(i + float2(0.0, 0.0));
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));
    
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// ===== Voronoi / Worley Noise 2D =====

float voronoi2D(float2 p, float randomness, out float2 cellCenter) {
    float2 i = floor(p);
    float2 f = frac(p);
    
    float minDist = 10.0;
    cellCenter = float2(0, 0);
    
    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            float2 neighbor = float2(x, y);
            float2 point = hash22(i + neighbor) * randomness;
            float2 diff = neighbor + point - f;
            float dist = dot(diff, diff);
            
            if (dist < minDist) {
                minDist = dist;
                cellCenter = (i + neighbor + point);
            }
        }
    }
    
    return sqrt(minDist);
}

// Voronoi F2 (second closest)
float voronoiF2_2D(float2 p, float randomness) {
    float2 i = floor(p);
    float2 f = frac(p);
    
    float f1 = 10.0;
    float f2 = 10.0;
    
    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            float2 neighbor = float2(x, y);
            float2 point = hash22(i + neighbor) * randomness;
            float2 diff = neighbor + point - f;
            float dist = dot(diff, diff);
            
            if (dist < f1) {
                f2 = f1;
                f1 = dist;
            } else if (dist < f2) {
                f2 = dist;
            }
        }
    }
    
    return sqrt(f2);
}

// ===== fBM (Fractal Brownian Motion) =====

float fbm2D(float2 p, int octaves, float roughness) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    for (int i = 0; i < octaves && i < 8; i++) {
        value += amplitude * perlinNoise2D(p * frequency);
        maxValue += amplitude;
        amplitude *= roughness;
        frequency *= 2.0;
    }
    
    return value / max(maxValue, 0.001);
}

float fbm3D(float3 p, int octaves, float roughness) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    for (int i = 0; i < octaves && i < 8; i++) {
        value += amplitude * perlinNoise3D(p * frequency);
        maxValue += amplitude;
        amplitude *= roughness;
        frequency *= 2.0;
    }
    
    return value / max(maxValue, 0.001);
}

// ===== Musgrave Noise =====

float musgraveNoise(float2 p, int octaves, float dimension, float lacunarity) {
    float value = 0.0;
    float weight = 1.0;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves && i < 8; i++) {
        float signal = perlinNoise2D(p * frequency);
        signal = abs(signal);
        signal = 1.0 - signal; // Ridged
        signal *= signal;
        signal *= weight;
        weight = saturate(signal * 2.0);
        
        value += signal * pow(frequency, -dimension);
        frequency *= lacunarity;
    }
    
    return value;
}

// ===== Turbulence =====

float turbulence2D(float2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves && i < 8; i++) {
        value += amplitude * abs(perlinNoise2D(p * frequency));
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value;
}

// ===== Pattern Functions =====

// Brick pattern
float brickPattern(float2 uv, float mortarSize) {
    float2 cell = uv;
    // Offset every other row
    if (fmod(floor(cell.y), 2.0) > 0.5) {
        cell.x += 0.5;
    }
    
    // Scale X by 2 (bricks are wider than tall)
    cell.x *= 2.0;
    
    float2 f = frac(cell);
    float2 mortar = step(mortarSize, f) * step(f, 1.0 - mortarSize);
    
    return mortar.x * mortar.y;
}

// ===== Color Space Conversion =====

float3 rgbToHsv(float3 c) {
    float4 K = float4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 hsvToRgb(float3 c) {
    float4 K = float4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

#endif // PROCEDURAL_HLSLI
