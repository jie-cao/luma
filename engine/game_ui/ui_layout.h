// Game UI System - Layout
// Layout containers: HBox, VBox, Grid, Stack
#pragma once

#include "ui_core.h"

namespace luma {
namespace ui {

// ===== Layout Base =====
class UILayout : public UIWidget {
public:
    UILayout(const std::string& name = "Layout") : UIWidget(name) {
        setInteractive(false);
    }
    
    // Padding
    void setPadding(const UIMargin& padding) { padding_ = padding; markLayoutDirty(); }
    void setPadding(float all) { padding_ = UIMargin(all); markLayoutDirty(); }
    const UIMargin& getPadding() const { return padding_; }
    
    // Spacing between children
    void setSpacing(float spacing) { spacing_ = spacing; markLayoutDirty(); }
    float getSpacing() const { return spacing_; }
    
    // Child alignment
    enum class ChildAlign { Start, Center, End, Stretch };
    void setChildAlignment(ChildAlign align) { childAlign_ = align; markLayoutDirty(); }
    ChildAlign getChildAlignment() const { return childAlign_; }
    
    // Fit content
    void setFitContent(bool fitWidth, bool fitHeight) {
        fitWidth_ = fitWidth;
        fitHeight_ = fitHeight;
        markLayoutDirty();
    }
    
    // Layout needs update
    void markLayoutDirty() { layoutDirty_ = true; markDirty(); }
    
    void update(float dt) override {
        if (layoutDirty_) {
            performLayout();
            layoutDirty_ = false;
        }
        UIWidget::update(dt);
    }
    
protected:
    virtual void performLayout() = 0;
    
    UIMargin padding_;
    float spacing_ = 4.0f;
    ChildAlign childAlign_ = ChildAlign::Start;
    bool fitWidth_ = false;
    bool fitHeight_ = false;
    bool layoutDirty_ = true;
};

// ===== Horizontal Layout =====
class UIHorizontalLayout : public UILayout {
public:
    UIHorizontalLayout(const std::string& name = "HorizontalLayout") : UILayout(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::HorizontalLayout; }
    
protected:
    void performLayout() override {
        const auto& children = getChildren();
        if (children.empty()) return;
        
        float contentWidth = getWidth() - padding_.left - padding_.right;
        float contentHeight = getHeight() - padding_.top - padding_.bottom;
        
        // Calculate total fixed width and flexible items
        float totalFixedWidth = 0.0f;
        int flexibleCount = 0;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float childMarginH = child->getMargin().left + child->getMargin().right;
            totalFixedWidth += child->getWidth() + childMarginH;
            // Could add flex factor support here
        }
        
        totalFixedWidth += (children.size() - 1) * spacing_;
        
        // Position children
        float x = padding_.left;
        float y = padding_.top;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float childMarginL = child->getMargin().left;
            float childMarginT = child->getMargin().top;
            float childMarginB = child->getMargin().bottom;
            
            float childX = x + childMarginL;
            float childY = y + childMarginT;
            float childH = child->getHeight();
            
            // Vertical alignment
            switch (childAlign_) {
                case ChildAlign::Start:
                    break;
                case ChildAlign::Center:
                    childY = y + (contentHeight - childH) * 0.5f;
                    break;
                case ChildAlign::End:
                    childY = y + contentHeight - childH - childMarginB;
                    break;
                case ChildAlign::Stretch:
                    childH = contentHeight - childMarginT - childMarginB;
                    break;
            }
            
            child->setPosition(childX, childY);
            if (childAlign_ == ChildAlign::Stretch) {
                child->setSize(child->getWidth(), childH);
            }
            
            x += child->getWidth() + child->getMargin().left + child->getMargin().right + spacing_;
        }
        
        // Fit content
        if (fitWidth_) {
            setSize(x - spacing_ + padding_.right, getHeight());
        }
    }
};

// ===== Vertical Layout =====
class UIVerticalLayout : public UILayout {
public:
    UIVerticalLayout(const std::string& name = "VerticalLayout") : UILayout(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::VerticalLayout; }
    
protected:
    void performLayout() override {
        const auto& children = getChildren();
        if (children.empty()) return;
        
        float contentWidth = getWidth() - padding_.left - padding_.right;
        float contentHeight = getHeight() - padding_.top - padding_.bottom;
        
        // Position children
        float x = padding_.left;
        float y = padding_.top;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float childMarginL = child->getMargin().left;
            float childMarginR = child->getMargin().right;
            float childMarginT = child->getMargin().top;
            
            float childX = x + childMarginL;
            float childY = y + childMarginT;
            float childW = child->getWidth();
            
            // Horizontal alignment
            switch (childAlign_) {
                case ChildAlign::Start:
                    break;
                case ChildAlign::Center:
                    childX = x + (contentWidth - childW) * 0.5f;
                    break;
                case ChildAlign::End:
                    childX = x + contentWidth - childW - childMarginR;
                    break;
                case ChildAlign::Stretch:
                    childW = contentWidth - childMarginL - childMarginR;
                    break;
            }
            
            child->setPosition(childX, childY);
            if (childAlign_ == ChildAlign::Stretch) {
                child->setSize(childW, child->getHeight());
            }
            
            y += child->getHeight() + child->getMargin().top + child->getMargin().bottom + spacing_;
        }
        
        // Fit content
        if (fitHeight_) {
            setSize(getWidth(), y - spacing_ + padding_.bottom);
        }
    }
};

// ===== Grid Layout =====
class UIGridLayout : public UILayout {
public:
    UIGridLayout(const std::string& name = "GridLayout") : UILayout(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::GridLayout; }
    
    // Grid settings
    void setColumns(int columns) { columns_ = std::max(1, columns); markLayoutDirty(); }
    int getColumns() const { return columns_; }
    
    void setCellSize(float width, float height) {
        cellWidth_ = width;
        cellHeight_ = height;
        markLayoutDirty();
    }
    
    void setSpacingH(float spacing) { spacingH_ = spacing; markLayoutDirty(); }
    void setSpacingV(float spacing) { spacingV_ = spacing; markLayoutDirty(); }
    
    float getCellWidth() const { return cellWidth_; }
    float getCellHeight() const { return cellHeight_; }
    
    // Auto calculate cell size
    void setAutoSize(bool autoSize) { autoSize_ = autoSize; markLayoutDirty(); }
    
protected:
    void performLayout() override {
        const auto& children = getChildren();
        if (children.empty()) return;
        
        float contentWidth = getWidth() - padding_.left - padding_.right;
        float contentHeight = getHeight() - padding_.top - padding_.bottom;
        
        float cellW = cellWidth_;
        float cellH = cellHeight_;
        
        // Auto calculate cell size
        if (autoSize_) {
            cellW = (contentWidth - (columns_ - 1) * spacingH_) / columns_;
        }
        
        int col = 0;
        int row = 0;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float x = padding_.left + col * (cellW + spacingH_);
            float y = padding_.top + row * (cellH + spacingV_);
            
            child->setPosition(x, y);
            
            // Optionally resize child to cell size
            if (childAlign_ == ChildAlign::Stretch) {
                child->setSize(cellW, cellH);
            }
            
            col++;
            if (col >= columns_) {
                col = 0;
                row++;
            }
        }
        
        // Fit content
        if (fitHeight_) {
            int rows = (int)children.size() / columns_ + ((int)children.size() % columns_ != 0 ? 1 : 0);
            float totalHeight = padding_.top + padding_.bottom + rows * cellH + (rows - 1) * spacingV_;
            setSize(getWidth(), totalHeight);
        }
    }
    
private:
    int columns_ = 3;
    float cellWidth_ = 100.0f;
    float cellHeight_ = 100.0f;
    float spacingH_ = 4.0f;
    float spacingV_ = 4.0f;
    bool autoSize_ = false;
};

// ===== Stack Layout (overlapping children) =====
class UIStackLayout : public UILayout {
public:
    UIStackLayout(const std::string& name = "StackLayout") : UILayout(name) {}
    
    // All children overlap at the same position
    
protected:
    void performLayout() override {
        const auto& children = getChildren();
        
        float contentWidth = getWidth() - padding_.left - padding_.right;
        float contentHeight = getHeight() - padding_.top - padding_.bottom;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float childW = child->getWidth();
            float childH = child->getHeight();
            float childX = padding_.left;
            float childY = padding_.top;
            
            // Apply alignment
            switch (childAlign_) {
                case ChildAlign::Center:
                    childX = padding_.left + (contentWidth - childW) * 0.5f;
                    childY = padding_.top + (contentHeight - childH) * 0.5f;
                    break;
                case ChildAlign::Stretch:
                    childW = contentWidth;
                    childH = contentHeight;
                    break;
                default:
                    break;
            }
            
            child->setPosition(childX, childY);
            if (childAlign_ == ChildAlign::Stretch) {
                child->setSize(childW, childH);
            }
        }
    }
};

// ===== Flow Layout (wrapping) =====
class UIFlowLayout : public UILayout {
public:
    UIFlowLayout(const std::string& name = "FlowLayout") : UILayout(name) {}
    
    void setSpacingH(float spacing) { spacingH_ = spacing; markLayoutDirty(); }
    void setSpacingV(float spacing) { spacingV_ = spacing; markLayoutDirty(); }
    
protected:
    void performLayout() override {
        const auto& children = getChildren();
        if (children.empty()) return;
        
        float contentWidth = getWidth() - padding_.left - padding_.right;
        
        float x = padding_.left;
        float y = padding_.top;
        float rowHeight = 0.0f;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            float childW = child->getWidth() + child->getMargin().left + child->getMargin().right;
            float childH = child->getHeight() + child->getMargin().top + child->getMargin().bottom;
            
            // Wrap to next row
            if (x + childW > padding_.left + contentWidth && x > padding_.left) {
                x = padding_.left;
                y += rowHeight + spacingV_;
                rowHeight = 0.0f;
            }
            
            child->setPosition(x + child->getMargin().left, y + child->getMargin().top);
            
            x += childW + spacingH_;
            rowHeight = std::max(rowHeight, childH);
        }
        
        // Fit content
        if (fitHeight_) {
            setSize(getWidth(), y + rowHeight + padding_.bottom);
        }
    }
    
private:
    float spacingH_ = 4.0f;
    float spacingV_ = 4.0f;
};

// ===== Anchor Layout (position by anchors) =====
class UIAnchorLayout : public UILayout {
public:
    UIAnchorLayout(const std::string& name = "AnchorLayout") : UILayout(name) {}
    
    // Children use their own anchor settings
    
protected:
    void performLayout() override {
        // Each child positions itself based on its anchor
        // This is handled by the base UIWidget::calculateWorldRect
        // Just ensure children fill available space if stretch
        
        const auto& children = getChildren();
        float contentWidth = getWidth() - padding_.left - padding_.right;
        float contentHeight = getHeight() - padding_.top - padding_.bottom;
        
        for (const auto& child : children) {
            if (!child->isVisible()) continue;
            
            if (child->getAnchor() == UIAnchor::Stretch) {
                child->setSize(contentWidth, contentHeight);
            }
        }
    }
};

}  // namespace ui
}  // namespace luma
