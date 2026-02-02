// Transform Gizmo - Visual manipulation handles for transforming objects
#pragma once

#include "engine/scene/entity.h"
#include "engine/scene/picking.h"
#include "engine/renderer/mesh.h"
#include <functional>
#include <cmath>

namespace luma {

// Gizmo operation modes
enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

// Gizmo space (local or world coordinates)
enum class GizmoSpace {
    Local,
    World
};

// Active axis during manipulation
enum class GizmoAxis {
    None,
    X,
    Y,
    Z,
    XY,
    XZ,
    YZ,
    XYZ  // All axes (center)
};

// Gizmo colors - brighter for better visibility
struct GizmoColors {
    float xAxis[4] = {1.0f, 0.2f, 0.2f, 1.0f};     // Bright Red
    float yAxis[4] = {0.2f, 1.0f, 0.2f, 1.0f};     // Bright Green
    float zAxis[4] = {0.2f, 0.4f, 1.0f, 1.0f};     // Bright Blue
    float hover[4] = {1.0f, 1.0f, 0.0f, 1.0f};     // Yellow (highlighted)
    float active[4] = {1.0f, 0.7f, 0.0f, 1.0f};   // Orange (active/dragging)
    float planeXY[4] = {0.2f, 0.2f, 0.9f, 0.3f};   // Blue translucent
    float planeXZ[4] = {0.2f, 0.9f, 0.2f, 0.3f};   // Green translucent
    float planeYZ[4] = {0.9f, 0.2f, 0.2f, 0.3f};   // Red translucent
    float center[4] = {1.0f, 1.0f, 1.0f, 1.0f};   // White (fully opaque)
    float outline[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // Black outline
};

// Gizmo line segment for rendering
struct GizmoLine {
    Vec3 start;
    Vec3 end;
    float color[4];
};

// Gizmo rendering data
struct GizmoRenderData {
    std::vector<GizmoLine> lines;
    Vec3 position;
    Mat4 orientation;  // For local space
    float size = 1.0f;
    GizmoAxis hoveredAxis = GizmoAxis::None;
    GizmoAxis activeAxis = GizmoAxis::None;
};

// ===== Transform Gizmo =====
class TransformGizmo {
public:
    TransformGizmo();
    
    // === Configuration ===
    void setMode(GizmoMode mode) { mode_ = mode; }
    GizmoMode getMode() const { return mode_; }
    
    void setSpace(GizmoSpace space) { space_ = space; }
    GizmoSpace getSpace() const { return space_; }
    
    void setSize(float size) { size_ = size; }
    float getSize() const { return size_; }
    
    void setColors(const GizmoColors& colors) { colors_ = colors; }
    const GizmoColors& getColors() const { return colors_; }
    
    // === Target ===
    void setTarget(Entity* entity) { target_ = entity; }
    Entity* getTarget() const { return target_; }
    
    // === Interaction ===
    
    // Test if gizmo is hovered at mouse position
    // Returns the hovered axis (None if not hovering)
    GizmoAxis testHover(const Ray& ray, float screenScale);
    
    // Begin drag operation (call on mouse down)
    // Returns true if gizmo was clicked
    bool beginDrag(const Ray& ray, float screenScale);
    
    // Update drag (call on mouse move while dragging)
    // dragRay: current mouse ray
    // Returns true if transform was modified
    bool updateDrag(const Ray& dragRay);
    
    // End drag operation (call on mouse up)
    void endDrag();
    
    // Check if currently dragging
    bool isDragging() const { return isDragging_; }
    
    // Get currently hovered axis
    GizmoAxis getHoveredAxis() const { return hoveredAxis_; }
    
    // Get active (dragging) axis
    GizmoAxis getActiveAxis() const { return activeAxis_; }
    
    // === Rendering ===
    
    // Generate line data for rendering (legacy, for thin lines)
    // screenScale: gizmo size in screen space (distance-independent sizing)
    GizmoRenderData generateRenderData(float screenScale) const;
    
    // Generate mesh data for rendering (thick cylinders, like Blender/Maya)
    // screenScale: gizmo size in screen space
    // Returns a list of meshes with their transforms and colors
    struct GizmoMeshData {
        struct AxisMesh {
            Mesh mesh;
            Mat4 transform;  // World transform for this axis
            float color[4];  // RGBA color
        };
        std::vector<AxisMesh> meshes;
    };
    GizmoMeshData generateMeshData(float screenScale) const;
    
    // Callback for transform changes (optional)
    std::function<void(Entity*)> onTransformChanged;
    
    // Calculate screen-space scale factor for consistent gizmo size
    // cameraPos: camera position in world space
    // screenPixelSize: desired gizmo size in pixels (e.g., 100)
    // screenHeight: viewport height in pixels
    // fovY: vertical field of view in radians
    static float calculateScreenScale(const Vec3& gizmoPos, const Vec3& cameraPos, 
                                       float screenPixelSize, float screenHeight, float fovY) {
        float distance = (gizmoPos - cameraPos).length();
        if (distance < 0.001f) distance = 0.001f;
        
        // Convert pixel size to world units at the gizmo's distance
        // At distance d, 1 world unit = (screenHeight / (2 * tan(fov/2) * d)) pixels
        float pixelsPerUnit = screenHeight / (2.0f * tanf(fovY * 0.5f) * distance);
        return screenPixelSize / pixelsPerUnit;
    }
    
private:
    // Internal helpers
    Vec3 getGizmoPosition() const;
    Mat4 getGizmoOrientation() const;
    float getAxisHitRadius(float screenScale) const;
    
    // Project point onto axis/plane for dragging
    Vec3 projectOntoAxis(const Ray& ray, const Vec3& axisOrigin, const Vec3& axisDir);
    Vec3 projectOntoPlane(const Ray& ray, const Vec3& planeOrigin, const Vec3& planeNormal);
    
    // Configuration
    GizmoMode mode_ = GizmoMode::Translate;
    GizmoSpace space_ = GizmoSpace::World;
    float size_ = 1.0f;
    GizmoColors colors_;
    
    // Target entity
    Entity* target_ = nullptr;
    
    // Interaction state
    GizmoAxis hoveredAxis_ = GizmoAxis::None;
    GizmoAxis activeAxis_ = GizmoAxis::None;
    bool isDragging_ = false;
    
    // Drag state
    Vec3 dragStartPos_;
    Vec3 dragStartGizmoPos_;    // Gizmo position at drag start (fixed during drag)
    Vec3 dragStartEntityPos_;
    Quat dragStartEntityRot_;
    Vec3 dragStartEntityScale_;
    Vec3 dragAxis_;
    Vec3 dragPlaneNormal_;
    float dragScreenScale_;     // Screen scale at drag start (for scale calculations)
};

// ===== Implementation =====

inline TransformGizmo::TransformGizmo() = default;

inline Vec3 TransformGizmo::getGizmoPosition() const {
    if (!target_) return Vec3(0, 0, 0);
    return target_->getWorldPosition();
}

inline Mat4 TransformGizmo::getGizmoOrientation() const {
    if (!target_ || space_ == GizmoSpace::World) {
        return Mat4::identity();
    }
    // Local space: use entity's rotation
    return Mat4::fromQuat(target_->localTransform.rotation);
}

inline float TransformGizmo::getAxisHitRadius(float screenScale) const {
    return size_ * screenScale * 0.15f;  // Hit cylinder radius (15% of axis length)
}

inline Vec3 TransformGizmo::projectOntoAxis(const Ray& ray, const Vec3& axisOrigin, const Vec3& axisDir) {
    // Find closest point on axis to ray
    Vec3 w = ray.origin - axisOrigin;
    float a = axisDir.x * axisDir.x + axisDir.y * axisDir.y + axisDir.z * axisDir.z;
    float b = axisDir.x * ray.direction.x + axisDir.y * ray.direction.y + axisDir.z * ray.direction.z;
    float c = ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y + ray.direction.z * ray.direction.z;
    float d = axisDir.x * w.x + axisDir.y * w.y + axisDir.z * w.z;
    float e = ray.direction.x * w.x + ray.direction.y * w.y + ray.direction.z * w.z;
    
    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f) {
        return axisOrigin;  // Parallel
    }
    
    float t = (b * e - c * d) / denom;
    return axisOrigin + axisDir * t;
}

inline Vec3 TransformGizmo::projectOntoPlane(const Ray& ray, const Vec3& planeOrigin, const Vec3& planeNormal) {
    float denom = planeNormal.x * ray.direction.x + planeNormal.y * ray.direction.y + planeNormal.z * ray.direction.z;
    if (std::abs(denom) < 1e-6f) {
        return planeOrigin;  // Parallel
    }
    
    Vec3 p0l0 = planeOrigin - ray.origin;
    float t = (planeNormal.x * p0l0.x + planeNormal.y * p0l0.y + planeNormal.z * p0l0.z) / denom;
    return ray.at(t);
}

inline GizmoAxis TransformGizmo::testHover(const Ray& ray, float screenScale) {
    if (!target_) return GizmoAxis::None;
    
    Vec3 pos = getGizmoPosition();
    Mat4 orient = getGizmoOrientation();
    float axisLen = size_ * screenScale;
    float hitRadius = getAxisHitRadius(screenScale);
    
    // Get axis directions
    Vec3 axisX(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 axisY(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 axisZ(orient.m[8], orient.m[9], orient.m[10]);
    
    // Test center first (for uniform scale)
    float centerRadius = hitRadius * 1.5f;
    AABB centerBox(pos - Vec3(centerRadius, centerRadius, centerRadius), 
                   pos + Vec3(centerRadius, centerRadius, centerRadius));
    if (rayAABBIntersect(ray, centerBox)) {
        hoveredAxis_ = GizmoAxis::XYZ;
        return GizmoAxis::XYZ;
    }
    
    // Test each axis using ray-cylinder intersection
    auto testAxis = [&](const Vec3& dir, GizmoAxis axis) -> bool {
        // Test ray against infinite cylinder along axis, then check bounds
        Vec3 axisStart = pos + dir * (hitRadius * 0.5f);  // Start slightly from center
        Vec3 axisEnd = pos + dir * axisLen;
        
        // Ray-cylinder intersection (simplified: project ray onto plane perpendicular to axis)
        Vec3 oc = ray.origin - pos;
        
        // Project ray direction and oc onto plane perpendicular to axis
        float rayDotAxis = ray.direction.dot(dir);
        float ocDotAxis = oc.dot(dir);
        
        Vec3 rayPerp = ray.direction - dir * rayDotAxis;
        Vec3 ocPerp = oc - dir * ocDotAxis;
        
        float a = rayPerp.dot(rayPerp);
        float b = 2.0f * ocPerp.dot(rayPerp);
        float c = ocPerp.dot(ocPerp) - hitRadius * hitRadius;
        
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0) return false;
        
        // Find intersection point
        float t = (-b - sqrtf(discriminant)) / (2.0f * a);
        if (t < 0) t = (-b + sqrtf(discriminant)) / (2.0f * a);
        if (t < 0) return false;
        
        // Check if intersection is within axis length
        Vec3 hitPoint = ray.origin + ray.direction * t;
        float projOnAxis = (hitPoint - pos).dot(dir);
        
        return projOnAxis > hitRadius * 0.3f && projOnAxis < axisLen;
    };
    
    // Find closest hit
    float minDist = std::numeric_limits<float>::max();
    GizmoAxis result = GizmoAxis::None;
    
    if (testAxis(axisX, GizmoAxis::X)) {
        Vec3 closest = projectOntoAxis(ray, pos, axisX);
        float dist = (closest - ray.origin).length();
        if (dist < minDist) { minDist = dist; result = GizmoAxis::X; }
    }
    if (testAxis(axisY, GizmoAxis::Y)) {
        Vec3 closest = projectOntoAxis(ray, pos, axisY);
        float dist = (closest - ray.origin).length();
        if (dist < minDist) { minDist = dist; result = GizmoAxis::Y; }
    }
    if (testAxis(axisZ, GizmoAxis::Z)) {
        Vec3 closest = projectOntoAxis(ray, pos, axisZ);
        float dist = (closest - ray.origin).length();
        if (dist < minDist) { minDist = dist; result = GizmoAxis::Z; }
    }
    
    hoveredAxis_ = result;
    return result;
}

inline bool TransformGizmo::beginDrag(const Ray& ray, float screenScale) {
    if (!target_) return false;
    
    GizmoAxis axis = testHover(ray, screenScale);
    if (axis == GizmoAxis::None) return false;
    
    activeAxis_ = axis;
    isDragging_ = true;
    
    // Store initial state
    Vec3 pos = getGizmoPosition();
    Mat4 orient = getGizmoOrientation();
    
    dragStartGizmoPos_ = pos;  // Save fixed gizmo position for entire drag operation
    dragStartEntityPos_ = target_->localTransform.position;
    dragStartEntityRot_ = target_->localTransform.rotation;
    dragStartEntityScale_ = target_->localTransform.scale;
    dragScreenScale_ = screenScale;  // Store screen scale for scale calculations
    
    // Set up drag axis/plane
    Vec3 axisX(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 axisY(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 axisZ(orient.m[8], orient.m[9], orient.m[10]);
    
    switch (axis) {
        case GizmoAxis::X:
            dragAxis_ = axisX;
            if (mode_ == GizmoMode::Rotate) {
                // For rotation, project onto plane perpendicular to axis
                Vec3 planeNormal = axisX;
                dragStartPos_ = projectOntoPlane(ray, pos, planeNormal);
            } else {
                dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
            }
            break;
        case GizmoAxis::Y:
            dragAxis_ = axisY;
            if (mode_ == GizmoMode::Rotate) {
                Vec3 planeNormal = axisY;
                dragStartPos_ = projectOntoPlane(ray, pos, planeNormal);
            } else {
                dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
            }
            break;
        case GizmoAxis::Z:
            dragAxis_ = axisZ;
            if (mode_ == GizmoMode::Rotate) {
                Vec3 planeNormal = axisZ;
                dragStartPos_ = projectOntoPlane(ray, pos, planeNormal);
            } else {
                dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
            }
            break;
        case GizmoAxis::XYZ:
            // For uniform scale, use camera-facing plane
            dragPlaneNormal_ = ray.direction * (-1.0f);
            dragStartPos_ = projectOntoPlane(ray, pos, dragPlaneNormal_);
            break;
        default:
            break;
    }
    
    return true;
}

inline bool TransformGizmo::updateDrag(const Ray& dragRay) {
    if (!isDragging_ || !target_) return false;
    
    // Use the STARTING gizmo position, not the current one!
    Vec3 pos = dragStartGizmoPos_;
    Vec3 currentPos;
    
    if (activeAxis_ == GizmoAxis::XYZ) {
        currentPos = projectOntoPlane(dragRay, pos, dragPlaneNormal_);
    } else if (mode_ == GizmoMode::Rotate) {
        // For rotation, project onto plane perpendicular to rotation axis
        currentPos = projectOntoPlane(dragRay, pos, dragAxis_);
    } else {
        currentPos = projectOntoAxis(dragRay, pos, dragAxis_);
    }
    
    Vec3 delta = currentPos - dragStartPos_;
    
    // Project delta onto the drag axis to get movement along the axis
    // This ensures movement is constrained to the axis direction
    float axisDeltaScalar = delta.x * dragAxis_.x + delta.y * dragAxis_.y + delta.z * dragAxis_.z;
    
    // Reverse direction to fix inverted movement
    // TODO: This might be due to coordinate system or projection matrix issue
    axisDeltaScalar = -axisDeltaScalar;
    
    Vec3 axisDelta = dragAxis_ * axisDeltaScalar;
    
    switch (mode_) {
        case GizmoMode::Translate: {
            // Apply movement along the axis
            target_->localTransform.position = dragStartEntityPos_ + axisDelta;
            break;
        }
        case GizmoMode::Scale: {
            float scaleFactor;
            if (activeAxis_ == GizmoAxis::XYZ) {
                // Uniform scale based on distance from gizmo center
                Vec3 startOffset = dragStartPos_ - pos;
                Vec3 currentOffset = currentPos - pos;
                float startDist = startOffset.length();
                float currentDist = currentOffset.length();
                scaleFactor = (startDist > 0.001f) ? currentDist / startDist : 1.0f;
                target_->localTransform.scale = dragStartEntityScale_ * scaleFactor;
            } else {
                // Single axis scale - use projected axisDelta (already computed above)
                // Reverse direction for scale too
                float scaleAxisDelta = -axisDeltaScalar;
                // Scale factor based on movement along axis
                // Use a reasonable sensitivity
                float axisLen = size_ * dragScreenScale_;
                scaleFactor = 1.0f + (scaleAxisDelta / axisLen) * 2.0f;  // 2x sensitivity
                Vec3 newScale = dragStartEntityScale_;
                if (activeAxis_ == GizmoAxis::X) newScale.x *= scaleFactor;
                else if (activeAxis_ == GizmoAxis::Y) newScale.y *= scaleFactor;
                else if (activeAxis_ == GizmoAxis::Z) newScale.z *= scaleFactor;
                target_->localTransform.scale = newScale;
            }
            break;
        }
        case GizmoMode::Rotate: {
            // Rotation around axis - project points onto plane perpendicular to axis
            float angle = 0.0f;
            if (activeAxis_ != GizmoAxis::XYZ) {
                // Project both start and current points onto rotation plane
                Vec3 startOffset = dragStartPos_ - pos;
                Vec3 currentOffset = currentPos - pos;
                
                // Remove component along axis to get 2D vectors in rotation plane
                float startProj = startOffset.x * dragAxis_.x + startOffset.y * dragAxis_.y + startOffset.z * dragAxis_.z;
                float currentProj = currentOffset.x * dragAxis_.x + currentOffset.y * dragAxis_.y + currentOffset.z * dragAxis_.z;
                
                Vec3 startInPlane = startOffset - dragAxis_ * startProj;
                Vec3 currentInPlane = currentOffset - dragAxis_ * currentProj;
                
                float startLen = startInPlane.length();
                float currentLen = currentInPlane.length();
                
                if (startLen > 0.001f && currentLen > 0.001f) {
                    Vec3 startNorm = startInPlane / startLen;
                    Vec3 currentNorm = currentInPlane / currentLen;
                    
                    // Calculate angle using dot and cross product
                    float dot = startNorm.x * currentNorm.x + startNorm.y * currentNorm.y + startNorm.z * currentNorm.z;
                    dot = std::max(-1.0f, std::min(1.0f, dot));  // Clamp to avoid NaN
                    
                    Vec3 cross = startNorm.cross(currentNorm);
                    float crossDotAxis = cross.x * dragAxis_.x + cross.y * dragAxis_.y + cross.z * dragAxis_.z;
                    
                    angle = std::acos(dot);
                    if (crossDotAxis < 0) angle = -angle;
                }
            }
            
            // Create rotation quaternion
            float halfAngle = angle * 0.5f;
            float s = std::sin(halfAngle);
            Quat deltaRot(dragAxis_.x * s, dragAxis_.y * s, dragAxis_.z * s, std::cos(halfAngle));
            target_->localTransform.rotation = deltaRot * dragStartEntityRot_;
            break;
        }
    }
    
    target_->updateWorldMatrix();
    
    if (onTransformChanged) {
        onTransformChanged(target_);
    }
    
    return true;
}

inline void TransformGizmo::endDrag() {
    isDragging_ = false;
    activeAxis_ = GizmoAxis::None;
}

inline GizmoRenderData TransformGizmo::generateRenderData(float screenScale) const {
    GizmoRenderData data;
    
    if (!target_) return data;
    
    data.position = getGizmoPosition();
    data.orientation = getGizmoOrientation();
    data.size = size_ * screenScale;
    data.hoveredAxis = hoveredAxis_;
    data.activeAxis = activeAxis_;
    
    float axisLen = data.size;
    Vec3 pos = data.position;
    
    // Get axis directions from orientation
    Vec3 axisX(data.orientation.m[0], data.orientation.m[1], data.orientation.m[2]);
    Vec3 axisY(data.orientation.m[4], data.orientation.m[5], data.orientation.m[6]);
    Vec3 axisZ(data.orientation.m[8], data.orientation.m[9], data.orientation.m[10]);
    
    // Helper to get axis color (with hover/active highlight)
    auto getAxisColor = [&](GizmoAxis axis, const float* baseColor) -> const float* {
        if (activeAxis_ == axis) return colors_.active;
        if (hoveredAxis_ == axis) return colors_.hover;
        return baseColor;
    };
    
    // Generate lines based on mode
    switch (mode_) {
        case GizmoMode::Translate:
        case GizmoMode::Scale:
        {
            // Helper to generate thick line by rendering multiple offset lines
            auto addThickLine = [&](const Vec3& start, const Vec3& end, const float* color, float thickness) {
                Vec3 dir = (end - start).normalized();
                Vec3 perp1, perp2;
                
                // Get perpendicular vectors for offset
                if (std::abs(dir.y) < 0.9f) {
                    perp1 = Vec3(-dir.z, 0, dir.x).normalized();
                } else {
                    perp1 = Vec3(1, 0, 0);
                }
                perp2 = dir.cross(perp1).normalized();
                
                // Render multiple lines with small offsets to create thickness
                int numLines = 5;  // Number of lines for thickness
                for (int i = 0; i < numLines; i++) {
                    float offset = (float)(i - numLines/2) * thickness / numLines;
                    Vec3 offsetVec = perp1 * offset;
                    data.lines.push_back({
                        start + offsetVec,
                        end + offsetVec,
                        {color[0], color[1], color[2], color[3]}
                    });
                }
            };
            
            float lineThickness = axisLen * 0.02f;  // 2% of axis length for thickness
            
            // X axis - bright red, thick
            Vec3 xEnd = pos + axisX * axisLen;
            const float* xCol = getAxisColor(GizmoAxis::X, colors_.xAxis);
            addThickLine(pos, xEnd, xCol, lineThickness);
            
            // Y axis - bright green, thick
            Vec3 yEnd = pos + axisY * axisLen;
            const float* yCol = getAxisColor(GizmoAxis::Y, colors_.yAxis);
            addThickLine(pos, yEnd, yCol, lineThickness);
            
            // Z axis - bright blue, thick
            Vec3 zEnd = pos + axisZ * axisLen;
            const float* zCol = getAxisColor(GizmoAxis::Z, colors_.zAxis);
            addThickLine(pos, zEnd, zCol, lineThickness);
            
            // Center box (for uniform scale)
            if (mode_ == GizmoMode::Scale) {
                const float* cCol = getAxisColor(GizmoAxis::XYZ, colors_.center);
                float s = axisLen * 0.15f;
                // Draw small box at center
                Vec3 corners[8] = {
                    pos + (axisX * (-s) + axisY * (-s) + axisZ * (-s)),
                    pos + (axisX * s + axisY * (-s) + axisZ * (-s)),
                    pos + (axisX * s + axisY * s + axisZ * (-s)),
                    pos + (axisX * (-s) + axisY * s + axisZ * (-s)),
                    pos + (axisX * (-s) + axisY * (-s) + axisZ * s),
                    pos + (axisX * s + axisY * (-s) + axisZ * s),
                    pos + (axisX * s + axisY * s + axisZ * s),
                    pos + (axisX * (-s) + axisY * s + axisZ * s),
                };
                // Bottom face
                data.lines.push_back({corners[0], corners[1], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[1], corners[2], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[2], corners[3], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[3], corners[0], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                // Top face
                data.lines.push_back({corners[4], corners[5], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[5], corners[6], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[6], corners[7], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[7], corners[4], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                // Vertical edges
                data.lines.push_back({corners[0], corners[4], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[1], corners[5], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[2], corners[6], {cCol[0], cCol[1], cCol[2], cCol[3]}});
                data.lines.push_back({corners[3], corners[7], {cCol[0], cCol[1], cCol[2], cCol[3]}});
            }
            break;
        }
        
        case GizmoMode::Rotate:
        {
            // Draw rotation circles
            int segments = 32;
            float radius = axisLen * 0.8f;
            
            // X rotation (YZ plane)
            const float* xCol = getAxisColor(GizmoAxis::X, colors_.xAxis);
            for (int i = 0; i < segments; i++) {
                float a1 = (float)i / segments * 6.28318f;
                float a2 = (float)(i + 1) / segments * 6.28318f;
                Vec3 p1 = pos + axisY * (std::cos(a1) * radius) + axisZ * (std::sin(a1) * radius);
                Vec3 p2 = pos + axisY * (std::cos(a2) * radius) + axisZ * (std::sin(a2) * radius);
                data.lines.push_back({p1, p2, {xCol[0], xCol[1], xCol[2], xCol[3]}});
            }
            
            // Y rotation (XZ plane)
            const float* yCol = getAxisColor(GizmoAxis::Y, colors_.yAxis);
            for (int i = 0; i < segments; i++) {
                float a1 = (float)i / segments * 6.28318f;
                float a2 = (float)(i + 1) / segments * 6.28318f;
                Vec3 p1 = pos + axisX * (std::cos(a1) * radius) + axisZ * (std::sin(a1) * radius);
                Vec3 p2 = pos + axisX * (std::cos(a2) * radius) + axisZ * (std::sin(a2) * radius);
                data.lines.push_back({p1, p2, {yCol[0], yCol[1], yCol[2], yCol[3]}});
            }
            
            // Z rotation (XY plane)
            const float* zCol = getAxisColor(GizmoAxis::Z, colors_.zAxis);
            for (int i = 0; i < segments; i++) {
                float a1 = (float)i / segments * 6.28318f;
                float a2 = (float)(i + 1) / segments * 6.28318f;
                Vec3 p1 = pos + axisX * (std::cos(a1) * radius) + axisY * (std::sin(a1) * radius);
                Vec3 p2 = pos + axisX * (std::cos(a2) * radius) + axisY * (std::sin(a2) * radius);
                data.lines.push_back({p1, p2, {zCol[0], zCol[1], zCol[2], zCol[3]}});
            }
            break;
        }
    }
    
    return data;
}

inline TransformGizmo::GizmoMeshData TransformGizmo::generateMeshData(float screenScale) const {
    GizmoMeshData data;
    
    if (!target_) return data;
    
    Vec3 pos = getGizmoPosition();
    Mat4 orient = getGizmoOrientation();
    float axisLen = size_ * screenScale;
    float cylinderRadius = axisLen * 0.03f;  // 3% of axis length for thickness
    
    // Get axis directions
    Vec3 axisX(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 axisY(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 axisZ(orient.m[8], orient.m[9], orient.m[10]);
    
    // Helper to get axis color
    auto getAxisColor = [&](GizmoAxis axis, const float* baseColor) -> const float* {
        if (activeAxis_ == axis) return colors_.active;
        if (hoveredAxis_ == axis) return colors_.hover;
        return baseColor;
    };
    
    // Helper to create transform matrix for a cylinder along an axis
    auto createCylinderTransform = [&](const Vec3& axisDir, float length) -> Mat4 {
        // Create rotation matrix to align cylinder with axis
        Vec3 up(0, 1, 0);
        Vec3 right = axisDir.cross(up).normalized();
        if (right.length() < 0.1f) {
            // Axis is parallel to up, use different reference
            right = Vec3(1, 0, 0);
        }
        Vec3 finalUp = right.cross(axisDir).normalized();
        
        Mat4 rot;
        rot.m[0] = right.x; rot.m[4] = right.y; rot.m[8] = right.z; rot.m[12] = 0;
        rot.m[1] = finalUp.x; rot.m[5] = finalUp.y; rot.m[9] = finalUp.z; rot.m[13] = 0;
        rot.m[2] = axisDir.x; rot.m[6] = axisDir.y; rot.m[10] = axisDir.z; rot.m[14] = 0;
        rot.m[3] = 0; rot.m[7] = 0; rot.m[11] = 0; rot.m[15] = 1;
        
        // Scale and translate
        Mat4 scaleMat = Mat4::scale(Vec3(cylinderRadius, length, cylinderRadius));
        Mat4 trans = Mat4::translation(pos + axisDir * (length * 0.5f));
        
        return trans * rot * scaleMat;
    };
    
    switch (mode_) {
        case GizmoMode::Translate:
        case GizmoMode::Scale:
        {
            // X axis cylinder
            Mesh xCylinder = create_cylinder(1.0f, 1.0f, 16);
            Mat4 xTransform = createCylinderTransform(axisX, axisLen);
            const float* xCol = getAxisColor(GizmoAxis::X, colors_.xAxis);
            data.meshes.push_back({xCylinder, xTransform, {xCol[0], xCol[1], xCol[2], xCol[3]}});
            
            // Y axis cylinder
            Mesh yCylinder = create_cylinder(1.0f, 1.0f, 16);
            Mat4 yTransform = createCylinderTransform(axisY, axisLen);
            const float* yCol = getAxisColor(GizmoAxis::Y, colors_.yAxis);
            data.meshes.push_back({yCylinder, yTransform, {yCol[0], yCol[1], yCol[2], yCol[3]}});
            
            // Z axis cylinder
            Mesh zCylinder = create_cylinder(1.0f, 1.0f, 16);
            Mat4 zTransform = createCylinderTransform(axisZ, axisLen);
            const float* zCol = getAxisColor(GizmoAxis::Z, colors_.zAxis);
            data.meshes.push_back({zCylinder, zTransform, {zCol[0], zCol[1], zCol[2], zCol[3]}});
            break;
        }
        case GizmoMode::Rotate:
            // For rotation, we still use lines (circles)
            break;
    }
    
    return data;
}

}  // namespace luma
