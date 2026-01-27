// Light Probe System - Indirect diffuse lighting
// Light probes store spherical harmonics for indirect diffuse illumination
#pragma once

#include "spherical_harmonics.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

namespace luma {

// ===== Light Probe =====
class LightProbe {
public:
    LightProbe() : id_(nextId_++) {}
    
    uint32_t getId() const { return id_; }
    
    // Position
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    // SH Coefficients
    void setSHCoefficients(const SHCoefficients& sh) { shCoefficients_ = sh; }
    SHCoefficients& getSHCoefficients() { return shCoefficients_; }
    const SHCoefficients& getSHCoefficients() const { return shCoefficients_; }
    
    // Evaluate irradiance for a normal direction
    Vec3 evaluateIrradiance(const Vec3& normal) const {
        return shCoefficients_.evaluateIrradiance(normal);
    }
    
    // State
    bool isDirty() const { return dirty_; }
    void setDirty(bool dirty) { dirty_ = dirty; }
    
    bool isValid() const { return valid_; }
    void setValid(bool valid) { valid_ = valid; }
    
private:
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
    
    Vec3 position_ = {0, 0, 0};
    SHCoefficients shCoefficients_;
    
    bool dirty_ = true;
    bool valid_ = false;
};

// ===== Light Probe Group =====
// A group of light probes for a specific area
class LightProbeGroup {
public:
    LightProbeGroup(const std::string& name = "LightProbeGroup")
        : name_(name) {}
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Add/remove probes
    LightProbe* addProbe(const Vec3& position) {
        probes_.push_back(std::make_unique<LightProbe>());
        probes_.back()->setPosition(position);
        return probes_.back().get();
    }
    
    void removeProbe(LightProbe* probe) {
        probes_.erase(
            std::remove_if(probes_.begin(), probes_.end(),
                [probe](const auto& p) { return p.get() == probe; }),
            probes_.end()
        );
    }
    
    void clear() { probes_.clear(); }
    
    // Access probes
    const std::vector<std::unique_ptr<LightProbe>>& getProbes() const { return probes_; }
    std::vector<std::unique_ptr<LightProbe>>& getProbes() { return probes_; }
    size_t getProbeCount() const { return probes_.size(); }
    
    // Find nearest probes for interpolation
    struct ProbeWeight {
        LightProbe* probe;
        float weight;
    };
    
    std::vector<ProbeWeight> findNearestProbes(const Vec3& position, int maxCount = 4) const {
        std::vector<std::pair<float, LightProbe*>> distances;
        
        for (const auto& probe : probes_) {
            Vec3 delta = probe->getPosition() - position;
            float dist = delta.length();
            distances.push_back({dist, probe.get()});
        }
        
        // Sort by distance
        std::sort(distances.begin(), distances.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
        
        // Take nearest probes
        std::vector<ProbeWeight> result;
        float totalWeight = 0.0f;
        
        int count = std::min(maxCount, (int)distances.size());
        for (int i = 0; i < count; i++) {
            float dist = distances[i].first;
            float weight = 1.0f / (dist + 0.001f);  // Inverse distance weighting
            result.push_back({distances[i].second, weight});
            totalWeight += weight;
        }
        
        // Normalize weights
        if (totalWeight > 0.0f) {
            for (auto& pw : result) {
                pw.weight /= totalWeight;
            }
        }
        
        return result;
    }
    
    // Interpolate SH at a position
    SHCoefficients interpolateSH(const Vec3& position) const {
        auto nearestProbes = findNearestProbes(position);
        
        SHCoefficients result;
        for (const auto& pw : nearestProbes) {
            SHCoefficients scaled = pw.probe->getSHCoefficients();
            scaled.scale(pw.weight);
            result.add(scaled);
        }
        
        return result;
    }
    
    // Mark all probes as dirty
    void markAllDirty() {
        for (auto& probe : probes_) {
            probe->setDirty(true);
        }
    }
    
    // Bounds
    void getBounds(Vec3& min, Vec3& max) const {
        if (probes_.empty()) {
            min = max = Vec3(0, 0, 0);
            return;
        }
        
        min = max = probes_[0]->getPosition();
        for (const auto& probe : probes_) {
            Vec3 pos = probe->getPosition();
            min.x = std::min(min.x, pos.x);
            min.y = std::min(min.y, pos.y);
            min.z = std::min(min.z, pos.z);
            max.x = std::max(max.x, pos.x);
            max.y = std::max(max.y, pos.y);
            max.z = std::max(max.z, pos.z);
        }
    }
    
private:
    std::string name_;
    std::vector<std::unique_ptr<LightProbe>> probes_;
};

// ===== Light Probe Grid =====
// Regular grid of light probes for efficient lookup
class LightProbeGrid {
public:
    LightProbeGrid() = default;
    
    void initialize(const Vec3& minBounds, const Vec3& maxBounds,
                    int resX, int resY, int resZ)
    {
        minBounds_ = minBounds;
        maxBounds_ = maxBounds;
        resX_ = resX;
        resY_ = resY;
        resZ_ = resZ;
        
        // Create probes
        probes_.clear();
        probes_.resize(resX * resY * resZ);
        
        Vec3 size = maxBounds - minBounds;
        cellSize_.x = size.x / (resX - 1);
        cellSize_.y = size.y / (resY - 1);
        cellSize_.z = size.z / (resZ - 1);
        
        for (int z = 0; z < resZ; z++) {
            for (int y = 0; y < resY; y++) {
                for (int x = 0; x < resX; x++) {
                    int idx = z * resY * resX + y * resX + x;
                    
                    Vec3 pos;
                    pos.x = minBounds.x + x * cellSize_.x;
                    pos.y = minBounds.y + y * cellSize_.y;
                    pos.z = minBounds.z + z * cellSize_.z;
                    
                    probes_[idx].setPosition(pos);
                }
            }
        }
    }
    
    // Get probe at grid index
    LightProbe* getProbe(int x, int y, int z) {
        if (x < 0 || x >= resX_ || y < 0 || y >= resY_ || z < 0 || z >= resZ_) {
            return nullptr;
        }
        return &probes_[z * resY_ * resX_ + y * resX_ + x];
    }
    
    const LightProbe* getProbe(int x, int y, int z) const {
        if (x < 0 || x >= resX_ || y < 0 || y >= resY_ || z < 0 || z >= resZ_) {
            return nullptr;
        }
        return &probes_[z * resY_ * resX_ + y * resX_ + x];
    }
    
    // Get grid cell for a world position
    void getCell(const Vec3& pos, int& x, int& y, int& z) const {
        Vec3 local = pos - minBounds_;
        x = (int)(local.x / cellSize_.x);
        y = (int)(local.y / cellSize_.y);
        z = (int)(local.z / cellSize_.z);
        
        x = std::max(0, std::min(x, resX_ - 1));
        y = std::max(0, std::min(y, resY_ - 1));
        z = std::max(0, std::min(z, resZ_ - 1));
    }
    
    // Trilinear interpolation of SH at a world position
    SHCoefficients sampleSH(const Vec3& position) const {
        // Get local coordinates
        Vec3 local = position - minBounds_;
        float fx = local.x / cellSize_.x;
        float fy = local.y / cellSize_.y;
        float fz = local.z / cellSize_.z;
        
        // Get cell indices
        int x0 = (int)std::floor(fx);
        int y0 = (int)std::floor(fy);
        int z0 = (int)std::floor(fz);
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        int z1 = z0 + 1;
        
        // Clamp
        x0 = std::max(0, std::min(x0, resX_ - 1));
        y0 = std::max(0, std::min(y0, resY_ - 1));
        z0 = std::max(0, std::min(z0, resZ_ - 1));
        x1 = std::max(0, std::min(x1, resX_ - 1));
        y1 = std::max(0, std::min(y1, resY_ - 1));
        z1 = std::max(0, std::min(z1, resZ_ - 1));
        
        // Fractional parts
        float tx = fx - std::floor(fx);
        float ty = fy - std::floor(fy);
        float tz = fz - std::floor(fz);
        
        // Sample 8 corners
        const SHCoefficients& c000 = getProbe(x0, y0, z0)->getSHCoefficients();
        const SHCoefficients& c100 = getProbe(x1, y0, z0)->getSHCoefficients();
        const SHCoefficients& c010 = getProbe(x0, y1, z0)->getSHCoefficients();
        const SHCoefficients& c110 = getProbe(x1, y1, z0)->getSHCoefficients();
        const SHCoefficients& c001 = getProbe(x0, y0, z1)->getSHCoefficients();
        const SHCoefficients& c101 = getProbe(x1, y0, z1)->getSHCoefficients();
        const SHCoefficients& c011 = getProbe(x0, y1, z1)->getSHCoefficients();
        const SHCoefficients& c111 = getProbe(x1, y1, z1)->getSHCoefficients();
        
        // Trilinear interpolation
        SHCoefficients c00 = SHCoefficients::lerp(c000, c100, tx);
        SHCoefficients c10 = SHCoefficients::lerp(c010, c110, tx);
        SHCoefficients c01 = SHCoefficients::lerp(c001, c101, tx);
        SHCoefficients c11 = SHCoefficients::lerp(c011, c111, tx);
        
        SHCoefficients c0 = SHCoefficients::lerp(c00, c10, ty);
        SHCoefficients c1 = SHCoefficients::lerp(c01, c11, ty);
        
        return SHCoefficients::lerp(c0, c1, tz);
    }
    
    // Accessors
    Vec3 getMinBounds() const { return minBounds_; }
    Vec3 getMaxBounds() const { return maxBounds_; }
    Vec3 getCellSize() const { return cellSize_; }
    int getResolutionX() const { return resX_; }
    int getResolutionY() const { return resY_; }
    int getResolutionZ() const { return resZ_; }
    size_t getProbeCount() const { return probes_.size(); }
    
    std::vector<LightProbe>& getProbes() { return probes_; }
    const std::vector<LightProbe>& getProbes() const { return probes_; }
    
private:
    Vec3 minBounds_ = {0, 0, 0};
    Vec3 maxBounds_ = {1, 1, 1};
    Vec3 cellSize_ = {1, 1, 1};
    int resX_ = 1;
    int resY_ = 1;
    int resZ_ = 1;
    std::vector<LightProbe> probes_;
};

}  // namespace luma
