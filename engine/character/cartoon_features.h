// LUMA Cartoon Features System
// Special features for cartoon/mascot characters: eyes, ears, noses, accessories
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/character/body_part_system.h"
#include <string>
#include <vector>
#include <cmath>

namespace luma {

// ============================================================================
// Eye Styles
// ============================================================================

enum class CartoonEyeStyle {
    Circle,         // Simple circle (Hello Kitty)
    Oval,           // Oval shape (anime)
    Dot,            // Small dot
    Button,         // Button eyes (plush toy)
    Anime,          // Large anime eyes with highlights
    Disney,         // Classic Disney style
    Realistic,      // More realistic eye
    Pie,            // Pie-cut eyes (early Mickey)
    Sleepy,         // Half-closed
    Angry,          // Angry shape
    Custom
};

struct CartoonEyeParams {
    CartoonEyeStyle style = CartoonEyeStyle::Circle;
    
    // Size
    float width = 0.08f;
    float height = 0.08f;
    
    // Position on face (0-1 normalized)
    float heightOnFace = 0.5f;
    float spacing = 0.15f;  // Distance between eyes
    
    // Colors
    Vec3 scleraColor{1, 1, 1};      // White of eye
    Vec3 irisColor{0.2f, 0.15f, 0.1f};  // Iris
    Vec3 pupilColor{0, 0, 0};       // Pupil
    Vec3 highlightColor{1, 1, 1};   // Highlight/catchlight
    
    // Features
    float irisSize = 0.6f;          // Relative to eye size
    float pupilSize = 0.3f;         // Relative to iris
    bool hasHighlight = true;
    float highlightSize = 0.15f;
    Vec2 highlightOffset{0.2f, 0.2f};
    
    // Outline
    bool hasOutline = true;
    Vec3 outlineColor{0, 0, 0};
    float outlineThickness = 0.02f;
    
    // Eyelids (for expressions)
    bool hasEyelids = false;
    float eyelidClosure = 0.0f;     // 0 = open, 1 = closed
};

// ============================================================================
// Ear Styles
// ============================================================================

enum class CartoonEarStyle {
    None,
    HumanRound,
    HumanPointed,   // Elf ears
    MouseRound,     // Mickey Mouse
    CatPointed,     // Pointed cat ears (Hello Kitty)
    DogFloppy,      // Floppy dog ears
    BunnyLong,      // Long bunny ears
    BearRound,      // Round bear ears
    Custom
};

struct CartoonEarParams {
    CartoonEarStyle style = CartoonEarStyle::None;
    
    // Size
    float width = 0.1f;
    float height = 0.1f;
    float depth = 0.05f;
    
    // Position
    float heightOnHead = 0.8f;      // 0-1 from bottom to top
    float angle = 45.0f;            // Degrees from vertical
    float sideOffset = 0.3f;        // Distance from center
    
    // Colors
    Vec3 outerColor{0.3f, 0.3f, 0.3f};
    Vec3 innerColor{1.0f, 0.8f, 0.8f};  // Pink inner ear
    
    // Shape modifiers
    float tipPointiness = 0.0f;     // 0 = round, 1 = pointed
    float baseWidth = 1.0f;         // Relative width at base
    float curvature = 0.0f;         // Ear curl/droop
};

// ============================================================================
// Nose Styles
// ============================================================================

enum class CartoonNoseStyle {
    None,
    Dot,            // Simple dot
    Triangle,       // Triangle/wedge
    Button,         // Round button (Mickey)
    Animal,         // Animal nose (Hello Kitty's whisker pad area)
    Human,          // Simplified human nose
    Beak,           // Bird beak
    Snout,          // Animal snout
    Custom
};

struct CartoonNoseParams {
    CartoonNoseStyle style = CartoonNoseStyle::Dot;
    
    // Size
    float width = 0.05f;
    float height = 0.05f;
    float depth = 0.03f;
    
    // Position
    float heightOnFace = 0.4f;
    
    // Color
    Vec3 color{0, 0, 0};
    
    // Shape
    float roundness = 1.0f;  // 0 = angular, 1 = round
    bool shiny = true;       // Glossy nose
};

// ============================================================================
// Mouth Styles
// ============================================================================

enum class CartoonMouthStyle {
    None,           // No mouth (Hello Kitty!)
    Line,           // Simple line
    Smile,          // Curved smile
    Open,           // Open mouth
    Cat,            // Cat mouth (w shape)
    Beak,           // Bird beak
    Custom
};

struct CartoonMouthParams {
    CartoonMouthStyle style = CartoonMouthStyle::Smile;
    
    // Size
    float width = 0.08f;
    float height = 0.02f;
    
    // Position
    float heightOnFace = 0.25f;
    
    // Colors
    Vec3 lipColor{0.8f, 0.4f, 0.4f};
    Vec3 innerColor{0.3f, 0.1f, 0.1f};
    Vec3 teethColor{1, 1, 1};
    Vec3 tongueColor{0.9f, 0.5f, 0.5f};
    
    // Shape
    float smileAmount = 0.5f;       // -1 = frown, 0 = neutral, 1 = smile
    float openAmount = 0.0f;        // 0 = closed, 1 = fully open
    bool showTeeth = false;
    bool showTongue = false;
};

// ============================================================================
// Accessory Types
// ============================================================================

enum class AccessoryType {
    None,
    Bow,            // Hair bow (Hello Kitty)
    Hat,
    Glasses,
    Collar,
    Ribbon,
    Crown,
    Flower,
    Custom
};

struct AccessoryParams {
    AccessoryType type = AccessoryType::None;
    
    // Position on character
    Vec3 position{0, 0, 0};
    Quat rotation;
    Vec3 scale{1, 1, 1};
    
    // Color
    Vec3 primaryColor{1, 0, 0};
    Vec3 secondaryColor{1, 1, 0};
    
    // Size
    float size = 0.1f;
};

// ============================================================================
// Cartoon Feature Generator
// ============================================================================

class CartoonFeatureGenerator {
public:
    // === Eyes ===
    
    static BodyPartDef createEye(const CartoonEyeParams& params, bool isLeft) {
        BodyPartDef part;
        part.id = isLeft ? "left_eye" : "right_eye";
        part.name = isLeft ? "Left Eye" : "Right Eye";
        part.type = isLeft ? BodyPartType::LeftEye : BodyPartType::RightEye;
        
        // Generate eye mesh based on style
        generateEyeMesh(params, part.customVertices, part.customIndices, isLeft);
        part.shape = PartShape::Custom;
        
        part.color = params.scleraColor;
        part.createBone = false;  // Eyes usually don't need bones
        
        return part;
    }
    
    // === Ears ===
    
    static BodyPartDef createEar(const CartoonEarParams& params, bool isLeft) {
        BodyPartDef part;
        part.id = isLeft ? "left_ear" : "right_ear";
        part.name = isLeft ? "Left Ear" : "Right Ear";
        part.type = isLeft ? BodyPartType::LeftEar : BodyPartType::RightEar;
        
        generateEarMesh(params, part.customVertices, part.customIndices, isLeft);
        part.shape = PartShape::Custom;
        
        part.color = params.outerColor;
        part.size = Vec3(params.width, params.height, params.depth);
        part.createBone = true;  // Ears can be animated
        part.boneName = isLeft ? "ear_L" : "ear_R";
        
        // Mirror for right side
        part.isMirrored = !isLeft;
        
        return part;
    }
    
    // === Nose ===
    
    static BodyPartDef createNose(const CartoonNoseParams& params) {
        BodyPartDef part;
        part.id = "nose";
        part.name = "Nose";
        part.type = BodyPartType::Nose;
        
        generateNoseMesh(params, part.customVertices, part.customIndices);
        part.shape = PartShape::Custom;
        
        part.color = params.color;
        part.createBone = false;
        
        return part;
    }
    
    // === Mouth ===
    
    static BodyPartDef createMouth(const CartoonMouthParams& params) {
        BodyPartDef part;
        part.id = "mouth";
        part.name = "Mouth";
        part.type = BodyPartType::Mouth;
        
        if (params.style == CartoonMouthStyle::None) {
            // No mouth (Hello Kitty style)
            part.shape = PartShape::Custom;
            // Empty mesh
            return part;
        }
        
        generateMouthMesh(params, part.customVertices, part.customIndices);
        part.shape = PartShape::Custom;
        
        part.color = params.lipColor;
        part.createBone = false;
        
        return part;
    }
    
    // === Accessories ===
    
    static BodyPartDef createAccessory(const AccessoryParams& params) {
        BodyPartDef part;
        
        switch (params.type) {
            case AccessoryType::Bow:
                part.id = "bow";
                part.name = "Bow";
                part.type = BodyPartType::Bow;
                generateBowMesh(params, part.customVertices, part.customIndices);
                break;
            case AccessoryType::Collar:
                part.id = "collar";
                part.name = "Collar";
                part.type = BodyPartType::Collar;
                generateCollarMesh(params, part.customVertices, part.customIndices);
                break;
            default:
                part.id = "accessory";
                part.name = "Accessory";
                part.type = BodyPartType::Custom;
                break;
        }
        
        part.shape = PartShape::Custom;
        part.offset = params.position;
        part.rotation = params.rotation;
        part.size = params.scale * params.size;
        part.color = params.primaryColor;
        part.createBone = true;
        
        return part;
    }
    
    // === Tail ===
    
    static BodyPartDef createTail(float length = 0.3f, float thickness = 0.03f,
                                  const Vec3& color = Vec3(0.3f, 0.3f, 0.3f)) {
        BodyPartDef part;
        part.id = "tail";
        part.name = "Tail";
        part.type = BodyPartType::Tail;
        part.shape = PartShape::Capsule;
        part.size = Vec3(thickness * 2, length, thickness * 2);
        part.offset = Vec3(0, 0.5f, -0.15f);  // Behind and below center
        part.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), -0.5f);  // Angled down/back
        part.color = color;
        part.createBone = true;
        part.boneName = "tail";
        part.segments = 8;
        
        return part;
    }
    
private:
    static void generateEyeMesh(const CartoonEyeParams& params,
                               std::vector<Vertex>& verts,
                               std::vector<uint32_t>& indices,
                               bool isLeft) {
        int segments = 24;
        float xSign = isLeft ? -1.0f : 1.0f;
        
        // Sclera (white part)
        generateEllipse(verts, indices, 
                       Vec3(0, 0, 0.01f),
                       params.width, params.height,
                       params.scleraColor, segments);
        
        // Iris
        float irisW = params.width * params.irisSize;
        float irisH = params.height * params.irisSize;
        generateEllipse(verts, indices,
                       Vec3(0, 0, 0.02f),
                       irisW, irisH,
                       params.irisColor, segments);
        
        // Pupil
        float pupilW = irisW * params.pupilSize;
        float pupilH = irisH * params.pupilSize;
        generateEllipse(verts, indices,
                       Vec3(0, 0, 0.03f),
                       pupilW, pupilH,
                       params.pupilColor, segments);
        
        // Highlight
        if (params.hasHighlight) {
            float hlW = params.width * params.highlightSize;
            float hlH = params.height * params.highlightSize;
            Vec3 hlPos(params.highlightOffset.x * params.width * xSign,
                      params.highlightOffset.y * params.height,
                      0.04f);
            generateEllipse(verts, indices, hlPos, hlW, hlH,
                           params.highlightColor, 12);
        }
        
        // Outline
        if (params.hasOutline) {
            generateEllipseOutline(verts, indices,
                                  Vec3(0, 0, 0.005f),
                                  params.width + params.outlineThickness,
                                  params.height + params.outlineThickness,
                                  params.outlineThickness,
                                  params.outlineColor, segments);
        }
    }
    
    static void generateEarMesh(const CartoonEarParams& params,
                               std::vector<Vertex>& verts,
                               std::vector<uint32_t>& indices,
                               bool isLeft) {
        float xSign = isLeft ? -1.0f : 1.0f;
        
        switch (params.style) {
            case CartoonEarStyle::MouseRound: {
                // Mickey Mouse style - large circular ears
                int segments = 24;
                float radius = params.width;
                
                // Outer ear
                generateDisc(verts, indices, Vec3(0, 0, 0),
                            radius, params.outerColor, segments);
                
                // Inner ear (slightly smaller, different color)
                generateDisc(verts, indices, Vec3(0, 0, 0.01f),
                            radius * 0.7f, params.innerColor, segments);
                break;
            }
            
            case CartoonEarStyle::CatPointed: {
                // Hello Kitty style - triangular cat ears
                generateTriangleEar(verts, indices,
                                   params.width, params.height,
                                   params.tipPointiness,
                                   params.outerColor, params.innerColor);
                break;
            }
            
            case CartoonEarStyle::BunnyLong: {
                // Long bunny ears
                generateCapsuleEar(verts, indices,
                                  params.width, params.height * 2,
                                  params.outerColor, params.innerColor);
                break;
            }
            
            case CartoonEarStyle::BearRound: {
                // Round bear ears (smaller than mouse)
                int segments = 16;
                float radius = params.width * 0.7f;
                
                generateDisc(verts, indices, Vec3(0, 0, 0),
                            radius, params.outerColor, segments);
                generateDisc(verts, indices, Vec3(0, 0, 0.005f),
                            radius * 0.5f, params.innerColor, segments);
                break;
            }
            
            default:
                break;
        }
        
        // Mirror for left side
        if (isLeft) {
            for (auto& v : verts) {
                v.position[0] *= -1;
                v.normal[0] *= -1;
            }
        }
    }
    
    static void generateNoseMesh(const CartoonNoseParams& params,
                                std::vector<Vertex>& verts,
                                std::vector<uint32_t>& indices) {
        switch (params.style) {
            case CartoonNoseStyle::Dot:
            case CartoonNoseStyle::Button: {
                // Simple sphere
                int segments = 16;
                generateSphere(verts, indices, Vec3(0, 0, params.depth),
                              params.width, params.color, segments);
                break;
            }
            
            case CartoonNoseStyle::Triangle: {
                // Triangular nose
                generateTriangle(verts, indices, Vec3(0, 0, 0),
                                params.width, params.height, params.color);
                break;
            }
            
            case CartoonNoseStyle::Animal: {
                // Oval with whisker area
                generateEllipse(verts, indices, Vec3(0, 0, 0),
                               params.width, params.height * 0.6f,
                               params.color, 16);
                break;
            }
            
            default:
                break;
        }
    }
    
    static void generateMouthMesh(const CartoonMouthParams& params,
                                 std::vector<Vertex>& verts,
                                 std::vector<uint32_t>& indices) {
        switch (params.style) {
            case CartoonMouthStyle::Line:
            case CartoonMouthStyle::Smile: {
                // Curved line for smile/frown
                generateCurvedLine(verts, indices,
                                  params.width, params.height,
                                  params.smileAmount,
                                  params.lipColor, 16);
                break;
            }
            
            case CartoonMouthStyle::Cat: {
                // W-shaped cat mouth
                generateCatMouth(verts, indices,
                                params.width, params.height,
                                params.lipColor);
                break;
            }
            
            case CartoonMouthStyle::Open: {
                // Open mouth with optional teeth
                generateOpenMouth(verts, indices,
                                 params.width, params.height * 2,
                                 params.openAmount,
                                 params.lipColor, params.innerColor,
                                 params.showTeeth, params.teethColor);
                break;
            }
            
            default:
                break;
        }
    }
    
    static void generateBowMesh(const AccessoryParams& params,
                               std::vector<Vertex>& verts,
                               std::vector<uint32_t>& indices) {
        float size = params.size;
        
        // Left loop
        generateEllipse(verts, indices,
                       Vec3(-size * 0.6f, 0, 0),
                       size * 0.5f, size * 0.3f,
                       params.primaryColor, 16);
        
        // Right loop
        generateEllipse(verts, indices,
                       Vec3(size * 0.6f, 0, 0),
                       size * 0.5f, size * 0.3f,
                       params.primaryColor, 16);
        
        // Center knot
        generateEllipse(verts, indices,
                       Vec3(0, 0, 0.01f),
                       size * 0.2f, size * 0.15f,
                       params.secondaryColor, 12);
    }
    
    static void generateCollarMesh(const AccessoryParams& params,
                                  std::vector<Vertex>& verts,
                                  std::vector<uint32_t>& indices) {
        // Simple torus for collar
        int segments = 24;
        int rings = 8;
        float majorRadius = params.size;
        float minorRadius = params.size * 0.1f;
        
        for (int ring = 0; ring <= rings; ring++) {
            float theta = ring * 2.0f * 3.14159f / rings;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);
            
            for (int seg = 0; seg <= segments; seg++) {
                float phi = seg * 2.0f * 3.14159f / segments;
                float cosPhi = std::cos(phi);
                float sinPhi = std::sin(phi);
                
                float x = (majorRadius + minorRadius * cosTheta) * cosPhi;
                float y = minorRadius * sinTheta;
                float z = (majorRadius + minorRadius * cosTheta) * sinPhi;
                
                Vertex v;
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;
                
                v.normal[0] = cosTheta * cosPhi;
                v.normal[1] = sinTheta;
                v.normal[2] = cosTheta * sinPhi;
                
                v.color[0] = params.primaryColor.x;
                v.color[1] = params.primaryColor.y;
                v.color[2] = params.primaryColor.z;
                
                verts.push_back(v);
            }
        }
        
        // Indices
        int vertsPerRow = segments + 1;
        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int current = ring * vertsPerRow + seg;
                int next = current + vertsPerRow;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }
    
    // === Helper geometry generators ===
    
    static void generateEllipse(std::vector<Vertex>& verts,
                               std::vector<uint32_t>& indices,
                               const Vec3& center, float width, float height,
                               const Vec3& color, int segments) {
        uint32_t centerIdx = verts.size();
        
        // Center vertex
        Vertex cv;
        cv.position[0] = center.x;
        cv.position[1] = center.y;
        cv.position[2] = center.z;
        cv.normal[0] = 0;
        cv.normal[1] = 0;
        cv.normal[2] = 1;
        cv.color[0] = color.x;
        cv.color[1] = color.y;
        cv.color[2] = color.z;
        cv.uv[0] = 0.5f;
        cv.uv[1] = 0.5f;
        verts.push_back(cv);
        
        // Edge vertices
        for (int i = 0; i <= segments; i++) {
            float angle = i * 2.0f * 3.14159f / segments;
            
            Vertex v;
            v.position[0] = center.x + std::cos(angle) * width;
            v.position[1] = center.y + std::sin(angle) * height;
            v.position[2] = center.z;
            v.normal[0] = 0;
            v.normal[1] = 0;
            v.normal[2] = 1;
            v.color[0] = color.x;
            v.color[1] = color.y;
            v.color[2] = color.z;
            v.uv[0] = std::cos(angle) * 0.5f + 0.5f;
            v.uv[1] = std::sin(angle) * 0.5f + 0.5f;
            verts.push_back(v);
        }
        
        // Triangles
        for (int i = 0; i < segments; i++) {
            indices.push_back(centerIdx);
            indices.push_back(centerIdx + 1 + i);
            indices.push_back(centerIdx + 1 + i + 1);
        }
    }
    
    static void generateEllipseOutline(std::vector<Vertex>& verts,
                                      std::vector<uint32_t>& indices,
                                      const Vec3& center,
                                      float outerW, float outerH,
                                      float thickness,
                                      const Vec3& color, int segments) {
        float innerW = outerW - thickness;
        float innerH = outerH - thickness;
        
        uint32_t baseIdx = verts.size();
        
        for (int i = 0; i <= segments; i++) {
            float angle = i * 2.0f * 3.14159f / segments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            
            // Outer vertex
            Vertex outer;
            outer.position[0] = center.x + cosA * outerW;
            outer.position[1] = center.y + sinA * outerH;
            outer.position[2] = center.z;
            outer.normal[0] = 0;
            outer.normal[1] = 0;
            outer.normal[2] = 1;
            outer.color[0] = color.x;
            outer.color[1] = color.y;
            outer.color[2] = color.z;
            verts.push_back(outer);
            
            // Inner vertex
            Vertex inner;
            inner.position[0] = center.x + cosA * innerW;
            inner.position[1] = center.y + sinA * innerH;
            inner.position[2] = center.z;
            inner.normal[0] = 0;
            inner.normal[1] = 0;
            inner.normal[2] = 1;
            inner.color[0] = color.x;
            inner.color[1] = color.y;
            inner.color[2] = color.z;
            verts.push_back(inner);
        }
        
        // Indices for ring
        for (int i = 0; i < segments; i++) {
            uint32_t o0 = baseIdx + i * 2;
            uint32_t i0 = baseIdx + i * 2 + 1;
            uint32_t o1 = baseIdx + (i + 1) * 2;
            uint32_t i1 = baseIdx + (i + 1) * 2 + 1;
            
            indices.push_back(o0);
            indices.push_back(i0);
            indices.push_back(o1);
            
            indices.push_back(o1);
            indices.push_back(i0);
            indices.push_back(i1);
        }
    }
    
    static void generateDisc(std::vector<Vertex>& verts,
                            std::vector<uint32_t>& indices,
                            const Vec3& center, float radius,
                            const Vec3& color, int segments) {
        generateEllipse(verts, indices, center, radius, radius, color, segments);
    }
    
    static void generateSphere(std::vector<Vertex>& verts,
                              std::vector<uint32_t>& indices,
                              const Vec3& center, float radius,
                              const Vec3& color, int segments) {
        uint32_t baseIdx = verts.size();
        int rings = segments / 2;
        
        for (int lat = 0; lat <= rings; lat++) {
            float theta = lat * 3.14159f / rings;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                
                Vertex v;
                v.position[0] = center.x + sinTheta * std::cos(phi) * radius;
                v.position[1] = center.y + cosTheta * radius;
                v.position[2] = center.z + sinTheta * std::sin(phi) * radius;
                
                v.normal[0] = sinTheta * std::cos(phi);
                v.normal[1] = cosTheta;
                v.normal[2] = sinTheta * std::sin(phi);
                
                v.color[0] = color.x;
                v.color[1] = color.y;
                v.color[2] = color.z;
                
                verts.push_back(v);
            }
        }
        
        int vertsPerRow = segments + 1;
        for (int lat = 0; lat < rings; lat++) {
            for (int lon = 0; lon < segments; lon++) {
                uint32_t current = baseIdx + lat * vertsPerRow + lon;
                uint32_t next = current + vertsPerRow;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }
    
    static void generateTriangle(std::vector<Vertex>& verts,
                                std::vector<uint32_t>& indices,
                                const Vec3& center, float width, float height,
                                const Vec3& color) {
        uint32_t baseIdx = verts.size();
        
        Vec3 points[3] = {
            {center.x, center.y + height * 0.5f, center.z},
            {center.x - width * 0.5f, center.y - height * 0.5f, center.z},
            {center.x + width * 0.5f, center.y - height * 0.5f, center.z}
        };
        
        for (int i = 0; i < 3; i++) {
            Vertex v;
            v.position[0] = points[i].x;
            v.position[1] = points[i].y;
            v.position[2] = points[i].z;
            v.normal[0] = 0;
            v.normal[1] = 0;
            v.normal[2] = 1;
            v.color[0] = color.x;
            v.color[1] = color.y;
            v.color[2] = color.z;
            verts.push_back(v);
        }
        
        indices.push_back(baseIdx);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
    }
    
    static void generateTriangleEar(std::vector<Vertex>& verts,
                                   std::vector<uint32_t>& indices,
                                   float width, float height, float pointiness,
                                   const Vec3& outerColor, const Vec3& innerColor) {
        // Outer triangle
        generateTriangle(verts, indices, Vec3(0, 0, 0), width, height, outerColor);
        
        // Inner triangle (smaller, different color)
        generateTriangle(verts, indices, Vec3(0, -height * 0.1f, 0.01f),
                        width * 0.6f, height * 0.6f, innerColor);
    }
    
    static void generateCapsuleEar(std::vector<Vertex>& verts,
                                  std::vector<uint32_t>& indices,
                                  float width, float height,
                                  const Vec3& outerColor, const Vec3& innerColor) {
        // Simple elongated ellipse for now
        generateEllipse(verts, indices, Vec3(0, 0, 0),
                       width * 0.5f, height * 0.5f, outerColor, 20);
        generateEllipse(verts, indices, Vec3(0, 0, 0.01f),
                       width * 0.3f, height * 0.35f, innerColor, 16);
    }
    
    static void generateCurvedLine(std::vector<Vertex>& verts,
                                  std::vector<uint32_t>& indices,
                                  float width, float thickness,
                                  float curvature,
                                  const Vec3& color, int segments) {
        uint32_t baseIdx = verts.size();
        
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            float x = (t - 0.5f) * width;
            float y = -curvature * (t - 0.5f) * (t - 0.5f) * 4.0f * width;
            
            // Top edge
            Vertex vTop;
            vTop.position[0] = x;
            vTop.position[1] = y + thickness * 0.5f;
            vTop.position[2] = 0;
            vTop.normal[0] = 0;
            vTop.normal[1] = 0;
            vTop.normal[2] = 1;
            vTop.color[0] = color.x;
            vTop.color[1] = color.y;
            vTop.color[2] = color.z;
            verts.push_back(vTop);
            
            // Bottom edge
            Vertex vBottom;
            vBottom.position[0] = x;
            vBottom.position[1] = y - thickness * 0.5f;
            vBottom.position[2] = 0;
            vBottom.normal[0] = 0;
            vBottom.normal[1] = 0;
            vBottom.normal[2] = 1;
            vBottom.color[0] = color.x;
            vBottom.color[1] = color.y;
            vBottom.color[2] = color.z;
            verts.push_back(vBottom);
        }
        
        for (int i = 0; i < segments; i++) {
            uint32_t t0 = baseIdx + i * 2;
            uint32_t b0 = baseIdx + i * 2 + 1;
            uint32_t t1 = baseIdx + (i + 1) * 2;
            uint32_t b1 = baseIdx + (i + 1) * 2 + 1;
            
            indices.push_back(t0);
            indices.push_back(b0);
            indices.push_back(t1);
            
            indices.push_back(t1);
            indices.push_back(b0);
            indices.push_back(b1);
        }
    }
    
    static void generateCatMouth(std::vector<Vertex>& verts,
                                std::vector<uint32_t>& indices,
                                float width, float height,
                                const Vec3& color) {
        // W shape: two curved lines
        generateCurvedLine(verts, indices,
                          width * 0.5f, height * 0.3f, -0.3f,
                          color, 8);
        
        // Offset second line
        uint32_t baseIdx = verts.size();
        generateCurvedLine(verts, indices,
                          width * 0.5f, height * 0.3f, -0.3f,
                          color, 8);
        
        // Shift second line
        for (uint32_t i = baseIdx; i < verts.size(); i++) {
            verts[i].position[0] += width * 0.3f;
        }
    }
    
    static void generateOpenMouth(std::vector<Vertex>& verts,
                                 std::vector<uint32_t>& indices,
                                 float width, float height, float openAmount,
                                 const Vec3& lipColor, const Vec3& innerColor,
                                 bool showTeeth, const Vec3& teethColor) {
        // Inner mouth (dark)
        generateEllipse(verts, indices, Vec3(0, 0, 0),
                       width * 0.8f, height * openAmount * 0.4f,
                       innerColor, 16);
        
        // Outer lip
        generateEllipseOutline(verts, indices, Vec3(0, 0, 0.01f),
                              width, height * openAmount * 0.5f,
                              height * 0.1f, lipColor, 16);
        
        // Teeth
        if (showTeeth && openAmount > 0.3f) {
            generateEllipse(verts, indices,
                           Vec3(0, height * openAmount * 0.15f, 0.02f),
                           width * 0.6f, height * 0.1f,
                           teethColor, 12);
        }
    }
};

}  // namespace luma
