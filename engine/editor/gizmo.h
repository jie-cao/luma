// Transform Gizmo - Visual manipulation handles for transforming objects
#pragma once

#include "engine/scene/entity.h"
#include "engine/scene/picking.h"
#include <functional>

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

// Gizmo colors
struct GizmoColors {
    float xAxis[4] = {0.9f, 0.2f, 0.2f, 1.0f};     // Red
    float yAxis[4] = {0.2f, 0.9f, 0.2f, 1.0f};     // Green
    float zAxis[4] = {0.2f, 0.2f, 0.9f, 1.0f};     // Blue
    float hover[4] = {1.0f, 1.0f, 0.0f, 1.0f};     // Yellow (highlighted)
    float active[4] = {1.0f, 0.5f, 0.0f, 1.0f};    // Orange (active/dragging)
    float planeXY[4] = {0.2f, 0.2f, 0.9f, 0.3f};   // Blue translucent
    float planeXZ[4] = {0.2f, 0.9f, 0.2f, 0.3f};   // Green translucent
    float planeYZ[4] = {0.9f, 0.2f, 0.2f, 0.3f};   // Red translucent
    float center[4] = {1.0f, 1.0f, 1.0f, 0.8f};    // White
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
    
    // Generate line data for rendering
    // screenScale: gizmo size in screen space (distance-independent sizing)
    GizmoRenderData generateRenderData(float screenScale) const;
    
    // Callback for transform changes (optional)
    std::function<void(Entity*)> onTransformChanged;
    
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
    Vec3 dragStartEntityPos_;
    Quat dragStartEntityRot_;
    Vec3 dragStartEntityScale_;
    Vec3 dragAxis_;
    Vec3 dragPlaneNormal_;
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
    return size_ * screenScale * 0.15f;  // 15% of axis length for hit testing
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
    
    // Test each axis
    auto testAxis = [&](const Vec3& dir, GizmoAxis axis) -> bool {
        Vec3 axisEnd = pos + dir * axisLen;
        // Create a thin box along the axis
        Vec3 p1 = pos + dir * hitRadius;
        Vec3 p2 = axisEnd;
        Vec3 perp1, perp2;
        
        // Get perpendicular vectors
        if (std::abs(dir.y) < 0.9f) {
            perp1 = Vec3(-dir.z, 0, dir.x).normalized();
        } else {
            perp1 = Vec3(1, 0, 0);
        }
        perp2 = Vec3(dir.y * perp1.z - dir.z * perp1.y,
                     dir.z * perp1.x - dir.x * perp1.z,
                     dir.x * perp1.y - dir.y * perp1.x);
        
        // Create AABB that encompasses the axis cylinder
        AABB axisBox;
        axisBox.expand(p1 - perp1 * hitRadius - perp2 * hitRadius);
        axisBox.expand(p1 + perp1 * hitRadius + perp2 * hitRadius);
        axisBox.expand(p2 - perp1 * hitRadius - perp2 * hitRadius);
        axisBox.expand(p2 + perp1 * hitRadius + perp2 * hitRadius);
        
        if (rayAABBIntersect(ray, axisBox)) {
            return true;
        }
        return false;
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
    
    dragStartEntityPos_ = target_->localTransform.position;
    dragStartEntityRot_ = target_->localTransform.rotation;
    dragStartEntityScale_ = target_->localTransform.scale;
    
    // Set up drag axis/plane
    Vec3 axisX(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 axisY(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 axisZ(orient.m[8], orient.m[9], orient.m[10]);
    
    switch (axis) {
        case GizmoAxis::X:
            dragAxis_ = axisX;
            dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
            break;
        case GizmoAxis::Y:
            dragAxis_ = axisY;
            dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
            break;
        case GizmoAxis::Z:
            dragAxis_ = axisZ;
            dragStartPos_ = projectOntoAxis(ray, pos, dragAxis_);
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
    
    Vec3 pos = getGizmoPosition();
    Vec3 currentPos;
    
    if (activeAxis_ == GizmoAxis::XYZ) {
        currentPos = projectOntoPlane(dragRay, pos, dragPlaneNormal_);
    } else {
        currentPos = projectOntoAxis(dragRay, pos, dragAxis_);
    }
    
    Vec3 delta = currentPos - dragStartPos_;
    
    switch (mode_) {
        case GizmoMode::Translate: {
            target_->localTransform.position = dragStartEntityPos_ + delta;
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
                // Single axis scale
                float axisDelta = delta.x * dragAxis_.x + delta.y * dragAxis_.y + delta.z * dragAxis_.z;
                scaleFactor = 1.0f + axisDelta * 0.01f;
                Vec3 newScale = dragStartEntityScale_;
                if (activeAxis_ == GizmoAxis::X) newScale.x *= scaleFactor;
                else if (activeAxis_ == GizmoAxis::Y) newScale.y *= scaleFactor;
                else if (activeAxis_ == GizmoAxis::Z) newScale.z *= scaleFactor;
                target_->localTransform.scale = newScale;
            }
            break;
        }
        case GizmoMode::Rotate: {
            // Simple rotation around axis
            float angle = 0.0f;
            if (activeAxis_ != GizmoAxis::XYZ) {
                Vec3 toStart = (dragStartPos_ - pos).normalized();
                Vec3 toCurrent = (currentPos - pos).normalized();
                // Cross product for rotation direction
                Vec3 cross(toStart.y * toCurrent.z - toStart.z * toCurrent.y,
                          toStart.z * toCurrent.x - toStart.x * toCurrent.z,
                          toStart.x * toCurrent.y - toStart.y * toCurrent.x);
                float dot = toStart.x * toCurrent.x + toStart.y * toCurrent.y + toStart.z * toCurrent.z;
                angle = std::atan2(cross.length(), dot);
                // Check rotation direction
                if (cross.x * dragAxis_.x + cross.y * dragAxis_.y + cross.z * dragAxis_.z < 0) {
                    angle = -angle;
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
            // X axis
            const float* xCol = getAxisColor(GizmoAxis::X, colors_.xAxis);
            data.lines.push_back({pos, pos + axisX * axisLen, {xCol[0], xCol[1], xCol[2], xCol[3]}});
            
            // Y axis
            const float* yCol = getAxisColor(GizmoAxis::Y, colors_.yAxis);
            data.lines.push_back({pos, pos + axisY * axisLen, {yCol[0], yCol[1], yCol[2], yCol[3]}});
            
            // Z axis
            const float* zCol = getAxisColor(GizmoAxis::Z, colors_.zAxis);
            data.lines.push_back({pos, pos + axisZ * axisLen, {zCol[0], zCol[1], zCol[2], zCol[3]}});
            
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

}  // namespace luma
