// Hair Rendering System - Strand-based realistic hair
// High-quality hair rendering with physically based shading
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <cmath>
#include <random>

namespace luma {

// ============================================================================
// Hair Strand - Individual hair fiber
// ============================================================================

struct HairControlPoint {
    Vec3 position;
    float radius = 0.001f;      // Hair thickness
    Vec3 color{0.15f, 0.1f, 0.08f};
    float ao = 1.0f;            // Ambient occlusion
};

struct HairStrand {
    std::vector<HairControlPoint> controlPoints;
    int strandIndex = 0;
    int groupIndex = 0;         // For grouping (bangs, side, back, etc.)
    
    // Interpolated data for rendering
    std::vector<Vec3> tessellatedPositions;
    std::vector<Vec3> tessellatedTangents;
    std::vector<float> tessellatedRadii;
    
    // Get strand length
    float getLength() const {
        float length = 0;
        for (size_t i = 1; i < controlPoints.size(); i++) {
            length += (controlPoints[i].position - controlPoints[i-1].position).length();
        }
        return length;
    }
    
    // Tessellate for smooth rendering
    void tessellate(int segmentsPerControl = 4) {
        if (controlPoints.size() < 2) return;
        
        tessellatedPositions.clear();
        tessellatedTangents.clear();
        tessellatedRadii.clear();
        
        int totalSegments = (controlPoints.size() - 1) * segmentsPerControl;
        
        for (int i = 0; i <= totalSegments; i++) {
            float t = static_cast<float>(i) / totalSegments;
            
            // Catmull-Rom interpolation
            Vec3 pos = interpolateCatmullRom(t);
            float radius = interpolateRadius(t);
            
            tessellatedPositions.push_back(pos);
            tessellatedRadii.push_back(radius);
        }
        
        // Calculate tangents
        for (size_t i = 0; i < tessellatedPositions.size(); i++) {
            Vec3 tangent;
            if (i == 0) {
                tangent = (tessellatedPositions[1] - tessellatedPositions[0]).normalized();
            } else if (i == tessellatedPositions.size() - 1) {
                tangent = (tessellatedPositions[i] - tessellatedPositions[i-1]).normalized();
            } else {
                tangent = (tessellatedPositions[i+1] - tessellatedPositions[i-1]).normalized();
            }
            tessellatedTangents.push_back(tangent);
        }
    }
    
private:
    Vec3 interpolateCatmullRom(float t) const {
        int n = controlPoints.size();
        float segment = t * (n - 1);
        int i = static_cast<int>(segment);
        float localT = segment - i;
        
        // Clamp indices
        int i0 = std::max(0, i - 1);
        int i1 = std::min(n - 1, i);
        int i2 = std::min(n - 1, i + 1);
        int i3 = std::min(n - 1, i + 2);
        
        Vec3 p0 = controlPoints[i0].position;
        Vec3 p1 = controlPoints[i1].position;
        Vec3 p2 = controlPoints[i2].position;
        Vec3 p3 = controlPoints[i3].position;
        
        // Catmull-Rom formula
        float t2 = localT * localT;
        float t3 = t2 * localT;
        
        return (p1 * 2.0f + (p2 - p0) * localT +
                (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
                (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
    }
    
    float interpolateRadius(float t) const {
        int n = controlPoints.size();
        float segment = t * (n - 1);
        int i = static_cast<int>(segment);
        float localT = segment - i;
        
        int i1 = std::min(n - 1, i);
        int i2 = std::min(n - 1, i + 1);
        
        return controlPoints[i1].radius * (1.0f - localT) + controlPoints[i2].radius * localT;
    }
};

// ============================================================================
// Hair Material - Marschner/Kajiya-Kay shading model
// ============================================================================

struct HairMaterialParams {
    // Base color
    Vec3 baseColor{0.15f, 0.1f, 0.08f};
    Vec3 tipColor{0.2f, 0.15f, 0.12f};
    float colorVariation = 0.1f;
    
    // Specular (Marschner model)
    float primarySpecularStrength = 1.0f;
    float primarySpecularShift = 0.1f;      // Shift along hair direction
    float primarySpecularWidth = 5.0f;       // Degrees
    Vec3 primarySpecularColor{1, 1, 1};
    
    float secondarySpecularStrength = 0.5f;
    float secondarySpecularShift = -0.05f;
    float secondarySpecularWidth = 10.0f;
    Vec3 secondarySpecularColor{1, 0.9f, 0.8f};
    
    // Transmission (light through hair)
    float transmissionStrength = 0.3f;
    Vec3 transmissionColor{0.8f, 0.6f, 0.5f};
    
    // Scattering
    float scatterAmount = 0.2f;
    float backScatter = 0.3f;
    
    // Ambient occlusion
    float aoStrength = 0.5f;
    float selfShadowStrength = 0.3f;
    
    // Strand properties
    float rootThickness = 0.0015f;
    float tipThickness = 0.0005f;
    float strandRandomness = 0.1f;
};

// ============================================================================
// Hair Shading - Marschner model implementation
// ============================================================================

class HairShader {
public:
    // Compute hair shading at a point
    static Vec3 shade(const Vec3& position,
                      const Vec3& tangent,
                      const Vec3& viewDir,
                      const Vec3& lightDir,
                      const Vec3& lightColor,
                      const HairMaterialParams& params,
                      float t,            // Parameter along strand (0=root, 1=tip)
                      float ao = 1.0f) {
        
        // Base color with variation along strand
        Vec3 baseColor = params.baseColor.lerp(params.tipColor, t);
        
        // Tangent-based shading (Kajiya-Kay)
        float TdotL = tangent.dot(lightDir);
        float TdotV = tangent.dot(viewDir);
        
        float sinTL = std::sqrt(1.0f - TdotL * TdotL);
        float sinTV = std::sqrt(1.0f - TdotV * TdotV);
        
        // Diffuse (modified Lambert for hair)
        float diffuse = sinTL;
        
        // Primary specular (R reflection)
        float cosTL = TdotL;
        float cosTV = TdotV;
        
        float shiftedAngle1 = std::asin(cosTL) + params.primarySpecularShift;
        float specular1 = kajiyaKaySpecular(shiftedAngle1, cosTV, 
                                            params.primarySpecularWidth);
        specular1 *= params.primarySpecularStrength;
        
        // Secondary specular (TRT - through hair, reflect, through)
        float shiftedAngle2 = std::asin(cosTL) + params.secondarySpecularShift;
        float specular2 = kajiyaKaySpecular(shiftedAngle2, cosTV,
                                            params.secondarySpecularWidth);
        specular2 *= params.secondarySpecularStrength;
        
        // Transmission (TT - through hair twice)
        float transmission = 0.0f;
        if (TdotL < 0) {  // Light from behind
            transmission = std::pow(-TdotL, 2.0f) * params.transmissionStrength;
        }
        
        // Back scatter
        float backScatter = std::max(0.0f, -tangent.dot(lightDir + viewDir).normalized().dot(tangent));
        backScatter *= params.backScatter;
        
        // Combine
        Vec3 result(0, 0, 0);
        
        // Diffuse
        result = result + baseColor * diffuse * 0.5f;
        
        // Specular highlights
        result = result + params.primarySpecularColor * specular1;
        result = result + params.secondarySpecularColor * specular2;
        
        // Transmission
        result = result + params.transmissionColor * transmission;
        
        // Back scatter
        result = result + baseColor * backScatter;
        
        // Apply light color
        result.x *= lightColor.x;
        result.y *= lightColor.y;
        result.z *= lightColor.z;
        
        // Ambient occlusion
        float aoFactor = 1.0f - params.aoStrength * (1.0f - ao);
        result = result * aoFactor;
        
        return result;
    }
    
private:
    static float kajiyaKaySpecular(float angle, float cosTV, float width) {
        float diff = std::cos(angle) - cosTV;
        float widthRad = width * 3.14159f / 180.0f;
        return std::exp(-diff * diff / (2.0f * widthRad * widthRad));
    }
};

// ============================================================================
// Hair Generator - Procedural strand generation
// ============================================================================

struct HairGenerationParams {
    // Density
    int strandCount = 50000;
    float density = 100.0f;         // Strands per unit area
    
    // Strand shape
    int controlPointsPerStrand = 8;
    float baseLength = 0.2f;
    float lengthVariation = 0.1f;
    
    // Curliness
    float curlFrequency = 2.0f;
    float curlAmplitude = 0.01f;
    float curlPhaseVariation = 1.0f;
    
    // Clumping
    float clumpStrength = 0.3f;
    int clumpsPerGroup = 10;
    
    // Frizz
    float frizzStrength = 0.005f;
    
    // Gravity
    float gravity = 0.02f;
    float stiffness = 0.8f;         // Root stiffness
    
    // Thickness
    float rootThickness = 0.0015f;
    float tipThickness = 0.0005f;
    
    // Groups
    bool generateBangs = true;
    bool generateSides = true;
    bool generateBack = true;
};

class HairGenerator {
public:
    // Generate hair from scalp mesh
    static std::vector<HairStrand> generateFromScalp(const Mesh& scalpMesh,
                                                      const HairGenerationParams& params) {
        std::vector<HairStrand> strands;
        strands.reserve(params.strandCount);
        
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        std::uniform_real_distribution<float> distNeg(-1.0f, 1.0f);
        
        // Calculate total surface area
        float totalArea = 0;
        std::vector<float> triangleAreas;
        
        for (size_t i = 0; i < scalpMesh.indices.size(); i += 3) {
            const Vec3& v0 = scalpMesh.vertices[scalpMesh.indices[i]].position;
            const Vec3& v1 = scalpMesh.vertices[scalpMesh.indices[i+1]].position;
            const Vec3& v2 = scalpMesh.vertices[scalpMesh.indices[i+2]].position;
            
            Vec3 e1 = v1 - v0;
            Vec3 e2 = v2 - v0;
            float area = e1.cross(e2).length() * 0.5f;
            triangleAreas.push_back(area);
            totalArea += area;
        }
        
        // Generate strands
        for (int s = 0; s < params.strandCount; s++) {
            // Pick random triangle weighted by area
            float r = dist01(rng) * totalArea;
            float accumulated = 0;
            int triIndex = 0;
            
            for (size_t i = 0; i < triangleAreas.size(); i++) {
                accumulated += triangleAreas[i];
                if (accumulated >= r) {
                    triIndex = static_cast<int>(i);
                    break;
                }
            }
            
            // Random point on triangle
            float u = dist01(rng);
            float v = dist01(rng);
            if (u + v > 1.0f) {
                u = 1.0f - u;
                v = 1.0f - v;
            }
            
            int i0 = scalpMesh.indices[triIndex * 3];
            int i1 = scalpMesh.indices[triIndex * 3 + 1];
            int i2 = scalpMesh.indices[triIndex * 3 + 2];
            
            const Vertex& v0 = scalpMesh.vertices[i0];
            const Vertex& v1 = scalpMesh.vertices[i1];
            const Vertex& v2 = scalpMesh.vertices[i2];
            
            Vec3 rootPos = v0.position * (1 - u - v) + v1.position * u + v2.position * v;
            Vec3 normal = (v0.normal * (1 - u - v) + v1.normal * u + v2.normal * v).normalized();
            
            // Generate strand
            HairStrand strand;
            strand.strandIndex = s;
            strand.groupIndex = determineGroup(rootPos, normal);
            
            float length = params.baseLength + distNeg(rng) * params.lengthVariation;
            float curlPhase = dist01(rng) * 3.14159f * 2.0f * params.curlPhaseVariation;
            
            for (int p = 0; p < params.controlPointsPerStrand; p++) {
                float t = static_cast<float>(p) / (params.controlPointsPerStrand - 1);
                
                HairControlPoint cp;
                
                // Base position along normal
                Vec3 basePos = rootPos + normal * length * t;
                
                // Apply gravity (more at tip)
                float gravityFactor = t * t * (1.0f - params.stiffness * (1.0f - t));
                basePos.y -= params.gravity * gravityFactor;
                
                // Apply curl
                float curlAngle = t * params.curlFrequency * 3.14159f * 2.0f + curlPhase;
                Vec3 perpendicular = normal.cross(Vec3(0, 1, 0)).normalized();
                if (perpendicular.length() < 0.01f) {
                    perpendicular = Vec3(1, 0, 0);
                }
                Vec3 perpendicular2 = normal.cross(perpendicular).normalized();
                
                float curlStrength = params.curlAmplitude * t;
                basePos = basePos + perpendicular * std::cos(curlAngle) * curlStrength;
                basePos = basePos + perpendicular2 * std::sin(curlAngle) * curlStrength;
                
                // Apply frizz
                Vec3 frizz(distNeg(rng), distNeg(rng), distNeg(rng));
                frizz = frizz * params.frizzStrength * t;
                basePos = basePos + frizz;
                
                cp.position = basePos;
                
                // Thickness taper
                cp.radius = params.rootThickness * (1.0f - t) + params.tipThickness * t;
                
                strand.controlPoints.push_back(cp);
            }
            
            strand.tessellate();
            strands.push_back(strand);
        }
        
        // Apply clumping
        applyClumping(strands, params);
        
        return strands;
    }
    
    // Generate simple card-based hair
    static Mesh generateHairCards(const std::vector<HairStrand>& strands,
                                   float cardWidth = 0.01f,
                                   int strandsPerCard = 50) {
        Mesh mesh;
        
        // Group strands into cards
        int numCards = (strands.size() + strandsPerCard - 1) / strandsPerCard;
        
        for (int card = 0; card < numCards; card++) {
            int startStrand = card * strandsPerCard;
            int endStrand = std::min(startStrand + strandsPerCard, (int)strands.size());
            
            // Find average strand for card center
            Vec3 avgRoot(0, 0, 0);
            Vec3 avgTip(0, 0, 0);
            int count = 0;
            
            for (int s = startStrand; s < endStrand; s++) {
                if (strands[s].controlPoints.size() >= 2) {
                    avgRoot = avgRoot + strands[s].controlPoints.front().position;
                    avgTip = avgTip + strands[s].controlPoints.back().position;
                    count++;
                }
            }
            
            if (count == 0) continue;
            
            avgRoot = avgRoot * (1.0f / count);
            avgTip = avgTip * (1.0f / count);
            
            // Create card quad
            Vec3 direction = (avgTip - avgRoot).normalized();
            Vec3 right = direction.cross(Vec3(0, 1, 0)).normalized();
            if (right.length() < 0.01f) {
                right = Vec3(1, 0, 0);
            }
            
            Vec3 halfWidth = right * cardWidth * 0.5f;
            
            int baseIdx = mesh.vertices.size();
            
            // Bottom vertices
            Vertex v0, v1, v2, v3;
            v0.position = avgRoot - halfWidth;
            v1.position = avgRoot + halfWidth;
            v2.position = avgTip + halfWidth;
            v3.position = avgTip - halfWidth;
            
            Vec3 cardNormal = right.cross(direction).normalized();
            v0.normal = v1.normal = v2.normal = v3.normal = cardNormal;
            
            v0.texCoord = Vec2(0, 0);
            v1.texCoord = Vec2(1, 0);
            v2.texCoord = Vec2(1, 1);
            v3.texCoord = Vec2(0, 1);
            
            mesh.vertices.push_back(v0);
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.vertices.push_back(v3);
            
            mesh.indices.push_back(baseIdx);
            mesh.indices.push_back(baseIdx + 1);
            mesh.indices.push_back(baseIdx + 2);
            mesh.indices.push_back(baseIdx);
            mesh.indices.push_back(baseIdx + 2);
            mesh.indices.push_back(baseIdx + 3);
        }
        
        return mesh;
    }
    
    // Generate tube geometry for each strand
    static Mesh generateHairTubes(const std::vector<HairStrand>& strands,
                                   int radialSegments = 4) {
        Mesh mesh;
        
        for (const auto& strand : strands) {
            if (strand.tessellatedPositions.size() < 2) continue;
            
            int baseIdx = mesh.vertices.size();
            int numPoints = strand.tessellatedPositions.size();
            
            for (int p = 0; p < numPoints; p++) {
                const Vec3& pos = strand.tessellatedPositions[p];
                const Vec3& tangent = strand.tessellatedTangents[p];
                float radius = strand.tessellatedRadii[p];
                
                // Calculate basis vectors
                Vec3 up(0, 1, 0);
                Vec3 right = tangent.cross(up).normalized();
                if (right.length() < 0.01f) {
                    right = Vec3(1, 0, 0);
                }
                Vec3 bitangent = tangent.cross(right).normalized();
                
                float t = static_cast<float>(p) / (numPoints - 1);
                
                // Create ring of vertices
                for (int r = 0; r < radialSegments; r++) {
                    float angle = static_cast<float>(r) / radialSegments * 3.14159f * 2.0f;
                    
                    Vec3 offset = right * std::cos(angle) * radius +
                                  bitangent * std::sin(angle) * radius;
                    
                    Vertex v;
                    v.position = pos + offset;
                    v.normal = offset.normalized();
                    v.texCoord = Vec2(static_cast<float>(r) / radialSegments, t);
                    
                    mesh.vertices.push_back(v);
                }
            }
            
            // Create indices
            for (int p = 0; p < numPoints - 1; p++) {
                for (int r = 0; r < radialSegments; r++) {
                    int current = baseIdx + p * radialSegments + r;
                    int next = baseIdx + p * radialSegments + (r + 1) % radialSegments;
                    int currentNext = baseIdx + (p + 1) * radialSegments + r;
                    int nextNext = baseIdx + (p + 1) * radialSegments + (r + 1) % radialSegments;
                    
                    mesh.indices.push_back(current);
                    mesh.indices.push_back(next);
                    mesh.indices.push_back(currentNext);
                    
                    mesh.indices.push_back(next);
                    mesh.indices.push_back(nextNext);
                    mesh.indices.push_back(currentNext);
                }
            }
        }
        
        return mesh;
    }
    
private:
    static int determineGroup(const Vec3& pos, const Vec3& normal) {
        // Simple group assignment based on position
        if (normal.z > 0.3f) return 0;  // Front/bangs
        if (pos.x < -0.05f) return 1;   // Left side
        if (pos.x > 0.05f) return 2;    // Right side
        return 3;                        // Back
    }
    
    static void applyClumping(std::vector<HairStrand>& strands,
                              const HairGenerationParams& params) {
        if (params.clumpStrength <= 0) return;
        
        // Find clump centers (every N strands)
        std::vector<int> clumpCenters;
        for (size_t i = 0; i < strands.size(); i += params.clumpsPerGroup) {
            clumpCenters.push_back(static_cast<int>(i));
        }
        
        // Apply clumping
        for (auto& strand : strands) {
            if (strand.controlPoints.empty()) continue;
            
            // Find nearest clump center
            int nearestCenter = 0;
            float minDist = std::numeric_limits<float>::max();
            
            for (int center : clumpCenters) {
                if (strands[center].controlPoints.empty()) continue;
                float dist = (strand.controlPoints[0].position - 
                             strands[center].controlPoints[0].position).length();
                if (dist < minDist) {
                    minDist = dist;
                    nearestCenter = center;
                }
            }
            
            // Blend towards clump center
            const auto& centerStrand = strands[nearestCenter];
            for (size_t p = 0; p < strand.controlPoints.size(); p++) {
                if (p >= centerStrand.controlPoints.size()) break;
                
                float t = static_cast<float>(p) / (strand.controlPoints.size() - 1);
                float clumpFactor = params.clumpStrength * t;  // More clumping at tips
                
                strand.controlPoints[p].position = 
                    strand.controlPoints[p].position * (1.0f - clumpFactor) +
                    centerStrand.controlPoints[p].position * clumpFactor;
            }
            
            strand.tessellate();
        }
    }
};

// ============================================================================
// Hair Simulation - Simple dynamics
// ============================================================================

struct HairSimParams {
    float gravity = 9.8f;
    float damping = 0.95f;
    float stiffness = 100.0f;
    float windStrength = 0.0f;
    Vec3 windDirection{1, 0, 0};
};

class HairSimulation {
public:
    void initialize(const std::vector<HairStrand>& strands) {
        velocities_.clear();
        velocities_.resize(strands.size());
        
        for (size_t i = 0; i < strands.size(); i++) {
            velocities_[i].resize(strands[i].controlPoints.size(), Vec3(0, 0, 0));
        }
    }
    
    void simulate(std::vector<HairStrand>& strands, float dt, const HairSimParams& params) {
        for (size_t s = 0; s < strands.size(); s++) {
            auto& strand = strands[s];
            auto& vels = velocities_[s];
            
            // First point is fixed at root
            for (size_t p = 1; p < strand.controlPoints.size(); p++) {
                Vec3& pos = strand.controlPoints[p].position;
                Vec3& vel = vels[p];
                
                // Gravity
                Vec3 force(0, -params.gravity * 0.001f, 0);
                
                // Wind
                force = force + params.windDirection * params.windStrength;
                
                // Spring to parent
                Vec3 toParent = strand.controlPoints[p-1].position - pos;
                float restLength = 0.01f;  // Approximate segment length
                float stretch = toParent.length() - restLength;
                force = force + toParent.normalized() * stretch * params.stiffness;
                
                // Integrate
                vel = vel + force * dt;
                vel = vel * params.damping;
                pos = pos + vel * dt;
            }
            
            strand.tessellate();
        }
    }
    
private:
    std::vector<std::vector<Vec3>> velocities_;
};

// ============================================================================
// Hair Texture Generator - Alpha texture for hair cards
// ============================================================================

class HairTextureGenerator {
public:
    // Generate alpha texture for hair cards
    static TextureData generateHairAlpha(int width = 256, int height = 512,
                                          int strandCount = 50, float strandWidth = 3.0f) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4, 0);
        
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(width));
        std::uniform_real_distribution<float> distOffset(-2.0f, 2.0f);
        
        // Generate strand silhouettes
        for (int s = 0; s < strandCount; s++) {
            float startX = distX(rng);
            
            for (int y = 0; y < height; y++) {
                float t = static_cast<float>(y) / height;
                
                // Strand curve
                float x = startX + std::sin(t * 3.14159f * 2.0f) * 5.0f;
                x += distOffset(rng);
                
                // Width taper
                float w = strandWidth * (1.0f - t * 0.7f);
                
                // Draw strand
                for (int dx = -static_cast<int>(w); dx <= static_cast<int>(w); dx++) {
                    int px = static_cast<int>(x) + dx;
                    if (px < 0 || px >= width) continue;
                    
                    float dist = std::abs(static_cast<float>(dx)) / w;
                    float alpha = 1.0f - dist * dist;
                    alpha *= 1.0f - t * 0.3f;  // Fade at tip
                    
                    int idx = (y * width + px) * 4;
                    uint8_t existing = tex.pixels[idx + 3];
                    tex.pixels[idx + 3] = static_cast<uint8_t>(std::min(255.0f, 
                        existing + alpha * 150.0f));
                }
            }
        }
        
        // Add some noise for realism
        for (int i = 0; i < width * height; i++) {
            if (tex.pixels[i * 4 + 3] > 0) {
                float noise = (rng() % 20 - 10) / 255.0f;
                tex.pixels[i * 4 + 3] = static_cast<uint8_t>(
                    std::clamp(tex.pixels[i * 4 + 3] + noise * 255.0f, 0.0f, 255.0f));
            }
        }
        
        return tex;
    }
    
    // Generate flow/direction map
    static TextureData generateHairFlow(int width = 256, int height = 512) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                // Default downward flow with slight variation
                float angle = 3.14159f / 2.0f + std::sin(u * 3.14159f * 4.0f) * 0.2f;
                
                Vec3 flowDir(std::cos(angle), std::sin(angle), 0);
                flowDir = flowDir.normalized();
                
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>((flowDir.x * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>((flowDir.y * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 2] = 128;  // Z = 0 normalized
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate depth/AO texture for hair cards
    static TextureData generateHairDepthAO(int width = 256, int height = 512) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                // Depth (center is higher)
                float depth = 1.0f - std::abs(u - 0.5f) * 2.0f;
                depth = depth * depth;
                
                // AO (darker at root, edges)
                float ao = 0.5f + 0.5f * v;
                ao *= 0.7f + 0.3f * (1.0f - std::abs(u - 0.5f) * 2.0f);
                
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(depth * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(ao * 255);
                tex.pixels[idx + 2] = 128;
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
};

// ============================================================================
// Hair LOD - Level of detail management
// ============================================================================

struct HairLODLevel {
    int strandCount;
    int controlPointsPerStrand;
    int tubeSegments;
    bool useCards;
};

class HairLODManager {
public:
    static HairLODLevel getLODLevel(float distance, float maxDistance = 20.0f) {
        float t = std::clamp(distance / maxDistance, 0.0f, 1.0f);
        
        if (t < 0.1f) {
            // Ultra close - full detail
            return {50000, 12, 6, false};
        } else if (t < 0.3f) {
            // Close - high detail
            return {30000, 8, 4, false};
        } else if (t < 0.5f) {
            // Medium - medium detail
            return {15000, 6, 3, false};
        } else if (t < 0.7f) {
            // Far - low detail with cards
            return {5000, 4, 0, true};
        } else {
            // Very far - cards only
            return {1000, 3, 0, true};
        }
    }
};

}  // namespace luma
