// LUMA Editor Mode UI Components
// Welcome screen, mode tabs, and mode-specific UI
#pragma once

#include "imgui.h"
#include "editor_mode.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/ui/localization.h"
#include <string>
#include <functional>

namespace luma {
namespace editor {

// Import localization function
using luma::ui::loc;

// ===== Style Constants =====
namespace ModeUIStyle {
    // Colors
    constexpr ImU32 kPrimaryColor = IM_COL32(66, 135, 245, 255);      // Blue accent
    constexpr ImU32 kPrimaryHover = IM_COL32(88, 155, 255, 255);
    constexpr ImU32 kSecondaryColor = IM_COL32(45, 50, 60, 255);      // Dark bg
    constexpr ImU32 kTextColor = IM_COL32(220, 220, 220, 255);
    constexpr ImU32 kTextDim = IM_COL32(140, 140, 140, 255);
    constexpr ImU32 kSuccess = IM_COL32(76, 175, 80, 255);            // Green
    constexpr ImU32 kWarning = IM_COL32(255, 193, 7, 255);            // Yellow
    constexpr ImU32 kDisabled = IM_COL32(80, 80, 80, 255);
    
    // Sizes
    constexpr float kModeTabHeight = 32.0f;
    constexpr float kModeTabPadding = 12.0f;
    constexpr float kWelcomeCardWidth = 160.0f;
    constexpr float kWelcomeCardHeight = 120.0f;
}

// ===== Welcome Screen =====
class WelcomeScreen {
public:
    bool isVisible = true;
    bool dontShowAgain = false;
    
    // Callbacks
    std::function<void()> onNewScene;
    std::function<void()> onOpenProject;
    std::function<void(const std::string&)> onOpenRecent;
    std::function<void(const std::string&)> onLoadPreset;
    
    void draw(float windowWidth, float windowHeight, 
              const std::vector<EditorModeManager::RecentProject>& recentProjects) {
        if (!isVisible) return;
        
        // Full-screen overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        
        if (ImGui::Begin("##Welcome", nullptr, flags)) {
            drawContent(windowWidth, windowHeight, recentProjects);
        }
        ImGui::End();
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    
private:
    void drawContent(float windowWidth, float windowHeight,
                     const std::vector<EditorModeManager::RecentProject>& recentProjects) {
        // Center content
        float contentWidth = 800.0f;
        float contentHeight = 600.0f;
        float startX = (windowWidth - contentWidth) / 2.0f;
        float startY = (windowHeight - contentHeight) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(startX, startY));
        ImGui::BeginChild("##WelcomeContent", ImVec2(contentWidth, contentHeight), false);
        
        // Logo and title
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 0 ? ImGui::GetIO().Fonts->Fonts[0] : nullptr);
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize("LUMA Studio").x * 2.5f) / 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
        
        // Large title
        ImGui::SetWindowFontScale(2.5f);
        ImGui::Text("LUMA Studio");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        
        // Subtitle
        const char* subtitle = loc("Real-time 3D Creation Platform");
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize(subtitle).x) / 2.0f);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", subtitle);
        
        ImGui::Dummy(ImVec2(0, 40));
        
        // Quick start cards - draw using buttons for reliable layout
        float cardWidth = 150.0f;
        float cardHeight = 100.0f;
        float cardSpacing = 20.0f;
        float totalCardsWidth = cardWidth * 3 + cardSpacing * 2;
        float cardsStartX = (contentWidth - totalCardsWidth) / 2.0f;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.30f, 0.40f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.25f, 0.35f, 1.0f));
        
        ImGui::SetCursorPosX(cardsStartX);
        
        // New Scene card - build button text dynamically
        char btnText1[128], btnText2[128], btnText3[128];
        snprintf(btnText1, sizeof(btnText1), "[+]\n\n%s\n\n%s", loc("New Scene"), loc("Create empty scene"));
        snprintf(btnText2, sizeof(btnText2), "[O]\n\n%s\n\n%s", loc("Open Project"), loc("Open .luma file"));
        snprintf(btnText3, sizeof(btnText3), "[T]\n\n%s\n\n%s", loc("Quick Start"), loc("From preset"));
        
        if (ImGui::Button(btnText1, ImVec2(cardWidth, cardHeight))) {
            if (onNewScene) onNewScene();
            isVisible = false;
        }
        
        ImGui::SameLine(0, cardSpacing);
        
        // Open Project card
        if (ImGui::Button(btnText2, ImVec2(cardWidth, cardHeight))) {
            if (onOpenProject) onOpenProject();
        }
        
        ImGui::SameLine(0, cardSpacing);
        
        // Quick Templates card
        if (ImGui::Button(btnText3, ImVec2(cardWidth, cardHeight))) {
            ImGui::OpenPopup("##QuickStartPopup");
        }
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
        
        // Quick start popup
        if (ImGui::BeginPopup("##QuickStartPopup")) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", loc("Select Preset"));
            ImGui::Separator();
            
            if (ImGui::Selectable(loc("Studio"))) {
                if (onLoadPreset) onLoadPreset("studio");
                isVisible = false;
            }
            if (ImGui::Selectable(loc("Outdoor Park"))) {
                if (onLoadPreset) onLoadPreset("park");
                isVisible = false;
            }
            if (ImGui::Selectable(loc("Medieval Castle"))) {
                if (onLoadPreset) onLoadPreset("castle");
                isVisible = false;
            }
            if (ImGui::Selectable(loc("Sci-Fi Spaceship"))) {
                if (onLoadPreset) onLoadPreset("spaceship");
                isVisible = false;
            }
            ImGui::EndPopup();
        }
        
        ImGui::Dummy(ImVec2(0, 30));
        
        // Recent projects section
        if (!recentProjects.empty()) {
            float recentWidth = 400.0f;
            ImGui::SetCursorPosX((contentWidth - recentWidth) / 2.0f);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", loc("Recent Projects"));
            ImGui::Dummy(ImVec2(0, 10));
            
            ImGui::SetCursorPosX((contentWidth - recentWidth) / 2.0f);
            ImGui::BeginChild("##RecentProjects", ImVec2(recentWidth, 120), true);
            
            for (const auto& proj : recentProjects) {
                if (ImGui::Selectable(proj.name.c_str(), false)) {
                    if (onOpenRecent) onOpenRecent(proj.path);
                    isVisible = false;
                }
            }
            
            ImGui::EndChild();
        }
        
        ImGui::Dummy(ImVec2(0, 20));
        
        // Bottom section: checkbox centered
        const char* dontShowText = loc("Don't show on startup");
        float checkboxWidth = ImGui::CalcTextSize(dontShowText).x + 30;
        ImGui::SetCursorPosX((contentWidth - checkboxWidth) / 2.0f);
        ImGui::Checkbox(dontShowText, &dontShowAgain);
        
        ImGui::Dummy(ImVec2(0, 15));
        
        // Skip button centered
        ImGui::SetCursorPosX((contentWidth - 80) / 2.0f);
        if (ImGui::Button(loc("Skip"), ImVec2(80, 28))) {
            isVisible = false;
        }
        
        ImGui::EndChild();
    }
    
    bool drawCard(const char* icon, const char* title, const char* desc,
                  float width, float height) {
        bool clicked = false;
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size(width, height);
        
        // Card background
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImU32 bgColor = hovered ? IM_COL32(50, 60, 80, 255) : IM_COL32(35, 40, 50, 255);
        ImU32 borderColor = hovered ? ModeUIStyle::kPrimaryColor : IM_COL32(60, 65, 75, 255);
        
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                               bgColor, 8.0f);
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                         borderColor, 8.0f, 0, hovered ? 2.0f : 1.0f);
        
        // Calculate centered positions
        float iconWidth = ImGui::CalcTextSize(icon).x;
        float titleWidth = ImGui::CalcTextSize(title).x;
        float descWidth = ImGui::CalcTextSize(desc).x;
        
        // Icon (centered)
        drawList->AddText(ImVec2(pos.x + (width - iconWidth) / 2.0f, pos.y + 20), 
                         IM_COL32(100, 160, 255, 255), icon);
        
        // Title (centered)
        drawList->AddText(ImVec2(pos.x + (width - titleWidth) / 2.0f, pos.y + 50), 
                         IM_COL32(230, 230, 230, 255), title);
        
        // Description (centered)
        drawList->AddText(ImVec2(pos.x + (width - descWidth) / 2.0f, pos.y + 75), 
                         IM_COL32(140, 140, 140, 255), desc);
        
        // Invisible button for click detection
        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton(("##Card_" + std::string(title)).c_str(), size)) {
            clicked = true;
        }
        
        // Move cursor past card for SameLine
        ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x, pos.y));
        
        return clicked;
    }
};

// ===== Mode Tab Bar =====
class ModeTabBar {
public:
    // Draw the mode tab bar at the top
    // Returns: the new mode if changed, or current mode if not
    EditorMode draw(EditorMode currentMode, const ModeAvailability& availability,
                    const std::string& projectName = "未命名场景") {
        EditorMode newMode = currentMode;
        
        float windowWidth = ImGui::GetIO().DisplaySize.x;
        
        ImGui::SetNextWindowPos(ImVec2(0, 19));  // Below menu bar
        ImGui::SetNextWindowSize(ImVec2(windowWidth, ModeUIStyle::kModeTabHeight));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.16f, 0.18f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
        
        if (ImGui::Begin("##ModeTabBar", nullptr, flags)) {
            ImGui::SetCursorPosY(4);
            
            // Mode tabs
            newMode = drawModeTab(EditorMode::Scene, currentMode, true, "");
            
            // Separator
            ImGui::SameLine(0, 5);
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "|");
            ImGui::SameLine(0, 5);
            
            EditorMode tabResult;
            
            tabResult = drawModeTab(EditorMode::Character, currentMode, 
                                   availability.character, availability.characterReason);
            if (tabResult != currentMode) newMode = tabResult;
            
            tabResult = drawModeTab(EditorMode::Edit, currentMode,
                                   availability.edit, availability.editReason);
            if (tabResult != currentMode) newMode = tabResult;
            
            tabResult = drawModeTab(EditorMode::Animation, currentMode,
                                   availability.animation, availability.animationReason);
            if (tabResult != currentMode) newMode = tabResult;
            
            // Separator
            ImGui::SameLine(0, 5);
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "|");
            ImGui::SameLine(0, 5);
            
            tabResult = drawModeTab(EditorMode::Play, currentMode, true, "");
            if (tabResult != currentMode) newMode = tabResult;
            
            // Project name on the right
            float projectNameWidth = ImGui::CalcTextSize(projectName.c_str()).x + 50;
            ImGui::SameLine(windowWidth - projectNameWidth);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "项目: %s", projectName.c_str());
        }
        ImGui::End();
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        return newMode;
    }
    
private:
    EditorMode drawModeTab(EditorMode mode, EditorMode currentMode,
                           bool enabled, const std::string& disabledReason) {
        EditorMode result = currentMode;
        
        bool isSelected = (mode == currentMode);
        
        ImGui::PushID(static_cast<int>(mode));
        
        // Colors
        ImVec4 bgColor, textColor;
        if (!enabled) {
            bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            textColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        } else if (isSelected) {
            bgColor = ImVec4(0.26f, 0.53f, 0.96f, 1.0f);
            textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            bgColor = ImVec4(0.22f, 0.23f, 0.26f, 1.0f);
            textColor = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
        }
        
        // Tab content
        const char* name = EditorModeManager::getModeName(mode);
        const char* shortcut = EditorModeManager::getModeShortcut(mode);
        
        char label[64];
        if (strlen(shortcut) > 0) {
            snprintf(label, sizeof(label), "%s (%s)", name, shortcut);
        } else {
            snprintf(label, sizeof(label), "%s", name);
        }
        
        float tabWidth = ImGui::CalcTextSize(label).x + ModeUIStyle::kModeTabPadding * 2;
        
        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 
                             enabled ? ImVec4(0.30f, 0.58f, 1.0f, 1.0f) : bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 
                             enabled ? ImVec4(0.22f, 0.48f, 0.90f, 1.0f) : bgColor);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        if (ImGui::Button(label, ImVec2(tabWidth, 24))) {
            if (enabled) {
                result = mode;
            }
        }
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        
        // Tooltip for disabled tabs
        if (!enabled && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", disabledReason.c_str());
        }
        
        ImGui::SameLine(0, 4);
        ImGui::PopID();
        
        return result;
    }
};

// ===== Empty Scene Guide =====
class EmptySceneGuide {
public:
    bool isVisible = true;
    
    std::function<void()> onCreateCharacter;
    std::function<void()> onAddObject;
    std::function<void()> onImportModel;
    std::function<void(const std::string&)> onLoadPreset;
    
    void draw(float viewportX, float viewportY, float viewportWidth, float viewportHeight) {
        if (!isVisible) return;
        
        // Center in viewport
        float guideWidth = 500.0f;
        float guideHeight = 350.0f;
        float guideX = viewportX + (viewportWidth - guideWidth) / 2.0f;
        float guideY = viewportY + (viewportHeight - guideHeight) / 2.0f;
        
        ImGui::SetNextWindowPos(ImVec2(guideX, guideY));
        ImGui::SetNextWindowSize(ImVec2(guideWidth, guideHeight));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollbar;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 0.95f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 25));
        
        if (ImGui::Begin("##EmptySceneGuide", nullptr, flags)) {
            // Close button in top-right corner
            ImGui::SetCursorPos(ImVec2(guideWidth - 55, 5));
            if (ImGui::Button("X", ImVec2(25, 25))) {
                isVisible = false;
            }
            
            // Title
            ImGui::SetCursorPos(ImVec2(30, 25));
            ImGui::SetWindowFontScale(1.3f);
            char guideTitle[64];
            snprintf(guideTitle, sizeof(guideTitle), "[ %s ]", loc("Start Creating"));
            float titleWidth = ImGui::CalcTextSize(guideTitle).x;
            ImGui::SetCursorPosX((guideWidth - titleWidth) / 2.0f - 15);
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", guideTitle);
            ImGui::SetWindowFontScale(1.0f);
            
            ImGui::Dummy(ImVec2(0, 20));
            
            // Action buttons
            float buttonWidth = 120.0f;
            float buttonHeight = 80.0f;
            float totalWidth = buttonWidth * 3 + 30;
            float startX = (guideWidth - totalWidth) / 2.0f - 15;
            
            ImGui::SetCursorPosX(startX);
            
            if (drawActionButton("[C]", loc("Create Character"), buttonWidth, buttonHeight)) {
                if (onCreateCharacter) onCreateCharacter();
            }
            
            ImGui::SameLine(0, 15);
            
            if (drawActionButton("[+]", loc("Add Object"), buttonWidth, buttonHeight)) {
                if (onAddObject) onAddObject();
            }
            
            ImGui::SameLine(0, 15);
            
            if (drawActionButton("[I]", loc("Import Model"), buttonWidth, buttonHeight)) {
                if (onImportModel) onImportModel();
            }
            
            ImGui::Dummy(ImVec2(0, 20));
            
            // Divider
            ImGui::SetCursorPosX(50);
            char divider[128];
            snprintf(divider, sizeof(divider), "------------ %s ------------", loc("Or"));
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "%s", divider);
            
            ImGui::Dummy(ImVec2(0, 15));
            
            // Presets
            ImGui::SetCursorPosX(60);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", loc("Start from preset:"));
            
            ImGui::Dummy(ImVec2(0, 10));
            
            ImGui::SetCursorPosX(50);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.24f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.35f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            if (ImGui::Button(loc("Studio"), ImVec2(90, 28))) {
                if (onLoadPreset) onLoadPreset("studio");
            }
            ImGui::SameLine(0, 8);
            if (ImGui::Button(loc("Outdoor Park"), ImVec2(90, 28))) {
                if (onLoadPreset) onLoadPreset("park");
            }
            ImGui::SameLine(0, 8);
            if (ImGui::Button(loc("Medieval Castle"), ImVec2(90, 28))) {
                if (onLoadPreset) onLoadPreset("castle");
            }
            ImGui::SameLine(0, 8);
            if (ImGui::Button(loc("Sci-Fi Spaceship"), ImVec2(90, 28))) {
                if (onLoadPreset) onLoadPreset("spaceship");
            }
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            
            ImGui::Dummy(ImVec2(0, 15));
            
            // Hint
            ImGui::SetCursorPosX(65);
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), 
                              "%s", loc("Tip: Right-click viewport or click [+ Add] to add objects"));
        }
        ImGui::End();
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    
private:
    bool drawActionButton(const char* icon, const char* label, float width, float height) {
        bool clicked = false;
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + width, pos.y + height));
        
        ImU32 bgColor = hovered ? IM_COL32(55, 65, 85, 255) : IM_COL32(40, 45, 55, 255);
        ImU32 borderColor = hovered ? ModeUIStyle::kPrimaryColor : IM_COL32(70, 75, 85, 255);
        
        drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgColor, 8.0f);
        drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height), borderColor, 8.0f, 0, 1.5f);
        
        // Icon
        ImGui::SetCursorScreenPos(ImVec2(pos.x + (width - 25) / 2.0f, pos.y + 15));
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("%s", icon);
        ImGui::SetWindowFontScale(1.0f);
        
        // Label
        float labelWidth = ImGui::CalcTextSize(label).x;
        ImGui::SetCursorScreenPos(ImVec2(pos.x + (width - labelWidth) / 2.0f, pos.y + height - 25));
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", label);
        
        // Invisible button
        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton(("##Action_" + std::string(label)).c_str(), ImVec2(width, height))) {
            clicked = true;
        }
        
        ImGui::SetCursorScreenPos(ImVec2(pos.x + width, pos.y));
        
        return clicked;
    }
};

// ===== Add Object Context Menu =====
class AddObjectContextMenu {
public:
    bool isOpen = false;
    ImVec2 popupPos;
    
    AddObjectMenu menu;
    
    void openAt(float x, float y) {
        popupPos = ImVec2(x, y);
        isOpen = true;
        ImGui::OpenPopup("##AddObjectMenu");
    }
    
    void draw() {
        ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.16f, 0.18f, 0.98f));
        
        if (ImGui::BeginPopup("##AddObjectMenu")) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "+ 添加到场景");
            ImGui::Separator();
            
            for (auto& category : menu.categories) {
                if (ImGui::BeginMenu((category.icon + " " + category.name).c_str())) {
                    for (auto& item : category.items) {
                        if (item.hasSubmenu && !item.submenu.empty()) {
                            if (ImGui::BeginMenu((item.icon + " " + item.name).c_str())) {
                                for (auto& subitem : item.submenu) {
                                    if (ImGui::MenuItem((subitem.icon + " " + subitem.name).c_str())) {
                                        if (subitem.createFunc) subitem.createFunc();
                                        // Handle built-in primitives
                                        if (menu.onCreatePrimitive) {
                                            menu.onCreatePrimitive(subitem.name);
                                        }
                                    }
                                    if (ImGui::IsItemHovered() && !subitem.tooltip.empty()) {
                                        ImGui::SetTooltip("%s", subitem.tooltip.c_str());
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        } else {
                            if (ImGui::MenuItem((item.icon + " " + item.name).c_str())) {
                                if (item.createFunc) item.createFunc();
                                handleMenuItemClick(category.name, item.name);
                            }
                            if (ImGui::IsItemHovered() && !item.tooltip.empty()) {
                                ImGui::SetTooltip("%s", item.tooltip.c_str());
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            
            ImGui::EndPopup();
        } else {
            isOpen = false;
        }
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
    
private:
    void handleMenuItemClick(const std::string& category, const std::string& item) {
        if (category == "角色") {
            if (menu.onCreateCharacter) menu.onCreateCharacter();
        } else if (category == "导入模型") {
            if (menu.onImportModel) menu.onImportModel();
        } else if (category == "光源") {
            if (menu.onCreateLight) {
                // Extract light type
                if (item.find("方向") != std::string::npos) menu.onCreateLight("Directional");
                else if (item.find("点") != std::string::npos) menu.onCreateLight("Point");
                else if (item.find("聚光") != std::string::npos) menu.onCreateLight("Spot");
                else if (item.find("区域") != std::string::npos) menu.onCreateLight("Area");
            }
        } else if (category == "场景预设") {
            if (menu.onLoadPreset) {
                if (item.find("摄影棚") != std::string::npos) menu.onLoadPreset("studio");
                else if (item.find("公园") != std::string::npos) menu.onLoadPreset("park");
                else if (item.find("城堡") != std::string::npos) menu.onLoadPreset("castle");
                else if (item.find("太空") != std::string::npos) menu.onLoadPreset("spaceship");
            }
        }
    }
};

// ===== Edit Mode Mesh List Panel =====
class EditModeMeshList {
public:
    int selectedMeshIndex = -1;
    
    std::function<void(int)> onMeshSelected;
    
    void draw(Entity* entity, float panelWidth) {
        if (!entity || !entity->hasModel) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", loc("No editable mesh"));
            return;
        }
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Mesh List"));
        ImGui::Separator();
        
        auto& meshes = entity->model.meshes;
        
        for (size_t i = 0; i < meshes.size(); ++i) {
            const auto& mesh = meshes[i];
            
            bool isSelected = (selectedMeshIndex == static_cast<int>(i));
            
            // Display mesh name if available, otherwise use index
            char label[256];
            if (!mesh.name.empty()) {
                snprintf(label, sizeof(label), "[M] %s", mesh.name.c_str());
            } else {
                snprintf(label, sizeof(label), "[M] Mesh %zu (%u %s)", 
                        i, mesh.indexCount, loc("indices"));
            }
            
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.53f, 0.96f, 0.6f));
            
            if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_None,
                                 ImVec2(panelWidth - 20, 24))) {
                selectedMeshIndex = static_cast<int>(i);
                printf("[MESH SELECT] Clicked mesh %d, selectedMeshIndex now = %d\n", (int)i, selectedMeshIndex);
                if (onMeshSelected) onMeshSelected(selectedMeshIndex);
            }
            
            ImGui::PopStyleColor();
            
            // Show material preview
            if (isSelected) {
                ImGui::Indent(20);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s: PBR", loc("Material"));
                
                // Texture indicators
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "贴图: ");
                ImGui::SameLine();
                if (mesh.hasDiffuseTexture) ImGui::Text("[D]");
                if (mesh.hasNormalTexture) { ImGui::SameLine(); ImGui::Text("[N]"); }
                if (mesh.hasSpecularTexture) { ImGui::SameLine(); ImGui::Text("[S]"); }
                if (!mesh.hasDiffuseTexture && !mesh.hasNormalTexture && !mesh.hasSpecularTexture) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[无]");
                }
                
                ImGui::Unindent(20);
            }
        }
    }
};

// ===== Edit Mode View Mode Toolbar =====
class EditModeViewToolbar {
public:
    ViewMode currentViewMode = ViewMode::Material;
    bool showWireframeOverlay = false;
    
    // Returns true if view mode changed
    bool draw(float panelWidth) {
        bool changed = false;
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("View Mode"));
        ImGui::Separator();
        
        // View mode buttons
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        float btnWidth = (panelWidth - 40) / 4.0f;
        
        auto drawViewBtn = [&](ViewMode mode, const char* label, const char* tooltip) {
            bool selected = (currentViewMode == mode);
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
            }
            
            if (ImGui::Button(label, ImVec2(btnWidth, 24))) {
                currentViewMode = mode;
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tooltip);
            }
            
            if (selected) {
                ImGui::PopStyleColor();
            }
        };
        
        drawViewBtn(ViewMode::Material, loc("Material"), loc("Full PBR material rendering"));
        ImGui::SameLine(0, 4);
        drawViewBtn(ViewMode::Solid, loc("Solid"), loc("Solid gray shading (clay)"));
        
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
        drawViewBtn(ViewMode::Wireframe, loc("Wire+"), loc("Material + wireframe overlay"));
        ImGui::SameLine(0, 4);
        drawViewBtn(ViewMode::WireframeOnly, loc("Wire"), loc("Wireframe only (no solid)"));
        
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        
        // Wireframe overlay checkbox (for Material and Solid modes)
        if (currentViewMode == ViewMode::Material || currentViewMode == ViewMode::Solid) {
            if (ImGui::Checkbox(loc("Show Wireframe Overlay"), &showWireframeOverlay)) {
                changed = true;
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        return changed;
    }
};

// ===== Edit Mode Material Editor =====
class EditModeMaterialEditor {
public:
    void draw(Entity* entity, int meshIndex, float panelWidth) {
        if (!entity || !entity->hasModel || meshIndex < 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", loc("Select a mesh to edit material"));
            return;
        }
        
        auto& meshes = entity->model.meshes;
        if (meshIndex >= static_cast<int>(meshes.size())) return;
        
        auto& mesh = meshes[meshIndex];
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("PBR Material Parameters"));
        ImGui::Separator();
        
        // Base Color
        ImGui::Text("%s", loc("Base Color"));
        float baseColor[3] = { mesh.baseColor[0], mesh.baseColor[1], mesh.baseColor[2] };
        if (ImGui::ColorEdit3("##BaseColor", baseColor, ImGuiColorEditFlags_NoInputs)) {
            mesh.baseColor[0] = baseColor[0];
            mesh.baseColor[1] = baseColor[1];
            mesh.baseColor[2] = baseColor[2];
            // Sync to entity material if exists
            if (entity->material) {
                entity->material->baseColor = {baseColor[0], baseColor[1], baseColor[2]};
            }
        }
        
        ImGui::Spacing();
        
        // Metallic
        ImGui::Text("%s", loc("Metallic"));
        if (ImGui::SliderFloat("##Metallic", &mesh.metallic, 0.0f, 1.0f, "%.2f")) {
            if (entity->material) {
                entity->material->metallic = mesh.metallic;
            }
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "?");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("0 = Non-metal (plastic, wood)\n1 = Pure metal (gold, silver, copper)"));
        }
        
        ImGui::Spacing();
        
        // Roughness
        ImGui::Text("%s", loc("Roughness"));
        if (ImGui::SliderFloat("##Roughness", &mesh.roughness, 0.0f, 1.0f, "%.2f")) {
            if (entity->material) {
                entity->material->roughness = mesh.roughness;
            }
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "?");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("0 = Smooth (specular reflection)\n1 = Rough (diffuse reflection)"));
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Textures section
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Textures"));
        ImGui::Spacing();
        
        drawTextureSlot(loc("Diffuse Map"), "[D]", mesh.hasDiffuseTexture);
        drawTextureSlot(loc("Normal Map"), "[N]", mesh.hasNormalTexture);
        drawTextureSlot(loc("Specular Map"), "[S]", mesh.hasSpecularTexture);
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Preview values
        if (ImGui::CollapsingHeader(loc("Live Preview"))) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                              "%s (%.2f, %.2f, %.2f)", loc("Color:"),
                              mesh.baseColor[0], mesh.baseColor[1], mesh.baseColor[2]);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                              "%s %.2f", loc("Metallic"), mesh.metallic);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                              "粗糙度: %.2f", mesh.roughness);
        }
    }
    
private:
    void drawTextureSlot(const char* label, const char* icon, bool hasTexture) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.22f, 0.25f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        char buttonLabel[128];
        snprintf(buttonLabel, sizeof(buttonLabel), "%s %s##%s", 
                icon, hasTexture ? "[已加载]" : "[无]", label);
        
        if (hasTexture) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }
        
        if (ImGui::Button(buttonLabel, ImVec2(-1, 26))) {
            // TODO: Open texture browser
        }
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", label);
    }
};

// ===== Edit Mode Selection & Tool Toolbar =====
// 选择模式和编辑工具切换
class EditModeToolbar {
public:
    // 选择模式
    enum class SelectMode { Vertex, Edge, Face };
    SelectMode selectMode = SelectMode::Face;
    
    // 编辑工具
    enum class EditTool { Select, Move, Rotate, Scale, Extrude };
    EditTool currentTool = EditTool::Select;
    
    // 线框显示选项
    bool showOriginalEdges = true;  // 显示四边面原始边
    bool showAllEdges = false;      // 显示所有边（包括三角化边）
    
    // 回调
    std::function<void(SelectMode)> onSelectModeChanged;
    std::function<void(EditTool)> onToolChanged;
    
    bool draw(float panelWidth) {
        bool changed = false;
        
        // === 选择模式 ===
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Selection Mode"));
        ImGui::Separator();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        float btnWidth = (panelWidth - 30) / 3.0f;
        
        auto drawSelectModeBtn = [&](SelectMode mode, const char* label, const char* shortcut) {
            bool selected = (selectMode == mode);
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
            }
            
            char fullLabel[64];
            snprintf(fullLabel, sizeof(fullLabel), "%s (%s)", label, shortcut);
            
            if (ImGui::Button(fullLabel, ImVec2(btnWidth, 28))) {
                selectMode = mode;
                changed = true;
                if (onSelectModeChanged) onSelectModeChanged(mode);
            }
            
            if (selected) ImGui::PopStyleColor();
        };
        
        drawSelectModeBtn(SelectMode::Vertex, loc("Vertex"), "1");
        ImGui::SameLine(0, 4);
        drawSelectModeBtn(SelectMode::Edge, loc("Edge"), "2");
        ImGui::SameLine(0, 4);
        drawSelectModeBtn(SelectMode::Face, loc("Face"), "3");
        
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // === 编辑工具 ===
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Edit Tools"));
        ImGui::Separator();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        float toolBtnWidth = (panelWidth - 20) / 2.5f;
        
        auto drawToolBtn = [&](EditTool tool, const char* icon, const char* label, const char* shortcut) {
            bool selected = (currentTool == tool);
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.96f, 0.53f, 0.26f, 1.0f));
            }
            
            char fullLabel[64];
            snprintf(fullLabel, sizeof(fullLabel), "%s %s", icon, label);
            
            if (ImGui::Button(fullLabel, ImVec2(toolBtnWidth, 32))) {
                currentTool = tool;
                changed = true;
                if (onToolChanged) onToolChanged(tool);
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s (%s)", label, shortcut);
            }
            
            if (selected) ImGui::PopStyleColor();
        };
        
        // 第一行工具
        drawToolBtn(EditTool::Select, "[S]", loc("Select"), "Q");
        ImGui::SameLine(0, 4);
        drawToolBtn(EditTool::Move, "[M]", loc("Move"), "G");
        
        // 第二行工具
        drawToolBtn(EditTool::Rotate, "[R]", loc("Rotate"), "R");
        ImGui::SameLine(0, 4);
        drawToolBtn(EditTool::Scale, "[S]", loc("Scale"), "S");
        
        // 第三行工具（特殊操作）
        drawToolBtn(EditTool::Extrude, "[E]", loc("Extrude"), "E");
        
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // === 线框显示选项 ===
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Wireframe Display"));
        ImGui::Separator();
        
        if (ImGui::Checkbox(loc("Show Quad Edges"), &showOriginalEdges)) {
            changed = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Show original quad/ngon edges (hide triangulation)"));
        }
        
        if (ImGui::Checkbox(loc("Show All Edges"), &showAllEdges)) {
            changed = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Show all edges including triangulation"));
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        return changed;
    }
    
    // 处理快捷键
    bool handleShortcuts() {
        bool changed = false;
        
        // 选择模式切换
        if (ImGui::IsKeyPressed(ImGuiKey_1, false)) {
            selectMode = SelectMode::Vertex;
            if (onSelectModeChanged) onSelectModeChanged(selectMode);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_2, false)) {
            selectMode = SelectMode::Edge;
            if (onSelectModeChanged) onSelectModeChanged(selectMode);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_3, false)) {
            selectMode = SelectMode::Face;
            if (onSelectModeChanged) onSelectModeChanged(selectMode);
            changed = true;
        }
        
        // 工具切换
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            currentTool = EditTool::Select;
            if (onToolChanged) onToolChanged(currentTool);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
            currentTool = EditTool::Move;
            if (onToolChanged) onToolChanged(currentTool);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            currentTool = EditTool::Rotate;
            if (onToolChanged) onToolChanged(currentTool);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S, false) && !ImGui::GetIO().KeyCtrl) {
            currentTool = EditTool::Scale;
            if (onToolChanged) onToolChanged(currentTool);
            changed = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            currentTool = EditTool::Extrude;
            if (onToolChanged) onToolChanged(currentTool);
            changed = true;
        }
        
        return changed;
    }
};

// ===== Edit Mode Statistics Panel =====
// 显示编辑网格的统计信息
class EditModeStats {
public:
    void draw(int vertexCount, int faceCount, int triangleCount, int quadCount, int ngonCount) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", loc("Mesh Statistics"));
        ImGui::Separator();
        
        ImGui::Text("%s: %d", loc("Vertices"), vertexCount);
        ImGui::Text("%s: %d", loc("Faces"), faceCount);
        ImGui::Text("  %s: %d", loc("Triangles"), triangleCount);
        ImGui::Text("  %s: %d", loc("Quads"), quadCount);
        if (ngonCount > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "  %s: %d", loc("N-gons"), ngonCount);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
    }
};

// ===== Edit Mode Undo/Redo Bar =====
// 撤销/重做按钮
class EditModeUndoRedo {
public:
    std::function<void()> onUndo;
    std::function<void()> onRedo;
    std::function<int()> getUndoCount;
    std::function<int()> getRedoCount;
    
    void draw(float panelWidth) {
        int undoCount = getUndoCount ? getUndoCount() : 0;
        int redoCount = getRedoCount ? getRedoCount() : 0;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        float btnWidth = (panelWidth - 30) / 2.0f;
        
        // Undo 按钮
        ImGui::BeginDisabled(undoCount == 0);
        if (ImGui::Button((std::string(loc("Undo")) + " (" + std::to_string(undoCount) + ")").c_str(), 
                          ImVec2(btnWidth, 28))) {
            if (onUndo) onUndo();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine(0, 8);
        
        // Redo 按钮
        ImGui::BeginDisabled(redoCount == 0);
        if (ImGui::Button((std::string(loc("Redo")) + " (" + std::to_string(redoCount) + ")").c_str(),
                          ImVec2(btnWidth, 28))) {
            if (onRedo) onRedo();
        }
        ImGui::EndDisabled();
        
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        ImGui::Separator();
    }
    
    // 处理快捷键
    bool handleShortcuts() {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            if (ImGui::GetIO().KeyShift) {
                // Ctrl+Shift+Z = Redo
                if (onRedo) { onRedo(); return true; }
            } else {
                // Ctrl+Z = Undo
                if (onUndo) { onUndo(); return true; }
            }
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            // Ctrl+Y = Redo (alternative)
            if (onRedo) { onRedo(); return true; }
        }
        return false;
    }
};

// ===== Edit Mode Save/Cancel Bar =====
// 保存/取消按钮
class EditModeSaveBar {
public:
    std::function<void()> onSave;
    std::function<void()> onCancel;
    
    void draw(bool hasChanges, float panelWidth) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        float btnWidth = (panelWidth - 30) / 2.0f;
        
        // 保存按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        
        if (ImGui::Button(loc("Save & Exit"), ImVec2(btnWidth, 32))) {
            if (onSave) onSave();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Save changes and return to Scene mode"));
        }
        
        ImGui::PopStyleColor(2);
        
        ImGui::SameLine(0, 8);
        
        // 取消按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        
        if (ImGui::Button(loc("Cancel"), ImVec2(btnWidth, 32))) {
            if (onCancel) onCancel();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Discard changes and return to Scene mode"));
        }
        
        ImGui::PopStyleColor(2);
        
        // 修改提示
        if (hasChanges) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", loc("* Unsaved changes"));
        }
    }
};

} // namespace editor
} // namespace luma
