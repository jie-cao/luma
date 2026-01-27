// Game UI System - Widgets
// Common UI controls: Text, Image, Button, Slider, etc.
#pragma once

#include "ui_core.h"
#include <sstream>
#include <iomanip>

namespace luma {
namespace ui {

// ===== UI Panel =====
class UIPanel : public UIWidget {
public:
    UIPanel(const std::string& name = "Panel") : UIWidget(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::Panel; }
    
    // Background
    void setBackgroundColor(const UIColor& color) { backgroundColor_ = color; }
    const UIColor& getBackgroundColor() const { return backgroundColor_; }
    
    // Border
    void setBorderColor(const UIColor& color) { borderColor_ = color; }
    const UIColor& getBorderColor() const { return borderColor_; }
    
    void setBorderWidth(float width) { borderWidth_ = width; }
    float getBorderWidth() const { return borderWidth_; }
    
    // Corner radius
    void setCornerRadius(float radius) { cornerRadius_ = radius; }
    float getCornerRadius() const { return cornerRadius_; }
    
private:
    UIColor backgroundColor_ = UIColor(0.2f, 0.2f, 0.2f, 0.9f);
    UIColor borderColor_ = UIColor(0.4f, 0.4f, 0.4f, 1.0f);
    float borderWidth_ = 1.0f;
    float cornerRadius_ = 4.0f;
};

// ===== UI Label =====
class UILabel : public UIWidget {
public:
    UILabel(const std::string& text = "", const std::string& name = "Label")
        : UIWidget(name), text_(text) {
        setInteractive(false);
    }
    
    UIWidgetType getType() const override { return UIWidgetType::Label; }
    
    // Text
    void setText(const std::string& text) { text_ = text; }
    const std::string& getText() const { return text_; }
    
    // Font
    void setFontSize(float size) { fontSize_ = size; }
    float getFontSize() const { return fontSize_; }
    
    void setFontName(const std::string& name) { fontName_ = name; }
    const std::string& getFontName() const { return fontName_; }
    
    // Text color (uses base color)
    void setTextColor(const UIColor& color) { setColor(color); }
    UIColor getTextColor() const { return getColor(); }
    
    // Alignment
    enum class HAlign { Left, Center, Right };
    enum class VAlign { Top, Middle, Bottom };
    
    void setHAlign(HAlign align) { hAlign_ = align; }
    HAlign getHAlign() const { return hAlign_; }
    
    void setVAlign(VAlign align) { vAlign_ = align; }
    VAlign getVAlign() const { return vAlign_; }
    
    // Word wrap
    void setWordWrap(bool wrap) { wordWrap_ = wrap; }
    bool getWordWrap() const { return wordWrap_; }
    
    // Shadow
    void setShadow(bool enabled) { shadowEnabled_ = enabled; }
    bool hasShadow() const { return shadowEnabled_; }
    
    void setShadowColor(const UIColor& color) { shadowColor_ = color; }
    void setShadowOffset(float x, float y) { shadowOffsetX_ = x; shadowOffsetY_ = y; }
    
private:
    std::string text_;
    std::string fontName_ = "default";
    float fontSize_ = 16.0f;
    HAlign hAlign_ = HAlign::Left;
    VAlign vAlign_ = VAlign::Middle;
    bool wordWrap_ = false;
    
    bool shadowEnabled_ = false;
    UIColor shadowColor_ = UIColor(0, 0, 0, 0.5f);
    float shadowOffsetX_ = 1.0f;
    float shadowOffsetY_ = 1.0f;
};

// ===== UI Image =====
class UIImage : public UIWidget {
public:
    UIImage(const std::string& name = "Image") : UIWidget(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::Image; }
    
    // Texture
    void setTexture(const std::string& path) { texturePath_ = path; }
    const std::string& getTexture() const { return texturePath_; }
    
    void setTextureHandle(uint64_t handle) { textureHandle_ = handle; }
    uint64_t getTextureHandle() const { return textureHandle_; }
    
    // UV rect (for sprites/atlases)
    void setUVRect(float u, float v, float w, float h) {
        uvRect_ = {u, v, w, h};
    }
    const UIRect& getUVRect() const { return uvRect_; }
    
    // Image type
    enum class Type {
        Simple,    // Stretch
        Sliced,    // 9-slice
        Tiled,     // Repeat
        Filled     // Progress fill
    };
    
    void setImageType(Type type) { imageType_ = type; }
    Type getImageType() const { return imageType_; }
    
    // Fill (for Filled type)
    enum class FillMethod { Horizontal, Vertical, Radial90, Radial180, Radial360 };
    void setFillMethod(FillMethod method) { fillMethod_ = method; }
    void setFillAmount(float amount) { fillAmount_ = std::max(0.0f, std::min(1.0f, amount)); }
    float getFillAmount() const { return fillAmount_; }
    
    // Preserve aspect ratio
    void setPreserveAspect(bool preserve) { preserveAspect_ = preserve; }
    bool getPreserveAspect() const { return preserveAspect_; }
    
    // 9-slice borders
    void setSliceBorder(float left, float right, float top, float bottom) {
        sliceBorder_ = {left, right, top, bottom};
    }
    
private:
    std::string texturePath_;
    uint64_t textureHandle_ = 0;
    UIRect uvRect_ = {0, 0, 1, 1};
    
    Type imageType_ = Type::Simple;
    FillMethod fillMethod_ = FillMethod::Horizontal;
    float fillAmount_ = 1.0f;
    bool preserveAspect_ = false;
    
    UIMargin sliceBorder_;
};

// ===== UI Button =====
class UIButton : public UIWidget {
public:
    UIButton(const std::string& text = "Button", const std::string& name = "Button")
        : UIWidget(name) {
        label_ = std::make_shared<UILabel>(text, "ButtonLabel");
        label_->setAnchor(UIAnchor::MiddleCenter);
        label_->setHAlign(UILabel::HAlign::Center);
        addChild(label_);
    }
    
    UIWidgetType getType() const override { return UIWidgetType::Button; }
    
    // Text
    void setText(const std::string& text) { label_->setText(text); }
    const std::string& getText() const { return label_->getText(); }
    
    // Label access
    UILabel* getLabel() { return label_.get(); }
    
    // Colors
    void setNormalColor(const UIColor& c) { normalColor_ = c; }
    void setHoverColor(const UIColor& c) { hoverColor_ = c; }
    void setPressedColor(const UIColor& c) { pressedColor_ = c; }
    void setDisabledColor(const UIColor& c) { disabledColor_ = c; }
    
    const UIColor& getNormalColor() const { return normalColor_; }
    const UIColor& getHoverColor() const { return hoverColor_; }
    const UIColor& getPressedColor() const { return pressedColor_; }
    const UIColor& getDisabledColor() const { return disabledColor_; }
    
    // Border
    void setBorderRadius(float radius) { borderRadius_ = radius; }
    float getBorderRadius() const { return borderRadius_; }
    
    // Icon
    void setIcon(const std::string& texturePath) { iconPath_ = texturePath; }
    const std::string& getIcon() const { return iconPath_; }
    
    // Get current color based on state
    UIColor getCurrentColor() const {
        if (!isEnabled()) return disabledColor_;
        if (isPressed()) return pressedColor_;
        if (isHovered()) return hoverColor_;
        return normalColor_;
    }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (event.type == UIEventType::Click) {
            // Button click sound/animation could be triggered here
        }
    }
    
private:
    std::shared_ptr<UILabel> label_;
    std::string iconPath_;
    
    UIColor normalColor_ = UIColor(0.3f, 0.3f, 0.3f, 1.0f);
    UIColor hoverColor_ = UIColor(0.4f, 0.4f, 0.4f, 1.0f);
    UIColor pressedColor_ = UIColor(0.2f, 0.2f, 0.2f, 1.0f);
    UIColor disabledColor_ = UIColor(0.2f, 0.2f, 0.2f, 0.5f);
    
    float borderRadius_ = 4.0f;
};

// ===== UI Checkbox =====
class UICheckbox : public UIWidget {
public:
    UICheckbox(const std::string& text = "Checkbox", const std::string& name = "Checkbox")
        : UIWidget(name) {
        label_ = std::make_shared<UILabel>(text, "CheckboxLabel");
        label_->setPosition(24, 0);
        addChild(label_);
    }
    
    UIWidgetType getType() const override { return UIWidgetType::Checkbox; }
    
    // Value
    bool isChecked() const { return checked_; }
    void setChecked(bool checked) {
        if (checked_ != checked) {
            checked_ = checked;
            if (onValueChanged_) onValueChanged_(checked_ ? 1.0f : 0.0f);
        }
    }
    
    void toggle() { setChecked(!checked_); }
    
    // Text
    void setText(const std::string& text) { label_->setText(text); }
    
    // Box size
    void setBoxSize(float size) { boxSize_ = size; }
    float getBoxSize() const { return boxSize_; }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (event.type == UIEventType::Click) {
            toggle();
        }
    }
    
private:
    std::shared_ptr<UILabel> label_;
    bool checked_ = false;
    float boxSize_ = 18.0f;
};

// ===== UI Slider =====
class UISlider : public UIWidget {
public:
    UISlider(const std::string& name = "Slider") : UIWidget(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::Slider; }
    
    // Value
    float getValue() const { return value_; }
    void setValue(float value) {
        float newValue = std::max(minValue_, std::min(maxValue_, value));
        if (newValue != value_) {
            value_ = newValue;
            if (onValueChanged_) onValueChanged_(value_);
        }
    }
    
    // Range
    float getMinValue() const { return minValue_; }
    float getMaxValue() const { return maxValue_; }
    void setRange(float min, float max) { minValue_ = min; maxValue_ = max; setValue(value_); }
    
    // Step
    void setStep(float step) { step_ = step; }
    float getStep() const { return step_; }
    
    // Normalized value (0-1)
    float getNormalizedValue() const {
        return (maxValue_ > minValue_) ? (value_ - minValue_) / (maxValue_ - minValue_) : 0.0f;
    }
    
    void setNormalizedValue(float normalized) {
        setValue(minValue_ + normalized * (maxValue_ - minValue_));
    }
    
    // Direction
    enum class Direction { Horizontal, Vertical };
    void setDirection(Direction dir) { direction_ = dir; }
    Direction getDirection() const { return direction_; }
    
    // Colors
    void setTrackColor(const UIColor& c) { trackColor_ = c; }
    void setFillColor(const UIColor& c) { fillColor_ = c; }
    void setHandleColor(const UIColor& c) { handleColor_ = c; }
    
    // Handle size
    void setHandleSize(float size) { handleSize_ = size; }
    float getHandleSize() const { return handleSize_; }
    
    // Show value
    void setShowValue(bool show) { showValue_ = show; }
    bool getShowValue() const { return showValue_; }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (event.type == UIEventType::PointerDown || 
            (event.type == UIEventType::PointerMove && isPressed())) {
            updateValueFromPointer(event.x, event.y);
        }
    }
    
private:
    void updateValueFromPointer(float x, float y) {
        const UIRect& rect = getWorldRect();
        float normalized;
        
        if (direction_ == Direction::Horizontal) {
            normalized = (x - rect.x) / rect.width;
        } else {
            normalized = 1.0f - (y - rect.y) / rect.height;
        }
        
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        
        if (step_ > 0) {
            float range = maxValue_ - minValue_;
            float steps = range / step_;
            normalized = std::round(normalized * steps) / steps;
        }
        
        setNormalizedValue(normalized);
    }
    
    float value_ = 0.0f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    float step_ = 0.0f;
    
    Direction direction_ = Direction::Horizontal;
    
    UIColor trackColor_ = UIColor(0.2f, 0.2f, 0.2f, 1.0f);
    UIColor fillColor_ = UIColor(0.3f, 0.6f, 1.0f, 1.0f);
    UIColor handleColor_ = UIColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    float handleSize_ = 16.0f;
    bool showValue_ = false;
};

// ===== UI Progress Bar =====
class UIProgressBar : public UIWidget {
public:
    UIProgressBar(const std::string& name = "ProgressBar") : UIWidget(name) {
        setInteractive(false);
    }
    
    UIWidgetType getType() const override { return UIWidgetType::ProgressBar; }
    
    // Value (0-1)
    float getValue() const { return value_; }
    void setValue(float value) { value_ = std::max(0.0f, std::min(1.0f, value)); }
    
    // Colors
    void setBackgroundColor(const UIColor& c) { backgroundColor_ = c; }
    void setFillColor(const UIColor& c) { fillColor_ = c; }
    
    const UIColor& getBackgroundColor() const { return backgroundColor_; }
    const UIColor& getFillColor() const { return fillColor_; }
    
    // Direction
    enum class Direction { LeftToRight, RightToLeft, BottomToTop, TopToBottom };
    void setDirection(Direction dir) { direction_ = dir; }
    
    // Show percentage text
    void setShowText(bool show) { showText_ = show; }
    bool getShowText() const { return showText_; }
    
    // Animated fill
    void setAnimated(bool animated) { animated_ = animated; }
    void setAnimationSpeed(float speed) { animationSpeed_ = speed; }
    
    void update(float dt) override {
        if (animated_) {
            float target = value_;
            displayValue_ += (target - displayValue_) * animationSpeed_ * dt;
        } else {
            displayValue_ = value_;
        }
        UIWidget::update(dt);
    }
    
    float getDisplayValue() const { return displayValue_; }
    
private:
    float value_ = 0.0f;
    float displayValue_ = 0.0f;
    
    UIColor backgroundColor_ = UIColor(0.2f, 0.2f, 0.2f, 1.0f);
    UIColor fillColor_ = UIColor(0.3f, 0.7f, 0.3f, 1.0f);
    
    Direction direction_ = Direction::LeftToRight;
    bool showText_ = false;
    bool animated_ = true;
    float animationSpeed_ = 5.0f;
};

// ===== UI Input Field =====
class UIInputField : public UIWidget {
public:
    UIInputField(const std::string& name = "InputField") : UIWidget(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::InputField; }
    
    // Text
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) {
        text_ = text;
        cursorPosition_ = text_.length();
        if (onTextChanged_) onTextChanged_(text_);
    }
    
    // Placeholder
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    const std::string& getPlaceholder() const { return placeholder_; }
    
    // Character limit
    void setMaxLength(int max) { maxLength_ = max; }
    int getMaxLength() const { return maxLength_; }
    
    // Input type
    enum class InputType { Standard, Password, Number, Email };
    void setInputType(InputType type) { inputType_ = type; }
    InputType getInputType() const { return inputType_; }
    
    // Password char
    void setPasswordChar(char c) { passwordChar_ = c; }
    
    // Read only
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool isReadOnly() const { return readOnly_; }
    
    // Selection
    int getCursorPosition() const { return cursorPosition_; }
    void setCursorPosition(int pos) { cursorPosition_ = std::max(0, std::min((int)text_.length(), pos)); }
    
    // Display text (masked for password)
    std::string getDisplayText() const {
        if (inputType_ == InputType::Password) {
            return std::string(text_.length(), passwordChar_);
        }
        return text_;
    }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (readOnly_) return;
        
        if (event.type == UIEventType::TextInput) {
            if (maxLength_ < 0 || (int)text_.length() < maxLength_) {
                // Validate character for input type
                bool valid = true;
                if (inputType_ == InputType::Number) {
                    valid = (event.character >= '0' && event.character <= '9') ||
                            event.character == '.' || event.character == '-';
                }
                
                if (valid) {
                    text_.insert(cursorPosition_, 1, event.character);
                    cursorPosition_++;
                    if (onTextChanged_) onTextChanged_(text_);
                }
            }
        }
        else if (event.type == UIEventType::KeyDown) {
            if (event.keyCode == 8) {  // Backspace
                if (cursorPosition_ > 0) {
                    text_.erase(cursorPosition_ - 1, 1);
                    cursorPosition_--;
                    if (onTextChanged_) onTextChanged_(text_);
                }
            }
            else if (event.keyCode == 127) {  // Delete
                if (cursorPosition_ < (int)text_.length()) {
                    text_.erase(cursorPosition_, 1);
                    if (onTextChanged_) onTextChanged_(text_);
                }
            }
            else if (event.keyCode == 37) {  // Left arrow
                cursorPosition_ = std::max(0, cursorPosition_ - 1);
            }
            else if (event.keyCode == 39) {  // Right arrow
                cursorPosition_ = std::min((int)text_.length(), cursorPosition_ + 1);
            }
        }
    }
    
private:
    std::string text_;
    std::string placeholder_ = "Enter text...";
    int maxLength_ = -1;
    InputType inputType_ = InputType::Standard;
    char passwordChar_ = '*';
    bool readOnly_ = false;
    int cursorPosition_ = 0;
};

// ===== UI Dropdown =====
class UIDropdown : public UIWidget {
public:
    UIDropdown(const std::string& name = "Dropdown") : UIWidget(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::Dropdown; }
    
    // Options
    void addOption(const std::string& option) { options_.push_back(option); }
    void clearOptions() { options_.clear(); selectedIndex_ = -1; }
    
    const std::vector<std::string>& getOptions() const { return options_; }
    
    // Selection
    int getSelectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index) {
        if (index >= -1 && index < (int)options_.size()) {
            selectedIndex_ = index;
            if (onValueChanged_) onValueChanged_((float)index);
        }
    }
    
    std::string getSelectedOption() const {
        return (selectedIndex_ >= 0 && selectedIndex_ < (int)options_.size())
            ? options_[selectedIndex_] : "";
    }
    
    // Expanded state
    bool isExpanded() const { return expanded_; }
    void setExpanded(bool expanded) { expanded_ = expanded; }
    void toggleExpanded() { expanded_ = !expanded_; }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (event.type == UIEventType::Click) {
            toggleExpanded();
        }
    }
    
private:
    std::vector<std::string> options_;
    int selectedIndex_ = -1;
    bool expanded_ = false;
};

// ===== UI Scroll View =====
class UIScrollView : public UIWidget {
public:
    UIScrollView(const std::string& name = "ScrollView") : UIWidget(name) {
        content_ = std::make_shared<UIWidget>("Content");
        content_->setInteractive(false);
        UIWidget::addChild(content_);
    }
    
    UIWidgetType getType() const override { return UIWidgetType::ScrollView; }
    
    // Content
    UIWidget* getContent() { return content_.get(); }
    
    void addContentChild(std::shared_ptr<UIWidget> widget) {
        content_->addChild(widget);
    }
    
    // Scroll position
    float getScrollX() const { return scrollX_; }
    float getScrollY() const { return scrollY_; }
    
    void setScroll(float x, float y) {
        scrollX_ = std::max(0.0f, std::min(getMaxScrollX(), x));
        scrollY_ = std::max(0.0f, std::min(getMaxScrollY(), y));
        updateContentPosition();
    }
    
    void scrollBy(float dx, float dy) {
        setScroll(scrollX_ + dx, scrollY_ + dy);
    }
    
    // Content size
    void setContentSize(float width, float height) {
        contentWidth_ = width;
        contentHeight_ = height;
        content_->setSize(width, height);
    }
    
    float getMaxScrollX() const { return std::max(0.0f, contentWidth_ - getWidth()); }
    float getMaxScrollY() const { return std::max(0.0f, contentHeight_ - getHeight()); }
    
    // Scrollbars
    void setHorizontalScrollbar(bool enabled) { horizontalScrollbar_ = enabled; }
    void setVerticalScrollbar(bool enabled) { verticalScrollbar_ = enabled; }
    
    // Inertia
    void setInertia(bool enabled) { inertiaEnabled_ = enabled; }
    void setDecelerationRate(float rate) { decelerationRate_ = rate; }
    
    void update(float dt) override {
        if (inertiaEnabled_) {
            if (std::abs(velocityX_) > 0.1f || std::abs(velocityY_) > 0.1f) {
                scrollBy(velocityX_ * dt, velocityY_ * dt);
                velocityX_ *= (1.0f - decelerationRate_ * dt);
                velocityY_ *= (1.0f - decelerationRate_ * dt);
            }
        }
        UIWidget::update(dt);
    }
    
protected:
    void handleEvent(UIEvent& event) override {
        if (event.type == UIEventType::Scroll) {
            scrollBy(-event.scrollX * scrollSpeed_, -event.scrollY * scrollSpeed_);
        }
        else if (event.type == UIEventType::DragStart) {
            isDragging_ = true;
            velocityX_ = velocityY_ = 0;
        }
        else if (event.type == UIEventType::Drag && isDragging_) {
            scrollBy(-event.deltaX, -event.deltaY);
            velocityX_ = -event.deltaX / 0.016f;
            velocityY_ = -event.deltaY / 0.016f;
        }
        else if (event.type == UIEventType::DragEnd) {
            isDragging_ = false;
        }
    }
    
private:
    void updateContentPosition() {
        content_->setPosition(-scrollX_, -scrollY_);
    }
    
    std::shared_ptr<UIWidget> content_;
    
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    float contentWidth_ = 0.0f;
    float contentHeight_ = 0.0f;
    
    bool horizontalScrollbar_ = true;
    bool verticalScrollbar_ = true;
    
    bool inertiaEnabled_ = true;
    float decelerationRate_ = 5.0f;
    float scrollSpeed_ = 20.0f;
    
    bool isDragging_ = false;
    float velocityX_ = 0.0f;
    float velocityY_ = 0.0f;
};

// ===== UI List View =====
class UIListView : public UIScrollView {
public:
    UIListView(const std::string& name = "ListView") : UIScrollView(name) {}
    
    UIWidgetType getType() const override { return UIWidgetType::ListView; }
    
    // Item template
    using ItemCreator = std::function<std::shared_ptr<UIWidget>(int index, const std::string& data)>;
    void setItemCreator(ItemCreator creator) { itemCreator_ = creator; }
    
    // Data
    void setItems(const std::vector<std::string>& items) {
        items_ = items;
        rebuildList();
    }
    
    void addItem(const std::string& item) {
        items_.push_back(item);
        rebuildList();
    }
    
    void removeItem(int index) {
        if (index >= 0 && index < (int)items_.size()) {
            items_.erase(items_.begin() + index);
            rebuildList();
        }
    }
    
    // Item height
    void setItemHeight(float height) { itemHeight_ = height; rebuildList(); }
    float getItemHeight() const { return itemHeight_; }
    
    // Selection
    int getSelectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index) {
        selectedIndex_ = index;
        if (onValueChanged_) onValueChanged_((float)index);
    }
    
    // Spacing
    void setSpacing(float spacing) { spacing_ = spacing; rebuildList(); }
    
private:
    void rebuildList() {
        // Clear content children (need non-const access)
        auto* content = getContent();
        if (content) {
            while (!content->getChildren().empty()) {
                content->removeChild(content->getChildren().front().get());
            }
        }
        
        float totalHeight = items_.size() * (itemHeight_ + spacing_) - spacing_;
        setContentSize(getWidth(), totalHeight);
        
        for (size_t i = 0; i < items_.size(); i++) {
            std::shared_ptr<UIWidget> item;
            
            if (itemCreator_) {
                item = itemCreator_((int)i, items_[i]);
            } else {
                auto label = std::make_shared<UILabel>(items_[i]);
                label->setSize(getWidth(), itemHeight_);
                item = label;
            }
            
            item->setPosition(0, i * (itemHeight_ + spacing_));
            item->setSize(getWidth(), itemHeight_);
            
            // Selection handling
            int index = (int)i;
            item->setOnClick([this, index](UIEvent&) {
                setSelectedIndex(index);
            });
            
            addContentChild(item);
        }
    }
    
    std::vector<std::string> items_;
    ItemCreator itemCreator_;
    
    float itemHeight_ = 30.0f;
    float spacing_ = 2.0f;
    int selectedIndex_ = -1;
};

}  // namespace ui
}  // namespace luma
