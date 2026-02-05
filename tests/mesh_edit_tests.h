// LUMA Mesh Edit Tests
// Unit tests for EditMesh operations

#pragma once

#include "engine/mesh/edit_mesh.h"
#include <iostream>
#include <cmath>
#include <functional>

namespace luma {
namespace test {

// Test helper macros
#define MESH_EXPECT_TRUE(expr) if (!(expr)) { std::cerr << "[FAIL] " << #expr << std::endl; return false; }
#define MESH_EXPECT_EQ(a, b) if ((a) != (b)) { std::cerr << "[FAIL] " << #a << " != " << #b << " (" << (a) << " vs " << (b) << ")" << std::endl; return false; }
#define MESH_EXPECT_NEAR(a, b, eps) if (std::abs((a) - (b)) > (eps)) { std::cerr << "[FAIL] " << #a << " !≈ " << #b << " (" << (a) << " vs " << (b) << ")" << std::endl; return false; }

namespace MeshEditTests {

// ============================================================================
// Basic Mesh Creation Tests
// ============================================================================

inline bool testAddVertex() {
    EditMesh mesh;
    
    uint32_t v0 = mesh.addVertex(0, 0, 0);
    uint32_t v1 = mesh.addVertex(1, 0, 0);
    uint32_t v2 = mesh.addVertex(0, 1, 0);
    
    MESH_EXPECT_EQ(mesh.vertices.size(), 3u);
    MESH_EXPECT_EQ(v0, 0u);
    MESH_EXPECT_EQ(v1, 1u);
    MESH_EXPECT_EQ(v2, 2u);
    
    MESH_EXPECT_NEAR(mesh.vertices[1].position[0], 1.0f, 0.001f);
    MESH_EXPECT_NEAR(mesh.vertices[2].position[1], 1.0f, 0.001f);
    
    return true;
}

inline bool testAddTriangleFace() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0, 1, 0);
    
    uint32_t faceIndex = mesh.addFace({0, 1, 2});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_EQ(faceIndex, 0u);
    MESH_EXPECT_TRUE(mesh.faces[0].isTriangle());
    MESH_EXPECT_EQ(mesh.faces[0].vertexCount(), 3);
    
    // Check that edges were created
    MESH_EXPECT_EQ(mesh.edges.size(), 3u);
    
    return true;
}

inline bool testAddQuadFace() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(1, 1, 0);
    mesh.addVertex(0, 1, 0);
    
    uint32_t faceIndex = mesh.addFace({0, 1, 2, 3});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_TRUE(mesh.faces[faceIndex].isQuad());
    MESH_EXPECT_EQ(mesh.faces[faceIndex].vertexCount(), 4);
    MESH_EXPECT_EQ(mesh.edges.size(), 4u);
    
    return true;
}

inline bool testAddNgonFace() {
    EditMesh mesh;
    
    // Create hexagon
    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f / 3.0f;
        mesh.addVertex(std::cos(angle), std::sin(angle), 0);
    }
    
    uint32_t faceIndex = mesh.addFace({0, 1, 2, 3, 4, 5});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_TRUE(mesh.faces[faceIndex].isNgon());
    MESH_EXPECT_EQ(mesh.faces[faceIndex].vertexCount(), 6);
    
    return true;
}

// ============================================================================
// Selection Tests
// ============================================================================

inline bool testVertexSelection() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2});
    
    // Select vertices
    mesh.selectedVertices.insert(0);
    mesh.selectedVertices.insert(2);
    
    MESH_EXPECT_EQ(mesh.selectedVertices.size(), 2u);
    MESH_EXPECT_TRUE(mesh.selectedVertices.count(0) > 0);
    MESH_EXPECT_TRUE(mesh.selectedVertices.count(2) > 0);
    MESH_EXPECT_TRUE(mesh.selectedVertices.count(1) == 0);
    
    // Clear selection
    mesh.clearSelection();
    MESH_EXPECT_EQ(mesh.selectedVertices.size(), 0u);
    
    return true;
}

inline bool testFaceSelection() {
    EditMesh mesh;
    
    // Create two triangles
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0.5f, 1, 0);
    mesh.addVertex(1.5f, 1, 0);
    
    mesh.addFace({0, 1, 2});
    mesh.addFace({1, 3, 2});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 2u);
    
    // Select first face
    mesh.selectedFaces.insert(0);
    MESH_EXPECT_EQ(mesh.selectedFaces.size(), 1u);
    
    // Select all
    mesh.selectAll();
    MESH_EXPECT_EQ(mesh.selectedFaces.size(), 2u);
    
    return true;
}

inline bool testEdgeSelection() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0.5f, 1, 0);
    mesh.addFace({0, 1, 2});
    
    MESH_EXPECT_EQ(mesh.edges.size(), 3u);
    
    // Select an edge
    mesh.selectedEdges.insert(0);
    MESH_EXPECT_EQ(mesh.selectedEdges.size(), 1u);
    
    return true;
}

// ============================================================================
// Transform Tests
// ============================================================================

inline bool testTranslateSelected() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2});
    
    // Select first vertex
    mesh.selectedVertices.insert(0);
    
    // Translate
    mesh.translateSelected(5.0f, 0, 0);
    
    MESH_EXPECT_NEAR(mesh.vertices[0].position[0], 5.0f, 0.001f);
    MESH_EXPECT_NEAR(mesh.vertices[1].position[0], 1.0f, 0.001f);  // Not selected, unchanged
    
    return true;
}

inline bool testScaleSelected() {
    EditMesh mesh;
    
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(2, 0, 0);
    mesh.addVertex(1.5f, 1, 0);
    mesh.addFace({0, 1, 2});
    
    // Select all
    mesh.selectedVertices.insert(0);
    mesh.selectedVertices.insert(1);
    mesh.selectedVertices.insert(2);
    
    // Scale around origin
    float pivot[3] = {0, 0, 0};
    mesh.scaleSelected(2.0f, 2.0f, 1.0f, pivot);
    
    MESH_EXPECT_NEAR(mesh.vertices[0].position[0], 2.0f, 0.001f);
    MESH_EXPECT_NEAR(mesh.vertices[1].position[0], 4.0f, 0.001f);
    
    return true;
}

// ============================================================================
// Modeling Operation Tests
// ============================================================================

inline bool testExtrudeFace() {
    EditMesh mesh;
    
    // Create a quad
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(1, 1, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2, 3});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_EQ(mesh.vertices.size(), 4u);
    
    // Select and extrude
    mesh.selectedFaces.insert(0);
    mesh.extrudeSelectedFaces(0, 0, 1.0f);
    
    // After extrude: 1 original face (moved up) + 4 side faces = 5 faces
    // Original 4 vertices + 4 new vertices = 8 vertices
    MESH_EXPECT_EQ(mesh.faces.size(), 5u);
    MESH_EXPECT_EQ(mesh.vertices.size(), 8u);
    
    // Check that top face is at z=1
    const EditFace& topFace = mesh.faces[0];
    for (const Loop& loop : topFace.loops) {
        MESH_EXPECT_NEAR(mesh.vertices[loop.vertexIndex].position[2], 1.0f, 0.001f);
    }
    
    return true;
}

inline bool testSubdivideFace() {
    EditMesh mesh;
    
    // Create a quad
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(2, 0, 0);
    mesh.addVertex(2, 2, 0);
    mesh.addVertex(0, 2, 0);
    mesh.addFace({0, 1, 2, 3});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_EQ(mesh.vertices.size(), 4u);
    
    // Select and subdivide
    mesh.selectedFaces.insert(0);
    mesh.subdivideSelectedFaces();
    
    // After subdivide: 4 quads (from corners), center point + 4 edge midpoints = 9 vertices
    MESH_EXPECT_EQ(mesh.faces.size(), 4u);
    MESH_EXPECT_EQ(mesh.vertices.size(), 9u);
    
    // All new faces should be quads
    for (const EditFace& face : mesh.faces) {
        MESH_EXPECT_TRUE(face.isQuad());
    }
    
    return true;
}

inline bool testDeleteFace() {
    EditMesh mesh;
    
    // Create two triangles
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0.5f, 1, 0);
    mesh.addVertex(1.5f, 1, 0);
    
    mesh.addFace({0, 1, 2});
    mesh.addFace({1, 3, 2});
    
    MESH_EXPECT_EQ(mesh.faces.size(), 2u);
    
    // Select and delete first face
    mesh.selectedFaces.insert(0);
    mesh.deleteSelectedFaces();
    
    MESH_EXPECT_EQ(mesh.faces.size(), 1u);
    MESH_EXPECT_EQ(mesh.selectedFaces.size(), 0u);
    
    return true;
}

// ============================================================================
// Undo/Redo Tests
// ============================================================================

inline bool testUndoRedo() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2});
    
    // Save initial state
    float originalX = mesh.vertices[0].position[0];
    
    // Select and translate
    mesh.selectedVertices.insert(0);
    mesh.translateSelected(5.0f, 0, 0);
    
    MESH_EXPECT_NEAR(mesh.vertices[0].position[0], 5.0f, 0.001f);
    MESH_EXPECT_TRUE(mesh.canUndo());
    
    // Undo
    mesh.undo();
    MESH_EXPECT_NEAR(mesh.vertices[0].position[0], originalX, 0.001f);
    MESH_EXPECT_TRUE(mesh.canRedo());
    
    // Redo
    mesh.redo();
    MESH_EXPECT_NEAR(mesh.vertices[0].position[0], 5.0f, 0.001f);
    
    return true;
}

// ============================================================================
// Edge Operations Tests
// ============================================================================

inline bool testEdgeDetection() {
    EditMesh mesh;
    
    // Create a quad
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(1, 1, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2, 3});
    
    // Check edges
    MESH_EXPECT_EQ(mesh.edges.size(), 4u);
    
    // All edges should be original edges (not from triangulation)
    for (const EditEdge& edge : mesh.edges) {
        MESH_EXPECT_TRUE(edge.originalEdge);
    }
    
    return true;
}

inline bool testEdgeHasVertex() {
    EditEdge edge(5, 10, true);
    
    MESH_EXPECT_TRUE(edge.hasVertex(5));
    MESH_EXPECT_TRUE(edge.hasVertex(10));
    MESH_EXPECT_TRUE(!edge.hasVertex(7));
    
    MESH_EXPECT_EQ(edge.otherVertex(5), 10u);
    MESH_EXPECT_EQ(edge.otherVertex(10), 5u);
    
    return true;
}

// ============================================================================
// Face Normal Tests
// ============================================================================

inline bool testFaceNormalCalculation() {
    EditMesh mesh;
    
    // Create a quad on XY plane
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(1, 1, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2, 3});
    
    // Normal should point in Z direction
    const EditFace& face = mesh.faces[0];
    float nz = face.loops[0].normal[2];
    
    MESH_EXPECT_TRUE(std::abs(nz) > 0.99f);  // Close to 1 or -1
    
    return true;
}

// ============================================================================
// Mesh Clear Tests
// ============================================================================

inline bool testClear() {
    EditMesh mesh;
    
    mesh.addVertex(0, 0, 0);
    mesh.addVertex(1, 0, 0);
    mesh.addVertex(0, 1, 0);
    mesh.addFace({0, 1, 2});
    mesh.selectedVertices.insert(0);
    mesh.selectedFaces.insert(0);
    
    mesh.clear();
    
    MESH_EXPECT_EQ(mesh.vertices.size(), 0u);
    MESH_EXPECT_EQ(mesh.faces.size(), 0u);
    MESH_EXPECT_EQ(mesh.edges.size(), 0u);
    MESH_EXPECT_EQ(mesh.selectedVertices.size(), 0u);
    MESH_EXPECT_EQ(mesh.selectedFaces.size(), 0u);
    
    return true;
}

// ============================================================================
// Test Registration
// ============================================================================

inline void registerMeshEditTests(UnitTestRunner& runner) {
    runner.addTest("MeshEdit", "Add Vertex", testAddVertex);
    runner.addTest("MeshEdit", "Add Triangle Face", testAddTriangleFace);
    runner.addTest("MeshEdit", "Add Quad Face", testAddQuadFace);
    runner.addTest("MeshEdit", "Add N-gon Face", testAddNgonFace);
    runner.addTest("MeshEdit", "Vertex Selection", testVertexSelection);
    runner.addTest("MeshEdit", "Face Selection", testFaceSelection);
    runner.addTest("MeshEdit", "Edge Selection", testEdgeSelection);
    runner.addTest("MeshEdit", "Translate Selected", testTranslateSelected);
    runner.addTest("MeshEdit", "Scale Selected", testScaleSelected);
    runner.addTest("MeshEdit", "Extrude Face", testExtrudeFace);
    runner.addTest("MeshEdit", "Subdivide Face", testSubdivideFace);
    runner.addTest("MeshEdit", "Delete Face", testDeleteFace);
    runner.addTest("MeshEdit", "Undo/Redo", testUndoRedo);
    runner.addTest("MeshEdit", "Edge Detection", testEdgeDetection);
    runner.addTest("MeshEdit", "Edge Has Vertex", testEdgeHasVertex);
    runner.addTest("MeshEdit", "Face Normal Calculation", testFaceNormalCalculation);
    runner.addTest("MeshEdit", "Clear Mesh", testClear);
}

} // namespace MeshEditTests
} // namespace test
} // namespace luma
