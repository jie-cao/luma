// LUMA Mesh Edit Gizmo
// Transform gizmo for manipulating selected mesh elements (vertices, edges, faces)
// Similar to Maya/Blender's mesh editing gizmo

#pragma once

#include "engine/mesh/edit_mesh.h"
#include "engine/scene/picking.h"
#include "engine/editor/gizmo.h"  // Reuse GizmoMode, GizmoAxis, GizmoColors, etc.
#include <cmath>
#include <functional>
#include <set>

namespace luma {
namespace editor {

// Mesh Edit Gizmo - operates on selected vertices/edges/faces
class MeshEditGizmo {
public:
    MeshEditGizmo();
    
    // === Configuration ===
    void setMode(GizmoMode mode) { m_mode = mode; }
    GizmoMode getMode() const { return m_mode; }
    
    void setSpace(GizmoSpace space) { m_space = space; }
    GizmoSpace getSpace() const { return m_space; }
    
    void setSize(float size) { m_size = size; }
    float getSize() const { return m_size; }
    
    void setColors(const GizmoColors& colors) { m_colors = colors; }
    const GizmoColors& getColors() const { return m_colors; }
    
    // === Target ===
    void setEditMesh(EditMesh* mesh) { m_editMesh = mesh; }
    EditMesh* getEditMesh() const { return m_editMesh; }
    
    void setWorldMatrix(const float* worldMatrix) {
        if (worldMatrix) {
            for (int i = 0; i < 16; ++i) m_worldMatrix[i] = worldMatrix[i];
        }
    }
    
    // === Selection ===
    // Check if there's a valid selection to show gizmo
    bool hasSelection() const;
    
    // Get center of selected elements in world space
    Vec3 getSelectionCenter() const;
    
    // === Interaction ===
    
    // Test if gizmo is hovered at mouse position
    // Note: Uses luma::Ray from scene/picking.h
    GizmoAxis testHover(const luma::Ray& ray, float screenScale);
    
    // Begin drag operation (returns true if gizmo was clicked)
    bool beginDrag(const luma::Ray& ray, float screenScale);
    
    // Update drag (call on mouse move while dragging)
    bool updateDrag(const luma::Ray& dragRay);
    
    // End drag operation
    void endDrag();
    
    // Check if currently dragging
    bool isDragging() const { return m_isDragging; }
    
    // Get axis states
    GizmoAxis getHoveredAxis() const { return m_hoveredAxis; }
    GizmoAxis getActiveAxis() const { return m_activeAxis; }
    
    // === Rendering ===
    
    // Generate line data for rendering
    GizmoRenderData generateRenderData(float screenScale) const;
    
    // Calculate screen-space scale factor (same as TransformGizmo)
    static float calculateScreenScale(const Vec3& gizmoPos, const Vec3& cameraPos,
                                      float screenPixelSize, float screenHeight, float fovY) {
        float distance = (gizmoPos - cameraPos).length();
        if (distance < 0.001f) distance = 0.001f;
        float pixelsPerUnit = screenHeight / (2.0f * tanf(fovY * 0.5f) * distance);
        return screenPixelSize / pixelsPerUnit;
    }
    
    // === Extrude Mode ===
    void setExtrudeMode(bool enabled) { m_extrudeMode = enabled; }
    bool isExtrudeMode() const { return m_extrudeMode; }
    void setExtrudeNormal(const Vec3& normal) { m_extrudeNormal = normal; }
    Vec3 getExtrudeNormal() const { return m_extrudeNormal; }
    bool wasExtrudePerformed() const { return m_extrudePerformed; }
    void resetExtrudeState() { m_extrudePerformed = false; }
    
    // Callback when mesh is modified
    std::function<void()> onMeshChanged;
    
private:
    // Internal helpers
    Vec3 getGizmoPosition() const;
    Mat4 getGizmoOrientation() const;
    float getAxisHitRadius(float screenScale) const;
    
    // Ray projection helpers
    Vec3 projectOntoAxis(const luma::Ray& ray, const Vec3& axisOrigin, const Vec3& axisDir);
    Vec3 projectOntoPlane(const luma::Ray& ray, const Vec3& planeOrigin, const Vec3& planeNormal);
    
    // Transform helpers
    void applyTranslation(const Vec3& delta);
    void applyRotation(const Vec3& axis, float angle);
    void applyScale(const Vec3& scale);
    
    // Get world position of a local vertex
    Vec3 transformToWorld(const Vec3& localPos) const;
    Vec3 transformToLocal(const Vec3& worldPos) const;
    
    // Store original vertex positions for undo
    void storeOriginalPositions();
    void restoreOriginalPositions();
    
    // Configuration
    GizmoMode m_mode = GizmoMode::Translate;
    GizmoSpace m_space = GizmoSpace::World;
    float m_size = 1.0f;
    GizmoColors m_colors;
    
    // Extrude mode: when true, the gizmo shows along the face normal
    // and the first beginDrag triggers the extrude operation
    bool m_extrudeMode = false;
    bool m_extrudePerformed = false;  // Whether extrude was performed in current drag
    Vec3 m_extrudeNormal = Vec3(0, 1, 0);  // Face normal for extrude direction
    
    // Target
    EditMesh* m_editMesh = nullptr;
    float m_worldMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    
    // Interaction state
    GizmoAxis m_hoveredAxis = GizmoAxis::None;
    GizmoAxis m_activeAxis = GizmoAxis::None;
    bool m_isDragging = false;
    
    // Drag state
    Vec3 m_dragStartPos;
    Vec3 m_dragStartGizmoPos;
    Vec3 m_dragAxis;
    Vec3 m_dragPlaneNormal;
    float m_dragScreenScale = 1.0f;
    float m_dragStartAngle = 0.0f;
    
    // Original positions for current drag (for undo/smooth updates)
    std::vector<std::pair<uint32_t, Vec3>> m_originalPositions;
};

// ============================================================================
// Implementation
// ============================================================================

inline MeshEditGizmo::MeshEditGizmo() = default;

inline bool MeshEditGizmo::hasSelection() const {
    if (!m_editMesh) return false;
    return !m_editMesh->selectedVertices.empty() ||
           !m_editMesh->selectedEdges.empty() ||
           !m_editMesh->selectedFaces.empty();
}

inline Vec3 MeshEditGizmo::getSelectionCenter() const {
    if (!m_editMesh) return Vec3(0, 0, 0);
    
    // Collect all selected vertex indices
    std::set<uint32_t> selectedVerts;
    
    // Direct vertex selection
    selectedVerts.insert(m_editMesh->selectedVertices.begin(), 
                         m_editMesh->selectedVertices.end());
    
    // Vertices from selected edges
    for (uint32_t ei : m_editMesh->selectedEdges) {
        if (ei < m_editMesh->edges.size()) {
            selectedVerts.insert(m_editMesh->edges[ei].v0);
            selectedVerts.insert(m_editMesh->edges[ei].v1);
        }
    }
    
    // Vertices from selected faces
    for (uint32_t fi : m_editMesh->selectedFaces) {
        if (fi < m_editMesh->faces.size()) {
            for (const auto& loop : m_editMesh->faces[fi].loops) {
                selectedVerts.insert(loop.vertexIndex);
            }
        }
    }
    
    if (selectedVerts.empty()) return Vec3(0, 0, 0);
    
    // Calculate center in local space, then transform to world
    Vec3 center(0, 0, 0);
    for (uint32_t vi : selectedVerts) {
        if (vi < m_editMesh->vertices.size()) {
            const auto& v = m_editMesh->vertices[vi];
            center.x += v.position[0];
            center.y += v.position[1];
            center.z += v.position[2];
        }
    }
    center = center * (1.0f / selectedVerts.size());
    
    // Transform to world space
    return transformToWorld(center);
}

inline Vec3 MeshEditGizmo::getGizmoPosition() const {
    return getSelectionCenter();
}

inline Mat4 MeshEditGizmo::getGizmoOrientation() const {
    if (m_space == GizmoSpace::World) {
        return Mat4::identity();
    }
    // Local space: use object's orientation
    Mat4 mat;
    for (int i = 0; i < 16; ++i) mat.m[i] = m_worldMatrix[i];
    // Extract rotation only (zero out translation)
    mat.m[12] = mat.m[13] = mat.m[14] = 0;
    mat.m[15] = 1;
    return mat;
}

inline float MeshEditGizmo::getAxisHitRadius(float screenScale) const {
    return m_size * screenScale * 0.15f;
}

inline Vec3 MeshEditGizmo::transformToWorld(const Vec3& localPos) const {
    float x = m_worldMatrix[0]*localPos.x + m_worldMatrix[4]*localPos.y + m_worldMatrix[8]*localPos.z + m_worldMatrix[12];
    float y = m_worldMatrix[1]*localPos.x + m_worldMatrix[5]*localPos.y + m_worldMatrix[9]*localPos.z + m_worldMatrix[13];
    float z = m_worldMatrix[2]*localPos.x + m_worldMatrix[6]*localPos.y + m_worldMatrix[10]*localPos.z + m_worldMatrix[14];
    return Vec3(x, y, z);
}

inline Vec3 MeshEditGizmo::transformToLocal(const Vec3& worldPos) const {
    // Compute inverse of world matrix (assuming no shear/non-uniform scale for simplicity)
    // For robust implementation, would need proper matrix inversion
    Vec3 t(m_worldMatrix[12], m_worldMatrix[13], m_worldMatrix[14]);
    Vec3 p = worldPos - t;
    
    // Transpose of rotation (inverse for orthogonal matrices)
    float x = m_worldMatrix[0]*p.x + m_worldMatrix[1]*p.y + m_worldMatrix[2]*p.z;
    float y = m_worldMatrix[4]*p.x + m_worldMatrix[5]*p.y + m_worldMatrix[6]*p.z;
    float z = m_worldMatrix[8]*p.x + m_worldMatrix[9]*p.y + m_worldMatrix[10]*p.z;
    return Vec3(x, y, z);
}

inline Vec3 MeshEditGizmo::projectOntoAxis(const luma::Ray& ray, const Vec3& axisOrigin, const Vec3& axisDir) {
    // Standard closest-point-on-two-lines formula
    // Line 1 (axis): P(s) = axisOrigin + s * axisDir
    // Line 2 (ray):  Q(t) = rayOrigin + t * rayDir
    // w0 = axisOrigin - rayOrigin  (P0 - Q0, standard convention)
    Vec3 rayOrigin = ray.origin;
    Vec3 rayDir = ray.direction;
    Vec3 w0 = axisOrigin - rayOrigin;
    float a = axisDir.dot(axisDir);
    float b = axisDir.dot(rayDir);
    float c = rayDir.dot(rayDir);
    float d = axisDir.dot(w0);
    float e = rayDir.dot(w0);
    
    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f) return axisOrigin;
    
    // s = closest parameter on axis line
    float s = (b * e - c * d) / denom;
    return axisOrigin + axisDir * s;
}

inline Vec3 MeshEditGizmo::projectOntoPlane(const luma::Ray& ray, const Vec3& planeOrigin, const Vec3& planeNormal) {
    Vec3 rayDir = ray.direction;
    float denom = planeNormal.dot(rayDir);
    if (std::abs(denom) < 1e-6f) return planeOrigin;
    
    Vec3 p0l0 = planeOrigin - ray.origin;
    float t = planeNormal.dot(p0l0) / denom;
    return ray.at(t);
}

inline void MeshEditGizmo::storeOriginalPositions() {
    m_originalPositions.clear();
    if (!m_editMesh) return;
    
    // Collect all affected vertices
    std::set<uint32_t> affectedVerts;
    affectedVerts.insert(m_editMesh->selectedVertices.begin(),
                         m_editMesh->selectedVertices.end());
    
    for (uint32_t ei : m_editMesh->selectedEdges) {
        if (ei < m_editMesh->edges.size()) {
            affectedVerts.insert(m_editMesh->edges[ei].v0);
            affectedVerts.insert(m_editMesh->edges[ei].v1);
        }
    }
    
    for (uint32_t fi : m_editMesh->selectedFaces) {
        if (fi < m_editMesh->faces.size()) {
            for (const auto& loop : m_editMesh->faces[fi].loops) {
                affectedVerts.insert(loop.vertexIndex);
            }
        }
    }
    
    // Store positions
    for (uint32_t vi : affectedVerts) {
        if (vi < m_editMesh->vertices.size()) {
            const auto& v = m_editMesh->vertices[vi];
            m_originalPositions.push_back({vi, Vec3(v.position[0], v.position[1], v.position[2])});
        }
    }
}

inline void MeshEditGizmo::restoreOriginalPositions() {
    if (!m_editMesh) return;
    for (const auto& [vi, pos] : m_originalPositions) {
        if (vi < m_editMesh->vertices.size()) {
            m_editMesh->vertices[vi].position[0] = pos.x;
            m_editMesh->vertices[vi].position[1] = pos.y;
            m_editMesh->vertices[vi].position[2] = pos.z;
        }
    }
}

inline GizmoAxis MeshEditGizmo::testHover(const luma::Ray& ray, float screenScale) {
    if (!hasSelection()) {
        m_hoveredAxis = GizmoAxis::None;
        return GizmoAxis::None;
    }
    
    Vec3 pos = getGizmoPosition();
    Mat4 orient = getGizmoOrientation();
    float axisLen = m_size * screenScale;
    // Use generous hit radius for easier interaction
    float hitRadius = m_size * screenScale * 0.2f;
    
    // Get axis directions (unit vectors)
    Vec3 xAxis(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 yAxis(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 zAxis(orient.m[8], orient.m[9], orient.m[10]);
    
    Vec3 ro = ray.origin;
    Vec3 rd = ray.direction.normalized();
    
    // Helper: closest distance from a ray to a line segment
    // Segment: P(s) = segStart + s * segDir, s in [0, segLen]
    // Ray:     Q(t) = ro + t * rd, t >= 0
    // Uses standard closest-point-between-two-lines formula
    auto distRayToSegment = [&](const Vec3& segStart, const Vec3& segDir, float segLen, float& tOnSeg) -> float {
        // w0 = segStart - ro (standard convention: P0 - Q0)
        Vec3 w0 = segStart - ro;
        float a = segDir.dot(segDir);   // |u|^2
        float b = segDir.dot(rd);       // u . v
        float c = rd.dot(rd);           // |v|^2
        float d = segDir.dot(w0);       // u . w0
        float e = rd.dot(w0);           // v . w0
        
        float denom = a * c - b * b;
        
        float s, t;
        if (std::abs(denom) < 1e-8f) {
            // Lines are nearly parallel
            s = 0.0f;
            t = e / c;
        } else {
            s = (b * e - c * d) / denom;  // parameter on segment line
            t = (a * e - b * d) / denom;  // parameter on ray line
        }
        
        // Clamp s to segment [0, segLen], then recompute t
        if (s < 0.0f) {
            s = 0.0f;
            t = e / c;
        } else if (s > segLen) {
            s = segLen;
            Vec3 w1 = (segStart + segDir * segLen) - ro;
            t = rd.dot(w1) / c;
        }
        
        // Clamp t to ray (t >= 0)
        if (t < 0.0f) t = 0.0f;
        
        Vec3 closestOnSeg = segStart + segDir * s;
        Vec3 closestOnRay = ro + rd * t;
        
        tOnSeg = (segLen > 0.0001f) ? (s / segLen) : 0.0f;
        return (closestOnSeg - closestOnRay).length();
    };
    
    // Test each axis - find the closest one within hit radius
    struct AxisTest { GizmoAxis axis; float dist; };
    AxisTest bestHit = { GizmoAxis::None, hitRadius };
    
    if (m_extrudeMode) {
        // Extrude mode: only test the normal direction handle
        // Map it to XYZ axis (used for unconstrained/normal movement)
        Vec3 normalDir = m_extrudeNormal.normalized();
        float extrudeLen = axisLen * 1.2f;
        float extrudeHitRadius = hitRadius * 2.0f;  // Generous hit area
        
        // Test center sphere
        float centerRadius = hitRadius * 1.5f;
        Vec3 toCenter = pos - ro;
        float tCenter = toCenter.dot(rd);
        if (tCenter > 0.0f) {
            Vec3 closest = ro + rd * tCenter;
            if ((closest - pos).length() < centerRadius) {
                m_hoveredAxis = GizmoAxis::XYZ;
                return GizmoAxis::XYZ;
            }
        }
        
        // Test normal direction line
        float tOnSeg = 0.0f;
        float dist = distRayToSegment(pos, normalDir, extrudeLen, tOnSeg);
        if (dist < extrudeHitRadius && tOnSeg > 0.0f) {
            m_hoveredAxis = GizmoAxis::XYZ;
            return GizmoAxis::XYZ;
        }
        
        m_hoveredAxis = GizmoAxis::None;
        return GizmoAxis::None;
    }
    
    // Test center sphere first (non-extrude modes)
    float centerRadius = hitRadius * 1.5f;
    Vec3 toCenter = pos - ro;
    float tCenter = toCenter.dot(rd);
    if (tCenter > 0.0f) {
        Vec3 closest = ro + rd * tCenter;
        if ((closest - pos).length() < centerRadius) {
            m_hoveredAxis = GizmoAxis::XYZ;
            return GizmoAxis::XYZ;
        }
    }
    
    if (m_mode == GizmoMode::Rotate) {
        // Rotate mode: test circles (ray-plane intersection, check distance to ring)
        float circleRadius = axisLen * 0.9f;
        float ringThickness = hitRadius * 1.5f;
        
        auto testCircle = [&](const Vec3& normal, GizmoAxis axisId) {
            float denom = normal.dot(rd);
            if (std::abs(denom) < 1e-6f) return;  // Ray parallel to plane
            
            Vec3 p0l0 = pos - ro;
            float t = normal.dot(p0l0) / denom;
            if (t < 0.0f) return;  // Behind camera
            
            Vec3 hitPoint = ro + rd * t;
            float distToCenter = (hitPoint - pos).length();
            float distToRing = std::abs(distToCenter - circleRadius);
            
            if (distToRing < ringThickness && distToRing < bestHit.dist) {
                bestHit = { axisId, distToRing };
            }
        };
        
        testCircle(xAxis, GizmoAxis::X);
        testCircle(yAxis, GizmoAxis::Y);
        testCircle(zAxis, GizmoAxis::Z);
    } else {
        // Translate/Scale mode: test axis lines
        auto testAxis = [&](const Vec3& dir, GizmoAxis axisId) {
            float tOnSeg = 0.0f;
            float dist = distRayToSegment(pos, dir, axisLen, tOnSeg);
            if (dist < bestHit.dist && tOnSeg > 0.05f && tOnSeg <= 1.0f) {
                bestHit = { axisId, dist };
            }
        };
        
        testAxis(xAxis, GizmoAxis::X);
        testAxis(yAxis, GizmoAxis::Y);
        testAxis(zAxis, GizmoAxis::Z);
    }
    
    m_hoveredAxis = bestHit.axis;
    return m_hoveredAxis;
}

inline bool MeshEditGizmo::beginDrag(const luma::Ray& ray, float screenScale) {
    GizmoAxis axis = testHover(ray, screenScale);
    if (axis == GizmoAxis::None) return false;
    
    m_activeAxis = axis;
    m_isDragging = true;
    m_dragScreenScale = screenScale;
    m_dragStartGizmoPos = getGizmoPosition();
    
    Mat4 orient = getGizmoOrientation();
    Vec3 xAxis(orient.m[0], orient.m[1], orient.m[2]);
    Vec3 yAxis(orient.m[4], orient.m[5], orient.m[6]);
    Vec3 zAxis(orient.m[8], orient.m[9], orient.m[10]);
    
    // Set drag axis/plane based on mode and axis
    if (m_extrudeMode && axis == GizmoAxis::XYZ) {
        // Extrude mode: constrain to face normal direction
        m_dragAxis = m_extrudeNormal.normalized();
        m_dragPlaneNormal = (ray.origin - m_dragStartGizmoPos).normalized();
    } else {
        switch (axis) {
            case GizmoAxis::X:
                m_dragAxis = xAxis;
                m_dragPlaneNormal = yAxis;
                break;
            case GizmoAxis::Y:
                m_dragAxis = yAxis;
                m_dragPlaneNormal = xAxis;
                break;
            case GizmoAxis::Z:
                m_dragAxis = zAxis;
                m_dragPlaneNormal = yAxis;
                break;
            case GizmoAxis::XYZ:
                m_dragAxis = Vec3(1, 1, 1);
                m_dragPlaneNormal = (ray.origin - m_dragStartGizmoPos).normalized();
                break;
            default:
                break;
        }
    }
    
    // Store initial drag position
    if (m_mode == GizmoMode::Translate) {
        if (m_extrudeMode || axis != GizmoAxis::XYZ) {
            // Extrude mode or single axis: project onto the axis line
            m_dragStartPos = projectOntoAxis(ray, m_dragStartGizmoPos, m_dragAxis);
        } else {
            // XYZ free mode: project onto camera-facing plane
            m_dragStartPos = projectOntoPlane(ray, m_dragStartGizmoPos, m_dragPlaneNormal);
        }
    } else if (m_mode == GizmoMode::Rotate) {
        Vec3 toMouse = projectOntoPlane(ray, m_dragStartGizmoPos, m_dragAxis) - m_dragStartGizmoPos;
        m_dragStartAngle = atan2f(toMouse.dot(m_dragPlaneNormal.cross(m_dragAxis)), 
                                  toMouse.dot(m_dragPlaneNormal));
    } else { // Scale
        m_dragStartPos = projectOntoPlane(ray, m_dragStartGizmoPos, m_dragPlaneNormal);
    }
    
    // Push undo state BEFORE the drag begins (so undo restores pre-drag state)
    // Skip if in extrude mode — extrude already pushed undo for the combined operation
    if (m_editMesh && !m_extrudeMode) {
        m_editMesh->pushUndo();
    }
    
    // Store original vertex positions
    storeOriginalPositions();
    
    return true;
}

inline bool MeshEditGizmo::updateDrag(const luma::Ray& dragRay) {
    if (!m_isDragging || !m_editMesh) return false;
    
    // First restore original positions
    restoreOriginalPositions();
    
    if (m_mode == GizmoMode::Translate) {
        Vec3 currentPos;
        if (m_extrudeMode || m_activeAxis != GizmoAxis::XYZ) {
            // Extrude mode or single axis: project onto the axis line
            currentPos = projectOntoAxis(dragRay, m_dragStartGizmoPos, m_dragAxis);
        } else {
            // XYZ free mode: project onto camera-facing plane
            currentPos = projectOntoPlane(dragRay, m_dragStartGizmoPos, m_dragPlaneNormal);
        }
        
        Vec3 delta = currentPos - m_dragStartPos;
        applyTranslation(delta);
    } else if (m_mode == GizmoMode::Rotate) {
        Vec3 toMouse = projectOntoPlane(dragRay, m_dragStartGizmoPos, m_dragAxis) - m_dragStartGizmoPos;
        float currentAngle = atan2f(toMouse.dot(m_dragPlaneNormal.cross(m_dragAxis)),
                                    toMouse.dot(m_dragPlaneNormal));
        float deltaAngle = currentAngle - m_dragStartAngle;
        applyRotation(m_dragAxis, deltaAngle);
    } else { // Scale
        Vec3 currentPos = projectOntoPlane(dragRay, m_dragStartGizmoPos, m_dragPlaneNormal);
        float startDist = (m_dragStartPos - m_dragStartGizmoPos).length();
        float currentDist = (currentPos - m_dragStartGizmoPos).length();
        
        if (startDist > 0.001f) {
            float scaleFactor = currentDist / startDist;
            Vec3 scale(1, 1, 1);
            
            if (m_activeAxis == GizmoAxis::X || m_activeAxis == GizmoAxis::XYZ)
                scale.x = scaleFactor;
            if (m_activeAxis == GizmoAxis::Y || m_activeAxis == GizmoAxis::XYZ)
                scale.y = scaleFactor;
            if (m_activeAxis == GizmoAxis::Z || m_activeAxis == GizmoAxis::XYZ)
                scale.z = scaleFactor;
            
            applyScale(scale);
        }
    }
    
    if (onMeshChanged) onMeshChanged();
    return true;
}

inline void MeshEditGizmo::endDrag() {
    // Note: undo state was pushed in beginDrag() before any modifications
    m_isDragging = false;
    m_activeAxis = GizmoAxis::None;
    m_originalPositions.clear();
}

inline void MeshEditGizmo::applyTranslation(const Vec3& worldDelta) {
    if (!m_editMesh) return;
    
    // Transform delta to local space
    Vec3 localDelta = transformToLocal(worldDelta + Vec3(m_worldMatrix[12], m_worldMatrix[13], m_worldMatrix[14])) -
                      transformToLocal(Vec3(m_worldMatrix[12], m_worldMatrix[13], m_worldMatrix[14]));
    
    for (const auto& [vi, origPos] : m_originalPositions) {
        if (vi < m_editMesh->vertices.size()) {
            m_editMesh->vertices[vi].position[0] = origPos.x + localDelta.x;
            m_editMesh->vertices[vi].position[1] = origPos.y + localDelta.y;
            m_editMesh->vertices[vi].position[2] = origPos.z + localDelta.z;
        }
    }
}

inline void MeshEditGizmo::applyRotation(const Vec3& worldAxis, float angle) {
    if (!m_editMesh || m_originalPositions.empty()) return;
    
    // Rotation center in local space
    Vec3 localCenter = transformToLocal(m_dragStartGizmoPos);
    
    // Transform axis to local space (direction only)
    Vec3 localAxis = transformToLocal(m_dragStartGizmoPos + worldAxis) - localCenter;
    localAxis = localAxis.normalized();
    
    // Rodrigues' rotation formula
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    
    for (const auto& [vi, origPos] : m_originalPositions) {
        if (vi < m_editMesh->vertices.size()) {
            Vec3 p = origPos - localCenter;
            
            // Rotate p around localAxis by angle
            Vec3 rotated = p * cosA + 
                           localAxis.cross(p) * sinA + 
                           localAxis * localAxis.dot(p) * (1 - cosA);
            
            Vec3 newPos = rotated + localCenter;
            m_editMesh->vertices[vi].position[0] = newPos.x;
            m_editMesh->vertices[vi].position[1] = newPos.y;
            m_editMesh->vertices[vi].position[2] = newPos.z;
        }
    }
}

inline void MeshEditGizmo::applyScale(const Vec3& scale) {
    if (!m_editMesh || m_originalPositions.empty()) return;
    
    // Scale center in local space
    Vec3 localCenter = transformToLocal(m_dragStartGizmoPos);
    
    for (const auto& [vi, origPos] : m_originalPositions) {
        if (vi < m_editMesh->vertices.size()) {
            Vec3 p = origPos - localCenter;
            
            // Apply scale relative to center
            Vec3 scaled(p.x * scale.x, p.y * scale.y, p.z * scale.z);
            Vec3 newPos = scaled + localCenter;
            
            m_editMesh->vertices[vi].position[0] = newPos.x;
            m_editMesh->vertices[vi].position[1] = newPos.y;
            m_editMesh->vertices[vi].position[2] = newPos.z;
        }
    }
}

inline GizmoRenderData MeshEditGizmo::generateRenderData(float screenScale) const {
    GizmoRenderData data;
    
    if (!hasSelection()) return data;
    
    Vec3 pos = getGizmoPosition();
    data.position = pos;
    data.orientation = getGizmoOrientation();
    data.size = m_size * screenScale;
    data.hoveredAxis = m_hoveredAxis;
    data.activeAxis = m_activeAxis;
    
    float axisLen = data.size;
    
    // Simple axis lines (world space for now)
    Vec3 axisX(1, 0, 0);
    Vec3 axisY(0, 1, 0);
    Vec3 axisZ(0, 0, 1);
    
    // Helper to get axis color (with hover/active highlight)
    auto getAxisColor = [this](GizmoAxis axis, const float* baseColor) -> const float* {
        if (m_activeAxis == axis) return m_colors.active;
        if (m_hoveredAxis == axis) return m_colors.hover;
        return baseColor;
    };
    
    // =============================================================
    // Professional Gizmo Design (Blender/Maya style)
    // =============================================================
    
    // Helper to generate thick line by rendering multiple offset lines
    auto addThickLine = [&](const Vec3& start, const Vec3& end, const float* color, float thickness) {
        Vec3 dir = (end - start);
        float len = dir.length();
        if (len < 0.0001f) return;
        dir = dir * (1.0f / len);
        
        Vec3 perp1, perp2;
        if (std::abs(dir.y) < 0.9f) {
            perp1 = Vec3(-dir.z, 0, dir.x).normalized();
        } else {
            perp1 = Vec3(1, 0, 0);
        }
        perp2 = dir.cross(perp1).normalized();
        
        int numLines = 5;
        for (int i = 0; i < numLines; i++) {
            float offset = (float)(i - numLines/2) * thickness / numLines;
            Vec3 offsetVec = perp1 * offset;
            data.lines.push_back({
                start + offsetVec,
                end + offsetVec,
                {color[0], color[1], color[2], color[3]}
            });
            data.lines.push_back({
                start + perp2 * offset,
                end + perp2 * offset,
                {color[0], color[1], color[2], color[3]}
            });
        }
    };
    
    // Helper to draw cone/arrowhead
    auto addArrowhead = [&](const Vec3& tip, const Vec3& dir, const Vec3& perp1, const Vec3& perp2,
                            const float* color, float coneLen, float coneRadius) {
        Vec3 base = tip - dir * coneLen;
        int segments = 8;
        
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 6.28318f;
            float a2 = (float)(i + 1) / segments * 6.28318f;
            
            Vec3 p1 = base + perp1 * (std::cos(a1) * coneRadius) + perp2 * (std::sin(a1) * coneRadius);
            Vec3 p2 = base + perp1 * (std::cos(a2) * coneRadius) + perp2 * (std::sin(a2) * coneRadius);
            
            data.lines.push_back({tip, p1, {color[0], color[1], color[2], color[3]}});
            data.lines.push_back({p1, p2, {color[0], color[1], color[2], color[3]}});
        }
    };
    
    // Helper to draw scale box
    auto addScaleBox = [&](const Vec3& center, const Vec3& dir, const Vec3& perp1, const Vec3& perp2,
                           const float* color, float boxSize) {
        Vec3 corners[8];
        float hs = boxSize * 0.5f;
        corners[0] = center - perp1*hs - perp2*hs - dir*hs;
        corners[1] = center + perp1*hs - perp2*hs - dir*hs;
        corners[2] = center + perp1*hs + perp2*hs - dir*hs;
        corners[3] = center - perp1*hs + perp2*hs - dir*hs;
        corners[4] = center - perp1*hs - perp2*hs + dir*hs;
        corners[5] = center + perp1*hs - perp2*hs + dir*hs;
        corners[6] = center + perp1*hs + perp2*hs + dir*hs;
        corners[7] = center - perp1*hs + perp2*hs + dir*hs;
        
        int edges[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges) {
            data.lines.push_back({corners[e[0]], corners[e[1]], {color[0], color[1], color[2], color[3]}});
        }
    };
    
    // Helper to draw rotation circle
    auto addRotationCircle = [&](const Vec3& center, const Vec3& normal, const Vec3& perp1, const Vec3& perp2,
                                 const float* color, float radius) {
        int segments = 32;
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 6.28318f;
            float a2 = (float)(i + 1) / segments * 6.28318f;
            
            Vec3 p1 = center + perp1 * (std::cos(a1) * radius) + perp2 * (std::sin(a1) * radius);
            Vec3 p2 = center + perp1 * (std::cos(a2) * radius) + perp2 * (std::sin(a2) * radius);
            
            data.lines.push_back({p1, p2, {color[0], color[1], color[2], color[3]}});
        }
    };
    
    float thickness = axisLen * 0.02f;
    float coneLen = axisLen * 0.2f;
    float coneRadius = axisLen * 0.06f;
    float boxSize = axisLen * 0.12f;
    
    if (m_extrudeMode) {
        // =============================================================
        // Extrude Mode: Only show normal direction handle (Maya/Blender style)
        // =============================================================
        Vec3 normalDir = m_extrudeNormal.normalized();
        float extrudeLen = axisLen * 1.2f;
        Vec3 normalEnd = pos + normalDir * extrudeLen;
        
        // Perpendicular vectors for arrowhead
        Vec3 perp1, perp2;
        if (std::abs(normalDir.y) < 0.9f) {
            perp1 = Vec3(-normalDir.z, 0, normalDir.x).normalized();
        } else {
            perp1 = Vec3(1, 0, 0);
        }
        perp2 = normalDir.cross(perp1).normalized();
        
        // Normal direction arrow (gold/yellow, prominent)
        float normalColor[4] = {1.0f, 0.85f, 0.0f, 1.0f};
        if (m_hoveredAxis == GizmoAxis::XYZ || m_activeAxis == GizmoAxis::XYZ) {
            normalColor[0] = 1.0f; normalColor[1] = 1.0f; normalColor[2] = 0.4f;
        }
        
        // Thick shaft
        Vec3 shaftEnd = pos + normalDir * (extrudeLen - coneLen * 1.2f);
        addThickLine(pos, shaftEnd, normalColor, thickness * 2.5f);
        
        // Arrowhead
        addArrowhead(normalEnd, normalDir, perp1, perp2, normalColor, 
                     coneLen * 1.3f, coneRadius * 1.3f);
        
        // Small ring at base to indicate "extrude" (like Maya)
        float ringColor[4] = {1.0f, 0.7f, 0.0f, 0.8f};
        addRotationCircle(pos, normalDir, perp1, perp2, ringColor, axisLen * 0.15f);
        
    } else {
        // =============================================================
        // Standard Gizmo (Translate / Rotate / Scale)
        // =============================================================
        
        // === X Axis ===
        {
            const float* color = getAxisColor(GizmoAxis::X, m_colors.xAxis);
            Vec3 end = pos + axisX * axisLen;
            Vec3 perpY = axisY, perpZ = axisZ;
            
            if (m_mode == GizmoMode::Translate) {
                Vec3 shaftEnd = pos + axisX * (axisLen - coneLen);
                addThickLine(pos, shaftEnd, color, thickness);
                addArrowhead(end, axisX, perpY, perpZ, color, coneLen, coneRadius);
            } else if (m_mode == GizmoMode::Scale) {
                addThickLine(pos, end, color, thickness);
                addScaleBox(end, axisX, perpY, perpZ, color, boxSize);
            } else { // Rotate
                addRotationCircle(pos, axisX, axisY, axisZ, color, axisLen * 0.9f);
            }
        }
        
        // === Y Axis ===
        {
            const float* color = getAxisColor(GizmoAxis::Y, m_colors.yAxis);
            Vec3 end = pos + axisY * axisLen;
            Vec3 perpX = axisX, perpZ = axisZ;
            
            if (m_mode == GizmoMode::Translate) {
                Vec3 shaftEnd = pos + axisY * (axisLen - coneLen);
                addThickLine(pos, shaftEnd, color, thickness);
                addArrowhead(end, axisY, perpZ, perpX, color, coneLen, coneRadius);
            } else if (m_mode == GizmoMode::Scale) {
                addThickLine(pos, end, color, thickness);
                addScaleBox(end, axisY, perpZ, perpX, color, boxSize);
            } else { // Rotate
                addRotationCircle(pos, axisY, axisZ, axisX, color, axisLen * 0.9f);
            }
        }
        
        // === Z Axis ===
        {
            const float* color = getAxisColor(GizmoAxis::Z, m_colors.zAxis);
            Vec3 end = pos + axisZ * axisLen;
            Vec3 perpX = axisX, perpY = axisY;
            
            if (m_mode == GizmoMode::Translate) {
                Vec3 shaftEnd = pos + axisZ * (axisLen - coneLen);
                addThickLine(pos, shaftEnd, color, thickness);
                addArrowhead(end, axisZ, perpX, perpY, color, coneLen, coneRadius);
            } else if (m_mode == GizmoMode::Scale) {
                addThickLine(pos, end, color, thickness);
                addScaleBox(end, axisZ, perpX, perpY, color, boxSize);
            } else { // Rotate
                addRotationCircle(pos, axisZ, axisX, axisY, color, axisLen * 0.9f);
            }
        }
        
        // === Center (XYZ) Handle ===
        {
            const float* color = getAxisColor(GizmoAxis::XYZ, m_colors.center);
            float centerSize = axisLen * 0.15f;
            
            // Draw small cube at center
            addScaleBox(pos, axisX, axisY, axisZ, color, centerSize);
        }
    }
    
    return data;
}

} // namespace editor
} // namespace luma
