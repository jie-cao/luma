// Game UI System - Manager
// Central manager for UI canvases and rendering
#pragma once

#include "ui_core.h"
#include "ui_widgets.h"
#include "ui_layout.h"
#include <map>

namespace luma {
namespace ui {

// ===== UI Render Command =====
struct UIRenderCommand {
    enum class Type {
        Rect,
        RectOutline,
        RoundedRect,
        Text,
        Image,
        Line,
        Circle,
        Clip,
        PopClip
    };
    
    Type type;
    UIRect rect;
    UIColor color;
    
    // Text
    std::string text;
    std::string fontName;
    float fontSize = 16.0f;
    UILabel::HAlign textHAlign = UILabel::HAlign::Left;
    UILabel::VAlign textVAlign = UILabel::VAlign::Middle;
    
    // Image
    uint64_t textureHandle = 0;
    UIRect uvRect = {0, 0, 1, 1};
    
    // Rounded rect
    float cornerRadius = 0.0f;
    
    // Border
    float borderWidth = 0.0f;
};

// ===== UI Renderer Interface =====
class IUIRenderer {
public:
    virtual ~IUIRenderer() = default;
    
    virtual void beginFrame(float screenWidth, float screenHeight) = 0;
    virtual void endFrame() = 0;
    
    virtual void drawRect(const UIRect& rect, const UIColor& color) = 0;
    virtual void drawRectOutline(const UIRect& rect, const UIColor& color, float width) = 0;
    virtual void drawRoundedRect(const UIRect& rect, const UIColor& color, float radius) = 0;
    virtual void drawText(const std::string& text, const UIRect& rect, const UIColor& color,
                          const std::string& font, float fontSize,
                          UILabel::HAlign hAlign, UILabel::VAlign vAlign) = 0;
    virtual void drawImage(const UIRect& rect, uint64_t textureHandle,
                           const UIRect& uvRect, const UIColor& tint) = 0;
    
    virtual void pushClip(const UIRect& rect) = 0;
    virtual void popClip() = 0;
};

// ===== Default UI Renderer (command buffer) =====
class UICommandRenderer : public IUIRenderer {
public:
    void beginFrame(float screenWidth, float screenHeight) override {
        commands_.clear();
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
    }
    
    void endFrame() override {}
    
    void drawRect(const UIRect& rect, const UIColor& color) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::Rect;
        cmd.rect = rect;
        cmd.color = color;
        commands_.push_back(cmd);
    }
    
    void drawRectOutline(const UIRect& rect, const UIColor& color, float width) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::RectOutline;
        cmd.rect = rect;
        cmd.color = color;
        cmd.borderWidth = width;
        commands_.push_back(cmd);
    }
    
    void drawRoundedRect(const UIRect& rect, const UIColor& color, float radius) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::RoundedRect;
        cmd.rect = rect;
        cmd.color = color;
        cmd.cornerRadius = radius;
        commands_.push_back(cmd);
    }
    
    void drawText(const std::string& text, const UIRect& rect, const UIColor& color,
                  const std::string& font, float fontSize,
                  UILabel::HAlign hAlign, UILabel::VAlign vAlign) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::Text;
        cmd.rect = rect;
        cmd.color = color;
        cmd.text = text;
        cmd.fontName = font;
        cmd.fontSize = fontSize;
        cmd.textHAlign = hAlign;
        cmd.textVAlign = vAlign;
        commands_.push_back(cmd);
    }
    
    void drawImage(const UIRect& rect, uint64_t textureHandle,
                   const UIRect& uvRect, const UIColor& tint) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::Image;
        cmd.rect = rect;
        cmd.textureHandle = textureHandle;
        cmd.uvRect = uvRect;
        cmd.color = tint;
        commands_.push_back(cmd);
    }
    
    void pushClip(const UIRect& rect) override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::Clip;
        cmd.rect = rect;
        commands_.push_back(cmd);
    }
    
    void popClip() override {
        UIRenderCommand cmd;
        cmd.type = UIRenderCommand::Type::PopClip;
        commands_.push_back(cmd);
    }
    
    const std::vector<UIRenderCommand>& getCommands() const { return commands_; }
    
private:
    std::vector<UIRenderCommand> commands_;
    float screenWidth_ = 0.0f;
    float screenHeight_ = 0.0f;
};

// ===== UI Widget Drawer =====
class UIWidgetDrawer {
public:
    UIWidgetDrawer(IUIRenderer* renderer) : renderer_(renderer) {}
    
    void draw(UIWidget* widget) {
        if (!widget || !widget->isVisible()) return;
        
        const UIRect& rect = widget->getWorldRect();
        
        // Draw based on widget type
        switch (widget->getType()) {
            case UIWidgetType::Panel:
                drawPanel(static_cast<UIPanel*>(widget));
                break;
            case UIWidgetType::Label:
                drawLabel(static_cast<UILabel*>(widget));
                break;
            case UIWidgetType::Image:
                drawImage(static_cast<UIImage*>(widget));
                break;
            case UIWidgetType::Button:
                drawButton(static_cast<UIButton*>(widget));
                break;
            case UIWidgetType::Checkbox:
                drawCheckbox(static_cast<UICheckbox*>(widget));
                break;
            case UIWidgetType::Slider:
                drawSlider(static_cast<UISlider*>(widget));
                break;
            case UIWidgetType::ProgressBar:
                drawProgressBar(static_cast<UIProgressBar*>(widget));
                break;
            case UIWidgetType::InputField:
                drawInputField(static_cast<UIInputField*>(widget));
                break;
            case UIWidgetType::Dropdown:
                drawDropdown(static_cast<UIDropdown*>(widget));
                break;
            default:
                break;
        }
        
        // Draw children
        for (const auto& child : widget->getChildren()) {
            draw(child.get());
        }
    }
    
private:
    void drawPanel(UIPanel* panel) {
        const UIRect& rect = panel->getWorldRect();
        renderer_->drawRoundedRect(rect, panel->getBackgroundColor(), panel->getCornerRadius());
        
        if (panel->getBorderWidth() > 0) {
            renderer_->drawRectOutline(rect, panel->getBorderColor(), panel->getBorderWidth());
        }
    }
    
    void drawLabel(UILabel* label) {
        const UIRect& rect = label->getWorldRect();
        renderer_->drawText(label->getText(), rect, label->getTextColor(),
                           label->getFontName(), label->getFontSize(),
                           label->getHAlign(), label->getVAlign());
    }
    
    void drawImage(UIImage* image) {
        const UIRect& rect = image->getWorldRect();
        renderer_->drawImage(rect, image->getTextureHandle(),
                            image->getUVRect(), image->getColor());
    }
    
    void drawButton(UIButton* button) {
        const UIRect& rect = button->getWorldRect();
        renderer_->drawRoundedRect(rect, button->getCurrentColor(), button->getBorderRadius());
    }
    
    void drawCheckbox(UICheckbox* checkbox) {
        const UIRect& rect = checkbox->getWorldRect();
        float boxSize = checkbox->getBoxSize();
        UIRect boxRect = {rect.x, rect.y + (rect.height - boxSize) * 0.5f, boxSize, boxSize};
        
        renderer_->drawRoundedRect(boxRect, UIColor(0.3f, 0.3f, 0.3f, 1.0f), 2.0f);
        
        if (checkbox->isChecked()) {
            UIRect checkRect = {boxRect.x + 3, boxRect.y + 3, boxSize - 6, boxSize - 6};
            renderer_->drawRoundedRect(checkRect, UIColor(0.3f, 0.7f, 0.3f, 1.0f), 2.0f);
        }
    }
    
    void drawSlider(UISlider* slider) {
        const UIRect& rect = slider->getWorldRect();
        float handleSize = slider->getHandleSize();
        float normalized = slider->getNormalizedValue();
        
        // Track
        UIRect trackRect = {rect.x, rect.y + rect.height * 0.5f - 2, rect.width, 4};
        renderer_->drawRoundedRect(trackRect, UIColor(0.2f, 0.2f, 0.2f, 1.0f), 2.0f);
        
        // Fill
        UIRect fillRect = {rect.x, trackRect.y, rect.width * normalized, 4};
        renderer_->drawRoundedRect(fillRect, UIColor(0.3f, 0.6f, 1.0f, 1.0f), 2.0f);
        
        // Handle
        float handleX = rect.x + normalized * rect.width - handleSize * 0.5f;
        UIRect handleRect = {handleX, rect.y + (rect.height - handleSize) * 0.5f, handleSize, handleSize};
        renderer_->drawRoundedRect(handleRect, UIColor(1.0f, 1.0f, 1.0f, 1.0f), handleSize * 0.5f);
    }
    
    void drawProgressBar(UIProgressBar* progressBar) {
        const UIRect& rect = progressBar->getWorldRect();
        float value = progressBar->getDisplayValue();
        
        renderer_->drawRoundedRect(rect, progressBar->getBackgroundColor(), 4.0f);
        
        UIRect fillRect = {rect.x + 2, rect.y + 2, (rect.width - 4) * value, rect.height - 4};
        renderer_->drawRoundedRect(fillRect, progressBar->getFillColor(), 2.0f);
        
        if (progressBar->getShowText()) {
            char text[16];
            snprintf(text, sizeof(text), "%.0f%%", value * 100);
            renderer_->drawText(text, rect, UIColor::White(), "default", 14.0f,
                               UILabel::HAlign::Center, UILabel::VAlign::Middle);
        }
    }
    
    void drawInputField(UIInputField* input) {
        const UIRect& rect = input->getWorldRect();
        
        UIColor bgColor = input->isFocused() 
            ? UIColor(0.25f, 0.25f, 0.25f, 1.0f) 
            : UIColor(0.2f, 0.2f, 0.2f, 1.0f);
        renderer_->drawRoundedRect(rect, bgColor, 4.0f);
        
        // Border
        UIColor borderColor = input->isFocused()
            ? UIColor(0.3f, 0.6f, 1.0f, 1.0f)
            : UIColor(0.4f, 0.4f, 0.4f, 1.0f);
        renderer_->drawRectOutline(rect, borderColor, 1.0f);
        
        // Text
        std::string displayText = input->getText().empty() && !input->isFocused()
            ? input->getPlaceholder()
            : input->getDisplayText();
        UIColor textColor = input->getText().empty() && !input->isFocused()
            ? UIColor(0.5f, 0.5f, 0.5f, 1.0f)
            : UIColor::White();
        
        UIRect textRect = {rect.x + 8, rect.y, rect.width - 16, rect.height};
        renderer_->drawText(displayText, textRect, textColor, "default", 14.0f,
                           UILabel::HAlign::Left, UILabel::VAlign::Middle);
    }
    
    void drawDropdown(UIDropdown* dropdown) {
        const UIRect& rect = dropdown->getWorldRect();
        
        renderer_->drawRoundedRect(rect, UIColor(0.25f, 0.25f, 0.25f, 1.0f), 4.0f);
        renderer_->drawRectOutline(rect, UIColor(0.4f, 0.4f, 0.4f, 1.0f), 1.0f);
        
        // Selected text
        UIRect textRect = {rect.x + 8, rect.y, rect.width - 32, rect.height};
        renderer_->drawText(dropdown->getSelectedOption(), textRect, UIColor::White(),
                           "default", 14.0f, UILabel::HAlign::Left, UILabel::VAlign::Middle);
        
        // Arrow
        UIRect arrowRect = {rect.x + rect.width - 24, rect.y, 16, rect.height};
        renderer_->drawText(dropdown->isExpanded() ? "^" : "v", arrowRect, UIColor::White(),
                           "default", 12.0f, UILabel::HAlign::Center, UILabel::VAlign::Middle);
        
        // Dropdown list
        if (dropdown->isExpanded()) {
            const auto& options = dropdown->getOptions();
            float itemHeight = 28.0f;
            float listHeight = options.size() * itemHeight;
            
            UIRect listRect = {rect.x, rect.y + rect.height, rect.width, listHeight};
            renderer_->drawRoundedRect(listRect, UIColor(0.2f, 0.2f, 0.2f, 0.95f), 4.0f);
            
            for (size_t i = 0; i < options.size(); i++) {
                UIRect itemRect = {rect.x, rect.y + rect.height + i * itemHeight, rect.width, itemHeight};
                bool selected = (int)i == dropdown->getSelectedIndex();
                
                if (selected) {
                    renderer_->drawRect(itemRect, UIColor(0.3f, 0.5f, 0.8f, 1.0f));
                }
                
                UIRect itemTextRect = {itemRect.x + 8, itemRect.y, itemRect.width - 16, itemRect.height};
                renderer_->drawText(options[i], itemTextRect, UIColor::White(),
                                   "default", 14.0f, UILabel::HAlign::Left, UILabel::VAlign::Middle);
            }
        }
    }
    
    IUIRenderer* renderer_;
};

// ===== UI System =====
class UISystem {
public:
    static UISystem& getInstance() {
        static UISystem instance;
        return instance;
    }
    
    // Initialize
    void initialize(float screenWidth, float screenHeight) {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
    }
    
    void setScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
        for (auto& [name, canvas] : canvases_) {
            canvas->setScreenSize(width, height);
        }
    }
    
    float getScreenWidth() const { return screenWidth_; }
    float getScreenHeight() const { return screenHeight_; }
    
    // Canvas management
    UICanvas* createCanvas(const std::string& name) {
        auto canvas = std::make_unique<UICanvas>(name);
        canvas->setScreenSize(screenWidth_, screenHeight_);
        UICanvas* ptr = canvas.get();
        canvases_[name] = std::move(canvas);
        return ptr;
    }
    
    UICanvas* getCanvas(const std::string& name) {
        auto it = canvases_.find(name);
        return it != canvases_.end() ? it->second.get() : nullptr;
    }
    
    void removeCanvas(const std::string& name) {
        canvases_.erase(name);
    }
    
    const std::map<std::string, std::unique_ptr<UICanvas>>& getCanvases() const {
        return canvases_;
    }
    
    // Update all canvases
    void update(float dt) {
        for (auto& [name, canvas] : canvases_) {
            if (canvas->isVisible()) {
                canvas->update(dt);
            }
        }
    }
    
    // Handle event
    void handleEvent(UIEvent& event) {
        // Process canvases in reverse render order (top to bottom)
        std::vector<UICanvas*> sortedCanvases;
        for (auto& [name, canvas] : canvases_) {
            if (canvas->isVisible()) {
                sortedCanvases.push_back(canvas.get());
            }
        }
        std::sort(sortedCanvases.begin(), sortedCanvases.end(),
            [](UICanvas* a, UICanvas* b) { return a->getRenderOrder() > b->getRenderOrder(); });
        
        for (auto* canvas : sortedCanvases) {
            canvas->handleEvent(event);
            if (event.consumed) break;
        }
    }
    
    // Render
    void render(IUIRenderer* renderer) {
        renderer->beginFrame(screenWidth_, screenHeight_);
        
        UIWidgetDrawer drawer(renderer);
        
        // Sort by render order
        std::vector<UICanvas*> sortedCanvases;
        for (auto& [name, canvas] : canvases_) {
            if (canvas->isVisible()) {
                sortedCanvases.push_back(canvas.get());
            }
        }
        std::sort(sortedCanvases.begin(), sortedCanvases.end(),
            [](UICanvas* a, UICanvas* b) { return a->getRenderOrder() < b->getRenderOrder(); });
        
        for (auto* canvas : sortedCanvases) {
            drawer.draw(canvas->getRoot());
        }
        
        renderer->endFrame();
    }
    
    // Focused widget across all canvases
    UIWidget* getFocusedWidget() const {
        for (auto& [name, canvas] : canvases_) {
            if (auto* focused = canvas->getFocusedWidget()) {
                return focused;
            }
        }
        return nullptr;
    }
    
private:
    UISystem() = default;
    
    std::map<std::string, std::unique_ptr<UICanvas>> canvases_;
    float screenWidth_ = 1920.0f;
    float screenHeight_ = 1080.0f;
};

// ===== Global Accessor =====
inline UISystem& getUISystem() {
    return UISystem::getInstance();
}

// ===== Widget Factory =====
namespace UIFactory {
    inline std::shared_ptr<UIPanel> createPanel(const std::string& name = "Panel") {
        return std::make_shared<UIPanel>(name);
    }
    
    inline std::shared_ptr<UILabel> createLabel(const std::string& text, const std::string& name = "Label") {
        return std::make_shared<UILabel>(text, name);
    }
    
    inline std::shared_ptr<UIImage> createImage(const std::string& name = "Image") {
        return std::make_shared<UIImage>(name);
    }
    
    inline std::shared_ptr<UIButton> createButton(const std::string& text, const std::string& name = "Button") {
        return std::make_shared<UIButton>(text, name);
    }
    
    inline std::shared_ptr<UICheckbox> createCheckbox(const std::string& text, const std::string& name = "Checkbox") {
        return std::make_shared<UICheckbox>(text, name);
    }
    
    inline std::shared_ptr<UISlider> createSlider(const std::string& name = "Slider") {
        return std::make_shared<UISlider>(name);
    }
    
    inline std::shared_ptr<UIProgressBar> createProgressBar(const std::string& name = "ProgressBar") {
        return std::make_shared<UIProgressBar>(name);
    }
    
    inline std::shared_ptr<UIInputField> createInputField(const std::string& name = "InputField") {
        return std::make_shared<UIInputField>(name);
    }
    
    inline std::shared_ptr<UIDropdown> createDropdown(const std::string& name = "Dropdown") {
        return std::make_shared<UIDropdown>(name);
    }
    
    inline std::shared_ptr<UIScrollView> createScrollView(const std::string& name = "ScrollView") {
        return std::make_shared<UIScrollView>(name);
    }
    
    inline std::shared_ptr<UIListView> createListView(const std::string& name = "ListView") {
        return std::make_shared<UIListView>(name);
    }
    
    inline std::shared_ptr<UIHorizontalLayout> createHBox(const std::string& name = "HBox") {
        return std::make_shared<UIHorizontalLayout>(name);
    }
    
    inline std::shared_ptr<UIVerticalLayout> createVBox(const std::string& name = "VBox") {
        return std::make_shared<UIVerticalLayout>(name);
    }
    
    inline std::shared_ptr<UIGridLayout> createGrid(int columns, const std::string& name = "Grid") {
        auto grid = std::make_shared<UIGridLayout>(name);
        grid->setColumns(columns);
        return grid;
    }
}

}  // namespace ui
}  // namespace luma
