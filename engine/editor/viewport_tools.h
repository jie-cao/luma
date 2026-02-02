// Viewport Tools - Measurement, annotation, and reference images
// Tools for precise 3D editing and visualization
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace luma {

// ============================================================================
// Measurement Tool
// ============================================================================

struct Measurement {
    std::string id;
    std::string name;
    
    enum class Type { Distance, Angle, Area, Volume };
    Type type = Type::Distance;
    
    // Points
    std::vector<Vec3> points;
    
    // Result
    float value = 0;
    std::string unit = "m";
    std::string displayValue;
    
    // Display
    bool visible = true;
    Vec3 color{1, 0.8f, 0};
    float lineWidth = 2.0f;
    bool showLabel = true;
    Vec3 labelPosition{0, 0, 0};
    
    // Calculate
    void calculate() {
        switch (type) {
            case Type::Distance:
                if (points.size() >= 2) {
                    value = (points[1] - points[0]).length();
                    displayValue = formatDistance(value);
                }
                break;
                
            case Type::Angle:
                if (points.size() >= 3) {
                    Vec3 v1 = (points[0] - points[1]).normalized();
                    Vec3 v2 = (points[2] - points[1]).normalized();
                    float dot = v1.dot(v2);
                    value = std::acos(std::clamp(dot, -1.0f, 1.0f)) * 180.0f / 3.14159f;
                    displayValue = std::to_string(static_cast<int>(value)) + "°";
                }
                break;
                
            case Type::Area:
                if (points.size() >= 3) {
                    // Calculate polygon area
                    value = calculatePolygonArea(points);
                    displayValue = formatArea(value);
                }
                break;
                
            case Type::Volume:
                // Simplified - bounding box volume
                if (points.size() >= 2) {
                    Vec3 size = points[1] - points[0];
                    value = std::abs(size.x * size.y * size.z);
                    displayValue = formatVolume(value);
                }
                break;
        }
        
        // Calculate label position (center of points)
        if (!points.empty()) {
            labelPosition = Vec3(0, 0, 0);
            for (const auto& p : points) {
                labelPosition = labelPosition + p;
            }
            labelPosition = labelPosition * (1.0f / points.size());
            labelPosition.y += 0.2f;  // Offset above
        }
    }
    
private:
    static std::string formatDistance(float d) {
        if (d < 0.01f) return std::to_string(static_cast<int>(d * 1000)) + " mm";
        if (d < 1.0f) return std::to_string(static_cast<int>(d * 100)) + " cm";
        return std::to_string(d).substr(0, 4) + " m";
    }
    
    static std::string formatArea(float a) {
        if (a < 0.01f) return std::to_string(static_cast<int>(a * 10000)) + " cm²";
        return std::to_string(a).substr(0, 4) + " m²";
    }
    
    static std::string formatVolume(float v) {
        if (v < 0.001f) return std::to_string(static_cast<int>(v * 1000000)) + " cm³";
        return std::to_string(v).substr(0, 4) + " m³";
    }
    
    static float calculatePolygonArea(const std::vector<Vec3>& points) {
        if (points.size() < 3) return 0;
        
        // Shoelace formula for polygon area (projected to XZ plane)
        float area = 0;
        int n = static_cast<int>(points.size());
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            area += points[i].x * points[j].z;
            area -= points[j].x * points[i].z;
        }
        return std::abs(area) * 0.5f;
    }
};

// ============================================================================
// Annotation
// ============================================================================

struct Annotation {
    std::string id;
    std::string text;
    std::string author;
    std::string timestamp;
    
    enum class Type { Note, Warning, Todo, Question };
    Type type = Type::Note;
    
    // Position
    Vec3 worldPosition{0, 0, 0};
    Vec3 screenOffset{0, 0, 0};  // Offset in screen space
    
    // Attachment
    std::string attachedObjectId;
    bool followObject = true;
    
    // Display
    bool visible = true;
    bool collapsed = false;
    Vec3 color{1, 1, 0.5f};
    float fontSize = 14.0f;
    
    // Interaction
    bool pinned = false;
    bool resolved = false;
    
    static Vec3 getColorForType(Type t) {
        switch (t) {
            case Type::Note: return Vec3(0.9f, 0.9f, 0.5f);
            case Type::Warning: return Vec3(1.0f, 0.6f, 0.2f);
            case Type::Todo: return Vec3(0.5f, 0.8f, 1.0f);
            case Type::Question: return Vec3(0.8f, 0.5f, 1.0f);
            default: return Vec3(0.9f, 0.9f, 0.9f);
        }
    }
    
    static std::string getIconForType(Type t) {
        switch (t) {
            case Type::Note: return "📝";
            case Type::Warning: return "⚠️";
            case Type::Todo: return "✅";
            case Type::Question: return "❓";
            default: return "💬";
        }
    }
};

// ============================================================================
// Reference Image
// ============================================================================

struct ReferenceImage {
    std::string id;
    std::string name;
    std::string path;
    
    // Transform
    Vec3 position{0, 0, 0};
    Quat rotation = Quat::identity();
    Vec2 size{1, 1};
    
    // Display
    float opacity = 0.5f;
    bool visible = true;
    bool locked = false;
    bool flipX = false;
    bool flipY = false;
    
    // Alignment helpers
    enum class Plane { XY, XZ, YZ, Camera };
    Plane plane = Plane::XY;
    
    // For front/side/top reference
    enum class View { Front, Back, Left, Right, Top, Bottom, Custom };
    View view = View::Custom;
    
    // Texture data (loaded)
    int textureWidth = 0;
    int textureHeight = 0;
    bool textureLoaded = false;
    
    // Set standard view
    void setView(View v) {
        view = v;
        switch (v) {
            case View::Front:
                rotation = Quat::identity();
                plane = Plane::XY;
                break;
            case View::Back:
                rotation = Quat::fromAxisAngle(Vec3(0, 1, 0), 3.14159f);
                plane = Plane::XY;
                break;
            case View::Left:
                rotation = Quat::fromAxisAngle(Vec3(0, 1, 0), -1.5708f);
                plane = Plane::YZ;
                break;
            case View::Right:
                rotation = Quat::fromAxisAngle(Vec3(0, 1, 0), 1.5708f);
                plane = Plane::YZ;
                break;
            case View::Top:
                rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), -1.5708f);
                plane = Plane::XZ;
                break;
            case View::Bottom:
                rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), 1.5708f);
                plane = Plane::XZ;
                break;
            default:
                break;
        }
    }
};

// ============================================================================
// Grid Settings
// ============================================================================

struct GridSettings {
    bool visible = true;
    
    // Size
    float size = 20.0f;        // Total grid size
    float cellSize = 1.0f;     // Size of each cell
    int subdivisions = 10;     // Subdivisions per cell
    
    // Colors
    Vec3 majorColor{0.4f, 0.4f, 0.4f};
    Vec3 minorColor{0.25f, 0.25f, 0.25f};
    Vec3 axisColorX{0.8f, 0.2f, 0.2f};
    Vec3 axisColorZ{0.2f, 0.2f, 0.8f};
    
    // Style
    float lineWidth = 1.0f;
    float opacity = 1.0f;
    bool fadeWithDistance = true;
    
    // Snapping
    bool snapToGrid = false;
    float snapSize = 0.1f;
    
    Vec3 snapPosition(const Vec3& pos) const {
        if (!snapToGrid) return pos;
        return Vec3(
            std::round(pos.x / snapSize) * snapSize,
            std::round(pos.y / snapSize) * snapSize,
            std::round(pos.z / snapSize) * snapSize
        );
    }
};

// ============================================================================
// Viewport Tools State
// ============================================================================

struct ViewportToolsState {
    // Active tool
    enum class Tool { 
        None, 
        MeasureDistance, 
        MeasureAngle, 
        MeasureArea,
        Annotate,
        ReferenceImage
    };
    Tool activeTool = Tool::None;
    
    // Measurement in progress
    Measurement currentMeasurement;
    bool isMeasuring = false;
    
    // Annotation in progress
    Annotation currentAnnotation;
    bool isAnnotating = false;
    
    // Selection
    std::string selectedMeasurementId;
    std::string selectedAnnotationId;
    std::string selectedReferenceId;
};

// ============================================================================
// Viewport Tools Manager
// ============================================================================

class ViewportToolsManager {
public:
    static ViewportToolsManager& getInstance() {
        static ViewportToolsManager instance;
        return instance;
    }
    
    void initialize() {
        initialized_ = true;
    }
    
    // === Measurements ===
    
    void startMeasurement(Measurement::Type type) {
        state_.activeTool = ViewportToolsState::Tool::MeasureDistance;
        state_.isMeasuring = true;
        state_.currentMeasurement = Measurement();
        state_.currentMeasurement.id = generateId("measure");
        state_.currentMeasurement.type = type;
    }
    
    void addMeasurementPoint(const Vec3& point) {
        if (!state_.isMeasuring) return;
        
        state_.currentMeasurement.points.push_back(point);
        state_.currentMeasurement.calculate();
        
        // Check if measurement is complete
        bool complete = false;
        switch (state_.currentMeasurement.type) {
            case Measurement::Type::Distance:
                complete = state_.currentMeasurement.points.size() >= 2;
                break;
            case Measurement::Type::Angle:
                complete = state_.currentMeasurement.points.size() >= 3;
                break;
            case Measurement::Type::Area:
            case Measurement::Type::Volume:
                // Require explicit finish
                break;
        }
        
        if (complete) {
            finishMeasurement();
        }
    }
    
    void finishMeasurement() {
        if (!state_.isMeasuring) return;
        
        state_.currentMeasurement.calculate();
        measurements_.push_back(state_.currentMeasurement);
        
        state_.isMeasuring = false;
        state_.activeTool = ViewportToolsState::Tool::None;
        
        if (onMeasurementAdded_) {
            onMeasurementAdded_(state_.currentMeasurement);
        }
    }
    
    void cancelMeasurement() {
        state_.isMeasuring = false;
        state_.currentMeasurement = Measurement();
        state_.activeTool = ViewportToolsState::Tool::None;
    }
    
    void removeMeasurement(const std::string& id) {
        measurements_.erase(
            std::remove_if(measurements_.begin(), measurements_.end(),
                          [&id](const Measurement& m) { return m.id == id; }),
            measurements_.end());
    }
    
    void clearMeasurements() {
        measurements_.clear();
    }
    
    const std::vector<Measurement>& getMeasurements() const { return measurements_; }
    
    // === Annotations ===
    
    void startAnnotation(const Vec3& position, Annotation::Type type = Annotation::Type::Note) {
        state_.activeTool = ViewportToolsState::Tool::Annotate;
        state_.isAnnotating = true;
        state_.currentAnnotation = Annotation();
        state_.currentAnnotation.id = generateId("note");
        state_.currentAnnotation.worldPosition = position;
        state_.currentAnnotation.type = type;
        state_.currentAnnotation.color = Annotation::getColorForType(type);
    }
    
    void finishAnnotation(const std::string& text) {
        if (!state_.isAnnotating) return;
        
        state_.currentAnnotation.text = text;
        annotations_.push_back(state_.currentAnnotation);
        
        state_.isAnnotating = false;
        state_.activeTool = ViewportToolsState::Tool::None;
        
        if (onAnnotationAdded_) {
            onAnnotationAdded_(state_.currentAnnotation);
        }
    }
    
    void cancelAnnotation() {
        state_.isAnnotating = false;
        state_.currentAnnotation = Annotation();
        state_.activeTool = ViewportToolsState::Tool::None;
    }
    
    void updateAnnotation(const std::string& id, const std::string& text) {
        for (auto& ann : annotations_) {
            if (ann.id == id) {
                ann.text = text;
                return;
            }
        }
    }
    
    void removeAnnotation(const std::string& id) {
        annotations_.erase(
            std::remove_if(annotations_.begin(), annotations_.end(),
                          [&id](const Annotation& a) { return a.id == id; }),
            annotations_.end());
    }
    
    void clearAnnotations() {
        annotations_.clear();
    }
    
    const std::vector<Annotation>& getAnnotations() const { return annotations_; }
    
    // === Reference Images ===
    
    void addReferenceImage(const std::string& path) {
        ReferenceImage ref;
        ref.id = generateId("ref");
        ref.path = path;
        
        // Extract name from path
        size_t pos = path.find_last_of("/\\");
        ref.name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        
        references_.push_back(ref);
        
        if (onReferenceAdded_) {
            onReferenceAdded_(ref);
        }
    }
    
    void removeReferenceImage(const std::string& id) {
        references_.erase(
            std::remove_if(references_.begin(), references_.end(),
                          [&id](const ReferenceImage& r) { return r.id == id; }),
            references_.end());
    }
    
    ReferenceImage* getReferenceImage(const std::string& id) {
        for (auto& ref : references_) {
            if (ref.id == id) return &ref;
        }
        return nullptr;
    }
    
    const std::vector<ReferenceImage>& getReferenceImages() const { return references_; }
    
    // === Grid ===
    
    GridSettings& getGridSettings() { return grid_; }
    const GridSettings& getGridSettings() const { return grid_; }
    
    void setGridVisible(bool visible) { grid_.visible = visible; }
    void setSnapToGrid(bool snap) { grid_.snapToGrid = snap; }
    void setGridSize(float size) { grid_.cellSize = size; }
    
    // === State ===
    
    ViewportToolsState& getState() { return state_; }
    const ViewportToolsState& getState() const { return state_; }
    
    void setActiveTool(ViewportToolsState::Tool tool) {
        state_.activeTool = tool;
    }
    
    // === Callbacks ===
    
    void setOnMeasurementAdded(std::function<void(const Measurement&)> callback) {
        onMeasurementAdded_ = callback;
    }
    
    void setOnAnnotationAdded(std::function<void(const Annotation&)> callback) {
        onAnnotationAdded_ = callback;
    }
    
    void setOnReferenceAdded(std::function<void(const ReferenceImage&)> callback) {
        onReferenceAdded_ = callback;
    }
    
private:
    ViewportToolsManager() = default;
    
    std::string generateId(const std::string& prefix) {
        return prefix + "_" + std::to_string(nextId_++);
    }
    
    ViewportToolsState state_;
    GridSettings grid_;
    
    std::vector<Measurement> measurements_;
    std::vector<Annotation> annotations_;
    std::vector<ReferenceImage> references_;
    
    int nextId_ = 1;
    bool initialized_ = false;
    
    std::function<void(const Measurement&)> onMeasurementAdded_;
    std::function<void(const Annotation&)> onAnnotationAdded_;
    std::function<void(const ReferenceImage&)> onReferenceAdded_;
};

inline ViewportToolsManager& getViewportTools() {
    return ViewportToolsManager::getInstance();
}

}  // namespace luma
