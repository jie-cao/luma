// LUMA Post-Processing Shader (HLSL)
// Full-screen effects: Bloom, Tone Mapping, Color Grading, Vignette, FXAA

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

cbuffer PostProcessConstants : register(b0) {
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

Texture2D sceneTexture : register(t0);
Texture2D bloomTexture : register(t1);
SamplerState linearSampler : register(s0);

struct VSInput {
    uint vertexId : SV_VertexID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Full-screen triangle vertex shader
PSInput VSMain(VSInput input) {
    PSInput output;
    
    // Generate full-screen triangle
    output.uv = float2((input.vertexId << 1) & 2, input.vertexId & 2);
    output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return output;
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
    color = max(0, color - 0.004);
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
    float3 whiteScale = 1.0 / uncharted2Tonemap(W);
    return curr * whiteScale;
}

float3 applyToneMapping(float3 color) {
    switch (toneMappingMode) {
        case TM_REINHARD:  return toneMapReinhard(color);
        case TM_ACES:      return toneMapACES(color);
        case TM_FILMIC:    return toneMapFilmic(color);
        case TM_UNCHARTED: return toneMapUncharted2(color);
        default:           return color;
    }
}

// ===== Color Grading Functions =====

float3 applyLiftGammaGain(float3 color) {
    // Lift (shadows)
    color = color + lift.rgb * (1.0 - color);
    
    // Gamma (midtones)
    color = pow(max(color, 0.0001), 1.0 / gamma_adj.rgb);
    
    // Gain (highlights)
    color = color * gain.rgb;
    
    return color;
}

float3 applyTemperatureTint(float3 color) {
    // Temperature (blue-yellow)
    float3 tempColor = float3(1.0 + temperature * 0.1, 1.0, 1.0 - temperature * 0.1);
    
    // Tint (green-magenta)
    float3 tintColor = float3(1.0 + tint * 0.05, 1.0 - abs(tint) * 0.05, 1.0 - tint * 0.05);
    
    return color * tempColor * tintColor;
}

float3 adjustSaturation(float3 color, float sat) {
    float lum = luminance(color);
    return lerp(float3(lum, lum, lum), color, sat);
}

float3 adjustContrast(float3 color, float con) {
    return (color - 0.5) * con + 0.5;
}

// ===== Vignette =====

float3 applyVignette(float3 color, float2 uv) {
    float2 coord = (uv - vignetteCenter.xy) * 2.0;
    coord.x *= lerp(1.0, screenWidth / screenHeight, vignetteRoundness);
    
    float dist = length(coord);
    float vignette = smoothstep(1.0, 1.0 - vignetteSmoothness, dist * (1.0 + vignetteIntensity));
    
    return color * vignette;
}

// ===== Chromatic Aberration =====

float3 applyChromaticAberration(float2 uv) {
    float2 dir = uv - 0.5;
    float dist = length(dir);
    float2 offset = dir * dist * chromaIntensity;
    
    float3 color;
    color.r = sceneTexture.Sample(linearSampler, uv + offset).r;
    color.g = sceneTexture.Sample(linearSampler, uv).g;
    color.b = sceneTexture.Sample(linearSampler, uv - offset).b;
    
    return color;
}

// ===== Film Grain =====

float random(float2 uv) {
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float3 applyFilmGrain(float3 color, float2 uv) {
    float grain = random(uv + grainTime) * 2.0 - 1.0;
    float lum = luminance(color);
    float response = lerp(grainResponse, 1.0, lum);
    return color + grain * grainIntensity * response;
}

// ===== FXAA =====

float3 applyFXAA(float2 uv) {
    float2 texelSize = float2(invScreenWidth, invScreenHeight);
    
    // Sample neighbors
    float3 colorCenter = sceneTexture.Sample(linearSampler, uv).rgb;
    float3 colorN = sceneTexture.Sample(linearSampler, uv + float2(0, -texelSize.y)).rgb;
    float3 colorS = sceneTexture.Sample(linearSampler, uv + float2(0, texelSize.y)).rgb;
    float3 colorE = sceneTexture.Sample(linearSampler, uv + float2(texelSize.x, 0)).rgb;
    float3 colorW = sceneTexture.Sample(linearSampler, uv + float2(-texelSize.x, 0)).rgb;
    
    // Compute luminances
    float lumCenter = luminance(colorCenter);
    float lumN = luminance(colorN);
    float lumS = luminance(colorS);
    float lumE = luminance(colorE);
    float lumW = luminance(colorW);
    
    // Find edge direction
    float lumMin = min(lumCenter, min(min(lumN, lumS), min(lumE, lumW)));
    float lumMax = max(lumCenter, max(max(lumN, lumS), max(lumE, lumW)));
    float lumRange = lumMax - lumMin;
    
    // Skip if contrast too low
    if (lumRange < max(fxaaContrastThreshold, lumMax * fxaaRelativeThreshold)) {
        return colorCenter;
    }
    
    // Compute blend direction
    float lumNS = lumN + lumS;
    float lumEW = lumE + lumW;
    
    float3 colorNW = sceneTexture.Sample(linearSampler, uv + float2(-texelSize.x, -texelSize.y)).rgb;
    float3 colorNE = sceneTexture.Sample(linearSampler, uv + float2(texelSize.x, -texelSize.y)).rgb;
    float3 colorSW = sceneTexture.Sample(linearSampler, uv + float2(-texelSize.x, texelSize.y)).rgb;
    float3 colorSE = sceneTexture.Sample(linearSampler, uv + float2(texelSize.x, texelSize.y)).rgb;
    
    float lumNW = luminance(colorNW);
    float lumNE = luminance(colorNE);
    float lumSW = luminance(colorSW);
    float lumSE = luminance(colorSE);
    
    // Compute edge
    float edgeH = abs(-2.0 * lumW + lumNW + lumSW) + abs(-2.0 * lumCenter + lumN + lumS) * 2.0 + abs(-2.0 * lumE + lumNE + lumSE);
    float edgeV = abs(-2.0 * lumN + lumNW + lumNE) + abs(-2.0 * lumCenter + lumW + lumE) * 2.0 + abs(-2.0 * lumS + lumSW + lumSE);
    
    bool isHorizontal = edgeH >= edgeV;
    
    // Subpixel blend
    float subpixelOffset = 0.5;
    float2 blendDir = isHorizontal ? float2(0, texelSize.y) : float2(texelSize.x, 0);
    
    float3 colorBlend = sceneTexture.Sample(linearSampler, uv + blendDir * subpixelOffset).rgb;
    
    return lerp(colorCenter, colorBlend, fxaaSubpixelBlending);
}

// ===== Main Pixel Shader =====

float4 PSMain(PSInput input) : SV_TARGET {
    float2 uv = input.uv;
    float3 color;
    
    // Chromatic aberration (before other effects)
    if (enabledEffects & PP_CHROMATIC) {
        color = applyChromaticAberration(uv);
    } else {
        color = sceneTexture.Sample(linearSampler, uv).rgb;
    }
    
    // Add bloom
    if (enabledEffects & PP_BLOOM) {
        float3 bloom = bloomTexture.Sample(linearSampler, uv).rgb;
        color += bloom * bloomIntensity;
    }
    
    // Exposure
    color *= exposure;
    
    // Tone mapping
    if (enabledEffects & PP_TONEMAPPING) {
        color = applyToneMapping(color);
    }
    
    // Color grading
    if (enabledEffects & PP_COLORGRADING) {
        color = applyLiftGammaGain(color);
        color = applyTemperatureTint(color);
        color = adjustSaturation(color, saturation);
        color = adjustContrast(color, contrast);
    }
    
    // Vignette
    if (enabledEffects & PP_VIGNETTE) {
        color = applyVignette(color, uv);
    }
    
    // Film grain
    if (enabledEffects & PP_FILMGRAIN) {
        color = applyFilmGrain(color, uv);
    }
    
    // Gamma correction
    color = pow(max(color, 0.0), 1.0 / gamma);
    
    return float4(saturate(color), 1.0);
}

// ===== Bloom Shaders =====

// Bloom threshold extraction
float4 PSBloomThreshold(PSInput input) : SV_TARGET {
    float3 color = sceneTexture.Sample(linearSampler, input.uv).rgb;
    
    float brightness = luminance(color);
    float soft = brightness - bloomThreshold + bloomSoftThreshold;
    soft = clamp(soft, 0.0, 2.0 * bloomSoftThreshold);
    soft = soft * soft / (4.0 * bloomSoftThreshold + 0.0001);
    
    float contribution = max(soft, brightness - bloomThreshold);
    contribution /= max(brightness, 0.0001);
    
    return float4(color * contribution, 1.0);
}

// Gaussian blur (horizontal)
float4 PSBlurH(PSInput input) : SV_TARGET {
    float2 texelSize = float2(invScreenWidth, invScreenHeight);
    float3 color = float3(0, 0, 0);
    
    // 9-tap Gaussian blur
    float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    
    color += sceneTexture.Sample(linearSampler, input.uv).rgb * weights[0];
    
    for (int i = 1; i < 5; i++) {
        float2 offset = float2(texelSize.x * i * bloomRadius, 0);
        color += sceneTexture.Sample(linearSampler, input.uv + offset).rgb * weights[i];
        color += sceneTexture.Sample(linearSampler, input.uv - offset).rgb * weights[i];
    }
    
    return float4(color, 1.0);
}

// Gaussian blur (vertical)
float4 PSBlurV(PSInput input) : SV_TARGET {
    float2 texelSize = float2(invScreenWidth, invScreenHeight);
    float3 color = float3(0, 0, 0);
    
    float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    
    color += sceneTexture.Sample(linearSampler, input.uv).rgb * weights[0];
    
    for (int i = 1; i < 5; i++) {
        float2 offset = float2(0, texelSize.y * i * bloomRadius);
        color += sceneTexture.Sample(linearSampler, input.uv + offset).rgb * weights[i];
        color += sceneTexture.Sample(linearSampler, input.uv - offset).rgb * weights[i];
    }
    
    return float4(color, 1.0);
}

// FXAA as separate pass
float4 PSFXAA(PSInput input) : SV_TARGET {
    float3 color = applyFXAA(input.uv);
    return float4(color, 1.0);
}
