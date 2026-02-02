// BlendShape System - Morph target animation for character customization
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t MAX_BLEND_SHAPES = 256;        // Maximum number of blend shapes per mesh
constexpr uint32_t MAX_ACTIVE_BLEND_SHAPES = 64;  // Maximum active at once (for GPU)

// ============================================================================
// BlendShape Delta - Vertex offset data
// ============================================================================

struct BlendShapeDelta {
    uint32_t vertexIndex;       // Index of the affected vertex
    Vec3 positionDelta;         // Position offset
    Vec3 normalDelta;           // Normal offset
    Vec3 tangentDelta;          // Tangent offset (optional, for normal mapping)
    
    BlendShapeDelta() 
        : vertexIndex(0)
        , positionDelta(0, 0, 0)
        , normalDelta(0, 0, 0)
        , tangentDelta(0, 0, 0) {}
        
    BlendShapeDelta(uint32_t idx, const Vec3& pos, const Vec3& nor = Vec3(0,0,0), const Vec3& tan = Vec3(0,0,0))
        : vertexIndex(idx)
        , positionDelta(pos)
        , normalDelta(nor)
        , tangentDelta(tan) {}
};

// ============================================================================
// BlendShape Target - A single morph target (e.g., "smile", "eyeWide")
// ============================================================================

struct BlendShapeTarget {
    std::string name;                           // Unique name for this target
    std::vector<BlendShapeDelta> deltas;        // Sparse delta data
    
    // Bounds for quick culling (in local space)
    Vec3 boundsMin{FLT_MAX, FLT_MAX, FLT_MAX};
    Vec3 boundsMax{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    // Category for UI organization
    std::string category;                       // e.g., "eyes", "mouth", "nose"
    
    BlendShapeTarget() = default;
    explicit BlendShapeTarget(const std::string& n) : name(n) {}
    
    // Add a delta and update bounds
    void addDelta(const BlendShapeDelta& delta) {
        deltas.push_back(delta);
        
        // Update bounds
        boundsMin.x = std::min(boundsMin.x, delta.positionDelta.x);
        boundsMin.y = std::min(boundsMin.y, delta.positionDelta.y);
        boundsMin.z = std::min(boundsMin.z, delta.positionDelta.z);
        boundsMax.x = std::max(boundsMax.x, delta.positionDelta.x);
        boundsMax.y = std::max(boundsMax.y, delta.positionDelta.y);
        boundsMax.z = std::max(boundsMax.z, delta.positionDelta.z);
    }
    
    // Calculate delta magnitude for importance sorting
    float getMaxMagnitude() const {
        float maxMag = 0.0f;
        for (const auto& d : deltas) {
            float mag = d.positionDelta.length();
            if (mag > maxMag) maxMag = mag;
        }
        return maxMag;
    }
    
    // Get memory usage in bytes
    size_t getMemoryUsage() const {
        return sizeof(BlendShapeTarget) + deltas.size() * sizeof(BlendShapeDelta);
    }
};

// ============================================================================
// BlendShape Channel - Controls multiple targets with a single weight
// ============================================================================

struct BlendShapeChannel {
    std::string name;                           // Channel name (may differ from target)
    float weight = 0.0f;                        // Current weight [0, 1] or [-1, 1] for some
    float minWeight = 0.0f;                     // Minimum allowed weight
    float maxWeight = 1.0f;                     // Maximum allowed weight
    float defaultWeight = 0.0f;                 // Default/neutral weight
    
    // A channel can drive multiple targets (for complex expressions)
    std::vector<uint32_t> targetIndices;        // Indices into BlendShapeMesh::targets
    std::vector<float> targetWeights;           // Weight multipliers for each target
    
    // UI hints
    std::string displayName;                    // Localized display name
    std::string tooltip;                        // UI tooltip
    std::string group;                          // UI grouping (e.g., "Face Shape", "Eyes")
    
    BlendShapeChannel() = default;
    explicit BlendShapeChannel(const std::string& n) : name(n), displayName(n) {}
    
    // Set weight with clamping
    void setWeight(float w) {
        weight = std::clamp(w, minWeight, maxWeight);
    }
    
    // Reset to default
    void reset() {
        weight = defaultWeight;
    }
    
    // Add a target to this channel
    void addTarget(uint32_t targetIdx, float targetWeight = 1.0f) {
        targetIndices.push_back(targetIdx);
        targetWeights.push_back(targetWeight);
    }
};

// ============================================================================
// BlendShape Preset - Named collection of channel weights
// ============================================================================

struct BlendShapePreset {
    std::string name;                           // Preset name (e.g., "Happy", "Angry")
    std::string category;                       // Category (e.g., "Expressions", "Body Types")
    std::unordered_map<std::string, float> weights;  // Channel name -> weight
    
    BlendShapePreset() = default;
    explicit BlendShapePreset(const std::string& n) : name(n) {}
    
    void setWeight(const std::string& channel, float w) {
        weights[channel] = w;
    }
    
    float getWeight(const std::string& channel, float defaultVal = 0.0f) const {
        auto it = weights.find(channel);
        return (it != weights.end()) ? it->second : defaultVal;
    }
};

// ============================================================================
// BlendShape Mesh - Container for all blend shape data
// ============================================================================

class BlendShapeMesh {
public:
    BlendShapeMesh() = default;
    
    // === Target Management ===
    
    // Add a target, returns target index
    int addTarget(const BlendShapeTarget& target) {
        if (targets_.size() >= MAX_BLEND_SHAPES) return -1;
        
        int idx = static_cast<int>(targets_.size());
        targets_.push_back(target);
        targetNameToIndex_[target.name] = idx;
        dirty_ = true;
        return idx;
    }
    
    // Create and add empty target
    int createTarget(const std::string& name) {
        return addTarget(BlendShapeTarget(name));
    }
    
    // Get target by index
    BlendShapeTarget* getTarget(int index) {
        return (index >= 0 && index < (int)targets_.size()) ? &targets_[index] : nullptr;
    }
    
    const BlendShapeTarget* getTarget(int index) const {
        return (index >= 0 && index < (int)targets_.size()) ? &targets_[index] : nullptr;
    }
    
    // Get target by name
    BlendShapeTarget* getTarget(const std::string& name) {
        auto it = targetNameToIndex_.find(name);
        return (it != targetNameToIndex_.end()) ? &targets_[it->second] : nullptr;
    }
    
    int findTargetIndex(const std::string& name) const {
        auto it = targetNameToIndex_.find(name);
        return (it != targetNameToIndex_.end()) ? it->second : -1;
    }
    
    size_t getTargetCount() const { return targets_.size(); }
    
    // === Channel Management ===
    
    // Add a channel, returns channel index
    int addChannel(const BlendShapeChannel& channel) {
        int idx = static_cast<int>(channels_.size());
        channels_.push_back(channel);
        channelNameToIndex_[channel.name] = idx;
        dirty_ = true;
        return idx;
    }
    
    // Create simple channel (1 target, same name)
    int createChannel(const std::string& name, int targetIndex = -1) {
        BlendShapeChannel ch(name);
        if (targetIndex >= 0) {
            ch.addTarget(targetIndex, 1.0f);
        }
        return addChannel(ch);
    }
    
    // Auto-create channels from targets (convenience)
    void createChannelsFromTargets() {
        for (size_t i = 0; i < targets_.size(); i++) {
            if (channelNameToIndex_.find(targets_[i].name) == channelNameToIndex_.end()) {
                createChannel(targets_[i].name, static_cast<int>(i));
            }
        }
    }
    
    // Get channel by index
    BlendShapeChannel* getChannel(int index) {
        return (index >= 0 && index < (int)channels_.size()) ? &channels_[index] : nullptr;
    }
    
    const BlendShapeChannel* getChannel(int index) const {
        return (index >= 0 && index < (int)channels_.size()) ? &channels_[index] : nullptr;
    }
    
    // Get channel by name
    BlendShapeChannel* getChannel(const std::string& name) {
        auto it = channelNameToIndex_.find(name);
        return (it != channelNameToIndex_.end()) ? &channels_[it->second] : nullptr;
    }
    
    const BlendShapeChannel* getChannel(const std::string& name) const {
        auto it = channelNameToIndex_.find(name);
        return (it != channelNameToIndex_.end()) ? &channels_[it->second] : nullptr;
    }
    
    int findChannelIndex(const std::string& name) const {
        auto it = channelNameToIndex_.find(name);
        return (it != channelNameToIndex_.end()) ? it->second : -1;
    }
    
    size_t getChannelCount() const { return channels_.size(); }
    
    const std::vector<BlendShapeChannel>& getChannels() const { return channels_; }
    
    // === Weight Control ===
    
    // Set channel weight by name
    bool setWeight(const std::string& channelName, float weight) {
        BlendShapeChannel* ch = getChannel(channelName);
        if (!ch) return false;
        ch->setWeight(weight);
        dirty_ = true;
        return true;
    }
    
    // Set channel weight by index
    bool setWeight(int channelIndex, float weight) {
        BlendShapeChannel* ch = getChannel(channelIndex);
        if (!ch) return false;
        ch->setWeight(weight);
        dirty_ = true;
        return true;
    }
    
    // Get channel weight
    float getWeight(const std::string& channelName) const {
        const BlendShapeChannel* ch = getChannel(channelName);
        return ch ? ch->weight : 0.0f;
    }
    
    float getWeight(int channelIndex) const {
        const BlendShapeChannel* ch = getChannel(channelIndex);
        return ch ? ch->weight : 0.0f;
    }
    
    // Reset all weights to defaults
    void resetAllWeights() {
        for (auto& ch : channels_) {
            ch.reset();
        }
        dirty_ = true;
    }
    
    // === Preset Management ===
    
    void addPreset(const BlendShapePreset& preset) {
        presets_.push_back(preset);
        presetNameToIndex_[preset.name] = static_cast<int>(presets_.size()) - 1;
    }
    
    void applyPreset(const std::string& presetName, float blend = 1.0f) {
        auto it = presetNameToIndex_.find(presetName);
        if (it == presetNameToIndex_.end()) return;
        applyPreset(it->second, blend);
    }
    
    void applyPreset(int presetIndex, float blend = 1.0f) {
        if (presetIndex < 0 || presetIndex >= (int)presets_.size()) return;
        
        const BlendShapePreset& preset = presets_[presetIndex];
        for (const auto& [channelName, weight] : preset.weights) {
            BlendShapeChannel* ch = getChannel(channelName);
            if (ch) {
                // Interpolate between current and preset weight
                float newWeight = ch->weight * (1.0f - blend) + weight * blend;
                ch->setWeight(newWeight);
            }
        }
        dirty_ = true;
    }
    
    const std::vector<BlendShapePreset>& getPresets() const { return presets_; }
    
    // === CPU Computation ===
    
    // Apply blend shapes to base mesh (CPU fallback)
    void applyToMesh(const std::vector<Vertex>& baseVertices,
                     std::vector<Vertex>& outVertices) const {
        outVertices = baseVertices;  // Start with base
        
        // Compute combined target weights
        std::unordered_map<int, float> targetWeights;
        for (const auto& channel : channels_) {
            if (std::abs(channel.weight) < 0.001f) continue;
            
            for (size_t i = 0; i < channel.targetIndices.size(); i++) {
                int targetIdx = channel.targetIndices[i];
                float weight = channel.weight * channel.targetWeights[i];
                targetWeights[targetIdx] += weight;
            }
        }
        
        // Apply weighted deltas
        for (const auto& [targetIdx, weight] : targetWeights) {
            if (std::abs(weight) < 0.001f) continue;
            
            const BlendShapeTarget& target = targets_[targetIdx];
            for (const auto& delta : target.deltas) {
                if (delta.vertexIndex >= outVertices.size()) continue;
                
                Vertex& v = outVertices[delta.vertexIndex];
                v.position[0] += delta.positionDelta.x * weight;
                v.position[1] += delta.positionDelta.y * weight;
                v.position[2] += delta.positionDelta.z * weight;
                
                v.normal[0] += delta.normalDelta.x * weight;
                v.normal[1] += delta.normalDelta.y * weight;
                v.normal[2] += delta.normalDelta.z * weight;
                
                // Renormalize normal
                float len = std::sqrt(v.normal[0]*v.normal[0] + 
                                     v.normal[1]*v.normal[1] + 
                                     v.normal[2]*v.normal[2]);
                if (len > 0.0001f) {
                    v.normal[0] /= len;
                    v.normal[1] /= len;
                    v.normal[2] /= len;
                }
            }
        }
    }
    
    // Get active blend shape data for GPU upload
    // Returns: list of (targetIndex, weight) pairs
    std::vector<std::pair<int, float>> getActiveTargetWeights() const {
        std::unordered_map<int, float> combined;
        
        for (const auto& channel : channels_) {
            if (std::abs(channel.weight) < 0.001f) continue;
            
            for (size_t i = 0; i < channel.targetIndices.size(); i++) {
                int targetIdx = channel.targetIndices[i];
                float weight = channel.weight * channel.targetWeights[i];
                combined[targetIdx] += weight;
            }
        }
        
        // Sort by absolute weight (most important first)
        std::vector<std::pair<int, float>> result;
        for (const auto& [idx, weight] : combined) {
            if (std::abs(weight) >= 0.001f) {
                result.push_back({idx, weight});
            }
        }
        
        std::sort(result.begin(), result.end(), 
            [](const auto& a, const auto& b) {
                return std::abs(a.second) > std::abs(b.second);
            });
        
        // Limit to max active
        if (result.size() > MAX_ACTIVE_BLEND_SHAPES) {
            result.resize(MAX_ACTIVE_BLEND_SHAPES);
        }
        
        return result;
    }
    
    // === State ===
    
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    void markDirty() { dirty_ = true; }
    
    // === Memory ===
    
    size_t getTotalMemoryUsage() const {
        size_t total = sizeof(BlendShapeMesh);
        for (const auto& t : targets_) {
            total += t.getMemoryUsage();
        }
        total += channels_.size() * sizeof(BlendShapeChannel);
        total += presets_.size() * sizeof(BlendShapePreset);
        return total;
    }
    
private:
    std::vector<BlendShapeTarget> targets_;
    std::vector<BlendShapeChannel> channels_;
    std::vector<BlendShapePreset> presets_;
    
    std::unordered_map<std::string, int> targetNameToIndex_;
    std::unordered_map<std::string, int> channelNameToIndex_;
    std::unordered_map<std::string, int> presetNameToIndex_;
    
    bool dirty_ = true;
};

// ============================================================================
// BlendShape Utilities
// ============================================================================

namespace BlendShapeUtils {

// Create a simple blend shape from two meshes (base and target)
inline BlendShapeTarget createFromMeshDifference(
    const std::string& name,
    const std::vector<Vertex>& baseMesh,
    const std::vector<Vertex>& targetMesh,
    float threshold = 0.0001f)
{
    BlendShapeTarget target(name);
    
    if (baseMesh.size() != targetMesh.size()) {
        return target;  // Mesh sizes must match
    }
    
    for (size_t i = 0; i < baseMesh.size(); i++) {
        Vec3 posDelta(
            targetMesh[i].position[0] - baseMesh[i].position[0],
            targetMesh[i].position[1] - baseMesh[i].position[1],
            targetMesh[i].position[2] - baseMesh[i].position[2]
        );
        
        Vec3 norDelta(
            targetMesh[i].normal[0] - baseMesh[i].normal[0],
            targetMesh[i].normal[1] - baseMesh[i].normal[1],
            targetMesh[i].normal[2] - baseMesh[i].normal[2]
        );
        
        // Only store if delta is significant
        if (posDelta.length() > threshold || norDelta.length() > threshold) {
            target.addDelta(BlendShapeDelta(static_cast<uint32_t>(i), posDelta, norDelta));
        }
    }
    
    return target;
}

// Compress blend shape by removing small deltas
inline void compressTarget(BlendShapeTarget& target, float threshold = 0.0001f) {
    std::vector<BlendShapeDelta> compressed;
    for (const auto& delta : target.deltas) {
        if (delta.positionDelta.length() > threshold) {
            compressed.push_back(delta);
        }
    }
    target.deltas = std::move(compressed);
}

// Combine multiple targets into one (for baking)
inline BlendShapeTarget combineTargets(
    const std::string& name,
    const std::vector<const BlendShapeTarget*>& targets,
    const std::vector<float>& weights)
{
    BlendShapeTarget result(name);
    
    if (targets.size() != weights.size() || targets.empty()) {
        return result;
    }
    
    // Collect all deltas by vertex index
    std::unordered_map<uint32_t, BlendShapeDelta> combined;
    
    for (size_t t = 0; t < targets.size(); t++) {
        float w = weights[t];
        for (const auto& delta : targets[t]->deltas) {
            auto& d = combined[delta.vertexIndex];
            d.vertexIndex = delta.vertexIndex;
            d.positionDelta.x += delta.positionDelta.x * w;
            d.positionDelta.y += delta.positionDelta.y * w;
            d.positionDelta.z += delta.positionDelta.z * w;
            d.normalDelta.x += delta.normalDelta.x * w;
            d.normalDelta.y += delta.normalDelta.y * w;
            d.normalDelta.z += delta.normalDelta.z * w;
        }
    }
    
    for (const auto& [idx, delta] : combined) {
        result.addDelta(delta);
    }
    
    return result;
}

// Create standard face blend shapes categories
inline std::vector<std::string> getStandardFaceCategories() {
    return {
        "face_shape",   // Overall face shape
        "eyes",         // Eye region
        "eyebrows",     // Eyebrow region
        "nose",         // Nose region
        "mouth",        // Mouth region
        "chin",         // Chin and jaw
        "cheeks",       // Cheeks
        "ears",         // Ears
        "expressions"   // Facial expressions
    };
}

// Create standard body blend shapes categories
inline std::vector<std::string> getStandardBodyCategories() {
    return {
        "overall",      // Height, overall scale
        "torso",        // Chest, waist, back
        "arms",         // Shoulder, upper arm, forearm
        "legs",         // Thighs, calves
        "hands",        // Hand size, finger length
        "feet",         // Foot size
        "proportions"   // Limb ratios
    };
}

} // namespace BlendShapeUtils

} // namespace luma
