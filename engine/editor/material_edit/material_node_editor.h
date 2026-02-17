// LUMA Material Node Editor
// ImGui-based node editor for visual material authoring
// Supports node canvas, link creation, property editing, and shader preview

#pragma once

#include "engine/material/material_node.h"
#include "engine/material/material_node_library.h"
#include "engine/material/shader_generator.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>

// Forward declare ImGui types to avoid header dependency
struct ImVec2;
struct ImVec4;

namespace luma {

// Forward declarations
inline void drawNodeProperties(MaterialNodeEditorState& state, MaterialGraph& graph,
                               VisualScriptNode& node);
inline void drawColorRampEditor(MaterialNodeEditorState& state, MaterialGraph& graph,
                                VisualScriptNode& node);

// ===== Import Mesh Material into Node Graph =====
// Creates a complete node graph from a mesh's PBR material data.
// Layout follows Blender Shader Editor conventions:
//   Column 1 (-400): Texture Coordinate (shared)
//   Column 2 (-100): Image Texture nodes  
//   Column 3 (+250): Value/Color fallback nodes
//   Column 4 (+600): PBR Material Output
inline void importMeshMaterialToGraph(
    MaterialGraph& graph,
    const float baseColor[3], float metallic, float roughness,
    bool hasDiffuse, const std::string& diffusePath = "",
    bool hasNormal = false, const std::string& normalPath = "",
    bool hasSpecular = false, const std::string& specularPath = "")
{
    // Reset graph
    graph.createDefaultGraph();
    
    auto& lib = MaterialNodeLibrary::instance();
    
    // Node definitions
    const auto* outputDef   = lib.findByType("Mat_PBROutput");
    const auto* texDef      = lib.findByType("Mat_ImageTexture");
    const auto* texCoordDef = lib.findByType("Mat_TexCoord");
    const auto* colorDef    = lib.findByType("Mat_ColorConst");
    const auto* valDef      = lib.findByType("Mat_Value");
    
    if (!outputDef) return;
    
    // Layout constants
    const float colTexCoord = -400.0f;
    const float colTexture  = -100.0f;
    const float colValue    = 250.0f;
    const float colOutput   = 600.0f;
    const float rowSpacing  = 200.0f;
    
    // ====== PBR Material Output (rightmost) ======
    auto* output = graph.createMaterialNode(*outputDef, colOutput, 200);
    
    // ====== Shared Texture Coordinate node ======
    // Like Blender, one TexCoord feeds all texture nodes
    VisualScriptNode* sharedUV = nullptr;
    bool anyTexture = hasDiffuse || hasNormal || hasSpecular;
    if (anyTexture && texCoordDef) {
        sharedUV = graph.createMaterialNode(*texCoordDef, colTexCoord, 200);
        sharedUV->displayName = "Texture Coordinate";
    }
    
    // ====== Row 0: Base Color (Diffuse) ======
    float row0Y = 50.0f;
    if (hasDiffuse && texDef) {
        // Image Texture → Base Color
        auto* diffTex = graph.createMaterialNode(*texDef, colTexture, row0Y);
        if (!diffusePath.empty()) {
            diffTex->properties["Texture"] = PinValue(diffusePath);
        }
        diffTex->displayName = "Diffuse Map";
        
        // UV → Texture
        if (sharedUV && !sharedUV->outputs.empty() && !diffTex->inputs.empty()) {
            graph.createLink(sharedUV->id, sharedUV->outputs[0].id,
                            diffTex->id, diffTex->inputs[0].id);
        }
        // Texture Color → Output Base Color
        if (!diffTex->outputs.empty() && !output->inputs.empty()) {
            graph.createLink(diffTex->id, diffTex->outputs[0].id,
                            output->id, output->inputs[0].id);
        }
        
        // Also add a color node showing the base color tint (multiplier, like Blender)
        if (colorDef) {
            auto* colorNode = graph.createMaterialNode(*colorDef, colValue, row0Y);
            colorNode->properties["Color"] = PinValue(Vec4(baseColor[0], baseColor[1], baseColor[2], 1.0f));
            colorNode->displayName = "Base Color Tint";
            // Not connected - available for user to multiply with texture
        }
    } else {
        // No texture: Color constant → Base Color
        if (colorDef) {
            auto* colorNode = graph.createMaterialNode(*colorDef, colValue, row0Y);
            colorNode->properties["Color"] = PinValue(Vec4(baseColor[0], baseColor[1], baseColor[2], 1.0f));
            colorNode->displayName = "Base Color";
            
            if (!colorNode->outputs.empty() && !output->inputs.empty()) {
                graph.createLink(colorNode->id, colorNode->outputs[0].id,
                                output->id, output->inputs[0].id);
            }
        }
    }
    
    // ====== Row 1: Metallic ======
    float row1Y = row0Y + rowSpacing;
    if (valDef) {
        auto* metalNode = graph.createMaterialNode(*valDef, colValue, row1Y);
        metalNode->properties["Value"] = PinValue(metallic);
        metalNode->displayName = "Metallic";
        
        if (!metalNode->outputs.empty() && output->inputs.size() > 1) {
            graph.createLink(metalNode->id, metalNode->outputs[0].id,
                            output->id, output->inputs[1].id);
        }
    }
    
    // ====== Row 2: Roughness ======
    float row2Y = row1Y + rowSpacing * 0.7f;
    if (valDef) {
        auto* roughNode = graph.createMaterialNode(*valDef, colValue, row2Y);
        roughNode->properties["Value"] = PinValue(roughness);
        roughNode->displayName = "Roughness";
        
        if (!roughNode->outputs.empty() && output->inputs.size() > 2) {
            graph.createLink(roughNode->id, roughNode->outputs[0].id,
                            output->id, output->inputs[2].id);
        }
    }
    
    // ====== Row 3: Normal Map ======
    float row3Y = row2Y + rowSpacing;
    if (hasNormal && texDef) {
        auto* normalTex = graph.createMaterialNode(*texDef, colTexture, row3Y);
        if (!normalPath.empty()) {
            normalTex->properties["Texture"] = PinValue(normalPath);
        }
        normalTex->displayName = "Normal Map";
        
        // UV → Normal Texture
        if (sharedUV && !sharedUV->outputs.empty() && !normalTex->inputs.empty()) {
            graph.createLink(sharedUV->id, sharedUV->outputs[0].id,
                            normalTex->id, normalTex->inputs[0].id);
        }
        // Normal Texture → Output Normal (index 3)
        if (!normalTex->outputs.empty() && output->inputs.size() > 3) {
            graph.createLink(normalTex->id, normalTex->outputs[0].id,
                            output->id, output->inputs[3].id);
        }
    }
    
    // ====== Row 4: Specular/Metallic-Roughness Map ======
    float row4Y = row3Y + rowSpacing;
    if (hasSpecular && texDef) {
        auto* specTex = graph.createMaterialNode(*texDef, colTexture, row4Y);
        if (!specularPath.empty()) {
            specTex->properties["Texture"] = PinValue(specularPath);
        }
        specTex->displayName = "Specular Map";
        
        // UV → Specular Texture
        if (sharedUV && !sharedUV->outputs.empty() && !specTex->inputs.empty()) {
            graph.createLink(sharedUV->id, sharedUV->outputs[0].id,
                            specTex->id, specTex->inputs[0].id);
        }
        // Not auto-connected to output - user decides how to route
        // (R=Metallic, G=Roughness, B=AO is common but varies)
    }
    
    graph.needsRecompile = true;
}

// ===== Pin Position Helper =====
// Calculates screen-space pin positions for a node
struct PinLayout {
    static ImVec2 getInputPinPos(const VisualScriptNode& node, int pinIndex,
                                  ImVec2 nodeScreenPos, float zoom) {
        float y = nodeScreenPos.y + (30 + pinIndex * 22) * zoom;
        return {nodeScreenPos.x, y};
    }
    
    static ImVec2 getOutputPinPos(const VisualScriptNode& node, int pinIndex,
                                   ImVec2 nodeScreenPos, ImVec2 nodeScreenSize, float zoom) {
        float y = nodeScreenPos.y + (30 + pinIndex * 22) * zoom;
        return {nodeScreenPos.x + nodeScreenSize.x, y};
    }
};

// ===== Material Node Editor =====
// Complete ImGui drawing function for the material node editor

inline void drawMaterialNodeEditor(MaterialNodeEditorState& state, bool& showWindow) {
    if (!showWindow) return;
    if (!state.graph) {
        state.init();
        // Create default output node
        const auto* outputDef = MaterialNodeLibrary::instance().findByType("Mat_PBROutput");
        if (outputDef) {
            state.graph->createMaterialNode(*outputDef, 400, 200);
        }
    }
    
    auto& graph = *state.graph;
    auto& lib = MaterialNodeLibrary::instance();
    
    ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;
    
    if (!ImGui::Begin("Material Node Editor", &showWindow, flags)) {
        ImGui::End();
        return;
    }
    
    // =========================================================================
    // MENU BAR
    // =========================================================================
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Material")) {
                state.graph = std::make_unique<MaterialGraph>();
                const auto* outDef = lib.findByType("Mat_PBROutput");
                if (outDef) state.graph->createMaterialNode(*outDef, 400, 200);
                state.reset();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Material...")) { /* TODO */ }
            if (ImGui::MenuItem("Load Material...")) { /* TODO */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Delete Node", "Del", false, state.selectedNodeId >= 0)) {
                graph.deleteNode(state.selectedNodeId);
                state.selectedNodeId = -1;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Compile Shader", "F5")) {
                ShaderGenerator gen;
                auto result = gen.generate(graph);
                state.generatedHLSL = result.hlslCode;
                state.lastError = result.error;
                state.isCompiling = false;
                graph.needsRecompile = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset View")) {
                state.scrollOffset = {0, 0};
                state.zoom = 1.0f;
            }
            if (ImGui::MenuItem("Zoom to Fit")) {
                // Calculate bounds of all nodes
                if (!graph.nodes.empty()) {
                    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
                    for (const auto& n : graph.nodes) {
                        minX = std::min(minX, n->position.x);
                        minY = std::min(minY, n->position.y);
                        maxX = std::max(maxX, n->position.x + n->size.x);
                        maxY = std::max(maxY, n->position.y + n->size.y);
                    }
                    state.scrollOffset.x = -(minX + maxX) * 0.5f * state.zoom + 400;
                    state.scrollOffset.y = -(minY + maxY) * 0.5f * state.zoom + 300;
                }
            }
            ImGui::Separator();
            ImGui::MenuItem("Show Generated HLSL", nullptr, &state.showGeneratedCode);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Add")) {
            for (auto cat : MaterialNodeLibrary::getMaterialCategories()) {
                if (cat == NodeCategory::Mat_Output) continue; // Only one output
                if (ImGui::BeginMenu(getCategoryName(cat))) {
                    auto nodes = lib.getByCategory(cat);
                    for (const auto* def : nodes) {
                        if (ImGui::MenuItem(def->displayName.c_str())) {
                            graph.createMaterialNode(*def, -state.scrollOffset.x / state.zoom + 100,
                                                      -state.scrollOffset.y / state.zoom + 100);
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }
        
        // Status indicator
        ImGui::Separator();
        if (graph.needsRecompile) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " [Modified]");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), " [Compiled]");
        }
        
        if (!state.lastError.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), " Error: %s", state.lastError.c_str());
        }
        
        ImGui::EndMenuBar();
    }
    
    // =========================================================================
    // LAYOUT: Left Sidebar | Canvas | Right Properties
    // =========================================================================
    
    float sidebarWidth = 180;
    float propPanelWidth = state.selectedNodeId >= 0 ? 240 : 0;
    float canvasWidth = ImGui::GetContentRegionAvail().x - sidebarWidth - propPanelWidth - 8;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    
    // === Left Sidebar: Node palette ===
    ImGui::BeginChild("NodePalette", ImVec2(sidebarWidth, totalHeight), true);
    
    ImGui::Text("Node Palette");
    ImGui::Separator();
    
    // Search
    ImGui::SetNextItemWidth(-1);
    static char paletteSearch[128] = {0};
    ImGui::InputTextWithHint("##palsearch", "Search...", paletteSearch, sizeof(paletteSearch));
    
    if (strlen(paletteSearch) > 0) {
        auto results = lib.search(paletteSearch);
        for (const auto* def : results) {
            if (def->isOutput) continue;
            if (ImGui::Selectable(def->displayName.c_str())) {
                graph.createMaterialNode(*def, -state.scrollOffset.x / state.zoom + 100,
                                          -state.scrollOffset.y / state.zoom + 100);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", def->description.c_str());
            }
        }
    } else {
        for (auto cat : MaterialNodeLibrary::getMaterialCategories()) {
            if (cat == NodeCategory::Mat_Output) continue;
            
            ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen;
            if (ImGui::TreeNodeEx(getCategoryName(cat), treeFlags)) {
                auto nodes = lib.getByCategory(cat);
                for (const auto* def : nodes) {
                    if (def->isOutput) continue;
                    if (ImGui::Selectable(def->displayName.c_str())) {
                        graph.createMaterialNode(*def, -state.scrollOffset.x / state.zoom + 100,
                                                  -state.scrollOffset.y / state.zoom + 100);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", def->description.c_str());
                    }
                }
                ImGui::TreePop();
            }
        }
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // === Canvas ===
    ImGui::BeginChild("NodeCanvas", ImVec2(canvasWidth, totalHeight), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    // Clip to canvas region
    drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
    
    // ---- Background Grid ----
    float gridSize = 32.0f * state.zoom;
    ImU32 gridColor = IM_COL32(45, 45, 50, 255);
    ImU32 gridColorBold = IM_COL32(60, 60, 68, 255);
    
    for (float x = fmodf(state.scrollOffset.x, gridSize); x < canvasSize.x; x += gridSize) {
        bool bold = (fmodf(x - state.scrollOffset.x, gridSize * 4) < 1.0f);
        drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                          ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
                          bold ? gridColorBold : gridColor);
    }
    for (float y = fmodf(state.scrollOffset.y, gridSize); y < canvasSize.y; y += gridSize) {
        bool bold = (fmodf(y - state.scrollOffset.y, gridSize * 4) < 1.0f);
        drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                          ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
                          bold ? gridColorBold : gridColor);
    }
    
    // ---- Draw Links ----
    for (const auto& link : graph.links) {
        auto* fromNode = graph.findNode(link.fromNode);
        auto* toNode = graph.findNode(link.toNode);
        if (!fromNode || !toNode) continue;
        
        auto* fromPin = fromNode->findPin(link.fromPin);
        auto* toPin = toNode->findPin(link.toPin);
        if (!fromPin || !toPin) continue;
        
        // Find pin indices
        int fromIdx = 0;
        for (int i = 0; i < (int)fromNode->outputs.size(); i++) {
            if (fromNode->outputs[i].id == link.fromPin) { fromIdx = i; break; }
        }
        int toIdx = 0;
        for (int i = 0; i < (int)toNode->inputs.size(); i++) {
            if (toNode->inputs[i].id == link.toPin) { toIdx = i; break; }
        }
        
        ImVec2 nodePos1(canvasPos.x + fromNode->position.x * state.zoom + state.scrollOffset.x,
                        canvasPos.y + fromNode->position.y * state.zoom + state.scrollOffset.y);
        ImVec2 nodeSize1(fromNode->size.x * state.zoom, fromNode->size.y * state.zoom);
        ImVec2 nodePos2(canvasPos.x + toNode->position.x * state.zoom + state.scrollOffset.x,
                        canvasPos.y + toNode->position.y * state.zoom + state.scrollOffset.y);
        
        ImVec2 p1 = PinLayout::getOutputPinPos(*fromNode, fromIdx, nodePos1, nodeSize1, state.zoom);
        ImVec2 p4 = PinLayout::getInputPinPos(*toNode, toIdx, nodePos2, state.zoom);
        
        float tangentLen = std::max(50.0f * state.zoom, std::abs(p4.x - p1.x) * 0.4f);
        ImVec2 p2(p1.x + tangentLen, p1.y);
        ImVec2 p3(p4.x - tangentLen, p4.y);
        
        ImU32 linkColor = getPinColor(fromPin->type);
        // Dim the link slightly
        linkColor = (linkColor & 0x00FFFFFF) | 0xCC000000;
        drawList->AddBezierCubic(p1, p2, p3, p4, linkColor, 2.5f * state.zoom);
    }
    
    // ---- Draw link being created ----
    if (state.creatingLink) {
        auto* startNode = graph.findNode(state.linkStartNode);
        if (startNode) {
            auto* startPin = startNode->findPin(state.linkStartPin);
            if (startPin) {
                ImVec2 nodePos(canvasPos.x + startNode->position.x * state.zoom + state.scrollOffset.x,
                               canvasPos.y + startNode->position.y * state.zoom + state.scrollOffset.y);
                ImVec2 nodeSize(startNode->size.x * state.zoom, startNode->size.y * state.zoom);
                
                ImVec2 startPos;
                if (state.linkStartIsOutput) {
                    int idx = 0;
                    for (int i = 0; i < (int)startNode->outputs.size(); i++) {
                        if (startNode->outputs[i].id == state.linkStartPin) { idx = i; break; }
                    }
                    startPos = PinLayout::getOutputPinPos(*startNode, idx, nodePos, nodeSize, state.zoom);
                } else {
                    int idx = 0;
                    for (int i = 0; i < (int)startNode->inputs.size(); i++) {
                        if (startNode->inputs[i].id == state.linkStartPin) { idx = i; break; }
                    }
                    startPos = PinLayout::getInputPinPos(*startNode, idx, nodePos, state.zoom);
                }
                
                ImVec2 mousePos = ImGui::GetMousePos();
                ImU32 previewColor = getPinColor(startPin->type);
                previewColor = (previewColor & 0x00FFFFFF) | 0x88000000;
                
                float tangent = std::max(50.0f, std::abs(mousePos.x - startPos.x) * 0.4f);
                if (state.linkStartIsOutput) {
                    drawList->AddBezierCubic(startPos,
                        ImVec2(startPos.x + tangent, startPos.y),
                        ImVec2(mousePos.x - tangent, mousePos.y),
                        mousePos, previewColor, 2.0f * state.zoom);
                } else {
                    drawList->AddBezierCubic(startPos,
                        ImVec2(startPos.x - tangent, startPos.y),
                        ImVec2(mousePos.x + tangent, mousePos.y),
                        mousePos, previewColor, 2.0f * state.zoom);
                }
            }
        }
    }
    
    // ---- Draw Nodes ----
    state.hoveredNodeId = -1;
    
    for (const auto& nodePtr : graph.nodes) {
        auto& node = *nodePtr;
        
        ImVec2 nodePos(canvasPos.x + node.position.x * state.zoom + state.scrollOffset.x,
                       canvasPos.y + node.position.y * state.zoom + state.scrollOffset.y);
        int maxPins = std::max((int)node.inputs.size(), (int)node.outputs.size());
        float nodeHeight = (30 + maxPins * 22 + 8) * state.zoom;
        ImVec2 nodeSize(node.size.x * state.zoom, nodeHeight);
        
        // Skip if fully outside canvas
        if (nodePos.x + nodeSize.x < canvasPos.x || nodePos.x > canvasPos.x + canvasSize.x ||
            nodePos.y + nodeSize.y < canvasPos.y || nodePos.y > canvasPos.y + canvasSize.y) {
            continue;
        }
        
        bool isSelected = (state.selectedNodeId == (int)node.id);
        bool isOutput = (node.id == graph.outputNodeId);
        
        // --- Node Shadow ---
        drawList->AddRectFilled(ImVec2(nodePos.x + 3, nodePos.y + 3),
                                ImVec2(nodePos.x + nodeSize.x + 3, nodePos.y + nodeSize.y + 3),
                                IM_COL32(0, 0, 0, 60), 6.0f);
        
        // --- Node Background ---
        ImU32 bgColor = isSelected ? IM_COL32(55, 55, 65, 245) : IM_COL32(38, 38, 43, 240);
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y),
                                bgColor, 6.0f);
        
        // --- Header ---
        float headerH = 26 * state.zoom;
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + headerH),
                                node.headerColor, 6.0f, ImDrawFlags_RoundCornersTop);
        // Header gradient overlay
        drawList->AddRectFilledMultiColor(
            ImVec2(nodePos.x, nodePos.y + headerH * 0.5f),
            ImVec2(nodePos.x + nodeSize.x, nodePos.y + headerH),
            IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
            IM_COL32(0, 0, 0, 60), IM_COL32(0, 0, 0, 60));
        
        // --- Title ---
        ImVec2 titlePos(nodePos.x + 10 * state.zoom, nodePos.y + 5 * state.zoom);
        if (state.zoom >= 0.6f) {
            drawList->AddText(titlePos, IM_COL32(255, 255, 255, 230), node.displayName.c_str());
        }
        
        // --- Output node indicator ---
        if (isOutput) {
            drawList->AddRectFilled(ImVec2(nodePos.x + 2, nodePos.y + headerH),
                                    ImVec2(nodePos.x + 4, nodePos.y + nodeSize.y - 2),
                                    IM_COL32(80, 200, 80, 200));
        }
        
        // --- Border ---
        ImU32 borderColor = isSelected ? IM_COL32(100, 160, 255, 255) : IM_COL32(70, 70, 80, 200);
        drawList->AddRect(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y),
                          borderColor, 6.0f, 0, isSelected ? 2.0f : 1.0f);
        
        // --- Input Pins ---
        float pinRadius = 5.0f * state.zoom;
        for (int i = 0; i < (int)node.inputs.size(); i++) {
            const auto& pin = node.inputs[i];
            ImVec2 pinPos = PinLayout::getInputPinPos(node, i, nodePos, state.zoom);
            
            ImU32 pinColor = getPinColor(pin.type);
            if (pin.connected) {
                drawList->AddCircleFilled(pinPos, pinRadius, pinColor);
            } else {
                drawList->AddCircle(pinPos, pinRadius, pinColor, 12, 1.5f);
                drawList->AddCircleFilled(pinPos, pinRadius * 0.4f, pinColor);
            }
            
            // Pin label
            if (state.zoom >= 0.5f) {
                drawList->AddText(ImVec2(pinPos.x + pinRadius + 6, pinPos.y - 7),
                                  IM_COL32(190, 190, 195, 255), pin.name.c_str());
            }
            
            // Pin interaction (invisible button for click)
            float hitSize = pinRadius * 3;
            ImGui::SetCursorScreenPos(ImVec2(pinPos.x - hitSize, pinPos.y - hitSize));
            std::string pinBtnId = "ipin_" + std::to_string(node.id) + "_" + std::to_string(pin.id);
            if (ImGui::InvisibleButton(pinBtnId.c_str(), ImVec2(hitSize * 2, hitSize * 2))) {
                if (state.creatingLink) {
                    // Complete link
                    if (state.linkStartIsOutput) {
                        graph.createLink(state.linkStartNode, state.linkStartPin, node.id, pin.id);
                    } else {
                        graph.createLink(node.id, pin.id, state.linkStartNode, state.linkStartPin);
                    }
                    state.creatingLink = false;
                } else {
                    // Start link from input (reverse)
                    state.creatingLink = true;
                    state.linkStartNode = node.id;
                    state.linkStartPin = pin.id;
                    state.linkStartIsOutput = false;
                }
            }
        }
        
        // --- Output Pins ---
        for (int i = 0; i < (int)node.outputs.size(); i++) {
            const auto& pin = node.outputs[i];
            ImVec2 pinPos = PinLayout::getOutputPinPos(node, i, nodePos, nodeSize, state.zoom);
            
            ImU32 pinColor = getPinColor(pin.type);
            if (pin.connected) {
                drawList->AddCircleFilled(pinPos, pinRadius, pinColor);
            } else {
                drawList->AddCircle(pinPos, pinRadius, pinColor, 12, 1.5f);
                drawList->AddCircleFilled(pinPos, pinRadius * 0.4f, pinColor);
            }
            
            // Pin label (right-aligned)
            if (state.zoom >= 0.5f) {
                ImVec2 textSize = ImGui::CalcTextSize(pin.name.c_str());
                drawList->AddText(ImVec2(pinPos.x - textSize.x - pinRadius - 6, pinPos.y - 7),
                                  IM_COL32(190, 190, 195, 255), pin.name.c_str());
            }
            
            // Pin interaction
            float hitSize = pinRadius * 3;
            ImGui::SetCursorScreenPos(ImVec2(pinPos.x - hitSize, pinPos.y - hitSize));
            std::string pinBtnId = "opin_" + std::to_string(node.id) + "_" + std::to_string(pin.id);
            if (ImGui::InvisibleButton(pinBtnId.c_str(), ImVec2(hitSize * 2, hitSize * 2))) {
                if (state.creatingLink) {
                    // Complete link
                    if (!state.linkStartIsOutput) {
                        graph.createLink(node.id, pin.id, state.linkStartNode, state.linkStartPin);
                    } else {
                        graph.createLink(state.linkStartNode, state.linkStartPin, node.id, pin.id);
                    }
                    state.creatingLink = false;
                } else {
                    // Start link from output
                    state.creatingLink = true;
                    state.linkStartNode = node.id;
                    state.linkStartPin = pin.id;
                    state.linkStartIsOutput = true;
                }
            }
        }
        
        // --- Node Body Interaction (drag + select) ---
        ImGui::SetCursorScreenPos(nodePos);
        std::string nodeBtnId = "node_" + std::to_string(node.id);
        ImGui::InvisibleButton(nodeBtnId.c_str(), nodeSize);
        
        bool nodeHovered = ImGui::IsItemHovered();
        if (nodeHovered) {
            state.hoveredNodeId = node.id;
        }
        
        if (ImGui::IsItemClicked(0)) {
            state.selectedNodeId = node.id;
            state.draggingNodeId = node.id;
            state.dragOffset = {ImGui::GetMousePos().x - nodePos.x,
                               ImGui::GetMousePos().y - nodePos.y};
        }
        
        if (state.draggingNodeId == (int)node.id && ImGui::IsMouseDragging(0)) {
            node.position.x += ImGui::GetIO().MouseDelta.x / state.zoom;
            node.position.y += ImGui::GetIO().MouseDelta.y / state.zoom;
        }
        
        if (ImGui::IsMouseReleased(0) && state.draggingNodeId == (int)node.id) {
            state.draggingNodeId = -1;
        }
    }
    
    // ---- Canvas Background Interaction ----
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("mat_canvas_bg", canvasSize);
    
    bool canvasHovered = ImGui::IsItemHovered();
    
    // Cancel link creation on right-click or escape
    if (state.creatingLink && (ImGui::IsMouseClicked(1) || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        state.creatingLink = false;
    }
    
    // Cancel link on left click on empty canvas
    if (state.creatingLink && ImGui::IsMouseClicked(0) && canvasHovered && state.hoveredNodeId < 0) {
        state.creatingLink = false;
    }
    
    // Pan with middle mouse
    if (canvasHovered && ImGui::IsMouseDragging(2)) {
        state.scrollOffset.x += ImGui::GetIO().MouseDelta.x;
        state.scrollOffset.y += ImGui::GetIO().MouseDelta.y;
    }
    
    // Also pan with Alt+Left drag
    if (canvasHovered && ImGui::IsMouseDragging(0) && ImGui::GetIO().KeyAlt) {
        state.scrollOffset.x += ImGui::GetIO().MouseDelta.x;
        state.scrollOffset.y += ImGui::GetIO().MouseDelta.y;
    }
    
    // Zoom with scroll wheel
    if (canvasHovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            float oldZoom = state.zoom;
            state.zoom = std::clamp(state.zoom + wheel * 0.1f, 0.2f, 3.0f);
            
            // Zoom toward mouse position
            ImVec2 mouseRel = {ImGui::GetMousePos().x - canvasPos.x, ImGui::GetMousePos().y - canvasPos.y};
            state.scrollOffset.x = mouseRel.x - (mouseRel.x - state.scrollOffset.x) * state.zoom / oldZoom;
            state.scrollOffset.y = mouseRel.y - (mouseRel.y - state.scrollOffset.y) * state.zoom / oldZoom;
        }
    }
    
    // Deselect on canvas click
    if (ImGui::IsItemClicked(0) && state.hoveredNodeId < 0) {
        state.selectedNodeId = -1;
    }
    
    // Delete key
    if (state.selectedNodeId >= 0 && ImGui::IsKeyPressed(ImGuiKey_Delete) && canvasHovered) {
        graph.deleteNode(state.selectedNodeId);
        state.selectedNodeId = -1;
    }
    
    // ---- Right-click context menu ----
    if (ImGui::IsItemClicked(1) && canvasHovered) {
        state.showContextMenu = true;
        state.contextMenuPos = {ImGui::GetMousePos().x - canvasPos.x,
                               ImGui::GetMousePos().y - canvasPos.y};
        state.searchBuffer[0] = '\0';
        ImGui::OpenPopup("MatNodeContextMenu");
    }
    
    if (ImGui::BeginPopup("MatNodeContextMenu")) {
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##matsearch", "Search nodes...", state.searchBuffer, sizeof(state.searchBuffer));
        ImGui::Separator();
        
        float addX = (state.contextMenuPos.x - state.scrollOffset.x) / state.zoom;
        float addY = (state.contextMenuPos.y - state.scrollOffset.y) / state.zoom;
        
        if (strlen(state.searchBuffer) > 0) {
            auto results = lib.search(state.searchBuffer);
            for (const auto* def : results) {
                if (def->isOutput) continue;
                if (ImGui::MenuItem(def->displayName.c_str())) {
                    graph.createMaterialNode(*def, addX, addY);
                }
            }
        } else {
            for (auto cat : MaterialNodeLibrary::getMaterialCategories()) {
                if (cat == NodeCategory::Mat_Output) continue;
                if (ImGui::BeginMenu(getCategoryName(cat))) {
                    auto nodes = lib.getByCategory(cat);
                    for (const auto* def : nodes) {
                        if (def->isOutput) continue;
                        if (ImGui::MenuItem(def->displayName.c_str())) {
                            graph.createMaterialNode(*def, addX, addY);
                        }
                    }
                    ImGui::EndMenu();
                }
            }
        }
        
        ImGui::EndPopup();
    }
    
    drawList->PopClipRect();
    ImGui::EndChild();
    
    // === Right Properties Panel ===
    if (state.selectedNodeId >= 0) {
        ImGui::SameLine();
        ImGui::BeginChild("NodeProperties", ImVec2(propPanelWidth, totalHeight), true);
        
        auto* selNode = graph.findNode(state.selectedNodeId);
        if (selNode) {
            drawNodeProperties(state, graph, *selNode);
        }
        
        ImGui::EndChild();
    }
    
    // =========================================================================
    // GENERATED HLSL VIEWER
    // =========================================================================
    if (state.showGeneratedCode) {
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Generated HLSL", &state.showGeneratedCode)) {
            if (ImGui::Button("Compile")) {
                ShaderGenerator gen;
                auto result = gen.generate(graph);
                state.generatedHLSL = result.hlslCode;
                state.lastError = result.error;
                graph.needsRecompile = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy to Clipboard") && !state.generatedHLSL.empty()) {
                ImGui::SetClipboardText(state.generatedHLSL.c_str());
            }
            
            ImGui::Separator();
            
            if (!state.generatedHLSL.empty()) {
                ImGui::BeginChild("HLSLCode", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.8f, 1.0f));
                ImGui::TextUnformatted(state.generatedHLSL.c_str());
                ImGui::PopStyleColor();
                ImGui::EndChild();
            } else {
                ImGui::TextDisabled("No shader generated yet. Click 'Compile' or press F5.");
            }
        }
        ImGui::End();
    }
    
    ImGui::End();
}

// ===== Node Properties Panel =====
inline void drawNodeProperties(MaterialNodeEditorState& state, MaterialGraph& graph,
                               VisualScriptNode& node) {
    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::Text("%s", node.displayName.c_str());
    ImGui::PopStyleColor();
    ImGui::TextDisabled("Type: %s", node.name.c_str());
    ImGui::TextDisabled("ID: %u", node.id);
    ImGui::Separator();
    
    // Get node definition for property descriptions
    const auto* def = MaterialNodeLibrary::instance().findByType(node.name);
    if (!def) return;
    
    // === Properties ===
    if (!def->properties.empty()) {
        ImGui::Text("Properties");
        ImGui::Separator();
        
        for (const auto& propDef : def->properties) {
            ImGui::PushID(propDef.name.c_str());
            
            auto it = node.properties.find(propDef.name);
            
            if (propDef.type == PinType::Float) {
                float val = 0.0f;
                if (it != node.properties.end() && std::holds_alternative<float>(it->second)) {
                    val = std::get<float>(it->second);
                }
                if (ImGui::SliderFloat(propDef.displayName.c_str(), &val, propDef.minValue, propDef.maxValue)) {
                    node.properties[propDef.name] = PinValue(val);
                    graph.needsRecompile = true;
                }
            }
            else if (propDef.type == PinType::Color) {
                float col[4] = {0.8f, 0.8f, 0.8f, 1.0f};
                if (it != node.properties.end() && std::holds_alternative<Vec4>(it->second)) {
                    Vec4 v = std::get<Vec4>(it->second);
                    col[0] = v.x; col[1] = v.y; col[2] = v.z; col[3] = v.w;
                }
                if (ImGui::ColorEdit4(propDef.displayName.c_str(), col, ImGuiColorEditFlags_Float)) {
                    node.properties[propDef.name] = PinValue(Vec4(col[0], col[1], col[2], col[3]));
                    graph.needsRecompile = true;
                }
            }
            else if (propDef.type == PinType::Vec3) {
                float v[3] = {0, 0, 0};
                if (it != node.properties.end() && std::holds_alternative<Vec3>(it->second)) {
                    Vec3 vec = std::get<Vec3>(it->second);
                    v[0] = vec.x; v[1] = vec.y; v[2] = vec.z;
                }
                if (ImGui::DragFloat3(propDef.displayName.c_str(), v, 0.01f)) {
                    node.properties[propDef.name] = PinValue(Vec3{v[0], v[1], v[2]});
                    graph.needsRecompile = true;
                }
            }
            else if (propDef.type == PinType::Int && !propDef.enumOptions.empty()) {
                int val = 0;
                if (it != node.properties.end() && std::holds_alternative<int>(it->second)) {
                    val = std::get<int>(it->second);
                }
                // Build combo string
                std::string combo;
                for (const auto& opt : propDef.enumOptions) {
                    combo += opt;
                    combo += '\0';
                }
                combo += '\0';
                if (ImGui::Combo(propDef.displayName.c_str(), &val, combo.c_str())) {
                    node.properties[propDef.name] = PinValue(val);
                    graph.needsRecompile = true;
                }
            }
            else if (propDef.type == PinType::Texture2D) {
                std::string path = "";
                if (it != node.properties.end() && std::holds_alternative<std::string>(it->second)) {
                    path = std::get<std::string>(it->second);
                }
                ImGui::Text("%s", propDef.displayName.c_str());
                
                // Texture path display
                if (path.empty()) {
                    ImGui::TextDisabled("No texture");
                } else {
                    // Show just filename
                    size_t lastSlash = path.find_last_of("/\\");
                    std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
                    ImGui::TextWrapped("%s", filename.c_str());
                }
                
                // Browse button
                if (ImGui::Button("Browse...")) {
                    // Platform file dialog would go here
                    // For now, just a text input
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear")) {
                    node.properties[propDef.name] = PinValue(std::string(""));
                    graph.needsRecompile = true;
                }
                
                // Manual path input
                static char texPath[512] = {0};
                strncpy(texPath, path.c_str(), sizeof(texPath) - 1);
                if (ImGui::InputText("Path", texPath, sizeof(texPath))) {
                    node.properties[propDef.name] = PinValue(std::string(texPath));
                    graph.needsRecompile = true;
                }
            }
            
            ImGui::PopID();
        }
    }
    
    // === Color Ramp Editor ===
    if (node.name == "Mat_ColorRamp") {
        ImGui::Separator();
        ImGui::Text("Color Ramp");
        drawColorRampEditor(state, graph, node);
    }
    
    // === Input Pin Defaults ===
    ImGui::Separator();
    ImGui::Text("Input Defaults");
    ImGui::Separator();
    
    for (auto& pin : node.inputs) {
        // Skip if connected (value comes from link)
        if (pin.connected) {
            ImGui::TextDisabled("%s: connected", pin.name.c_str());
            continue;
        }
        
        ImGui::PushID(pin.id);
        
        if (pin.type == PinType::Float) {
            float val = 0.0f;
            if (std::holds_alternative<float>(pin.defaultValue)) {
                val = std::get<float>(pin.defaultValue);
            }
            if (ImGui::DragFloat(pin.name.c_str(), &val, 0.01f)) {
                pin.defaultValue = PinValue(val);
                graph.needsRecompile = true;
            }
        }
        else if (pin.type == PinType::Color) {
            float col[4] = {0, 0, 0, 1};
            if (std::holds_alternative<Vec4>(pin.defaultValue)) {
                Vec4 v = std::get<Vec4>(pin.defaultValue);
                col[0] = v.x; col[1] = v.y; col[2] = v.z; col[3] = v.w;
            }
            if (ImGui::ColorEdit4(pin.name.c_str(), col)) {
                pin.defaultValue = PinValue(Vec4(col[0], col[1], col[2], col[3]));
                graph.needsRecompile = true;
            }
        }
        else if (pin.type == PinType::Vec3 || pin.type == PinType::Normal) {
            float v[3] = {0, 0, 0};
            if (std::holds_alternative<Vec3>(pin.defaultValue)) {
                Vec3 vec = std::get<Vec3>(pin.defaultValue);
                v[0] = vec.x; v[1] = vec.y; v[2] = vec.z;
            }
            if (ImGui::DragFloat3(pin.name.c_str(), v, 0.01f)) {
                pin.defaultValue = PinValue(Vec3{v[0], v[1], v[2]});
                graph.needsRecompile = true;
            }
        }
        else if (pin.type == PinType::Vec2 || pin.type == PinType::UV) {
            float v[2] = {0, 0};
            if (std::holds_alternative<Vec2>(pin.defaultValue)) {
                Vec2 vec = std::get<Vec2>(pin.defaultValue);
                v[0] = vec.x; v[1] = vec.y;
            }
            if (ImGui::DragFloat2(pin.name.c_str(), v, 0.01f)) {
                pin.defaultValue = PinValue(Vec2{v[0], v[1]});
                graph.needsRecompile = true;
            }
        }
        
        ImGui::PopID();
    }
}

// ===== Color Ramp Editor Widget =====
inline void drawColorRampEditor(MaterialNodeEditorState& state, MaterialGraph& graph,
                                VisualScriptNode& node) {
    auto& ramps = graph.colorRamps;
    auto it = ramps.find(node.id);
    
    // Initialize with default stops if not present
    if (it == ramps.end()) {
        ramps[node.id] = {
            {0.0f, {0, 0, 0, 1}},
            {1.0f, {1, 1, 1, 1}}
        };
        it = ramps.find(node.id);
    }
    
    auto& stops = it->second;
    
    // Draw gradient preview bar
    ImVec2 barPos = ImGui::GetCursorScreenPos();
    float barWidth = ImGui::GetContentRegionAvail().x;
    float barHeight = 24;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    // Sort stops for drawing
    auto sorted = stops;
    std::sort(sorted.begin(), sorted.end(), [](const ColorRampStop& a, const ColorRampStop& b) {
        return a.position < b.position;
    });
    
    // Draw gradient segments
    for (size_t i = 0; i < sorted.size() - 1; i++) {
        float x0 = barPos.x + sorted[i].position * barWidth;
        float x1 = barPos.x + sorted[i + 1].position * barWidth;
        
        ImU32 col0 = ImGui::ColorConvertFloat4ToU32(ImVec4(
            sorted[i].color[0], sorted[i].color[1], sorted[i].color[2], sorted[i].color[3]));
        ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(
            sorted[i+1].color[0], sorted[i+1].color[1], sorted[i+1].color[2], sorted[i+1].color[3]));
        
        dl->AddRectFilledMultiColor(
            ImVec2(x0, barPos.y), ImVec2(x1, barPos.y + barHeight),
            col0, col1, col1, col0);
    }
    
    dl->AddRect(barPos, ImVec2(barPos.x + barWidth, barPos.y + barHeight),
                IM_COL32(100, 100, 100, 255));
    
    // Draw stop markers
    for (size_t i = 0; i < stops.size(); i++) {
        float x = barPos.x + stops[i].position * barWidth;
        float y = barPos.y + barHeight;
        
        ImVec2 tri[3] = {
            ImVec2(x - 5, y + 8),
            ImVec2(x + 5, y + 8),
            ImVec2(x, y)
        };
        ImU32 stopColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            stops[i].color[0], stops[i].color[1], stops[i].color[2], 1.0f));
        dl->AddTriangleFilled(tri[0], tri[1], tri[2], stopColor);
        dl->AddTriangle(tri[0], tri[1], tri[2], IM_COL32(200, 200, 200, 255));
    }
    
    ImGui::Dummy(ImVec2(barWidth, barHeight + 12));
    
    // Stop editors
    for (size_t i = 0; i < stops.size(); i++) {
        ImGui::PushID((int)i);
        
        ImGui::DragFloat("Pos", &stops[i].position, 0.01f, 0.0f, 1.0f);
        ImGui::ColorEdit4("Color", stops[i].color, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs);
        
        ImGui::SameLine();
        if (stops.size() > 2 && ImGui::SmallButton("X")) {
            stops.erase(stops.begin() + i);
            graph.needsRecompile = true;
            ImGui::PopID();
            break;
        }
        
        ImGui::PopID();
    }
    
    if (ImGui::Button("Add Stop")) {
        stops.push_back({0.5f, {0.5f, 0.5f, 0.5f, 1.0f}});
        graph.needsRecompile = true;
    }
}

} // namespace luma
