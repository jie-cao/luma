// Game UI System - Core
// Runtime UI for games (Canvas, Widget base, Events)
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>

namespace luma {
namespace ui {

// ===== Forward Declarations =====
class UIWidget;
class UICanvas;

// ===== UI Anchor =====
enum class UIAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Stretch
};

// ===== UI Pivot =====
struct UIPivot {
    float x = 0.5f;  // 0 = left, 1 = right
    float y = 0.5f;  // 0 = top, 1 = bottom
};

// ===== UI Rect =====
struct UIRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 100.0f;
    
    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool intersects(const UIRect& other) const {
        return !(x + width <= other.x || other.x + other.width <= x ||
                 y + height <= other.y || other.y + other.height <= y);
    }
    
    UIRect intersection(const UIRect& other) const {
        float nx = std::max(x, other.x);
        float ny = std::max(y, other.y);
        float nw = std::min(x + width, other.x + other.width) - nx;
        float nh = std::min(y + height, other.y + other.height) - ny;
        return {nx, ny, std::max(0.0f, nw), std::max(0.0f, nh)};
    }
};

// ===== UI Color =====
struct UIColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
    
    UIColor() = default;
    UIColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    static UIColor White() { return {1, 1, 1, 1}; }
    static UIColor Black() { return {0, 0, 0, 1}; }
    static UIColor Red() { return {1, 0, 0, 1}; }
    static UIColor Green() { return {0, 1, 0, 1}; }
    static UIColor Blue() { return {0, 0, 1, 1}; }
    static UIColor Yellow() { return {1, 1, 0, 1}; }
    static UIColor Cyan() { return {0, 1, 1, 1}; }
    static UIColor Magenta() { return {1, 0, 1, 1}; }
    static UIColor Transparent() { return {0, 0, 0, 0}; }
    
    UIColor operator*(float s) const { return {r*s, g*s, b*s, a}; }
    UIColor withAlpha(float newA) const { return {r, g, b, newA}; }
};

// ===== UI Margin =====
struct UIMargin {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
    
    UIMargin() = default;
    UIMargin(float all) : left(all), right(all), top(all), bottom(all) {}
    UIMargin(float h, float v) : left(h), right(h), top(v), bottom(v) {}
    UIMargin(float l, float r, float t, float b) : left(l), right(r), top(t), bottom(b) {}
};

// ===== UI Event Types =====
enum class UIEventType {
    None,
    PointerDown,
    PointerUp,
    PointerMove,
    PointerEnter,
    PointerExit,
    Click,
    DoubleClick,
    DragStart,
    Drag,
    DragEnd,
    Scroll,
    KeyDown,
    KeyUp,
    TextInput,
    Focus,
    Blur
};

// ===== UI Event =====
struct UIEvent {
    UIEventType type = UIEventType::None;
    
    // Pointer
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    int button = 0;  // 0 = left, 1 = right, 2 = middle
    
    // Keyboard
    int keyCode = 0;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    char character = 0;
    
    // Scroll
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    
    // State
    bool consumed = false;
    UIWidget* target = nullptr;
    
    void consume() { consumed = true; }
};

// ===== UI Event Handler =====
using UIEventHandler = std::function<void(UIEvent&)>;

// ===== Widget Type =====
enum class UIWidgetType {
    Base,
    Panel,
    Label,
    Image,
    Button,
    Checkbox,
    Slider,
    ProgressBar,
    InputField,
    Dropdown,
    ScrollView,
    ListView,
    // Layout
    HorizontalLayout,
    VerticalLayout,
    GridLayout
};

// ===== UI Widget =====
class UIWidget : public std::enable_shared_from_this<UIWidget> {
public:
    UIWidget(const std::string& name = "Widget")
        : name_(name), id_(nextId_++) {}
    virtual ~UIWidget() = default;
    
    // Identity
    uint32_t getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    virtual UIWidgetType getType() const { return UIWidgetType::Base; }
    
    // Visibility
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }
    
    bool isInteractive() const { return interactive_; }
    void setInteractive(bool i) { interactive_ = i; }
    
    // Transform
    void setPosition(float x, float y) { localRect_.x = x; localRect_.y = y; markDirty(); }
    void setSize(float w, float h) { localRect_.width = w; localRect_.height = h; markDirty(); }
    void setRect(const UIRect& rect) { localRect_ = rect; markDirty(); }
    
    float getX() const { return localRect_.x; }
    float getY() const { return localRect_.y; }
    float getWidth() const { return localRect_.width; }
    float getHeight() const { return localRect_.height; }
    const UIRect& getLocalRect() const { return localRect_; }
    
    // Computed world rect
    const UIRect& getWorldRect() const { return worldRect_; }
    
    // Anchor & Pivot
    void setAnchor(UIAnchor anchor) { anchor_ = anchor; markDirty(); }
    UIAnchor getAnchor() const { return anchor_; }
    
    void setPivot(float x, float y) { pivot_ = {x, y}; markDirty(); }
    const UIPivot& getPivot() const { return pivot_; }
    
    // Margin
    void setMargin(const UIMargin& margin) { margin_ = margin; markDirty(); }
    const UIMargin& getMargin() const { return margin_; }
    
    // Appearance
    void setColor(const UIColor& color) { color_ = color; }
    const UIColor& getColor() const { return color_; }
    
    void setAlpha(float alpha) { color_.a = alpha; }
    float getAlpha() const { return color_.a; }
    
    // Hierarchy
    UIWidget* getParent() const { return parent_; }
    const std::vector<std::shared_ptr<UIWidget>>& getChildren() const { return children_; }
    
    void addChild(std::shared_ptr<UIWidget> child) {
        if (!child || child.get() == this) return;
        child->removeFromParent();
        child->parent_ = this;
        children_.push_back(child);
        markDirty();
    }
    
    void removeChild(UIWidget* child) {
        children_.erase(
            std::remove_if(children_.begin(), children_.end(),
                [child](const auto& c) { return c.get() == child; }),
            children_.end()
        );
        if (child) child->parent_ = nullptr;
        markDirty();
    }
    
    void removeFromParent() {
        if (parent_) {
            parent_->removeChild(this);
        }
    }
    
    // Events
    void addEventListener(UIEventType type, UIEventHandler handler) {
        eventHandlers_[type].push_back(handler);
    }
    
    void setOnClick(UIEventHandler handler) { onClick_ = handler; }
    void setOnHover(UIEventHandler handler) { onHover_ = handler; }
    void setOnValueChanged(std::function<void(float)> handler) { onValueChanged_ = handler; }
    void setOnTextChanged(std::function<void(const std::string&)> handler) { onTextChanged_ = handler; }
    
    // Hit test
    virtual bool hitTest(float x, float y) const {
        return visible_ && worldRect_.contains(x, y);
    }
    
    UIWidget* hitTestRecursive(float x, float y) {
        if (!visible_ || !worldRect_.contains(x, y)) return nullptr;
        
        // Check children in reverse order (top to bottom)
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            UIWidget* hit = (*it)->hitTestRecursive(x, y);
            if (hit) return hit;
        }
        
        return interactive_ ? this : nullptr;
    }
    
    // Update
    virtual void update(float dt) {
        for (auto& child : children_) {
            child->update(dt);
        }
    }
    
    // Layout
    void updateLayout(const UIRect& parentRect) {
        calculateWorldRect(parentRect);
        
        for (auto& child : children_) {
            child->updateLayout(worldRect_);
        }
        
        dirty_ = false;
    }
    
    // Event dispatch
    virtual void dispatchEvent(UIEvent& event) {
        if (!enabled_) return;
        
        // Handle locally
        handleEvent(event);
        
        // Notify handlers
        auto it = eventHandlers_.find(event.type);
        if (it != eventHandlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
                if (event.consumed) break;
            }
        }
        
        // Special callbacks
        if (event.type == UIEventType::Click && onClick_) {
            onClick_(event);
        }
        if ((event.type == UIEventType::PointerEnter || event.type == UIEventType::PointerMove) && onHover_) {
            onHover_(event);
        }
    }
    
    // State
    bool isHovered() const { return hovered_; }
    bool isPressed() const { return pressed_; }
    bool isFocused() const { return focused_; }
    
    void setHovered(bool h) { hovered_ = h; }
    void setPressed(bool p) { pressed_ = p; }
    void setFocused(bool f) { focused_ = f; }
    
    // Z-order
    int getZOrder() const { return zOrder_; }
    void setZOrder(int z) { zOrder_ = z; }
    
protected:
    virtual void handleEvent(UIEvent& event) {}
    
    virtual void calculateWorldRect(const UIRect& parentRect) {
        float x = localRect_.x + margin_.left;
        float y = localRect_.y + margin_.top;
        float w = localRect_.width;
        float h = localRect_.height;
        
        // Apply anchor
        switch (anchor_) {
            case UIAnchor::TopLeft:
                x += parentRect.x;
                y += parentRect.y;
                break;
            case UIAnchor::TopCenter:
                x += parentRect.x + parentRect.width * 0.5f - w * pivot_.x;
                y += parentRect.y;
                break;
            case UIAnchor::TopRight:
                x = parentRect.x + parentRect.width - w - margin_.right + localRect_.x;
                y += parentRect.y;
                break;
            case UIAnchor::MiddleLeft:
                x += parentRect.x;
                y += parentRect.y + parentRect.height * 0.5f - h * pivot_.y;
                break;
            case UIAnchor::MiddleCenter:
                x += parentRect.x + parentRect.width * 0.5f - w * pivot_.x;
                y += parentRect.y + parentRect.height * 0.5f - h * pivot_.y;
                break;
            case UIAnchor::MiddleRight:
                x = parentRect.x + parentRect.width - w - margin_.right + localRect_.x;
                y += parentRect.y + parentRect.height * 0.5f - h * pivot_.y;
                break;
            case UIAnchor::BottomLeft:
                x += parentRect.x;
                y = parentRect.y + parentRect.height - h - margin_.bottom + localRect_.y;
                break;
            case UIAnchor::BottomCenter:
                x += parentRect.x + parentRect.width * 0.5f - w * pivot_.x;
                y = parentRect.y + parentRect.height - h - margin_.bottom + localRect_.y;
                break;
            case UIAnchor::BottomRight:
                x = parentRect.x + parentRect.width - w - margin_.right + localRect_.x;
                y = parentRect.y + parentRect.height - h - margin_.bottom + localRect_.y;
                break;
            case UIAnchor::Stretch:
                x = parentRect.x + margin_.left;
                y = parentRect.y + margin_.top;
                w = parentRect.width - margin_.left - margin_.right;
                h = parentRect.height - margin_.top - margin_.bottom;
                break;
        }
        
        worldRect_ = {x, y, w, h};
    }
    
    void markDirty() { dirty_ = true; }
    
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
    
    std::string name_;
    bool visible_ = true;
    bool enabled_ = true;
    bool interactive_ = true;
    
    UIRect localRect_;
    UIRect worldRect_;
    UIAnchor anchor_ = UIAnchor::TopLeft;
    UIPivot pivot_;
    UIMargin margin_;
    UIColor color_ = UIColor::White();
    int zOrder_ = 0;
    
    bool dirty_ = true;
    bool hovered_ = false;
    bool pressed_ = false;
    bool focused_ = false;
    
    UIWidget* parent_ = nullptr;
    std::vector<std::shared_ptr<UIWidget>> children_;
    
    std::unordered_map<UIEventType, std::vector<UIEventHandler>> eventHandlers_;
    UIEventHandler onClick_;
    UIEventHandler onHover_;
    std::function<void(float)> onValueChanged_;
    std::function<void(const std::string&)> onTextChanged_;
};

// ===== UI Canvas =====
class UICanvas {
public:
    UICanvas(const std::string& name = "Canvas")
        : name_(name) {
        root_ = std::make_shared<UIWidget>("Root");
        root_->setInteractive(false);
    }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Screen size
    void setScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
    }
    
    float getScreenWidth() const { return screenWidth_; }
    float getScreenHeight() const { return screenHeight_; }
    
    // Root
    UIWidget* getRoot() { return root_.get(); }
    
    void addWidget(std::shared_ptr<UIWidget> widget) {
        root_->addChild(widget);
    }
    
    void removeWidget(UIWidget* widget) {
        root_->removeChild(widget);
    }
    
    // Update
    void update(float dt) {
        UIRect screenRect = {0, 0, screenWidth_, screenHeight_};
        root_->setRect(screenRect);
        root_->updateLayout(screenRect);
        root_->update(dt);
    }
    
    // Event handling
    void handleEvent(UIEvent& event) {
        switch (event.type) {
            case UIEventType::PointerDown:
            case UIEventType::PointerUp:
            case UIEventType::PointerMove:
            case UIEventType::Click:
                handlePointerEvent(event);
                break;
            case UIEventType::KeyDown:
            case UIEventType::KeyUp:
            case UIEventType::TextInput:
                handleKeyEvent(event);
                break;
            default:
                break;
        }
    }
    
    // Focus
    UIWidget* getFocusedWidget() const { return focusedWidget_; }
    void setFocusedWidget(UIWidget* widget) {
        if (focusedWidget_ != widget) {
            if (focusedWidget_) {
                focusedWidget_->setFocused(false);
                UIEvent blurEvent;
                blurEvent.type = UIEventType::Blur;
                focusedWidget_->dispatchEvent(blurEvent);
            }
            focusedWidget_ = widget;
            if (focusedWidget_) {
                focusedWidget_->setFocused(true);
                UIEvent focusEvent;
                focusEvent.type = UIEventType::Focus;
                focusedWidget_->dispatchEvent(focusEvent);
            }
        }
    }
    
    // Hovered widget
    UIWidget* getHoveredWidget() const { return hoveredWidget_; }
    
    // Visibility
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    
    // Render order
    int getRenderOrder() const { return renderOrder_; }
    void setRenderOrder(int order) { renderOrder_ = order; }
    
private:
    void handlePointerEvent(UIEvent& event) {
        UIWidget* hit = root_->hitTestRecursive(event.x, event.y);
        
        // Hover handling
        if (hit != hoveredWidget_) {
            if (hoveredWidget_) {
                hoveredWidget_->setHovered(false);
                UIEvent exitEvent = event;
                exitEvent.type = UIEventType::PointerExit;
                hoveredWidget_->dispatchEvent(exitEvent);
            }
            hoveredWidget_ = hit;
            if (hoveredWidget_) {
                hoveredWidget_->setHovered(true);
                UIEvent enterEvent = event;
                enterEvent.type = UIEventType::PointerEnter;
                hoveredWidget_->dispatchEvent(enterEvent);
            }
        }
        
        // Click/press handling
        if (event.type == UIEventType::PointerDown && hit) {
            pressedWidget_ = hit;
            hit->setPressed(true);
            setFocusedWidget(hit);
            hit->dispatchEvent(event);
        }
        else if (event.type == UIEventType::PointerUp) {
            if (pressedWidget_) {
                pressedWidget_->setPressed(false);
                if (hit == pressedWidget_) {
                    UIEvent clickEvent = event;
                    clickEvent.type = UIEventType::Click;
                    pressedWidget_->dispatchEvent(clickEvent);
                }
                pressedWidget_->dispatchEvent(event);
                pressedWidget_ = nullptr;
            }
        }
        else if (event.type == UIEventType::PointerMove && hit) {
            hit->dispatchEvent(event);
        }
    }
    
    void handleKeyEvent(UIEvent& event) {
        if (focusedWidget_) {
            focusedWidget_->dispatchEvent(event);
        }
    }
    
    std::string name_;
    std::shared_ptr<UIWidget> root_;
    
    float screenWidth_ = 1920.0f;
    float screenHeight_ = 1080.0f;
    
    bool visible_ = true;
    int renderOrder_ = 0;
    
    UIWidget* focusedWidget_ = nullptr;
    UIWidget* hoveredWidget_ = nullptr;
    UIWidget* pressedWidget_ = nullptr;
};

}  // namespace ui
}  // namespace luma
