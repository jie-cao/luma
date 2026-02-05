// LUMA Selection System Tests
// Unit tests for selection logic (box, circle, lasso, ray picking)

#pragma once

#include "engine/foundation/math_types.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <set>

namespace luma {
namespace test {

// Test helper macros
#define SEL_EXPECT_TRUE(expr) if (!(expr)) { std::cerr << "[FAIL] " << #expr << std::endl; return false; }
#define SEL_EXPECT_EQ(a, b) if ((a) != (b)) { std::cerr << "[FAIL] " << #a << " != " << #b << " (" << (a) << " vs " << (b) << ")" << std::endl; return false; }
#define SEL_EXPECT_NEAR(a, b, eps) if (std::abs((a) - (b)) > (eps)) { std::cerr << "[FAIL] " << #a << " !≈ " << #b << std::endl; return false; }

namespace SelectionTests {

// ============================================================================
// Selection Utilities (will be moved to SelectionSystem later)
// ============================================================================

// Check if point is inside box (2D screen space)
inline bool pointInBox(float px, float py, float minX, float minY, float maxX, float maxY) {
    return px >= minX && px <= maxX && py >= minY && py <= maxY;
}

// Check if point is inside circle (2D screen space)
inline bool pointInCircle(float px, float py, float cx, float cy, float radius) {
    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= (radius * radius);
}

// Check if point is inside polygon (lasso selection) using ray casting
inline bool pointInPolygon(float px, float py, const std::vector<Vec2>& polygon) {
    if (polygon.size() < 3) return false;
    
    bool inside = false;
    size_t j = polygon.size() - 1;
    
    for (size_t i = 0; i < polygon.size(); i++) {
        if (((polygon[i].y > py) != (polygon[j].y > py)) &&
            (px < (polygon[j].x - polygon[i].x) * (py - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)) {
            inside = !inside;
        }
        j = i;
    }
    
    return inside;
}

// Ray-triangle intersection (for picking)
inline bool rayTriangleIntersect(
    const Vec3& rayOrigin, const Vec3& rayDir,
    const Vec3& v0, const Vec3& v1, const Vec3& v2,
    float& outT, float& outU, float& outV
) {
    const float EPSILON = 1e-6f;
    
    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    
    Vec3 h = rayDir.cross(e2);
    float a = e1.dot(h);
    
    if (std::abs(a) < EPSILON) return false;
    
    float f = 1.0f / a;
    Vec3 s = rayOrigin - v0;
    outU = f * s.dot(h);
    
    if (outU < 0.0f || outU > 1.0f) return false;
    
    Vec3 q = s.cross(e1);
    outV = f * rayDir.dot(q);
    
    if (outV < 0.0f || outU + outV > 1.0f) return false;
    
    outT = f * e2.dot(q);
    
    return outT > EPSILON;
}

// Project 3D point to screen space
inline Vec2 projectToScreen(const Vec3& worldPos, const float* viewProj, float screenWidth, float screenHeight) {
    // viewProj is column-major 4x4 matrix
    float x = worldPos.x * viewProj[0] + worldPos.y * viewProj[4] + worldPos.z * viewProj[8] + viewProj[12];
    float y = worldPos.x * viewProj[1] + worldPos.y * viewProj[5] + worldPos.z * viewProj[9] + viewProj[13];
    float w = worldPos.x * viewProj[3] + worldPos.y * viewProj[7] + worldPos.z * viewProj[11] + viewProj[15];
    
    if (std::abs(w) < 1e-6f) return Vec2(-9999, -9999);
    
    float ndcX = x / w;
    float ndcY = y / w;
    
    float screenX = (ndcX + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndcY) * 0.5f * screenHeight;
    
    return Vec2(screenX, screenY);
}

// ============================================================================
// Box Selection Tests
// ============================================================================

inline bool testBoxSelectionBasic() {
    // Test points
    struct TestPoint { float x, y; bool shouldBeSelected; };
    TestPoint points[] = {
        {50, 50, true},    // Inside
        {100, 100, true},  // Inside (edge)
        {150, 75, true},   // Inside
        {10, 50, false},   // Outside left
        {50, 10, false},   // Outside top
        {200, 50, false},  // Outside right
        {50, 200, false},  // Outside bottom
    };
    
    float minX = 25, minY = 25, maxX = 175, maxY = 175;
    
    for (const auto& pt : points) {
        bool selected = pointInBox(pt.x, pt.y, minX, minY, maxX, maxY);
        SEL_EXPECT_EQ(selected, pt.shouldBeSelected);
    }
    
    return true;
}

inline bool testBoxSelectionNormalization() {
    // Box with swapped min/max should still work
    float x1 = 100, y1 = 100;
    float x2 = 50, y2 = 50;  // x2 < x1, y2 < y1
    
    float minX = std::min(x1, x2);
    float maxX = std::max(x1, x2);
    float minY = std::min(y1, y2);
    float maxY = std::max(y1, y2);
    
    // Point at (75, 75) should be inside
    SEL_EXPECT_TRUE(pointInBox(75, 75, minX, minY, maxX, maxY));
    
    return true;
}

// ============================================================================
// Circle Selection Tests
// ============================================================================

inline bool testCircleSelectionBasic() {
    float cx = 100, cy = 100, radius = 50;
    
    // Test points
    struct TestPoint { float x, y; bool shouldBeSelected; };
    TestPoint points[] = {
        {100, 100, true},   // Center
        {100, 50, true},    // Top edge
        {150, 100, true},   // Right edge
        {100, 149, true},   // Near bottom edge
        {100, 151, false},  // Just outside bottom
        {200, 100, false},  // Far outside
        {130, 130, true},   // Inside at diagonal
        {145, 145, false},  // Outside at diagonal
    };
    
    for (const auto& pt : points) {
        bool selected = pointInCircle(pt.x, pt.y, cx, cy, radius);
        SEL_EXPECT_EQ(selected, pt.shouldBeSelected);
    }
    
    return true;
}

inline bool testCircleSelectionZeroRadius() {
    // Zero radius circle should only contain center point (with floating point tolerance)
    float cx = 100, cy = 100, radius = 0;
    
    SEL_EXPECT_TRUE(pointInCircle(100, 100, cx, cy, radius));
    SEL_EXPECT_TRUE(!pointInCircle(100.001f, 100, cx, cy, radius));
    
    return true;
}

// ============================================================================
// Lasso Selection Tests
// ============================================================================

inline bool testLassoSelectionTriangle() {
    // Triangle polygon
    std::vector<Vec2> polygon = {
        {100, 0},
        {200, 200},
        {0, 200}
    };
    
    // Test points
    SEL_EXPECT_TRUE(pointInPolygon(100, 100, polygon));   // Center
    SEL_EXPECT_TRUE(pointInPolygon(100, 150, polygon));   // Inside lower
    SEL_EXPECT_TRUE(!pointInPolygon(50, 50, polygon));    // Outside top-left
    SEL_EXPECT_TRUE(!pointInPolygon(150, 50, polygon));   // Outside top-right
    
    return true;
}

inline bool testLassoSelectionComplex() {
    // L-shaped polygon
    std::vector<Vec2> polygon = {
        {0, 0},
        {100, 0},
        {100, 50},
        {50, 50},
        {50, 100},
        {0, 100}
    };
    
    // Inside the L
    SEL_EXPECT_TRUE(pointInPolygon(25, 25, polygon));    // Top-left area
    SEL_EXPECT_TRUE(pointInPolygon(25, 75, polygon));    // Bottom-left area
    
    // Outside the L (in the cut-out corner)
    SEL_EXPECT_TRUE(!pointInPolygon(75, 75, polygon));
    
    // Outside completely
    SEL_EXPECT_TRUE(!pointInPolygon(150, 50, polygon));
    
    return true;
}

inline bool testLassoSelectionEmptyPolygon() {
    std::vector<Vec2> polygon;
    
    // Empty polygon should not contain any point
    SEL_EXPECT_TRUE(!pointInPolygon(0, 0, polygon));
    
    // Single point polygon
    polygon.push_back({100, 100});
    SEL_EXPECT_TRUE(!pointInPolygon(100, 100, polygon));
    
    // Two point polygon (line)
    polygon.push_back({200, 200});
    SEL_EXPECT_TRUE(!pointInPolygon(150, 150, polygon));
    
    return true;
}

// ============================================================================
// Ray Picking Tests
// ============================================================================

inline bool testRayTriangleIntersection() {
    Vec3 v0(0, 0, 0);
    Vec3 v1(1, 0, 0);
    Vec3 v2(0, 1, 0);
    
    float t, u, v;
    
    // Ray hitting center of triangle
    Vec3 rayOrigin(0.25f, 0.25f, 1);
    Vec3 rayDir(0, 0, -1);
    
    SEL_EXPECT_TRUE(rayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, u, v));
    SEL_EXPECT_NEAR(t, 1.0f, 0.001f);
    
    // Ray missing triangle
    Vec3 missOrigin(2, 2, 1);
    SEL_EXPECT_TRUE(!rayTriangleIntersect(missOrigin, rayDir, v0, v1, v2, t, u, v));
    
    return true;
}

inline bool testRayTriangleParallel() {
    Vec3 v0(0, 0, 0);
    Vec3 v1(1, 0, 0);
    Vec3 v2(0, 1, 0);
    
    float t, u, v;
    
    // Ray parallel to triangle (should not intersect)
    Vec3 rayOrigin(0, 0, 1);
    Vec3 rayDir(1, 0, 0);  // Parallel to XY plane
    
    SEL_EXPECT_TRUE(!rayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, u, v));
    
    return true;
}

inline bool testRayTriangleBackface() {
    Vec3 v0(0, 0, 0);
    Vec3 v1(1, 0, 0);
    Vec3 v2(0, 1, 0);
    
    float t, u, v;
    
    // Ray from behind triangle
    Vec3 rayOrigin(0.25f, 0.25f, -1);
    Vec3 rayDir(0, 0, 1);  // Pointing toward triangle from behind
    
    // Should still intersect (backface hit)
    SEL_EXPECT_TRUE(rayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, u, v));
    
    return true;
}

// ============================================================================
// Screen Projection Tests
// ============================================================================

inline bool testScreenProjection() {
    // Identity view-projection matrix
    float viewProj[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    float screenWidth = 800;
    float screenHeight = 600;
    
    // Point at origin should project to center
    Vec3 origin(0, 0, 0);
    Vec2 projected = projectToScreen(origin, viewProj, screenWidth, screenHeight);
    
    SEL_EXPECT_NEAR(projected.x, screenWidth / 2, 1.0f);
    SEL_EXPECT_NEAR(projected.y, screenHeight / 2, 1.0f);
    
    return true;
}

// ============================================================================
// Selection Mode Tests
// ============================================================================

inline bool testSelectionModeSwitch() {
    enum class SelectionMode { Vertex, Edge, Face };
    
    SelectionMode mode = SelectionMode::Vertex;
    
    // Test mode switching
    mode = SelectionMode::Edge;
    SEL_EXPECT_TRUE(mode == SelectionMode::Edge);
    
    mode = SelectionMode::Face;
    SEL_EXPECT_TRUE(mode == SelectionMode::Face);
    
    return true;
}

inline bool testMultiSelection() {
    std::set<uint32_t> selection;
    
    // Add items
    selection.insert(0);
    selection.insert(5);
    selection.insert(10);
    
    SEL_EXPECT_EQ(selection.size(), 3u);
    SEL_EXPECT_TRUE(selection.count(5) > 0);
    
    // Toggle selection (remove if exists)
    if (selection.count(5)) {
        selection.erase(5);
    }
    
    SEL_EXPECT_EQ(selection.size(), 2u);
    SEL_EXPECT_TRUE(selection.count(5) == 0);
    
    // Clear
    selection.clear();
    SEL_EXPECT_EQ(selection.size(), 0u);
    
    return true;
}

// ============================================================================
// Test Registration
// ============================================================================

inline void registerSelectionTests(UnitTestRunner& runner) {
    runner.addTest("Selection", "Box Selection Basic", testBoxSelectionBasic);
    runner.addTest("Selection", "Box Selection Normalization", testBoxSelectionNormalization);
    runner.addTest("Selection", "Circle Selection Basic", testCircleSelectionBasic);
    runner.addTest("Selection", "Circle Selection Zero Radius", testCircleSelectionZeroRadius);
    runner.addTest("Selection", "Lasso Selection Triangle", testLassoSelectionTriangle);
    runner.addTest("Selection", "Lasso Selection Complex", testLassoSelectionComplex);
    runner.addTest("Selection", "Lasso Selection Empty", testLassoSelectionEmptyPolygon);
    runner.addTest("Selection", "Ray-Triangle Intersection", testRayTriangleIntersection);
    runner.addTest("Selection", "Ray-Triangle Parallel", testRayTriangleParallel);
    runner.addTest("Selection", "Ray-Triangle Backface", testRayTriangleBackface);
    runner.addTest("Selection", "Screen Projection", testScreenProjection);
    runner.addTest("Selection", "Selection Mode Switch", testSelectionModeSwitch);
    runner.addTest("Selection", "Multi Selection", testMultiSelection);
}

} // namespace SelectionTests
} // namespace test
} // namespace luma
