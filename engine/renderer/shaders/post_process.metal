// LUMA Post-Processing Shader for Metal
// Full-screen effects: Bloom, Tone Mapping, Color Grading, Vignette, FXAA

#include <metal_stdlib>
using namespace metal;

// Effect flags
#define PP_BLOOM        (1 << 0)
#define PP_TONEMAPPING  (1 << 1)
#define PP_COLORGRADING (1 << 2)
#define PP_VIGNETTE     (1 << 3)
#define PP_CHROMATIC    (1 << 4)
#define PP_FILMGRAIN    (1 << 5)
#define PP_FXAA         (1 << 6)

// Tone mapping modes
#define TM_NONE      0
#define TM_REINHARD  1
#define TM_ACES      2
#define TM_FILMIC    3
#define TM_UNCHARTED 4

struct PostProcessConstants {
    // Bloom
    float bloomThreshold;
    float bloomIntensity;
    float bloomRadius;
    float bloomSoftThreshold;
    
    // Tone mapping
    float exposure;
    float gamma;
    float contrast;
    float saturation;
    
    // Color grading
    float4 lift;
    float4 gamma_adj;
    float4 gain;
    float temperature;
    float tint;
    float2 padding1;
    
    // Vignette
    float vignetteIntensity;
    float vignetteSmoothness;
    float vignetteRoundness;
    float vignettePadding;
    float4 vignetteCenter;
    
    // Chromatic aberration
    float chromaIntensity;
    float3 padding2;
    
    // Film grain
    float grainIntensity;
    float grainResponse;
    float grainTime;
    float padding3;
    
    // FXAA
    float fxaaContrastThreshold;
    float fxaaRelativeThreshold;
    float fxaaSubpixelBlending;
    float padding4;
    
    // Screen info
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    
    // Flags
    uint enabledEffects;
    uint toneMappingMode;
    float2 padding5;
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

// Full-screen triangle vertex shader
vertex VertexOut postProcessVertex(uint vertexId [[vertex_id]]) {
    VertexOut out;
    
    // Generate full-screen triangle
    out.uv = float2((vertexId << 1) & 2, vertexId & 2);
    out.position = float4(out.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return out;
}

// ===== Helper Functions =====

float luminance(float3 color) {
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// ===== Tone Mapping Functions =====

float3 toneMapReinhard(float3 color) {
    return color / (1.0 + color);
}

float3 toneMapACES(float3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 toneMapFilmic(float3 color) {
    color = max(float3(0), color - 0.004);
    return (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
}

float3 uncharted2Tonemap(float3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 toneMapUncharted2(float3 color) {
    float W = 11.2;
    float exposureBias = 2.0;
    float3 curr = uncharted2Tonemap(exposureBias * color);
    float3 whiteScale = 1.0 / uncharted2Tonemap(float3(W));
    return curr * whiteScale;
}

float3 applyToneMapping(float3 color, uint mode) {
    switch (mode) {
        case TM_REINHARD:  return toneMapReinhard(color);
        case TM_ACES:      return toneMapACES(color);
        case TM_FILMIC:    return toneMapFilmic(color);
        case TM_UNCHARTED: return toneMapUncharted2(color);
        default:           return color;
    }
}

// ===== Color Grading Functions =====

float3 applyLiftGammaGain(float3 color, float3 lift, float3 gamma_adj, float3 gain) {
    color = color + lift * (1.0 - color);
    color = pow(max(color, float3(0.0001)), 1.0 / gamma_adj);
    color = color * gain;
    return color;
}

float3 applyTemperatureTint(float3 color, float temperature, float tint) {
    float3 tempColor = float3(1.0 + temperature * 0.1, 1.0, 1.0 - temperature * 0.1);
    float3 tintColor = float3(1.0 + tint * 0.05, 1.0 - abs(tint) * 0.05, 1.0 - tint * 0.05);
    return color * tempColor * tintColor;
}

float3 adjustSaturation(float3 color, float sat) {
    float lum = luminance(color);
    return mix(float3(lum), color, sat);
}

float3 adjustContrast(float3 color, float con) {
    return (color - 0.5) * con + 0.5;
}

// ===== Vignette =====

float3 applyVignette(float3 color, float2 uv, constant PostProcessConstants& c) {
    float2 coord = (uv - c.vignetteCenter.xy) * 2.0;
    coord.x *= mix(1.0, c.screenWidth / c.screenHeight, c.vignetteRoundness);
    
    float dist = length(coord);
    float vignette = smoothstep(1.0, 1.0 - c.vignetteSmoothness, dist * (1.0 + c.vignetteIntensity));
    
    return color * vignette;
}

// ===== Chromatic Aberration =====

float3 applyChromaticAberration(float2 uv, float intensity, 
                                 texture2d<float> sceneTexture, sampler linearSampler) {
    float2 dir = uv - 0.5;
    float dist = length(dir);
    float2 offset = dir * dist * intensity;
    
    float3 color;
    color.r = sceneTexture.sample(linearSampler, uv + offset).r;
    color.g = sceneTexture.sample(linearSampler, uv).g;
    color.b = sceneTexture.sample(linearSampler, uv - offset).b;
    
    return color;
}

// ===== Film Grain =====

float random(float2 uv) {
    return fract(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float3 applyFilmGrain(float3 color, float2 uv, float intensity, float response, float time) {
    float grain = random(uv + time) * 2.0 - 1.0;
    float lum = luminance(color);
    float resp = mix(response, 1.0, lum);
    return color + grain * intensity * resp;
}

// ===== FXAA =====

float3 applyFXAA(float2 uv, constant PostProcessConstants& c,
                 texture2d<float> sceneTexture, sampler linearSampler) {
    float2 texelSize = float2(c.invScreenWidth, c.invScreenHeight);
    
    float3 colorCenter = sceneTexture.sample(linearSampler, uv).rgb;
    float3 colorN = sceneTexture.sample(linearSampler, uv + float2(0, -texelSize.y)).rgb;
    float3 colorS = sceneTexture.sample(linearSampler, uv + float2(0, texelSize.y)).rgb;
    float3 colorE = sceneTexture.sample(linearSampler, uv + float2(texelSize.x, 0)).rgb;
    float3 colorW = sceneTexture.sample(linearSampler, uv + float2(-texelSize.x, 0)).rgb;
    
    float lumCenter = luminance(colorCenter);
    float lumN = luminance(colorN);
    float lumS = luminance(colorS);
    float lumE = luminance(colorE);
    float lumW = luminance(colorW);
    
    float lumMin = min(lumCenter, min(min(lumN, lumS), min(lumE, lumW)));
    float lumMax = max(lumCenter, max(max(lumN, lumS), max(lumE, lumW)));
    float lumRange = lumMax - lumMin;
    
    if (lumRange < max(c.fxaaContrastThreshold, lumMax * c.fxaaRelativeThreshold)) {
        return colorCenter;
    }
    
    float lumNS = lumN + lumS;
    float lumEW = lumE + lumW;
    
    float3 colorNW = sceneTexture.sample(linearSampler, uv + float2(-texelSize.x, -texelSize.y)).rgb;
    float3 colorNE = sceneTexture.sample(linearSampler, uv + float2(texelSize.x, -texelSize.y)).rgb;
    float3 colorSW = sceneTexture.sample(linearSampler, uv + float2(-texelSize.x, texelSize.y)).rgb;
    float3 colorSE = sceneTexture.sample(linearSampler, uv + float2(texelSize.x, texelSize.y)).rgb;
    
    float lumNW = luminance(colorNW);
    float lumNE = luminance(colorNE);
    float lumSW = luminance(colorSW);
    float lumSE = luminance(colorSE);
    
    float edgeH = abs(-2.0 * lumW + lumNW + lumSW) + abs(-2.0 * lumCenter + lumN + lumS) * 2.0 + abs(-2.0 * lumE + lumNE + lumSE);
    float edgeV = abs(-2.0 * lumN + lumNW + lumNE) + abs(-2.0 * lumCenter + lumW + lumE) * 2.0 + abs(-2.0 * lumS + lumSW + lumSE);
    
    bool isHorizontal = edgeH >= edgeV;
    
    float subpixelOffset = 0.5;
    float2 blendDir = isHorizontal ? float2(0, texelSize.y) : float2(texelSize.x, 0);
    
    float3 colorBlend = sceneTexture.sample(linearSampler, uv + blendDir * subpixelOffset).rgb;
    
    return mix(colorCenter, colorBlend, c.fxaaSubpixelBlending);
}

// ===== Main Fragment Shader =====

fragment float4 postProcessFragment(
    VertexOut in [[stage_in]],
    constant PostProcessConstants& c [[buffer(0)]],
    texture2d<float> sceneTexture [[texture(0)]],
    texture2d<float> bloomTexture [[texture(1)]],
    sampler linearSampler [[sampler(0)]]
) {
    float2 uv = in.uv;
    float3 color;
    
    // Chromatic aberration
    if (c.enabledEffects & PP_CHROMATIC) {
        color = applyChromaticAberration(uv, c.chromaIntensity, sceneTexture, linearSampler);
    } else {
        color = sceneTexture.sample(linearSampler, uv).rgb;
    }
    
    // Add bloom
    if (c.enabledEffects & PP_BLOOM) {
        float3 bloom = bloomTexture.sample(linearSampler, uv).rgb;
        color += bloom * c.bloomIntensity;
    }
    
    // Exposure
    color *= c.exposure;
    
    // Tone mapping
    if (c.enabledEffects & PP_TONEMAPPING) {
        color = applyToneMapping(color, c.toneMappingMode);
    }
    
    // Color grading
    if (c.enabledEffects & PP_COLORGRADING) {
        color = applyLiftGammaGain(color, c.lift.rgb, c.gamma_adj.rgb, c.gain.rgb);
        color = applyTemperatureTint(color, c.temperature, c.tint);
        color = adjustSaturation(color, c.saturation);
        color = adjustContrast(color, c.contrast);
    }
    
    // Vignette
    if (c.enabledEffects & PP_VIGNETTE) {
        color = applyVignette(color, uv, c);
    }
    
    // Film grain
    if (c.enabledEffects & PP_FILMGRAIN) {
        color = applyFilmGrain(color, uv, c.grainIntensity, c.grainResponse, c.grainTime);
    }
    
    // Gamma correction
    color = pow(max(color, float3(0)), 1.0 / c.gamma);
    
    return float4(saturate(color), 1.0);
}

// ===== Bloom Shaders =====

fragment float4 bloomThresholdFragment(
    VertexOut in [[stage_in]],
    constant PostProcessConstants& c [[buffer(0)]],
    texture2d<float> sceneTexture [[texture(0)]],
    sampler linearSampler [[sampler(0)]]
) {
    float3 color = sceneTexture.sample(linearSampler, in.uv).rgb;
    
    float brightness = luminance(color);
    float soft = brightness - c.bloomThreshold + c.bloomSoftThreshold;
    soft = clamp(soft, 0.0, 2.0 * c.bloomSoftThreshold);
    soft = soft * soft / (4.0 * c.bloomSoftThreshold + 0.0001);
    
    float contribution = max(soft, brightness - c.bloomThreshold);
    contribution /= max(brightness, 0.0001);
    
    return float4(color * contribution, 1.0);
}

fragment float4 blurHFragment(
    VertexOut in [[stage_in]],
    constant PostProcessConstants& c [[buffer(0)]],
    texture2d<float> sceneTexture [[texture(0)]],
    sampler linearSampler [[sampler(0)]]
) {
    float2 texelSize = float2(c.invScreenWidth, c.invScreenHeight);
    float3 color = float3(0);
    
    float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    
    color += sceneTexture.sample(linearSampler, in.uv).rgb * weights[0];
    
    for (int i = 1; i < 5; i++) {
        float2 offset = float2(texelSize.x * i * c.bloomRadius, 0);
        color += sceneTexture.sample(linearSampler, in.uv + offset).rgb * weights[i];
        color += sceneTexture.sample(linearSampler, in.uv - offset).rgb * weights[i];
    }
    
    return float4(color, 1.0);
}

fragment float4 blurVFragment(
    VertexOut in [[stage_in]],
    constant PostProcessConstants& c [[buffer(0)]],
    texture2d<float> sceneTexture [[texture(0)]],
    sampler linearSampler [[sampler(0)]]
) {
    float2 texelSize = float2(c.invScreenWidth, c.invScreenHeight);
    float3 color = float3(0);
    
    float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    
    color += sceneTexture.sample(linearSampler, in.uv).rgb * weights[0];
    
    for (int i = 1; i < 5; i++) {
        float2 offset = float2(0, texelSize.y * i * c.bloomRadius);
        color += sceneTexture.sample(linearSampler, in.uv + offset).rgb * weights[i];
        color += sceneTexture.sample(linearSampler, in.uv - offset).rgb * weights[i];
    }
    
    return float4(color, 1.0);
}

// FXAA pass
fragment float4 fxaaFragment(
    VertexOut in [[stage_in]],
    constant PostProcessConstants& c [[buffer(0)]],
    texture2d<float> sceneTexture [[texture(0)]],
    sampler linearSampler [[sampler(0)]]
) {
    float3 color = applyFXAA(in.uv, c, sceneTexture, linearSampler);
    return float4(color, 1.0);
}
