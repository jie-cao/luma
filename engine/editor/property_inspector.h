// Property Inspector - Generic object property editing
// Reflection-based property display and modification
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <any>

namespace luma {

// ============================================================================
// Property Types
// ============================================================================

enum class PropertyType {
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Color3,
    Color4,
    Enum,
    Flags,
    Asset,       // Asset reference
    Object,      // Nested object
    Array,       // Array of properties
    Custom       // Custom widget
};

// ============================================================================
// Property Metadata
// ============================================================================

struct PropertyMeta {
    std::string name;
    std::string displayName;
    std::string tooltip;
    std::string category;
    
    PropertyType type = PropertyType::Float;
    
    // Constraints
    bool readOnly = false;
    bool hidden = false;
    
    // For numeric types
    float minValue = -1e10f;
    float maxValue = 1e10f;
    float step = 0.1f;
    bool slider = false;
    bool logarithmic = false;
    
    // For enums
    std::vector<std::string> enumValues;
    std::vector<std::string> enumDisplayNames;
    
    // For arrays
    int minArraySize = 0;
    int maxArraySize = 100;
    bool fixedArraySize = false;
    
    // For assets
    std::string assetType;  // "Texture", "Model", etc.
    
    // Grouping
    int order = 0;
    bool collapsedByDefault = false;
    
    // Validation
    std::function<bool(const std::any&)> validator;
    
    // Custom widget
    std::string customWidgetType;
};

// ============================================================================
// Property Value
// ============================================================================

struct PropertyValue {
    PropertyType type = PropertyType::Float;
    std::any value;
    
    // Getters
    bool getBool() const { return std::any_cast<bool>(value); }
    int getInt() const { return std::any_cast<int>(value); }
    float getFloat() const { return std::any_cast<float>(value); }
    std::string getString() const { return std::any_cast<std::string>(value); }
    Vec2 getVec2() const { return std::any_cast<Vec2>(value); }
    Vec3 getVec3() const { return std::any_cast<Vec3>(value); }
    Vec4 getVec4() const { return std::any_cast<Vec4>(value); }
    Quat getQuat() const { return std::any_cast<Quat>(value); }
    
    // Setters
    void setBool(bool v) { type = PropertyType::Bool; value = v; }
    void setInt(int v) { type = PropertyType::Int; value = v; }
    void setFloat(float v) { type = PropertyType::Float; value = v; }
    void setString(const std::string& v) { type = PropertyType::String; value = v; }
    void setVec2(const Vec2& v) { type = PropertyType::Vec2; value = v; }
    void setVec3(const Vec3& v) { type = PropertyType::Vec3; value = v; }
    void setVec4(const Vec4& v) { type = PropertyType::Vec4; value = v; }
    void setQuat(const Quat& v) { type = PropertyType::Quat; value = v; }
    void setColor3(const Vec3& v) { type = PropertyType::Color3; value = v; }
    void setColor4(const Vec4& v) { type = PropertyType::Color4; value = v; }
};

// ============================================================================
// Property Definition
// ============================================================================

struct PropertyDef {
    PropertyMeta meta;
    
    // Getter/setter
    std::function<PropertyValue()> getter;
    std::function<void(const PropertyValue&)> setter;
    
    // Children (for nested objects)
    std::vector<PropertyDef> children;
};

// ============================================================================
// Inspectable Interface
// ============================================================================

class IInspectable {
public:
    virtual ~IInspectable() = default;
    
    // Get all properties for this object
    virtual std::vector<PropertyDef> getProperties() = 0;
    
    // Get display name
    virtual std::string getDisplayName() const = 0;
    
    // Get type name
    virtual std::string getTypeName() const = 0;
    
    // Get icon
    virtual std::string getIcon() const { return ""; }
};

// ============================================================================
// Property Group
// ============================================================================

struct PropertyGroup {
    std::string name;
    std::string displayName;
    bool expanded = true;
    int order = 0;
    
    std::vector<PropertyDef*> properties;
};

// ============================================================================
// Inspector State
// ============================================================================

struct InspectorState {
    // Current target
    IInspectable* target = nullptr;
    std::string targetId;
    
    // Display settings
    bool showCategories = true;
    bool showAdvanced = false;
    bool showReadOnly = true;
    
    // Search
    std::string searchQuery;
    
    // Expanded groups
    std::unordered_map<std::string, bool> expandedGroups;
    
    // Modified properties (for highlighting)
    std::vector<std::string> modifiedProperties;
    
    // Lock to prevent target change
    bool locked = false;
};

// ============================================================================
// Property Inspector
// ============================================================================

class PropertyInspector {
public:
    static PropertyInspector& getInstance() {
        static PropertyInspector instance;
        return instance;
    }
    
    void initialize() {
        initialized_ = true;
    }
    
    // Set inspection target
    void setTarget(IInspectable* target) {
        if (state_.locked) return;
        
        state_.target = target;
        if (target) {
            state_.targetId = target->getTypeName() + "_" + target->getDisplayName();
            properties_ = target->getProperties();
            buildGroups();
        } else {
            state_.targetId = "";
            properties_.clear();
            groups_.clear();
        }
        
        if (onTargetChanged_) {
            onTargetChanged_(target);
        }
    }
    
    IInspectable* getTarget() const { return state_.target; }
    
    // Get organized properties
    const std::vector<PropertyGroup>& getGroups() const { return groups_; }
    const std::vector<PropertyDef>& getProperties() const { return properties_; }
    
    // Get filtered properties
    std::vector<PropertyDef*> getFilteredProperties() {
        std::vector<PropertyDef*> result;
        
        std::string lowerQuery = state_.searchQuery;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        for (auto& prop : properties_) {
            if (prop.meta.hidden && !state_.showAdvanced) continue;
            if (prop.meta.readOnly && !state_.showReadOnly) continue;
            
            if (!state_.searchQuery.empty()) {
                std::string lowerName = prop.meta.displayName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                
                if (lowerName.find(lowerQuery) == std::string::npos) {
                    continue;
                }
            }
            
            result.push_back(&prop);
        }
        
        return result;
    }
    
    // Modify property
    void setPropertyValue(const std::string& name, const PropertyValue& value) {
        for (auto& prop : properties_) {
            if (prop.meta.name == name && prop.setter) {
                // Validate if validator exists
                if (prop.meta.validator && !prop.meta.validator(value.value)) {
                    return;
                }
                
                prop.setter(value);
                
                // Mark as modified
                if (std::find(state_.modifiedProperties.begin(), 
                              state_.modifiedProperties.end(), name) 
                    == state_.modifiedProperties.end()) {
                    state_.modifiedProperties.push_back(name);
                }
                
                if (onPropertyChanged_) {
                    onPropertyChanged_(name, value);
                }
                
                return;
            }
        }
    }
    
    // Get property value
    PropertyValue getPropertyValue(const std::string& name) const {
        for (const auto& prop : properties_) {
            if (prop.meta.name == name && prop.getter) {
                return prop.getter();
            }
        }
        return PropertyValue{};
    }
    
    // Reset modified tracking
    void clearModified() {
        state_.modifiedProperties.clear();
    }
    
    bool isModified(const std::string& name) const {
        return std::find(state_.modifiedProperties.begin(),
                        state_.modifiedProperties.end(), name) 
               != state_.modifiedProperties.end();
    }
    
    // Group expansion
    void setGroupExpanded(const std::string& name, bool expanded) {
        state_.expandedGroups[name] = expanded;
    }
    
    bool isGroupExpanded(const std::string& name) const {
        auto it = state_.expandedGroups.find(name);
        return (it != state_.expandedGroups.end()) ? it->second : true;
    }
    
    void expandAll() {
        for (auto& group : groups_) {
            state_.expandedGroups[group.name] = true;
        }
    }
    
    void collapseAll() {
        for (auto& group : groups_) {
            state_.expandedGroups[group.name] = false;
        }
    }
    
    // State
    InspectorState& getState() { return state_; }
    const InspectorState& getState() const { return state_; }
    
    // Callbacks
    void setOnPropertyChanged(std::function<void(const std::string&, const PropertyValue&)> callback) {
        onPropertyChanged_ = callback;
    }
    
    void setOnTargetChanged(std::function<void(IInspectable*)> callback) {
        onTargetChanged_ = callback;
    }
    
    // Lock/unlock
    void lock() { state_.locked = true; }
    void unlock() { state_.locked = false; }
    bool isLocked() const { return state_.locked; }
    
private:
    PropertyInspector() = default;
    
    void buildGroups() {
        groups_.clear();
        
        std::unordered_map<std::string, PropertyGroup> groupMap;
        
        for (auto& prop : properties_) {
            std::string category = prop.meta.category.empty() ? "General" : prop.meta.category;
            
            if (groupMap.find(category) == groupMap.end()) {
                PropertyGroup group;
                group.name = category;
                group.displayName = category;
                group.expanded = !prop.meta.collapsedByDefault;
                groupMap[category] = group;
            }
            
            groupMap[category].properties.push_back(&prop);
        }
        
        // Sort groups and properties
        for (auto& [name, group] : groupMap) {
            std::sort(group.properties.begin(), group.properties.end(),
                     [](PropertyDef* a, PropertyDef* b) {
                         return a->meta.order < b->meta.order;
                     });
            groups_.push_back(std::move(group));
        }
        
        std::sort(groups_.begin(), groups_.end(),
                 [](const PropertyGroup& a, const PropertyGroup& b) {
                     return a.order < b.order;
                 });
    }
    
    InspectorState state_;
    std::vector<PropertyDef> properties_;
    std::vector<PropertyGroup> groups_;
    
    bool initialized_ = false;
    
    std::function<void(const std::string&, const PropertyValue&)> onPropertyChanged_;
    std::function<void(IInspectable*)> onTargetChanged_;
};

inline PropertyInspector& getPropertyInspector() {
    return PropertyInspector::getInstance();
}

// ============================================================================
// Property Builder - Helper for creating properties
// ============================================================================

class PropertyBuilder {
public:
    PropertyBuilder& name(const std::string& n) { def_.meta.name = n; return *this; }
    PropertyBuilder& displayName(const std::string& n) { def_.meta.displayName = n; return *this; }
    PropertyBuilder& tooltip(const std::string& t) { def_.meta.tooltip = t; return *this; }
    PropertyBuilder& category(const std::string& c) { def_.meta.category = c; return *this; }
    PropertyBuilder& type(PropertyType t) { def_.meta.type = t; return *this; }
    PropertyBuilder& readOnly(bool r = true) { def_.meta.readOnly = r; return *this; }
    PropertyBuilder& hidden(bool h = true) { def_.meta.hidden = h; return *this; }
    PropertyBuilder& range(float min, float max) { 
        def_.meta.minValue = min; 
        def_.meta.maxValue = max; 
        return *this; 
    }
    PropertyBuilder& step(float s) { def_.meta.step = s; return *this; }
    PropertyBuilder& slider(bool s = true) { def_.meta.slider = s; return *this; }
    PropertyBuilder& enumValues(const std::vector<std::string>& v) { 
        def_.meta.enumValues = v; 
        def_.meta.type = PropertyType::Enum;
        return *this; 
    }
    PropertyBuilder& order(int o) { def_.meta.order = o; return *this; }
    
    PropertyBuilder& getter(std::function<PropertyValue()> g) { def_.getter = g; return *this; }
    PropertyBuilder& setter(std::function<void(const PropertyValue&)> s) { def_.setter = s; return *this; }
    
    PropertyDef build() { return def_; }
    
private:
    PropertyDef def_;
};

// Convenience function
inline PropertyBuilder property() {
    return PropertyBuilder();
}

}  // namespace luma
