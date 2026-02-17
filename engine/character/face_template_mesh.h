// Face Template Mesh - MetaHuman-style BlendShape generation for standard head mesh
// No longer procedurally generates mesh - instead loads standard topology head and generates BlendShape targets
#pragma once

#include "engine/renderer/mesh.h"
#include "engine/character/blend_shape.h"
#include "engine/character/head_mesh_loader.h"
#include "engine/character/facial_rig.h"
#include "engine/foundation/math_types.h"
#include <vector>
#include <array>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace luma {

// ============================================================================
// Face Template Mesh - BlendShape Generator for Standard Head Mesh
// ============================================================================

class FaceTemplateMesh {
public:
    // Anatomical landmark Y positions (normalized 0-1 of head height)
    static constexpr float Y_CHIN       = 0.00f;
    static constexpr float Y_LOWER_LIP  = 0.08f;
    static constexpr float Y_MOUTH      = 0.12f;
    static constexpr float Y_UPPER_LIP  = 0.16f;
    static constexpr float Y_PHILTRUM   = 0.20f;
    static constexpr float Y_NOSE_BASE  = 0.26f;
    static constexpr float Y_NOSE_TIP   = 0.30f;
    static constexpr float Y_NOSE_BRIDGE = 0.36f;
    static constexpr float Y_EYE_BOT   = 0.42f;
    static constexpr float Y_EYE_CENTER = 0.46f;
    static constexpr float Y_EYE_TOP   = 0.50f;
    static constexpr float Y_BROW      = 0.55f;
    static constexpr float Y_FOREHEAD  = 0.65f;
    static constexpr float Y_CROWN     = 0.85f;
    static constexpr float Y_TOP       = 1.00f;

    struct GeneratedMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        BlendShapeMesh blendShapes;
        
        // Neck boundary for body stitching
        int neckBoundaryStart = 0;
        int neckBoundaryCount = 0;
    };

    // Generate mesh with BlendShapes from standard head mesh
    // If headMeshPath is empty or load fails, falls back to simple procedural head
    static GeneratedMesh generate(const std::string& headMeshPath, float headHeight = 0.23f) {
        GeneratedMesh result;
        
        HeadMeshLoader::HeadMesh loadedMesh;
        bool useLoadedMesh = false;
        
        if (!headMeshPath.empty()) {
            printf("[FaceTemplate] Loading standard head mesh: %s\n", headMeshPath.c_str());
            useLoadedMesh = HeadMeshLoader::load(headMeshPath, loadedMesh);
            if (useLoadedMesh) {
                printf("[FaceTemplate] Successfully loaded standard head mesh\n");
            } else {
                printf("[FaceTemplate] Failed to load head mesh, using fallback\n");
            }
        }
        
        if (useLoadedMesh) {
            // Use loaded standard topology head
            result.vertices = loadedMesh.vertices;
            result.indices = loadedMesh.indices;
            result.neckBoundaryStart = loadedMesh.neckBoundaryStart;
            result.neckBoundaryCount = loadedMesh.neckBoundaryCount;
            
            // Generate BlendShapes for the loaded mesh
            generateBlendShapesForMesh(result, loadedMesh, headHeight);
        } else {
            // Fallback: generate simple procedural head
            printf("[FaceTemplate] Using procedural fallback head\n");
            generateSimpleHead(result.vertices, result.indices, headHeight);
            recomputeNormals(result.vertices, result.indices);
            generateBlendShapesSimple(result, headHeight);
        }

        printf("[FaceTemplate] Generated mesh: %zu vertices, %zu triangles, %zu blend shapes\n",
               result.vertices.size(), result.indices.size() / 3,
               result.blendShapes.getChannelCount());

        return result;
    }
    
    // Legacy generate function for backward compatibility
    static GeneratedMesh generate(float headHeight = 0.23f) {
        return generate("", headHeight);
    }

private:
    // ========================================================================
    // BlendShape Generation for Standard Head Mesh
    // ========================================================================
    
    static void generateBlendShapesForMesh(GeneratedMesh& mesh, 
                                            const HeadMeshLoader::HeadMesh& headMesh,
                                            float H) {
        int numVerts = (int)mesh.vertices.size();
        
        // Helper: add a blend shape that applies a vertex function
        auto addBlendShape = [&](const std::string& name,
                                  std::function<Vec3(const Vertex& v, const HeadMeshLoader::RegionWeights& w)> fn) {
            BlendShapeTarget target(name);
            for (int i = 0; i < numVerts; i++) {
                HeadMeshLoader::RegionWeights weights = 
                    HeadMeshLoader::getRegionWeights(mesh.vertices[i], headMesh);
                Vec3 delta = fn(mesh.vertices[i], weights);
                if (delta.length() > 0.00001f) {
                    BlendShapeDelta d;
                    d.vertexIndex = (uint32_t)i;
                    d.positionDelta = delta;
                    d.normalDelta = Vec3(0, 0, 0);
                    target.addDelta(d);
                }
            }
            int targetIdx = mesh.blendShapes.addTarget(target);
            int channelIdx = mesh.blendShapes.createChannel(name, targetIdx);
            if (auto* ch = mesh.blendShapes.getChannel(channelIdx)) {
                // Identity sculpt channels should be bidirectional around neutral.
                ch->minWeight = -1.0f;
                ch->maxWeight = 1.0f;
                ch->defaultWeight = 0.0f;
                ch->weight = 0.0f;
            }
        };
        
        // Normalized position helpers
        auto normalizedY = [&](const Vertex& v) -> float {
            float height = headMesh.maxBounds.y - headMesh.minBounds.y;
            return (v.position[1] - headMesh.minBounds.y) / height;
        };
        
        auto normalizedX = [&](const Vertex& v) -> float {
            float width = headMesh.maxBounds.x - headMesh.minBounds.x;
            return (v.position[0] - headMesh.center.x) / (width * 0.5f);
        };
        
        auto normalizedZ = [&](const Vertex& v) -> float {
            float depth = headMesh.maxBounds.z - headMesh.minBounds.z;
            return (v.position[2] - headMesh.minBounds.z) / depth;
        };
        
        // Smooth weight function
        auto smoothWeight = [](float value, float center, float radius) -> float {
            float d = std::abs(value - center) / radius;
            d = std::clamp(d, 0.0f, 1.0f);
            return (1.0f - d) * (1.0f - d);
        };
        
        // === OVERALL FACE SHAPE ===
        
        addBlendShape("faceWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float weight = smoothWeight(nY, 0.4f, 0.5f);
            float frontWeight = 1.0f - w.skull;
            return Vec3(v.position[0] * 0.15f * weight * frontWeight, 0, 0);
        });
        
        addBlendShape("faceLength", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            if (nY > 0.6f) return Vec3(0, 0, 0);
            float weight = (0.6f - nY) / 0.6f;
            float frontWeight = 1.0f - w.skull;
            return Vec3(0, -H * 0.03f * weight * frontWeight, 0);
        });
        
        addBlendShape("faceRoundness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float weight = smoothWeight(nY, 0.35f, 0.3f);
            float frontWeight = 1.0f - w.skull;
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(std::abs(v.position[0]) * 0.12f * weight * frontWeight * sign, 0, 0);
        });
        
        // === FOREHEAD ===
        
        addBlendShape("foreheadHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, H * 0.02f * w.forehead, 0);
        });
        
        addBlendShape("foreheadWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(v.position[0] * 0.1f * w.forehead, 0, 0);
        });
        
        addBlendShape("foreheadSlope", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, -H * 0.02f * w.forehead);
        });
        
        // === EYES ===
        
        addBlendShape("eyeSize", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, -H * 0.008f * w.eyes);
        });
        
        addBlendShape("eyeSpacing", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w.eyes, 0, 0);
        });
        
        addBlendShape("eyeHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, H * 0.01f * w.eyes, 0);
        });
        
        addBlendShape("eyeDepth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, -H * 0.012f * w.eyes);
        });
        
        addBlendShape("eyeAngle", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float outerFactor = std::clamp((nX - 0.3f) / 0.3f, 0.0f, 1.0f);
            return Vec3(0, H * 0.005f * sign * w.eyes * outerFactor, 0);
        });
        
        addBlendShape("eyeWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float outerFactor = std::clamp((nX - 0.2f) / 0.3f, 0.0f, 1.0f);
            return Vec3(H * 0.004f * sign * w.eyes * outerFactor, 0, 0);
        });
        
        // === EYEBROWS ===
        
        addBlendShape("browHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float browWeight = smoothWeight(nY, Y_BROW, 0.05f);
            return Vec3(0, H * 0.012f * browWeight * (1.0f - w.skull), 0);
        });
        
        addBlendShape("browAngle", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float browWeight = smoothWeight(nY, Y_BROW, 0.04f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            return Vec3(0, H * 0.006f * sign * browWeight * nX * (1.0f - w.skull), 0);
        });
        
        addBlendShape("browThickness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float browWeight = smoothWeight(nY, Y_BROW, 0.03f);
            return Vec3(0, 0, H * 0.006f * browWeight * (1.0f - w.skull));
        });
        
        // === NOSE ===
        
        addBlendShape("noseLength", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, -H * 0.01f * w.nose, 0);
        });
        
        addBlendShape("noseWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w.nose, 0, 0);
        });
        
        addBlendShape("noseHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, H * 0.02f * w.nose);
        });
        
        addBlendShape("noseBridge", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float bridgeWeight = smoothWeight(nY, Y_NOSE_BRIDGE, 0.05f);
            return Vec3(0, 0, H * 0.01f * bridgeWeight * w.nose);
        });
        
        addBlendShape("noseTip", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float tipWeight = smoothWeight(nY, Y_NOSE_TIP, 0.03f);
            return Vec3(0, 0, H * 0.015f * tipWeight * w.nose);
        });
        
        addBlendShape("nostrilWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float baseWeight = smoothWeight(nY, Y_NOSE_BASE, 0.03f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float nostrilRegion = (nX > 0.05f && nX < 0.25f) ? 1.0f : 0.0f;
            return Vec3(H * 0.006f * sign * baseWeight * nostrilRegion, 0, 0);
        });
        
        // === MOUTH ===
        
        addBlendShape("mouthWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float mouthRegion = (nX > 0.1f) ? 1.0f : 0.0f;
            return Vec3(H * 0.008f * sign * w.mouth * mouthRegion, 0, 0);
        });
        
        addBlendShape("upperLipThickness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float upperWeight = smoothWeight(nY, Y_UPPER_LIP, 0.025f);
            return Vec3(0, 0, H * 0.008f * upperWeight * w.mouth);
        });
        
        addBlendShape("lowerLipThickness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float lowerWeight = smoothWeight(nY, Y_LOWER_LIP, 0.025f);
            return Vec3(0, 0, H * 0.008f * lowerWeight * w.mouth);
        });
        
        addBlendShape("lipProtrusion", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, H * 0.012f * w.mouth);
        });
        
        addBlendShape("mouthCorners", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nX = std::abs(normalizedX(v));
            float cornerRegion = std::clamp((nX - 0.2f) / 0.2f, 0.0f, 1.0f);
            return Vec3(0, H * 0.004f * w.mouth * cornerRegion, 0);
        });
        
        // === CHIN ===
        
        addBlendShape("chinLength", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, -H * 0.015f * w.chin, 0);
        });
        
        addBlendShape("chinWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w.chin, 0, 0);
        });
        
        addBlendShape("chinProtrusion", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, 0, H * 0.015f * w.chin);
        });
        
        // === JAW ===
        
        addBlendShape("jawWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.01f * sign * w.jaw, 0, 0);
        });
        
        addBlendShape("jawAngle", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            return Vec3(H * 0.006f * sign * w.jaw * nX, -H * 0.003f * w.jaw * nX, 0);
        });
        
        // === CHEEKS ===
        
        addBlendShape("cheekboneProminence", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w.cheeks, 0, H * 0.005f * w.cheeks);
        });
        
        addBlendShape("cheekFullness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.006f * sign * w.cheeks, 0, H * 0.004f * w.cheeks);
        });
        
        // === EARS ===
        
        addBlendShape("earSize", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.01f * sign * w.ears, 0, 0);
        });
        
        addBlendShape("earAngle", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w.ears, 0, 0);
        });
        
        addBlendShape("earLobe", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float lobeWeight = smoothWeight(nY, 0.30f, 0.05f);
            return Vec3(0, -H * 0.004f * lobeWeight * w.ears, 0);
        });
        
        addBlendShape("earPointiness", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float topWeight = smoothWeight(nY, 0.48f, 0.04f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.004f * sign * topWeight * w.ears,
                        H * 0.006f * topWeight * w.ears, 0);
        });
        
        // === ADDITIONAL DETAIL SHAPES ===
        
        addBlendShape("upperEyelid", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float upperWeight = smoothWeight(nY, Y_EYE_TOP - 0.01f, 0.03f);
            return Vec3(0, H * 0.004f * upperWeight * w.eyes, 0);
        });
        
        addBlendShape("lowerEyelid", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float lowerWeight = smoothWeight(nY, Y_EYE_BOT + 0.01f, 0.03f);
            return Vec3(0, -H * 0.003f * lowerWeight * w.eyes, 0);
        });
        
        addBlendShape("browLength", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float browWeight = smoothWeight(nY, Y_BROW, 0.04f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float outerFactor = std::clamp((nX - 0.3f) / 0.2f, 0.0f, 1.0f);
            return Vec3(H * 0.005f * sign * browWeight * outerFactor * (1.0f - w.skull), 0, 0);
        });
        
        addBlendShape("browSpacing", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float browWeight = smoothWeight(nY, Y_BROW, 0.04f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float innerFactor = std::max(0.0f, 1.0f - nX / 0.2f);
            return Vec3(H * 0.004f * sign * browWeight * innerFactor * (1.0f - w.skull), 0, 0);
        });
        
        addBlendShape("noseBridgeCurve", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float midWeight = smoothWeight(nY, (Y_NOSE_TIP + Y_NOSE_BRIDGE) * 0.5f, 0.06f);
            return Vec3(0, 0, H * 0.008f * midWeight * w.nose);
        });
        
        addBlendShape("noseTipAngle", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float tipWeight = smoothWeight(nY, Y_NOSE_TIP, 0.03f);
            return Vec3(0, H * 0.006f * tipWeight * w.nose, H * 0.004f * tipWeight * w.nose);
        });
        
        addBlendShape("nostrilFlare", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float baseWeight = smoothWeight(nY, Y_NOSE_BASE, 0.025f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float nostrilSide = (nX > 0.05f && nX < 0.2f) ? 1.0f : 0.0f;
            return Vec3(H * 0.005f * sign * baseWeight * nostrilSide, 0, -H * 0.002f * baseWeight * nostrilSide);
        });
        
        addBlendShape("mouthHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, H * 0.008f * w.mouth, 0);
        });
        
        addBlendShape("philtrum", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float philtrumWeight = smoothWeight(nY, Y_PHILTRUM, 0.03f);
            float nX = std::abs(normalizedX(v));
            float centerFactor = std::max(0.0f, 1.0f - nX / 0.1f);
            return Vec3(0, 0, -H * 0.006f * philtrumWeight * centerFactor);
        });
        
        addBlendShape("lipCurve", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float upperWeight = smoothWeight(nY, Y_UPPER_LIP, 0.02f);
            float nX = std::abs(normalizedX(v));
            float cupid = std::cos(nX / 0.1f * 3.14159f);
            cupid = std::max(0.0f, cupid);
            return Vec3(0, H * 0.003f * upperWeight * cupid * w.mouth, 0);
        });
        
        addBlendShape("chinShape", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            return Vec3(H * 0.006f * sign * w.chin * nX, 0, -H * 0.003f * w.chin * (1.0f - nX));
        });
        
        addBlendShape("chinCleft", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float nY = normalizedY(v);
            float cleftWeight = smoothWeight(nY, 0.03f, 0.03f);
            float nX = std::abs(normalizedX(v));
            float centerFactor = std::max(0.0f, 1.0f - nX / 0.08f);
            centerFactor *= centerFactor;
            return Vec3(0, 0, -H * 0.008f * cleftWeight * centerFactor * w.chin);
        });
        
        addBlendShape("jawLine", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float nX = std::abs(normalizedX(v));
            float jawEdge = nX * nX;
            return Vec3(H * 0.004f * sign * w.jaw * jawEdge, 0, H * 0.003f * w.jaw * jawEdge);
        });
        
        addBlendShape("cheekboneHeight", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            return Vec3(0, H * 0.005f * w.cheeks, 0);
        });
        
        addBlendShape("cheekboneWidth", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.006f * sign * w.cheeks, 0, 0);
        });
        
        addBlendShape("cheekFat", [&](const Vertex& v, const HeadMeshLoader::RegionWeights& w) -> Vec3 {
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.005f * sign * w.cheeks, -H * 0.002f * w.cheeks, H * 0.004f * w.cheeks);
        });

        createARKitCompatibilityChannels(mesh);
        
        printf("[FaceTemplate] Generated %zu blend shape channels for standard head mesh\n",
               mesh.blendShapes.getChannelCount());
    }
    
    // ========================================================================
    // Fallback: Simple Procedural Head (when no standard mesh available)
    // ========================================================================
    
    static void generateSimpleHead(std::vector<Vertex>& verts,
                                    std::vector<uint32_t>& indices,
                                    float H) {
        float headWidth = H * 0.7f;
        float headDepth = H * 0.85f;

        int numRows = 32;
        int numCols = 48;

        const float PI = 3.14159265f;

        for (int row = 0; row <= numRows; row++) {
            float t = (float)row / numRows;
            float y = t * H;

            float radius;
            if (t < 0.1f) {
                radius = 0.5f + t * 3.0f;
            } else if (t < 0.25f) {
                radius = 0.8f + (t - 0.1f) * 1.33f;
            } else if (t < 0.6f) {
                radius = 1.0f;
            } else if (t < 0.85f) {
                radius = 1.0f - (t - 0.6f) * 0.4f;
            } else {
                radius = 0.9f - (t - 0.85f) * 3.0f;
            }
            radius = std::max(0.15f, radius);

            for (int col = 0; col <= numCols; col++) {
                float u = (float)col / numCols;
                float angle = u * 2.0f * PI;

                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                float x = radius * headWidth * 0.5f * cosA;
                float z = radius * headDepth * 0.5f * (-sinA);

                float frontness = sinA;

                if (frontness > 0.3f && t > 0.15f && t < 0.65f) {
                    float faceRegion = std::sin((t - 0.15f) / 0.5f * PI);
                    float flatten = (frontness - 0.3f) / 0.7f;
                    flatten = flatten * flatten * faceRegion * 0.2f;
                    z = z * (1.0f - flatten);
                }

                if (frontness > 0.85f && t > 0.3f && t < 0.45f) {
                    float noseProfile = 1.0f - std::abs(t - 0.38f) / 0.08f;
                    noseProfile = std::max(0.0f, noseProfile);
                    float noseCenterX = 1.0f - std::abs(cosA) * 2.0f;
                    noseCenterX = std::max(0.0f, noseCenterX);
                    z -= H * 0.025f * noseProfile * noseCenterX;
                }

                Vertex vert = {};
                vert.position[0] = x;
                vert.position[1] = y;
                vert.position[2] = z;

                vert.uv[0] = u;
                vert.uv[1] = t;

                vert.color[0] = 1.0f;
                vert.color[1] = 1.0f;
                vert.color[2] = 1.0f;

                verts.push_back(vert);
            }
        }

        for (int row = 0; row < numRows; row++) {
            for (int col = 0; col < numCols; col++) {
                int curr = row * (numCols + 1) + col;
                int next = curr + numCols + 1;

                indices.push_back(curr);
                indices.push_back(curr + 1);
                indices.push_back(next);

                indices.push_back(next);
                indices.push_back(curr + 1);
                indices.push_back(next + 1);
            }
        }
    }
    
    // ========================================================================
    // BlendShapes for Simple Procedural Head
    // ========================================================================
    
    static void generateBlendShapesSimple(GeneratedMesh& mesh, float H) {
        int numVerts = (int)mesh.vertices.size();
        
        auto addBlendShape = [&](const std::string& name,
                                  std::function<Vec3(const Vertex& v)> fn) {
            BlendShapeTarget target(name);
            for (int i = 0; i < numVerts; i++) {
                Vec3 delta = fn(mesh.vertices[i]);
                if (delta.length() > 0.00001f) {
                    BlendShapeDelta d;
                    d.vertexIndex = (uint32_t)i;
                    d.positionDelta = delta;
                    d.normalDelta = Vec3(0, 0, 0);
                    target.addDelta(d);
                }
            }
            int targetIdx = mesh.blendShapes.addTarget(target);
            int channelIdx = mesh.blendShapes.createChannel(name, targetIdx);
            if (auto* ch = mesh.blendShapes.getChannel(channelIdx)) {
                ch->minWeight = -1.0f;
                ch->maxWeight = 1.0f;
                ch->defaultWeight = 0.0f;
                ch->weight = 0.0f;
            }
        };
        
        auto headY = [&](const Vertex& v) -> float {
            return v.position[1] / H;
        };
        
        auto lateralDist = [&](const Vertex& v) -> float {
            return std::abs(v.position[0]);
        };
        
        auto yWeightSmooth = [](float nY, float center, float radius) -> float {
            float d = std::abs(nY - center) / radius;
            d = std::clamp(d, 0.0f, 1.0f);
            return (1.0f - d) * (1.0f - d);
        };
        
        auto frontWeight = [&](const Vertex& v) -> float {
            float z = v.position[2];
            float scale = 1.0f / (0.3f * H + 0.001f);
            return std::clamp(z * scale * 0.8f + 0.5f, 0.0f, 1.0f);
        };
        
        // Basic face shape controls
        addBlendShape("faceWidth", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, 0.4f, 0.5f);
            return Vec3(v.position[0] * 0.15f * w, 0, 0);
        });
        
        addBlendShape("faceLength", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            if (nY > 0.6f) return Vec3(0, 0, 0);
            float w = (0.6f - nY) / 0.6f;
            return Vec3(0, -H * 0.03f * w, 0);
        });
        
        addBlendShape("foreheadHeight", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, Y_FOREHEAD, 0.15f);
            return Vec3(0, H * 0.02f * w, 0);
        });
        
        addBlendShape("eyeSize", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, Y_EYE_CENTER, 0.06f);
            float lat = lateralDist(v);
            float eyeRegion = (lat > 0.09f * H && lat < 0.27f * H) ? 1.0f : 0.0f;
            return Vec3(0, 0, -H * 0.008f * w * eyeRegion);
        });
        
        addBlendShape("noseLength", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            if (nY < Y_NOSE_BASE - 0.02f || nY > Y_NOSE_BRIDGE + 0.02f) return Vec3(0, 0, 0);
            float w = yWeightSmooth(nY, Y_NOSE_TIP, 0.08f);
            float center = std::max(0.0f, 1.0f - lateralDist(v) / (0.09f * H));
            return Vec3(0, -H * 0.01f * w * center, 0);
        });
        
        addBlendShape("noseWidth", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, Y_NOSE_BASE, 0.06f);
            float center = std::max(0.0f, 1.0f - lateralDist(v) / (0.13f * H));
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w * center, 0, 0);
        });
        
        addBlendShape("mouthWidth", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, Y_MOUTH, 0.05f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            float mouthRegion = (lateralDist(v) > 0.04f * H) ? 1.0f : 0.0f;
            return Vec3(H * 0.008f * sign * w * mouthRegion, 0, 0);
        });
        
        addBlendShape("chinLength", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            if (nY > Y_LOWER_LIP) return Vec3(0, 0, 0);
            float w = 1.0f - nY / Y_LOWER_LIP;
            return Vec3(0, -H * 0.015f * w, 0);
        });
        
        addBlendShape("jawWidth", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, Y_MOUTH, 0.12f);
            if (nY > Y_NOSE_BASE) w *= 0.3f;
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.01f * sign * w, 0, 0);
        });
        
        addBlendShape("cheekboneProminence", [&](const Vertex& v) -> Vec3 {
            float nY = headY(v);
            float w = yWeightSmooth(nY, 0.38f, 0.08f);
            float lat = lateralDist(v) / (0.35f * H);
            float cheekRegion = std::sin(std::clamp(lat, 0.0f, 1.0f) * 3.14159f);
            float sign = v.position[0] > 0 ? 1.0f : -1.0f;
            return Vec3(H * 0.008f * sign * w * cheekRegion, 0, H * 0.005f * w * cheekRegion);
        });

        createARKitCompatibilityChannels(mesh);
        
        printf("[FaceTemplate] Generated %zu blend shape channels (simple fallback)\n",
               mesh.blendShapes.getChannelCount());
    }

    // Build ARKit 52-channel compatibility by remapping semantic sculpt channels.
    // This keeps runtime and animation pipeline engine-agnostic while using our own topology.
    static void createARKitCompatibilityChannels(GeneratedMesh& mesh) {
        auto addARKitChannel = [&](const std::string& arkitName,
                                   const std::vector<std::pair<std::string, float>>& sourceChannels) {
            if (mesh.blendShapes.findChannelIndex(arkitName) >= 0) {
                return;
            }

            BlendShapeChannel ch(arkitName);
            ch.displayName = arkitName;
            ch.group = "arkit";
            ch.minWeight = 0.0f;
            ch.maxWeight = 1.0f;
            ch.defaultWeight = 0.0f;
            ch.weight = 0.0f;

            for (const auto& [srcName, scale] : sourceChannels) {
                const BlendShapeChannel* src = mesh.blendShapes.getChannel(srcName);
                if (!src) continue;
                for (size_t i = 0; i < src->targetIndices.size(); i++) {
                    ch.addTarget(src->targetIndices[i], src->targetWeights[i] * scale);
                }
            }

            if (!ch.targetIndices.empty()) {
                mesh.blendShapes.addChannel(ch);
            }
        };

        // Eye
        addARKitChannel(ARKitBlendShapes::eyeBlinkLeft, {{"upperEyelid", 0.8f}, {"lowerEyelid", 0.5f}});
        addARKitChannel(ARKitBlendShapes::eyeBlinkRight, {{"upperEyelid", 0.8f}, {"lowerEyelid", 0.5f}});
        addARKitChannel(ARKitBlendShapes::eyeSquintLeft, {{"eyeDepth", 0.5f}, {"upperEyelid", 0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeSquintRight, {{"eyeDepth", 0.5f}, {"upperEyelid", 0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeWideLeft, {{"eyeHeight", 0.5f}, {"upperEyelid", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeWideRight, {{"eyeHeight", 0.5f}, {"upperEyelid", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookDownLeft, {{"eyeHeight", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookDownRight, {{"eyeHeight", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookUpLeft, {{"eyeHeight", 0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookUpRight, {{"eyeHeight", 0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookInLeft, {{"eyeSpacing", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookInRight, {{"eyeSpacing", -0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookOutLeft, {{"eyeSpacing", 0.4f}});
        addARKitChannel(ARKitBlendShapes::eyeLookOutRight, {{"eyeSpacing", 0.4f}});

        // Jaw / mouth core
        addARKitChannel(ARKitBlendShapes::jawOpen, {{"mouthHeight", -0.9f}, {"chinLength", 0.4f}});
        addARKitChannel(ARKitBlendShapes::jawForward, {{"chinProtrusion", 0.7f}});
        addARKitChannel(ARKitBlendShapes::jawLeft, {{"mouthWidth", -0.25f}});
        addARKitChannel(ARKitBlendShapes::jawRight, {{"mouthWidth", 0.25f}});
        addARKitChannel(ARKitBlendShapes::mouthClose, {{"mouthHeight", 0.8f}});
        addARKitChannel(ARKitBlendShapes::mouthFunnel, {{"mouthWidth", -0.8f}, {"lipProtrusion", 0.4f}});
        addARKitChannel(ARKitBlendShapes::mouthPucker, {{"mouthWidth", -0.6f}, {"lipProtrusion", 0.7f}});
        addARKitChannel(ARKitBlendShapes::mouthLeft, {{"mouthWidth", -0.35f}});
        addARKitChannel(ARKitBlendShapes::mouthRight, {{"mouthWidth", 0.35f}});
        addARKitChannel(ARKitBlendShapes::mouthSmileLeft, {{"mouthCorners", 0.8f}, {"cheekFullness", 0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthSmileRight, {{"mouthCorners", 0.8f}, {"cheekFullness", 0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthFrownLeft, {{"mouthCorners", -0.8f}});
        addARKitChannel(ARKitBlendShapes::mouthFrownRight, {{"mouthCorners", -0.8f}});
        addARKitChannel(ARKitBlendShapes::mouthDimpleLeft, {{"cheekboneProminence", 0.3f}});
        addARKitChannel(ARKitBlendShapes::mouthDimpleRight, {{"cheekboneProminence", 0.3f}});
        addARKitChannel(ARKitBlendShapes::mouthStretchLeft, {{"mouthWidth", 0.6f}});
        addARKitChannel(ARKitBlendShapes::mouthStretchRight, {{"mouthWidth", 0.6f}});
        addARKitChannel(ARKitBlendShapes::mouthRollLower, {{"lowerLipThickness", -0.7f}});
        addARKitChannel(ARKitBlendShapes::mouthRollUpper, {{"upperLipThickness", -0.7f}});
        addARKitChannel(ARKitBlendShapes::mouthShrugLower, {{"lowerLipThickness", 0.7f}});
        addARKitChannel(ARKitBlendShapes::mouthShrugUpper, {{"upperLipThickness", 0.7f}});
        addARKitChannel(ARKitBlendShapes::mouthPressLeft, {{"mouthWidth", -0.3f}, {"lipProtrusion", -0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthPressRight, {{"mouthWidth", -0.3f}, {"lipProtrusion", -0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthLowerDownLeft, {{"mouthCorners", -0.6f}, {"chinLength", 0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthLowerDownRight, {{"mouthCorners", -0.6f}, {"chinLength", 0.2f}});
        addARKitChannel(ARKitBlendShapes::mouthUpperUpLeft, {{"upperLipThickness", 0.5f}, {"philtrum", 0.4f}});
        addARKitChannel(ARKitBlendShapes::mouthUpperUpRight, {{"upperLipThickness", 0.5f}, {"philtrum", 0.4f}});

        // Brow / cheek / nose
        addARKitChannel(ARKitBlendShapes::browDownLeft, {{"browHeight", -0.7f}, {"browAngle", -0.3f}});
        addARKitChannel(ARKitBlendShapes::browDownRight, {{"browHeight", -0.7f}, {"browAngle", 0.3f}});
        addARKitChannel(ARKitBlendShapes::browInnerUp, {{"browHeight", 0.6f}, {"browSpacing", -0.2f}});
        addARKitChannel(ARKitBlendShapes::browOuterUpLeft, {{"browAngle", 0.5f}, {"browHeight", 0.2f}});
        addARKitChannel(ARKitBlendShapes::browOuterUpRight, {{"browAngle", -0.5f}, {"browHeight", 0.2f}});
        addARKitChannel(ARKitBlendShapes::cheekPuff, {{"cheekFullness", 0.8f}});
        addARKitChannel(ARKitBlendShapes::cheekSquintLeft, {{"cheekboneProminence", 0.5f}, {"upperEyelid", 0.2f}});
        addARKitChannel(ARKitBlendShapes::cheekSquintRight, {{"cheekboneProminence", 0.5f}, {"upperEyelid", 0.2f}});
        addARKitChannel(ARKitBlendShapes::noseSneerLeft, {{"noseWidth", 0.4f}, {"nostrilFlare", 0.8f}});
        addARKitChannel(ARKitBlendShapes::noseSneerRight, {{"noseWidth", 0.4f}, {"nostrilFlare", 0.8f}});

        // TongueOut currently has no dedicated mesh target; keep compatibility channel.
        addARKitChannel(ARKitBlendShapes::tongueOut, {{"mouthHeight", -0.2f}});
    }
    
    // ========================================================================
    // Normal Recomputation
    // ========================================================================

    static void recomputeNormals(std::vector<Vertex>& verts,
                                  const std::vector<uint32_t>& indices) {
        for (auto& v : verts) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0.0f;
        }

        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];

            Vec3 p0(verts[i0].position[0], verts[i0].position[1], verts[i0].position[2]);
            Vec3 p1(verts[i1].position[0], verts[i1].position[1], verts[i1].position[2]);
            Vec3 p2(verts[i2].position[0], verts[i2].position[1], verts[i2].position[2]);

            Vec3 e1 = p1 - p0;
            Vec3 e2 = p2 - p0;
            Vec3 n = e1.cross(e2);

            for (uint32_t idx : {i0, i1, i2}) {
                verts[idx].normal[0] += n.x;
                verts[idx].normal[1] += n.y;
                verts[idx].normal[2] += n.z;
            }
        }

        for (auto& v : verts) {
            float len = std::sqrt(v.normal[0]*v.normal[0] +
                                  v.normal[1]*v.normal[1] +
                                  v.normal[2]*v.normal[2]);
            if (len > 0.0001f) {
                v.normal[0] /= len;
                v.normal[1] /= len;
                v.normal[2] /= len;
            }
        }
    }
};

} // namespace luma
