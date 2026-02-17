// LUMA Material Preview
// Real-time 3D preview of materials in the node editor
// Renders a preview sphere/cube/plane with the generated material shader

#pragma once

#include "engine/material/material_node.h"
#include "engine/material/shader_generator.h"
#include <string>
#include <cmath>

namespace luma {

// ===== Preview Shape =====
enum class PreviewShape {
    Sphere = 0,
    Cube,
    Plane,
    Cylinder,
    Torus,
    Count
};

inline const char* previewShapeName(PreviewShape shape) {
    switch (shape) {
        case PreviewShape::Sphere:   return "Sphere";
        case PreviewShape::Cube:     return "Cube";
        case PreviewShape::Plane:    return "Plane";
        case PreviewShape::Cylinder: return "Cylinder";
        case PreviewShape::Torus:    return "Torus";
        default:                     return "Sphere";
    }
}

// ===== Preview State =====
struct MaterialPreviewState {
    PreviewShape shape = PreviewShape::Sphere;
    float yaw = 0.4f;       // Camera yaw
    float pitch = 0.3f;     // Camera pitch
    float distance = 3.0f;  // Camera distance
    bool autoRotate = false;
    float rotationSpeed = 0.5f;
    
    // Environment
    bool showEnvironment = true;
    float envIntensity = 1.0f;
    float lightYaw = 0.8f;
    float lightPitch = 0.6f;
    
    // Display
    bool showWireframe = false;
    bool showUVs = false;
    int resolution = 256;  // Preview render resolution
    
    // Runtime (managed by renderer)
    void* previewRenderTarget = nullptr;
    void* previewSRV = nullptr;
    bool needsUpdate = true;
    
    // Compiled material
    uint64_t lastShaderHash = 0;
    void* compiledPSO = nullptr;
    std::vector<uint8_t> materialCBData;
};

// Forward declaration
inline void drawPlaceholderPreview(ImDrawList* dl, ImVec2 pos, ImVec2 size,
                                    MaterialPreviewState& preview,
                                    MaterialNodeEditorState& editorState);

// ===== Material Preview Panel =====
// ImGui window that shows a real-time 3D preview of the material

inline void drawMaterialPreviewPanel(MaterialPreviewState& preview, 
                                      MaterialNodeEditorState& editorState,
                                      bool& showWindow) {
    if (!showWindow) return;
    
    ImGui::SetNextWindowSize(ImVec2(300, 380), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Material Preview", &showWindow)) {
        ImGui::End();
        return;
    }
    
    // Preview image area
    ImVec2 previewSize = ImGui::GetContentRegionAvail();
    previewSize.y = previewSize.x; // Square
    if (previewSize.y > ImGui::GetContentRegionAvail().y - 150) {
        previewSize.y = ImGui::GetContentRegionAvail().y - 150;
        previewSize.x = previewSize.y;
    }
    
    // Preview area background
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Draw checkerboard background (for transparency preview)
    float checkSize = 8;
    for (float y = 0; y < previewSize.y; y += checkSize) {
        for (float x = 0; x < previewSize.x; x += checkSize) {
            int cx = (int)(x / checkSize);
            int cy = (int)(y / checkSize);
            ImU32 col = ((cx + cy) % 2 == 0) ? IM_COL32(60, 60, 60, 255) : IM_COL32(40, 40, 40, 255);
            dl->AddRectFilled(ImVec2(pos.x + x, pos.y + y),
                             ImVec2(pos.x + std::min(x + checkSize, previewSize.x),
                                    pos.y + std::min(y + checkSize, previewSize.y)), col);
        }
    }
    
    // If we have a render target, display it
    if (preview.previewSRV) {
        // ImGui::Image would use the SRV here
        // ImGui::Image((ImTextureID)preview.previewSRV, previewSize);
    } else {
        // Placeholder: Draw a simple preview sphere using ImGui draw primitives
        drawPlaceholderPreview(dl, pos, previewSize, preview, editorState);
    }
    
    // Border
    dl->AddRect(pos, ImVec2(pos.x + previewSize.x, pos.y + previewSize.y),
                IM_COL32(80, 80, 80, 255));
    
    // Interaction: rotate preview with mouse drag
    ImGui::InvisibleButton("preview_area", previewSize);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        preview.yaw += ImGui::GetIO().MouseDelta.x * 0.01f;
        preview.pitch = std::clamp(preview.pitch + ImGui::GetIO().MouseDelta.y * 0.01f, -1.5f, 1.5f);
        preview.needsUpdate = true;
    }
    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            preview.distance = std::clamp(preview.distance - wheel * 0.3f, 1.0f, 10.0f);
            preview.needsUpdate = true;
        }
    }
    
    // Auto rotate
    if (preview.autoRotate) {
        preview.yaw += preview.rotationSpeed * ImGui::GetIO().DeltaTime;
        preview.needsUpdate = true;
    }
    
    // Controls
    ImGui::Separator();
    
    // Shape selector
    ImGui::Text("Shape:");
    ImGui::SameLine();
    for (int i = 0; i < (int)PreviewShape::Count; i++) {
        if (i > 0) ImGui::SameLine();
        bool selected = ((int)preview.shape == i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::SmallButton(previewShapeName((PreviewShape)i))) {
            preview.shape = (PreviewShape)i;
            preview.needsUpdate = true;
        }
        if (selected) ImGui::PopStyleColor();
    }
    
    ImGui::Checkbox("Auto Rotate", &preview.autoRotate);
    if (preview.autoRotate) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("##speed", &preview.rotationSpeed, 0.1f, 3.0f);
    }
    
    ImGui::Checkbox("Wireframe", &preview.showWireframe);
    ImGui::SameLine();
    ImGui::Checkbox("Show UVs", &preview.showUVs);
    
    // Light direction
    if (ImGui::CollapsingHeader("Lighting")) {
        if (ImGui::SliderFloat("Light Yaw", &preview.lightYaw, -3.14f, 3.14f)) {
            preview.needsUpdate = true;
        }
        if (ImGui::SliderFloat("Light Pitch", &preview.lightPitch, 0.0f, 1.5f)) {
            preview.needsUpdate = true;
        }
        if (ImGui::SliderFloat("Env Intensity", &preview.envIntensity, 0.0f, 3.0f)) {
            preview.needsUpdate = true;
        }
    }
    
    // Compile button
    ImGui::Separator();
    if (ImGui::Button("Compile & Preview", ImVec2(-1, 0))) {
        if (editorState.graph) {
            ShaderGenerator gen;
            auto result = gen.generate(*editorState.graph);
            editorState.generatedHLSL = result.hlslCode;
            editorState.lastError = result.error;
            editorState.graph->needsRecompile = false;
            preview.needsUpdate = true;
        }
    }
    
    ImGui::End();
}

// ===== Placeholder Preview (CPU-rendered approximation) =====
inline void drawPlaceholderPreview(ImDrawList* dl, ImVec2 pos, ImVec2 size,
                                    MaterialPreviewState& preview,
                                    MaterialNodeEditorState& editorState) {
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float radius = size.x * 0.35f;
    
    // Get base color from material graph output node
    float baseR = 0.8f, baseG = 0.8f, baseB = 0.8f;
    float metallic = 0.0f, roughness = 0.5f;
    
    if (editorState.graph) {
        auto* outputNode = editorState.graph->findNode(editorState.graph->outputNodeId);
        if (outputNode) {
            // Try to get base color from connected Color Constant node
            for (const auto& pin : outputNode->inputs) {
                if (pin.name == "Base Color" && !pin.connected) {
                    if (std::holds_alternative<Vec4>(pin.defaultValue)) {
                        Vec4 c = std::get<Vec4>(pin.defaultValue);
                        baseR = c.x; baseG = c.y; baseB = c.z;
                    }
                }
                if (pin.name == "Metallic" && !pin.connected) {
                    if (std::holds_alternative<float>(pin.defaultValue)) {
                        metallic = std::get<float>(pin.defaultValue);
                    }
                }
                if (pin.name == "Roughness" && !pin.connected) {
                    if (std::holds_alternative<float>(pin.defaultValue)) {
                        roughness = std::get<float>(pin.defaultValue);
                    }
                }
            }
        }
    }
    
    // Simple sphere shading preview
    if (preview.shape == PreviewShape::Sphere) {
        float lightX = std::cos(preview.lightYaw) * std::cos(preview.lightPitch);
        float lightY = std::sin(preview.lightPitch);
        float lightZ = std::sin(preview.lightYaw) * std::cos(preview.lightPitch);
        
        int steps = 32;
        float stepSize = radius * 2.0f / steps;
        
        for (int iy = 0; iy < steps; iy++) {
            for (int ix = 0; ix < steps; ix++) {
                float sx = (ix - steps * 0.5f + 0.5f) / (steps * 0.5f);
                float sy = (iy - steps * 0.5f + 0.5f) / (steps * 0.5f);
                float d = sx * sx + sy * sy;
                if (d > 1.0f) continue;
                
                float sz = std::sqrt(1.0f - d);
                
                // Simple diffuse + specular
                float ndotl = std::max(0.0f, sx * lightX + sy * lightY + sz * lightZ);
                
                // View direction (from camera)
                float vx = 0, vy = 0, vz = 1;
                float hx = lightX + vx, hy = lightY + vy, hz = lightZ + vz;
                float hlen = std::sqrt(hx*hx + hy*hy + hz*hz);
                if (hlen > 0.001f) { hx /= hlen; hy /= hlen; hz /= hlen; }
                float ndoth = std::max(0.0f, sx * hx + sy * hy + sz * hz);
                
                float spec = std::pow(ndoth, 2.0f / (roughness * roughness + 0.01f)) * (1.0f - roughness * 0.5f);
                float ambient = 0.15f;
                
                float r = baseR * (ndotl * 0.7f + ambient) + spec * (metallic > 0.5f ? baseR : 1.0f) * 0.3f;
                float g = baseG * (ndotl * 0.7f + ambient) + spec * (metallic > 0.5f ? baseG : 1.0f) * 0.3f;
                float b = baseB * (ndotl * 0.7f + ambient) + spec * (metallic > 0.5f ? baseB : 1.0f) * 0.3f;
                
                r = std::clamp(r, 0.0f, 1.0f);
                g = std::clamp(g, 0.0f, 1.0f);
                b = std::clamp(b, 0.0f, 1.0f);
                
                ImU32 col = IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
                float px = cx + sx * radius;
                float py = cy + sy * radius;
                dl->AddRectFilled(ImVec2(px - stepSize * 0.5f, py - stepSize * 0.5f),
                                 ImVec2(px + stepSize * 0.5f, py + stepSize * 0.5f), col);
            }
        }
    }
    else if (preview.shape == PreviewShape::Plane) {
        // Flat plane preview
        float planeW = radius * 1.5f;
        float planeH = radius * 1.5f;
        
        ImU32 col = IM_COL32((int)(baseR * 200), (int)(baseG * 200), (int)(baseB * 200), 255);
        dl->AddRectFilled(ImVec2(cx - planeW, cy - planeH * 0.3f),
                         ImVec2(cx + planeW, cy + planeH * 0.7f), col);
    }
    else {
        // Default: colored circle
        ImU32 col = IM_COL32((int)(baseR * 200), (int)(baseG * 200), (int)(baseB * 200), 255);
        dl->AddCircleFilled(ImVec2(cx, cy), radius, col, 48);
    }
    
    // Label
    dl->AddText(ImVec2(pos.x + 5, pos.y + size.y - 18),
                IM_COL32(200, 200, 200, 180), "CPU Preview (compile for GPU)");
}

} // namespace luma
