// Navigation Mesh System
// NavMesh generation, storage, and querying
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <algorithm>
#include <limits>

namespace luma {

// ===== NavMesh Constants =====
constexpr float NAV_EPSILON = 0.001f;
constexpr int NAV_MAX_VERTS_PER_POLY = 6;

// ===== Nav Polygon =====
struct NavPoly {
    int indices[NAV_MAX_VERTS_PER_POLY];  // Vertex indices
    int vertCount = 0;
    
    int neighbors[NAV_MAX_VERTS_PER_POLY];  // Adjacent polygon indices (-1 = none)
    
    Vec3 center;       // Polygon center
    Vec3 normal;       // Surface normal
    float area = 0.0f;
    
    // Flags
    uint32_t flags = 0;
    uint8_t areaType = 0;  // 0 = walkable, 1 = water, etc.
    
    bool isValid() const { return vertCount >= 3; }
};

// ===== Nav Edge =====
struct NavEdge {
    int polyA;
    int polyB;
    Vec3 start;
    Vec3 end;
    float width;
};

// ===== Nav Node (for pathfinding) =====
struct NavNode {
    int polyIndex;
    Vec3 position;
    
    float gCost = 0.0f;  // Cost from start
    float hCost = 0.0f;  // Heuristic to goal
    float fCost() const { return gCost + hCost; }
    
    int parentIndex = -1;
    int edgeIndex = -1;  // Edge used to reach this node
    
    bool operator>(const NavNode& other) const {
        return fCost() > other.fCost();
    }
};

// ===== Path Point =====
struct PathPoint {
    Vec3 position;
    int polyIndex;
    uint8_t areaType;
};

// ===== Nav Path =====
struct NavPath {
    std::vector<PathPoint> points;
    float totalLength = 0.0f;
    bool valid = false;
    
    void clear() {
        points.clear();
        totalLength = 0.0f;
        valid = false;
    }
    
    size_t getPointCount() const { return points.size(); }
    
    Vec3 getPoint(size_t index) const {
        return index < points.size() ? points[index].position : Vec3(0, 0, 0);
    }
    
    // Get position along path by distance
    Vec3 getPositionAtDistance(float distance) const {
        if (points.empty()) return Vec3(0, 0, 0);
        if (distance <= 0) return points[0].position;
        if (distance >= totalLength) return points.back().position;
        
        float accumulated = 0.0f;
        for (size_t i = 0; i < points.size() - 1; i++) {
            Vec3 delta = points[i + 1].position - points[i].position;
            float segmentLength = delta.length();
            
            if (accumulated + segmentLength >= distance) {
                float t = (distance - accumulated) / segmentLength;
                return points[i].position + delta * t;
            }
            
            accumulated += segmentLength;
        }
        
        return points.back().position;
    }
};

// ===== NavMesh Build Settings =====
struct NavMeshBuildSettings {
    // Agent properties
    float agentHeight = 2.0f;
    float agentRadius = 0.5f;
    float agentMaxClimb = 0.5f;     // Max step height
    float agentMaxSlope = 45.0f;    // Max walkable slope in degrees
    
    // Voxelization
    float cellSize = 0.3f;          // XZ cell size
    float cellHeight = 0.2f;        // Y cell size
    
    // Region building
    int minRegionArea = 8;          // Min cells for region
    int mergeRegionArea = 20;       // Merge regions smaller than this
    
    // Polygon mesh
    float maxEdgeLen = 12.0f;
    float maxSimplificationError = 1.3f;
    int maxVertsPerPoly = NAV_MAX_VERTS_PER_POLY;
    
    // Detail mesh
    float detailSampleDist = 6.0f;
    float detailSampleMaxError = 1.0f;
};

// ===== NavMesh =====
class NavMesh {
public:
    NavMesh() = default;
    
    // Build from geometry (simplified)
    bool build(const std::vector<Vec3>& vertices, const std::vector<int>& indices,
               const NavMeshBuildSettings& settings);
    
    // Build from heightmap/terrain
    bool buildFromHeightmap(const float* heightmap, int width, int height,
                            float worldWidth, float worldHeight, float maxHeight,
                            const NavMeshBuildSettings& settings);
    
    // Manual polygon addition
    int addPolygon(const Vec3* vertices, int vertCount, uint8_t areaType = 0);
    void connectPolygons();
    
    // Query
    int findNearestPoly(const Vec3& position, float maxDistance = 10.0f) const;
    Vec3 getClosestPointOnPoly(int polyIndex, const Vec3& position) const;
    bool isPointInPoly(int polyIndex, const Vec3& position) const;
    
    // Raycast
    bool raycast(const Vec3& start, const Vec3& end, Vec3& hitPoint, int& hitPoly) const;
    
    // Get data
    const std::vector<Vec3>& getVertices() const { return vertices_; }
    const std::vector<NavPoly>& getPolygons() const { return polygons_; }
    const std::vector<NavEdge>& getEdges() const { return edges_; }
    
    size_t getVertexCount() const { return vertices_.size(); }
    size_t getPolyCount() const { return polygons_.size(); }
    
    // Bounds
    Vec3 getMinBounds() const { return minBounds_; }
    Vec3 getMaxBounds() const { return maxBounds_; }
    
    // Clear
    void clear() {
        vertices_.clear();
        polygons_.clear();
        edges_.clear();
        minBounds_ = maxBounds_ = Vec3(0, 0, 0);
    }
    
    bool isValid() const { return !polygons_.empty(); }
    
private:
    void calculatePolyProperties(NavPoly& poly);
    void buildEdges();
    void updateBounds();
    
    std::vector<Vec3> vertices_;
    std::vector<NavPoly> polygons_;
    std::vector<NavEdge> edges_;
    
    Vec3 minBounds_ = {0, 0, 0};
    Vec3 maxBounds_ = {0, 0, 0};
    
    NavMeshBuildSettings settings_;
};

// ===== A* Pathfinder =====
class NavPathfinder {
public:
    NavPathfinder(const NavMesh* navMesh) : navMesh_(navMesh) {}
    
    // Find path between two points
    bool findPath(const Vec3& start, const Vec3& end, NavPath& outPath);
    
    // Find path with area cost weights
    bool findPath(const Vec3& start, const Vec3& end, NavPath& outPath,
                  const std::unordered_map<uint8_t, float>& areaCosts);
    
    // String pulling (funnel algorithm) for smooth paths
    void smoothPath(NavPath& path);
    
    // Settings
    void setMaxIterations(int maxIter) { maxIterations_ = maxIter; }
    void setHeuristicWeight(float weight) { heuristicWeight_ = weight; }
    
private:
    float heuristic(const Vec3& a, const Vec3& b) const;
    float edgeCost(int polyA, int polyB, const std::unordered_map<uint8_t, float>& areaCosts) const;
    void reconstructPath(const std::vector<NavNode>& nodes, int endNodeIndex,
                         const Vec3& start, const Vec3& end, NavPath& outPath);
    
    const NavMesh* navMesh_;
    int maxIterations_ = 10000;
    float heuristicWeight_ = 1.0f;
};

// ===== NavMesh Implementation =====

inline bool NavMesh::build(const std::vector<Vec3>& vertices, const std::vector<int>& indices,
                           const NavMeshBuildSettings& settings) {
    clear();
    settings_ = settings;
    
    if (vertices.empty() || indices.size() < 3) return false;
    
    // Store vertices
    vertices_ = vertices;
    
    // Create triangles as polygons
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vec3 v0 = vertices[indices[i]];
        Vec3 v1 = vertices[indices[i + 1]];
        Vec3 v2 = vertices[indices[i + 2]];
        
        // Calculate normal
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.cross(edge2).normalized();
        
        // Check slope
        float slope = std::acos(normal.y) * 180.0f / 3.14159f;
        if (slope > settings.agentMaxSlope) continue;
        
        NavPoly poly;
        poly.indices[0] = indices[i];
        poly.indices[1] = indices[i + 1];
        poly.indices[2] = indices[i + 2];
        poly.vertCount = 3;
        poly.normal = normal;
        
        for (int j = 0; j < NAV_MAX_VERTS_PER_POLY; j++) {
            poly.neighbors[j] = -1;
        }
        
        calculatePolyProperties(poly);
        polygons_.push_back(poly);
    }
    
    connectPolygons();
    buildEdges();
    updateBounds();
    
    return !polygons_.empty();
}

inline bool NavMesh::buildFromHeightmap(const float* heightmap, int width, int height,
                                        float worldWidth, float worldHeight, float maxHeight,
                                        const NavMeshBuildSettings& settings) {
    clear();
    settings_ = settings;
    
    float cellSizeX = worldWidth / (width - 1);
    float cellSizeZ = worldHeight / (height - 1);
    float halfWidth = worldWidth * 0.5f;
    float halfHeight = worldHeight * 0.5f;
    
    // Generate vertices
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float h = heightmap[z * width + x] * maxHeight;
            float wx = x * cellSizeX - halfWidth;
            float wz = z * cellSizeZ - halfHeight;
            vertices_.push_back(Vec3(wx, h, wz));
        }
    }
    
    // Generate quads as two triangles
    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            int i00 = z * width + x;
            int i10 = i00 + 1;
            int i01 = (z + 1) * width + x;
            int i11 = i01 + 1;
            
            // Check slope for first triangle
            Vec3 v0 = vertices_[i00];
            Vec3 v1 = vertices_[i10];
            Vec3 v2 = vertices_[i01];
            Vec3 normal1 = (v1 - v0).cross(v2 - v0).normalized();
            float slope1 = std::acos(std::max(-1.0f, std::min(1.0f, normal1.y))) * 180.0f / 3.14159f;
            
            if (slope1 <= settings.agentMaxSlope) {
                NavPoly poly1;
                poly1.indices[0] = i00;
                poly1.indices[1] = i10;
                poly1.indices[2] = i01;
                poly1.vertCount = 3;
                poly1.normal = normal1;
                for (int j = 0; j < NAV_MAX_VERTS_PER_POLY; j++) poly1.neighbors[j] = -1;
                calculatePolyProperties(poly1);
                polygons_.push_back(poly1);
            }
            
            // Check slope for second triangle
            Vec3 v3 = vertices_[i11];
            Vec3 normal2 = (v2 - v1).cross(v3 - v1).normalized();
            float slope2 = std::acos(std::max(-1.0f, std::min(1.0f, normal2.y))) * 180.0f / 3.14159f;
            
            if (slope2 <= settings.agentMaxSlope) {
                NavPoly poly2;
                poly2.indices[0] = i10;
                poly2.indices[1] = i11;
                poly2.indices[2] = i01;
                poly2.vertCount = 3;
                poly2.normal = normal2;
                for (int j = 0; j < NAV_MAX_VERTS_PER_POLY; j++) poly2.neighbors[j] = -1;
                calculatePolyProperties(poly2);
                polygons_.push_back(poly2);
            }
        }
    }
    
    connectPolygons();
    buildEdges();
    updateBounds();
    
    return !polygons_.empty();
}

inline int NavMesh::addPolygon(const Vec3* verts, int vertCount, uint8_t areaType) {
    if (vertCount < 3 || vertCount > NAV_MAX_VERTS_PER_POLY) return -1;
    
    NavPoly poly;
    poly.vertCount = vertCount;
    poly.areaType = areaType;
    
    for (int i = 0; i < vertCount; i++) {
        poly.indices[i] = (int)vertices_.size();
        vertices_.push_back(verts[i]);
    }
    
    for (int j = 0; j < NAV_MAX_VERTS_PER_POLY; j++) {
        poly.neighbors[j] = -1;
    }
    
    calculatePolyProperties(poly);
    polygons_.push_back(poly);
    
    return (int)polygons_.size() - 1;
}

inline void NavMesh::connectPolygons() {
    // Find shared edges between polygons
    for (size_t i = 0; i < polygons_.size(); i++) {
        NavPoly& polyA = polygons_[i];
        
        for (size_t j = i + 1; j < polygons_.size(); j++) {
            NavPoly& polyB = polygons_[j];
            
            // Check for shared edge
            for (int ai = 0; ai < polyA.vertCount; ai++) {
                int ai2 = (ai + 1) % polyA.vertCount;
                int va1 = polyA.indices[ai];
                int va2 = polyA.indices[ai2];
                
                for (int bi = 0; bi < polyB.vertCount; bi++) {
                    int bi2 = (bi + 1) % polyB.vertCount;
                    int vb1 = polyB.indices[bi];
                    int vb2 = polyB.indices[bi2];
                    
                    // Check if edges match (in reverse order)
                    bool match = (va1 == vb2 && va2 == vb1);
                    
                    // Or check by position (for separate vertex arrays)
                    if (!match) {
                        float d1 = (vertices_[va1] - vertices_[vb2]).length();
                        float d2 = (vertices_[va2] - vertices_[vb1]).length();
                        match = (d1 < NAV_EPSILON && d2 < NAV_EPSILON);
                    }
                    
                    if (match) {
                        polyA.neighbors[ai] = (int)j;
                        polyB.neighbors[bi] = (int)i;
                    }
                }
            }
        }
    }
}

inline void NavMesh::calculatePolyProperties(NavPoly& poly) {
    if (poly.vertCount < 3) return;
    
    // Calculate center
    poly.center = Vec3(0, 0, 0);
    for (int i = 0; i < poly.vertCount; i++) {
        poly.center = poly.center + vertices_[poly.indices[i]];
    }
    poly.center = poly.center * (1.0f / poly.vertCount);
    
    // Calculate area (sum of triangle areas)
    poly.area = 0.0f;
    Vec3 v0 = vertices_[poly.indices[0]];
    for (int i = 1; i < poly.vertCount - 1; i++) {
        Vec3 v1 = vertices_[poly.indices[i]];
        Vec3 v2 = vertices_[poly.indices[i + 1]];
        Vec3 cross = (v1 - v0).cross(v2 - v0);
        poly.area += cross.length() * 0.5f;
    }
}

inline void NavMesh::buildEdges() {
    edges_.clear();
    
    for (size_t i = 0; i < polygons_.size(); i++) {
        const NavPoly& poly = polygons_[i];
        
        for (int e = 0; e < poly.vertCount; e++) {
            int neighbor = poly.neighbors[e];
            if (neighbor > (int)i) {  // Avoid duplicates
                NavEdge edge;
                edge.polyA = (int)i;
                edge.polyB = neighbor;
                edge.start = vertices_[poly.indices[e]];
                edge.end = vertices_[poly.indices[(e + 1) % poly.vertCount]];
                edge.width = (edge.end - edge.start).length();
                edges_.push_back(edge);
            }
        }
    }
}

inline void NavMesh::updateBounds() {
    if (vertices_.empty()) {
        minBounds_ = maxBounds_ = Vec3(0, 0, 0);
        return;
    }
    
    minBounds_ = maxBounds_ = vertices_[0];
    for (const auto& v : vertices_) {
        minBounds_.x = std::min(minBounds_.x, v.x);
        minBounds_.y = std::min(minBounds_.y, v.y);
        minBounds_.z = std::min(minBounds_.z, v.z);
        maxBounds_.x = std::max(maxBounds_.x, v.x);
        maxBounds_.y = std::max(maxBounds_.y, v.y);
        maxBounds_.z = std::max(maxBounds_.z, v.z);
    }
}

inline int NavMesh::findNearestPoly(const Vec3& position, float maxDistance) const {
    int nearestPoly = -1;
    float nearestDist = maxDistance * maxDistance;
    
    for (size_t i = 0; i < polygons_.size(); i++) {
        Vec3 closest = getClosestPointOnPoly((int)i, position);
        float dist = (closest - position).lengthSquared();
        
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestPoly = (int)i;
        }
    }
    
    return nearestPoly;
}

inline Vec3 NavMesh::getClosestPointOnPoly(int polyIndex, const Vec3& position) const {
    if (polyIndex < 0 || polyIndex >= (int)polygons_.size()) {
        return position;
    }
    
    const NavPoly& poly = polygons_[polyIndex];
    
    // Project point onto polygon plane
    Vec3 v0 = vertices_[poly.indices[0]];
    float d = poly.normal.dot(position - v0);
    Vec3 projected = position - poly.normal * d;
    
    // Check if inside polygon (simplified - use center as fallback)
    if (isPointInPoly(polyIndex, projected)) {
        return projected;
    }
    
    // Find closest point on edges
    Vec3 closest = poly.center;
    float minDist = (closest - position).lengthSquared();
    
    for (int i = 0; i < poly.vertCount; i++) {
        Vec3 a = vertices_[poly.indices[i]];
        Vec3 b = vertices_[poly.indices[(i + 1) % poly.vertCount]];
        
        Vec3 ab = b - a;
        float t = std::max(0.0f, std::min(1.0f, (position - a).dot(ab) / ab.lengthSquared()));
        Vec3 point = a + ab * t;
        
        float dist = (point - position).lengthSquared();
        if (dist < minDist) {
            minDist = dist;
            closest = point;
        }
    }
    
    return closest;
}

inline bool NavMesh::isPointInPoly(int polyIndex, const Vec3& position) const {
    if (polyIndex < 0 || polyIndex >= (int)polygons_.size()) return false;
    
    const NavPoly& poly = polygons_[polyIndex];
    
    // Check using cross products (point should be on same side of all edges)
    for (int i = 0; i < poly.vertCount; i++) {
        Vec3 a = vertices_[poly.indices[i]];
        Vec3 b = vertices_[poly.indices[(i + 1) % poly.vertCount]];
        
        Vec3 edge = b - a;
        Vec3 toPoint = position - a;
        
        float cross = edge.x * toPoint.z - edge.z * toPoint.x;
        if (cross < -NAV_EPSILON) return false;
    }
    
    return true;
}

inline bool NavMesh::raycast(const Vec3& start, const Vec3& end, Vec3& hitPoint, int& hitPoly) const {
    // Simplified raycast - check against all polygons
    Vec3 dir = end - start;
    float maxT = dir.length();
    if (maxT < NAV_EPSILON) return false;
    dir = dir * (1.0f / maxT);
    
    float nearestT = maxT;
    hitPoly = -1;
    
    for (size_t i = 0; i < polygons_.size(); i++) {
        const NavPoly& poly = polygons_[i];
        
        // Ray-plane intersection
        float denom = poly.normal.dot(dir);
        if (std::abs(denom) < NAV_EPSILON) continue;
        
        Vec3 v0 = vertices_[poly.indices[0]];
        float t = poly.normal.dot(v0 - start) / denom;
        
        if (t < 0 || t >= nearestT) continue;
        
        Vec3 point = start + dir * t;
        if (isPointInPoly((int)i, point)) {
            nearestT = t;
            hitPoly = (int)i;
            hitPoint = point;
        }
    }
    
    return hitPoly >= 0;
}

// ===== A* Pathfinder Implementation =====

inline bool NavPathfinder::findPath(const Vec3& start, const Vec3& end, NavPath& outPath) {
    std::unordered_map<uint8_t, float> defaultCosts;
    return findPath(start, end, outPath, defaultCosts);
}

inline bool NavPathfinder::findPath(const Vec3& start, const Vec3& end, NavPath& outPath,
                                    const std::unordered_map<uint8_t, float>& areaCosts) {
    outPath.clear();
    
    if (!navMesh_ || !navMesh_->isValid()) return false;
    
    // Find start and end polygons
    int startPoly = navMesh_->findNearestPoly(start);
    int endPoly = navMesh_->findNearestPoly(end);
    
    if (startPoly < 0 || endPoly < 0) return false;
    
    // Same polygon - direct path
    if (startPoly == endPoly) {
        outPath.points.push_back({start, startPoly, navMesh_->getPolygons()[startPoly].areaType});
        outPath.points.push_back({end, endPoly, navMesh_->getPolygons()[endPoly].areaType});
        outPath.totalLength = (end - start).length();
        outPath.valid = true;
        return true;
    }
    
    // A* search
    const auto& polygons = navMesh_->getPolygons();
    
    std::vector<NavNode> nodes;
    std::vector<bool> closed(polygons.size(), false);
    std::priority_queue<NavNode, std::vector<NavNode>, std::greater<NavNode>> openSet;
    std::unordered_map<int, int> nodeMap;  // polyIndex -> node index
    
    // Start node
    NavNode startNode;
    startNode.polyIndex = startPoly;
    startNode.position = navMesh_->getClosestPointOnPoly(startPoly, start);
    startNode.gCost = 0.0f;
    startNode.hCost = heuristic(startNode.position, end) * heuristicWeight_;
    nodes.push_back(startNode);
    nodeMap[startPoly] = 0;
    openSet.push(startNode);
    
    int iterations = 0;
    int endNodeIndex = -1;
    
    while (!openSet.empty() && iterations < maxIterations_) {
        iterations++;
        
        NavNode current = openSet.top();
        openSet.pop();
        
        if (closed[current.polyIndex]) continue;
        closed[current.polyIndex] = true;
        
        // Found goal
        if (current.polyIndex == endPoly) {
            endNodeIndex = nodeMap[current.polyIndex];
            break;
        }
        
        // Expand neighbors
        const NavPoly& poly = polygons[current.polyIndex];
        for (int i = 0; i < poly.vertCount; i++) {
            int neighborIdx = poly.neighbors[i];
            if (neighborIdx < 0 || closed[neighborIdx]) continue;
            
            const NavPoly& neighbor = polygons[neighborIdx];
            
            // Calculate edge midpoint
            Vec3 edgeStart = navMesh_->getVertices()[poly.indices[i]];
            Vec3 edgeEnd = navMesh_->getVertices()[poly.indices[(i + 1) % poly.vertCount]];
            Vec3 edgeMid = (edgeStart + edgeEnd) * 0.5f;
            
            // Calculate cost
            float moveCost = (edgeMid - current.position).length();
            moveCost *= edgeCost(current.polyIndex, neighborIdx, areaCosts);
            float newGCost = current.gCost + moveCost;
            
            // Check if better path
            auto it = nodeMap.find(neighborIdx);
            if (it != nodeMap.end()) {
                if (newGCost >= nodes[it->second].gCost) continue;
                nodes[it->second].gCost = newGCost;
                nodes[it->second].parentIndex = nodeMap[current.polyIndex];
                nodes[it->second].position = edgeMid;
            } else {
                NavNode newNode;
                newNode.polyIndex = neighborIdx;
                newNode.position = edgeMid;
                newNode.gCost = newGCost;
                newNode.hCost = heuristic(neighbor.center, end) * heuristicWeight_;
                newNode.parentIndex = nodeMap[current.polyIndex];
                nodes.push_back(newNode);
                nodeMap[neighborIdx] = (int)nodes.size() - 1;
            }
            
            NavNode queueNode = nodes[nodeMap[neighborIdx]];
            openSet.push(queueNode);
        }
    }
    
    if (endNodeIndex < 0) return false;
    
    // Reconstruct path
    reconstructPath(nodes, endNodeIndex, start, end, outPath);
    smoothPath(outPath);
    
    return outPath.valid;
}

inline float NavPathfinder::heuristic(const Vec3& a, const Vec3& b) const {
    // Euclidean distance
    return (b - a).length();
}

inline float NavPathfinder::edgeCost(int polyA, int polyB,
                                     const std::unordered_map<uint8_t, float>& areaCosts) const {
    const auto& polygons = navMesh_->getPolygons();
    uint8_t areaType = polygons[polyB].areaType;
    
    auto it = areaCosts.find(areaType);
    if (it != areaCosts.end()) {
        return it->second;
    }
    
    return 1.0f;  // Default cost
}

inline void NavPathfinder::reconstructPath(const std::vector<NavNode>& nodes, int endNodeIndex,
                                           const Vec3& start, const Vec3& end, NavPath& outPath) {
    std::vector<int> nodeIndices;
    int current = endNodeIndex;
    
    while (current >= 0) {
        nodeIndices.push_back(current);
        current = nodes[current].parentIndex;
    }
    
    std::reverse(nodeIndices.begin(), nodeIndices.end());
    
    // Build path points
    const auto& polygons = navMesh_->getPolygons();
    
    outPath.points.push_back({start, nodes[nodeIndices[0]].polyIndex,
                             polygons[nodes[nodeIndices[0]].polyIndex].areaType});
    
    for (size_t i = 1; i < nodeIndices.size(); i++) {
        const NavNode& node = nodes[nodeIndices[i]];
        outPath.points.push_back({node.position, node.polyIndex,
                                 polygons[node.polyIndex].areaType});
    }
    
    outPath.points.push_back({end, nodes[nodeIndices.back()].polyIndex,
                             polygons[nodes[nodeIndices.back()].polyIndex].areaType});
    
    // Calculate total length
    outPath.totalLength = 0.0f;
    for (size_t i = 0; i < outPath.points.size() - 1; i++) {
        outPath.totalLength += (outPath.points[i + 1].position - outPath.points[i].position).length();
    }
    
    outPath.valid = true;
}

inline void NavPathfinder::smoothPath(NavPath& path) {
    if (path.points.size() < 3) return;
    
    // Simple line-of-sight smoothing
    std::vector<PathPoint> smoothed;
    smoothed.push_back(path.points[0]);
    
    size_t current = 0;
    while (current < path.points.size() - 1) {
        size_t farthest = current + 1;
        
        // Find farthest visible point
        for (size_t i = current + 2; i < path.points.size(); i++) {
            Vec3 hitPoint;
            int hitPoly;
            if (!navMesh_->raycast(path.points[current].position,
                                   path.points[i].position, hitPoint, hitPoly)) {
                farthest = i;
            }
        }
        
        smoothed.push_back(path.points[farthest]);
        current = farthest;
    }
    
    path.points = smoothed;
    
    // Recalculate length
    path.totalLength = 0.0f;
    for (size_t i = 0; i < path.points.size() - 1; i++) {
        path.totalLength += (path.points[i + 1].position - path.points[i].position).length();
    }
}

// ===== Global NavMesh =====
inline NavMesh& getNavMesh() {
    static NavMesh navMesh;
    return navMesh;
}

}  // namespace luma
