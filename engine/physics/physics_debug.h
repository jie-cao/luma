// Physics Debug Renderer - Visualize colliders, constraints, contacts
// Generates line data for rendering
#pragma once

#include "physics_world.h"
#include "constraints.h"
#include <vector>
#include <cmath>

namespace luma {

// ===== Debug Line =====
struct DebugLine {
    Vec3 start;
    Vec3 end;
    Vec4 color;
    
    DebugLine() = default;
    DebugLine(const Vec3& s, const Vec3& e, const Vec4& c)
        : start(s), end(e), color(c) {}
};

// ===== Debug Colors =====
namespace DebugColors {
    constexpr Vec4 StaticCollider = {0.5f, 0.5f, 0.5f, 1.0f};    // Gray
    constexpr Vec4 DynamicCollider = {0.2f, 0.8f, 0.2f, 1.0f};   // Green
    constexpr Vec4 KinematicCollider = {0.2f, 0.2f, 0.8f, 1.0f}; // Blue
    constexpr Vec4 SleepingCollider = {0.4f, 0.4f, 0.6f, 1.0f};  // Purple-gray
    constexpr Vec4 TriggerCollider = {1.0f, 1.0f, 0.0f, 0.5f};   // Yellow (transparent)
    
    constexpr Vec4 AABB = {1.0f, 0.5f, 0.0f, 0.5f};              // Orange
    constexpr Vec4 ContactPoint = {1.0f, 0.0f, 0.0f, 1.0f};      // Red
    constexpr Vec4 ContactNormal = {1.0f, 0.5f, 0.5f, 1.0f};     // Light red
    
    constexpr Vec4 LinearVelocity = {0.0f, 1.0f, 1.0f, 1.0f};    // Cyan
    constexpr Vec4 AngularVelocity = {1.0f, 0.0f, 1.0f, 1.0f};   // Magenta
    
    constexpr Vec4 ConstraintOK = {0.0f, 1.0f, 0.0f, 1.0f};      // Green
    constexpr Vec4 ConstraintStressed = {1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
    constexpr Vec4 ConstraintBroken = {1.0f, 0.0f, 0.0f, 1.0f};   // Red
    
    constexpr Vec4 RaycastHit = {1.0f, 0.0f, 0.0f, 1.0f};        // Red
    constexpr Vec4 RaycastMiss = {0.5f, 0.5f, 0.5f, 0.5f};       // Gray
}

// ===== Physics Debug Renderer =====
class PhysicsDebugRenderer {
public:
    PhysicsDebugRenderer() = default;
    
    // Settings
    void setDrawColliders(bool draw) { drawColliders_ = draw; }
    void setDrawAABBs(bool draw) { drawAABBs_ = draw; }
    void setDrawContacts(bool draw) { drawContacts_ = draw; }
    void setDrawConstraints(bool draw) { drawConstraints_ = draw; }
    void setDrawVelocities(bool draw) { drawVelocities_ = draw; }
    void setVelocityScale(float scale) { velocityScale_ = scale; }
    
    // Generate debug lines for current physics state
    void update(const PhysicsWorld& world, const ConstraintManager& constraints) {
        lines_.clear();
        
        // Draw colliders
        if (drawColliders_) {
            for (const auto& body : world.getBodies()) {
                drawCollider(body.get());
            }
        }
        
        // Draw AABBs
        if (drawAABBs_) {
            for (const auto& body : world.getBodies()) {
                drawAABB(body->getAABB());
            }
        }
        
        // Draw contacts
        if (drawContacts_) {
            for (const auto& contact : world.getCollisions()) {
                drawContact(contact);
            }
        }
        
        // Draw constraints
        if (drawConstraints_) {
            for (const auto& constraint : constraints.getConstraints()) {
                drawConstraint(constraint.get());
            }
        }
        
        // Draw velocities
        if (drawVelocities_) {
            for (const auto& body : world.getBodies()) {
                if (body->getType() != RigidBodyType::Static) {
                    drawVelocity(body.get());
                }
            }
        }
    }
    
    // Get generated lines
    const std::vector<DebugLine>& getLines() const { return lines_; }
    
    // Convert to flat array for rendering (startX, startY, startZ, endX, endY, endZ, r, g, b, a)
    std::vector<float> getLineData() const {
        std::vector<float> data;
        data.reserve(lines_.size() * 10);
        
        for (const auto& line : lines_) {
            data.push_back(line.start.x);
            data.push_back(line.start.y);
            data.push_back(line.start.z);
            data.push_back(line.end.x);
            data.push_back(line.end.y);
            data.push_back(line.end.z);
            data.push_back(line.color.x);
            data.push_back(line.color.y);
            data.push_back(line.color.z);
            data.push_back(line.color.w);
        }
        
        return data;
    }
    
    size_t getLineCount() const { return lines_.size(); }
    
private:
    void drawCollider(const RigidBody* body) {
        if (!body || !body->getCollider()) return;
        
        Vec4 color;
        if (body->getCollider()->isTrigger()) {
            color = DebugColors::TriggerCollider;
        } else if (body->isSleeping()) {
            color = DebugColors::SleepingCollider;
        } else {
            switch (body->getType()) {
                case RigidBodyType::Static: color = DebugColors::StaticCollider; break;
                case RigidBodyType::Dynamic: color = DebugColors::DynamicCollider; break;
                case RigidBodyType::Kinematic: color = DebugColors::KinematicCollider; break;
            }
        }
        
        const Collider* col = body->getCollider();
        Vec3 pos = body->getPosition() + body->getRotation().rotate(col->getOffset());
        Quat rot = body->getRotation() * col->getRotation();
        
        switch (col->getType()) {
            case ColliderType::Sphere:
                drawSphere(pos, col->asSphere().radius, color);
                break;
            case ColliderType::Box:
                drawBox(pos, col->asBox().halfExtents, rot, color);
                break;
            case ColliderType::Capsule:
                drawCapsule(pos, col->asCapsule().radius, col->asCapsule().height, rot, color);
                break;
            case ColliderType::Plane:
                drawPlane(pos, col->asPlane().normal, color);
                break;
            default:
                break;
        }
    }
    
    void drawSphere(const Vec3& center, float radius, const Vec4& color) {
        const int segments = 16;
        
        // XY circle
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * 3.14159f;
            float a2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
            
            Vec3 p1(center.x + cosf(a1) * radius, center.y + sinf(a1) * radius, center.z);
            Vec3 p2(center.x + cosf(a2) * radius, center.y + sinf(a2) * radius, center.z);
            lines_.push_back({p1, p2, color});
        }
        
        // XZ circle
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * 3.14159f;
            float a2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
            
            Vec3 p1(center.x + cosf(a1) * radius, center.y, center.z + sinf(a1) * radius);
            Vec3 p2(center.x + cosf(a2) * radius, center.y, center.z + sinf(a2) * radius);
            lines_.push_back({p1, p2, color});
        }
        
        // YZ circle
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * 3.14159f;
            float a2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
            
            Vec3 p1(center.x, center.y + cosf(a1) * radius, center.z + sinf(a1) * radius);
            Vec3 p2(center.x, center.y + cosf(a2) * radius, center.z + sinf(a2) * radius);
            lines_.push_back({p1, p2, color});
        }
    }
    
    void drawBox(const Vec3& center, const Vec3& halfExtents, const Quat& rotation, const Vec4& color) {
        // 8 corners
        Vec3 corners[8];
        for (int i = 0; i < 8; i++) {
            Vec3 local(
                (i & 1) ? halfExtents.x : -halfExtents.x,
                (i & 2) ? halfExtents.y : -halfExtents.y,
                (i & 4) ? halfExtents.z : -halfExtents.z
            );
            corners[i] = center + rotation.rotate(local);
        }
        
        // 12 edges
        // Bottom face
        lines_.push_back({corners[0], corners[1], color});
        lines_.push_back({corners[1], corners[3], color});
        lines_.push_back({corners[3], corners[2], color});
        lines_.push_back({corners[2], corners[0], color});
        
        // Top face
        lines_.push_back({corners[4], corners[5], color});
        lines_.push_back({corners[5], corners[7], color});
        lines_.push_back({corners[7], corners[6], color});
        lines_.push_back({corners[6], corners[4], color});
        
        // Vertical edges
        lines_.push_back({corners[0], corners[4], color});
        lines_.push_back({corners[1], corners[5], color});
        lines_.push_back({corners[2], corners[6], color});
        lines_.push_back({corners[3], corners[7], color});
    }
    
    void drawCapsule(const Vec3& center, float radius, float height, const Quat& rotation, const Vec4& color) {
        float halfHeight = (height - 2.0f * radius) * 0.5f;
        Vec3 up = rotation.rotate(Vec3(0, 1, 0));
        Vec3 top = center + up * halfHeight;
        Vec3 bottom = center - up * halfHeight;
        
        // Draw hemispheres at ends
        const int segments = 12;
        
        // Calculate local right and forward vectors
        Vec3 right = rotation.rotate(Vec3(1, 0, 0));
        Vec3 forward = rotation.rotate(Vec3(0, 0, 1));
        
        // Vertical lines connecting hemispheres
        for (int i = 0; i < 4; i++) {
            float angle = (float)i / 4 * 2.0f * 3.14159f;
            Vec3 offset = right * cosf(angle) * radius + forward * sinf(angle) * radius;
            lines_.push_back({top + offset, bottom + offset, color});
        }
        
        // Top hemisphere circles
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * 3.14159f;
            float a2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
            
            Vec3 p1 = top + right * cosf(a1) * radius + forward * sinf(a1) * radius;
            Vec3 p2 = top + right * cosf(a2) * radius + forward * sinf(a2) * radius;
            lines_.push_back({p1, p2, color});
        }
        
        // Bottom hemisphere circles
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * 3.14159f;
            float a2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
            
            Vec3 p1 = bottom + right * cosf(a1) * radius + forward * sinf(a1) * radius;
            Vec3 p2 = bottom + right * cosf(a2) * radius + forward * sinf(a2) * radius;
            lines_.push_back({p1, p2, color});
        }
        
        // Top half circles
        for (int i = 0; i < segments / 2; i++) {
            float a1 = (float)i / segments * 3.14159f;
            float a2 = (float)(i + 1) / segments * 3.14159f;
            
            Vec3 p1 = top + up * sinf(a1) * radius + right * cosf(a1) * radius;
            Vec3 p2 = top + up * sinf(a2) * radius + right * cosf(a2) * radius;
            lines_.push_back({p1, p2, color});
            
            p1 = top + up * sinf(a1) * radius + forward * cosf(a1) * radius;
            p2 = top + up * sinf(a2) * radius + forward * cosf(a2) * radius;
            lines_.push_back({p1, p2, color});
        }
        
        // Bottom half circles
        for (int i = 0; i < segments / 2; i++) {
            float a1 = (float)i / segments * 3.14159f;
            float a2 = (float)(i + 1) / segments * 3.14159f;
            
            Vec3 p1 = bottom - up * sinf(a1) * radius + right * cosf(a1) * radius;
            Vec3 p2 = bottom - up * sinf(a2) * radius + right * cosf(a2) * radius;
            lines_.push_back({p1, p2, color});
            
            p1 = bottom - up * sinf(a1) * radius + forward * cosf(a1) * radius;
            p2 = bottom - up * sinf(a2) * radius + forward * cosf(a2) * radius;
            lines_.push_back({p1, p2, color});
        }
    }
    
    void drawPlane(const Vec3& center, const Vec3& normal, const Vec4& color) {
        // Draw a grid on the plane
        const float size = 10.0f;
        const int divisions = 10;
        
        // Calculate tangent vectors
        Vec3 tangent, bitangent;
        if (std::abs(normal.y) < 0.9f) {
            tangent = normal.cross(Vec3(0, 1, 0)).normalized();
        } else {
            tangent = normal.cross(Vec3(1, 0, 0)).normalized();
        }
        bitangent = normal.cross(tangent);
        
        // Grid lines
        for (int i = -divisions; i <= divisions; i++) {
            float t = (float)i / divisions * size;
            
            Vec3 start1 = center + tangent * t - bitangent * size;
            Vec3 end1 = center + tangent * t + bitangent * size;
            lines_.push_back({start1, end1, color});
            
            Vec3 start2 = center - tangent * size + bitangent * t;
            Vec3 end2 = center + tangent * size + bitangent * t;
            lines_.push_back({start2, end2, color});
        }
        
        // Normal arrow
        Vec3 normalEnd = center + normal * 1.0f;
        lines_.push_back({center, normalEnd, Vec4(0, 1, 0, 1)});
    }
    
    void drawAABB(const AABB& aabb) {
        Vec3 min = aabb.min;
        Vec3 max = aabb.max;
        Vec4 color = DebugColors::AABB;
        
        // 12 edges
        // Bottom
        lines_.push_back({Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z), color});
        lines_.push_back({Vec3(max.x, min.y, min.z), Vec3(max.x, min.y, max.z), color});
        lines_.push_back({Vec3(max.x, min.y, max.z), Vec3(min.x, min.y, max.z), color});
        lines_.push_back({Vec3(min.x, min.y, max.z), Vec3(min.x, min.y, min.z), color});
        
        // Top
        lines_.push_back({Vec3(min.x, max.y, min.z), Vec3(max.x, max.y, min.z), color});
        lines_.push_back({Vec3(max.x, max.y, min.z), Vec3(max.x, max.y, max.z), color});
        lines_.push_back({Vec3(max.x, max.y, max.z), Vec3(min.x, max.y, max.z), color});
        lines_.push_back({Vec3(min.x, max.y, max.z), Vec3(min.x, max.y, min.z), color});
        
        // Verticals
        lines_.push_back({Vec3(min.x, min.y, min.z), Vec3(min.x, max.y, min.z), color});
        lines_.push_back({Vec3(max.x, min.y, min.z), Vec3(max.x, max.y, min.z), color});
        lines_.push_back({Vec3(max.x, min.y, max.z), Vec3(max.x, max.y, max.z), color});
        lines_.push_back({Vec3(min.x, min.y, max.z), Vec3(min.x, max.y, max.z), color});
    }
    
    void drawContact(const CollisionInfo& contact) {
        // Contact point
        Vec3 point = contact.contactPoint;
        float size = 0.05f;
        
        // Cross at contact point
        lines_.push_back({point + Vec3(-size, 0, 0), point + Vec3(size, 0, 0), DebugColors::ContactPoint});
        lines_.push_back({point + Vec3(0, -size, 0), point + Vec3(0, size, 0), DebugColors::ContactPoint});
        lines_.push_back({point + Vec3(0, 0, -size), point + Vec3(0, 0, size), DebugColors::ContactPoint});
        
        // Normal arrow
        Vec3 normalEnd = point + contact.normal * 0.3f;
        lines_.push_back({point, normalEnd, DebugColors::ContactNormal});
        
        // Arrow head
        Vec3 perp = contact.normal.cross(Vec3(0, 1, 0));
        if (perp.lengthSquared() < 0.01f) {
            perp = contact.normal.cross(Vec3(1, 0, 0));
        }
        perp = perp.normalized() * 0.05f;
        
        lines_.push_back({normalEnd, normalEnd - contact.normal * 0.08f + perp, DebugColors::ContactNormal});
        lines_.push_back({normalEnd, normalEnd - contact.normal * 0.08f - perp, DebugColors::ContactNormal});
        
        // Draw all contact points if multiple
        for (int i = 0; i < contact.contactCount; i++) {
            Vec3 cp = contact.contactPoints[i];
            lines_.push_back({cp + Vec3(-size*0.5f, 0, 0), cp + Vec3(size*0.5f, 0, 0), DebugColors::ContactPoint});
            lines_.push_back({cp + Vec3(0, -size*0.5f, 0), cp + Vec3(0, size*0.5f, 0), DebugColors::ContactPoint});
        }
    }
    
    void drawConstraint(const Constraint* constraint) {
        if (!constraint) return;
        
        Vec4 color = DebugColors::ConstraintOK;
        if (constraint->isBroken()) {
            color = DebugColors::ConstraintBroken;
        }
        
        RigidBody* bodyA = constraint->getBodyA();
        RigidBody* bodyB = constraint->getBodyB();
        
        Vec3 posA = bodyA->getPosition();
        Vec3 posB = bodyB->getPosition();
        
        // Draw line between bodies
        lines_.push_back({posA, posB, color});
        
        // Draw anchor points
        float size = 0.08f;
        
        // Body A marker (X shape)
        lines_.push_back({posA + Vec3(-size, -size, 0), posA + Vec3(size, size, 0), color});
        lines_.push_back({posA + Vec3(-size, size, 0), posA + Vec3(size, -size, 0), color});
        
        // Body B marker (+ shape)
        lines_.push_back({posB + Vec3(-size, 0, 0), posB + Vec3(size, 0, 0), color});
        lines_.push_back({posB + Vec3(0, -size, 0), posB + Vec3(0, size, 0), color});
        
        // Type-specific visualization
        switch (constraint->getType()) {
            case ConstraintType::Spring: {
                // Draw spring coil
                drawSpring(posA, posB, color);
                break;
            }
            case ConstraintType::Hinge: {
                // Draw hinge axis
                Vec3 center = (posA + posB) * 0.5f;
                // Would need axis from constraint - simplified
                lines_.push_back({center + Vec3(0, -0.2f, 0), center + Vec3(0, 0.2f, 0), color});
                break;
            }
            default:
                break;
        }
    }
    
    void drawSpring(const Vec3& start, const Vec3& end, const Vec4& color) {
        Vec3 dir = end - start;
        float length = dir.length();
        if (length < 0.01f) return;
        
        dir = dir * (1.0f / length);
        
        // Calculate perpendicular
        Vec3 perp = dir.cross(Vec3(0, 1, 0));
        if (perp.lengthSquared() < 0.01f) {
            perp = dir.cross(Vec3(1, 0, 0));
        }
        perp = perp.normalized();
        
        // Draw coils
        const int coils = 8;
        const float radius = 0.05f;
        Vec3 prev = start;
        
        for (int i = 1; i <= coils * 4; i++) {
            float t = (float)i / (coils * 4);
            float angle = t * coils * 2.0f * 3.14159f;
            
            Vec3 offset = perp * cosf(angle) * radius + perp.cross(dir) * sinf(angle) * radius;
            Vec3 point = start + dir * (t * length) + offset;
            
            lines_.push_back({prev, point, color});
            prev = point;
        }
    }
    
    void drawVelocity(const RigidBody* body) {
        Vec3 pos = body->getPosition();
        
        // Linear velocity
        Vec3 linVel = body->getLinearVelocity();
        if (linVel.lengthSquared() > 0.001f) {
            Vec3 end = pos + linVel * velocityScale_;
            lines_.push_back({pos, end, DebugColors::LinearVelocity});
            
            // Arrow head
            Vec3 dir = linVel.normalized();
            Vec3 perp = dir.cross(Vec3(0, 1, 0));
            if (perp.lengthSquared() < 0.01f) {
                perp = dir.cross(Vec3(1, 0, 0));
            }
            perp = perp.normalized() * 0.05f;
            
            lines_.push_back({end, end - dir * 0.1f + perp, DebugColors::LinearVelocity});
            lines_.push_back({end, end - dir * 0.1f - perp, DebugColors::LinearVelocity});
        }
        
        // Angular velocity
        Vec3 angVel = body->getAngularVelocity();
        if (angVel.lengthSquared() > 0.001f) {
            // Draw rotation axis
            Vec3 axis = angVel.normalized();
            float magnitude = angVel.length();
            
            Vec3 start = pos - axis * 0.2f;
            Vec3 end = pos + axis * 0.2f;
            lines_.push_back({start, end, DebugColors::AngularVelocity});
            
            // Draw rotation direction arc
            Vec3 perp = axis.cross(Vec3(0, 1, 0));
            if (perp.lengthSquared() < 0.01f) {
                perp = axis.cross(Vec3(1, 0, 0));
            }
            perp = perp.normalized() * 0.15f;
            
            const int arcSegments = 8;
            Vec3 prevPoint = pos + perp;
            for (int i = 1; i <= arcSegments; i++) {
                float angle = (float)i / arcSegments * std::min(magnitude * 0.5f, 3.14159f);
                Quat rot = Quat::fromAxisAngle(axis, angle);
                Vec3 point = pos + rot.rotate(perp);
                lines_.push_back({prevPoint, point, DebugColors::AngularVelocity});
                prevPoint = point;
            }
        }
    }
    
    std::vector<DebugLine> lines_;
    
    bool drawColliders_ = true;
    bool drawAABBs_ = false;
    bool drawContacts_ = true;
    bool drawConstraints_ = true;
    bool drawVelocities_ = false;
    float velocityScale_ = 0.2f;
};

// ===== Global Debug Renderer =====
inline PhysicsDebugRenderer& getPhysicsDebugRenderer() {
    static PhysicsDebugRenderer renderer;
    return renderer;
}

}  // namespace luma
