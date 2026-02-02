// Skeleton - Bone hierarchy for skeletal animation
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace luma {

// ===== Bone =====
struct Bone {
    std::string name;
    int32_t parentIndex = -1;  // -1 for root bones
    
    // Bind pose (inverse bind matrix transforms from model space to bone space)
    Mat4 inverseBindMatrix;
    
    // Local transform relative to parent (rest pose)
    Vec3 localPosition{0, 0, 0};
    Quat localRotation{};
    Vec3 localScale{1, 1, 1};
    
    // Get local transform matrix
    Mat4 getLocalMatrix() const {
        return Mat4::translation(localPosition) * Mat4::fromQuat(localRotation) * Mat4::scale(localScale);
    }
};

// ===== Skeleton =====
class Skeleton {
public:
    Skeleton() = default;
    
    // === Construction ===
    
    // Add a bone, returns bone index
    int addBone(const std::string& name, int parentIndex = -1);
    
    // Set bone's inverse bind matrix
    void setInverseBindMatrix(int boneIndex, const Mat4& matrix);
    
    // Set bone's local transform (rest pose)
    void setBoneLocalTransform(int boneIndex, const Vec3& pos, const Quat& rot, const Vec3& scale);
    
    // === Queries ===
    
    int getBoneCount() const { return (int)bones_.size(); }
    
    const Bone* getBone(int index) const {
        return (index >= 0 && index < (int)bones_.size()) ? &bones_[index] : nullptr;
    }
    
    Bone* getBone(int index) {
        return (index >= 0 && index < (int)bones_.size()) ? &bones_[index] : nullptr;
    }
    
    int findBoneByName(const std::string& name) const {
        auto it = boneNameToIndex_.find(name);
        return (it != boneNameToIndex_.end()) ? it->second : -1;
    }
    
    std::string getBoneName(int index) const {
        if (index >= 0 && index < (int)bones_.size()) {
            return bones_[index].name;
        }
        return "";
    }
    
    const std::vector<Bone>& getBones() const { return bones_; }
    
    // === Pose Computation ===
    
    // Compute bone matrices from current local transforms
    // Output: array of model-space bone matrices (for rendering)
    void computeBoneMatrices(Mat4* outMatrices) const;
    
    // Compute final skinning matrices (bone matrix * inverse bind matrix)
    void computeSkinningMatrices(Mat4* outMatrices) const;
    
    // Reset to bind pose
    void resetToBindPose();
    
private:
    std::vector<Bone> bones_;
    std::unordered_map<std::string, int> boneNameToIndex_;
    
    // Cached model-space matrices (computed from local transforms)
    mutable std::vector<Mat4> modelSpaceMatrices_;
    mutable bool matricesDirty_ = true;
    
    void computeModelSpaceMatrices() const;
};

// ===== Implementation =====

inline int Skeleton::addBone(const std::string& name, int parentIndex) {
    int index = (int)bones_.size();
    if (index >= MAX_BONES) {
        return -1;  // Too many bones
    }
    
    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    
    bones_.push_back(bone);
    boneNameToIndex_[name] = index;
    matricesDirty_ = true;
    
    return index;
}

inline void Skeleton::setInverseBindMatrix(int boneIndex, const Mat4& matrix) {
    if (boneIndex >= 0 && boneIndex < (int)bones_.size()) {
        bones_[boneIndex].inverseBindMatrix = matrix;
    }
}

inline void Skeleton::setBoneLocalTransform(int boneIndex, const Vec3& pos, const Quat& rot, const Vec3& scale) {
    if (boneIndex >= 0 && boneIndex < (int)bones_.size()) {
        bones_[boneIndex].localPosition = pos;
        bones_[boneIndex].localRotation = rot;
        bones_[boneIndex].localScale = scale;
        matricesDirty_ = true;
    }
}

inline void Skeleton::computeModelSpaceMatrices() const {
    if (!matricesDirty_) return;
    
    modelSpaceMatrices_.resize(bones_.size());
    
    for (size_t i = 0; i < bones_.size(); i++) {
        const Bone& bone = bones_[i];
        Mat4 localMatrix = bone.getLocalMatrix();
        
        if (bone.parentIndex >= 0 && bone.parentIndex < (int)i) {
            modelSpaceMatrices_[i] = modelSpaceMatrices_[bone.parentIndex] * localMatrix;
        } else {
            modelSpaceMatrices_[i] = localMatrix;
        }
    }
    
    matricesDirty_ = false;
}

inline void Skeleton::computeBoneMatrices(Mat4* outMatrices) const {
    computeModelSpaceMatrices();
    for (size_t i = 0; i < bones_.size(); i++) {
        outMatrices[i] = modelSpaceMatrices_[i];
    }
}

inline void Skeleton::computeSkinningMatrices(Mat4* outMatrices) const {
    computeModelSpaceMatrices();
    for (size_t i = 0; i < bones_.size(); i++) {
        outMatrices[i] = modelSpaceMatrices_[i] * bones_[i].inverseBindMatrix;
    }
}

inline void Skeleton::resetToBindPose() {
    // Reset all bones to identity local transform
    // (actual bind pose is encoded in inverseBindMatrix)
    for (auto& bone : bones_) {
        bone.localPosition = Vec3(0, 0, 0);
        bone.localRotation = Quat();
        bone.localScale = Vec3(1, 1, 1);
    }
    matricesDirty_ = true;
}

}  // namespace luma
