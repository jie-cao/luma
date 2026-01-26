// Inverse Kinematics System
// Two-Bone IK, Look-At IK, Foot IK, FABRIK
#pragma once

#include "skeleton.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace luma {

// ===== IK Utility Functions =====
namespace IKUtils {

// Clamp value between min and max
inline float clamp(float value, float minVal, float maxVal) {
    return std::max(minVal, std::min(maxVal, value));
}

// Safe acos that handles numerical errors
inline float safeAcos(float value) {
    return std::acos(clamp(value, -1.0f, 1.0f));
}

// Get rotation from one vector to another
inline Quat rotationBetweenVectors(const Vec3& from, const Vec3& to) {
    Vec3 f = from.normalized();
    Vec3 t = to.normalized();
    
    float dot = f.dot(t);
    
    if (dot > 0.9999f) {
        return Quat();  // Identity
    }
    
    if (dot < -0.9999f) {
        // 180 degree rotation - find perpendicular axis
        Vec3 axis = Vec3(1, 0, 0).cross(f);
        if (axis.length() < 0.0001f) {
            axis = Vec3(0, 1, 0).cross(f);
        }
        axis = axis.normalized();
        return Quat::fromAxisAngle(axis, 3.14159265f);
    }
    
    Vec3 axis = f.cross(t).normalized();
    float angle = safeAcos(dot);
    return Quat::fromAxisAngle(axis, angle);
}

// Extract position from Mat4
inline Vec3 getPosition(const Mat4& m) {
    return Vec3(m.m[12], m.m[13], m.m[14]);
}

// Transform point by matrix
inline Vec3 transformPoint(const Mat4& m, const Vec3& p) {
    return Vec3(
        m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
        m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
        m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]
    );
}

// Transform direction by matrix (ignores translation)
inline Vec3 transformDirection(const Mat4& m, const Vec3& d) {
    return Vec3(
        m.m[0] * d.x + m.m[4] * d.y + m.m[8] * d.z,
        m.m[1] * d.x + m.m[5] * d.y + m.m[9] * d.z,
        m.m[2] * d.x + m.m[6] * d.y + m.m[10] * d.z
    ).normalized();
}

}  // namespace IKUtils

// ===== Two-Bone IK =====
// Solves IK for a two-bone chain (e.g., arm: shoulder->elbow->hand)
class TwoBoneIK {
public:
    // Bone indices
    int rootBoneIndex = -1;    // e.g., shoulder/thigh
    int midBoneIndex = -1;     // e.g., elbow/knee
    int endBoneIndex = -1;     // e.g., hand/foot
    
    // Target
    Vec3 targetPosition;
    float weight = 1.0f;
    
    // Pole target (controls bend direction)
    Vec3 poleTarget;
    bool usePoleTarget = false;
    
    // Constraints
    float minBendAngle = 0.01f;    // Prevent hyperextension
    float maxBendAngle = 3.14f;    // ~180 degrees
    
    // Solve the IK chain
    void solve(Skeleton& skeleton) const {
        if (weight <= 0.0f) return;
        if (rootBoneIndex < 0 || midBoneIndex < 0 || endBoneIndex < 0) return;
        
        // Get bone world positions (model-space matrices)
        Mat4 worldMatrices[MAX_BONES];
        skeleton.computeBoneMatrices(worldMatrices);
        
        Vec3 rootPos = IKUtils::getPosition(worldMatrices[rootBoneIndex]);
        Vec3 midPos = IKUtils::getPosition(worldMatrices[midBoneIndex]);
        Vec3 endPos = IKUtils::getPosition(worldMatrices[endBoneIndex]);
        
        // Calculate bone lengths
        float upperLength = (midPos - rootPos).length();
        float lowerLength = (endPos - midPos).length();
        float totalLength = upperLength + lowerLength;
        
        // Target direction and distance
        Vec3 targetDir = targetPosition - rootPos;
        float targetDist = targetDir.length();
        
        if (targetDist < 0.0001f) return;
        targetDir = targetDir.normalized();
        
        // Clamp target distance to reachable range
        targetDist = IKUtils::clamp(targetDist, std::abs(upperLength - lowerLength) + 0.001f, 
                                     totalLength - 0.001f);
        
        // Calculate bend angle using law of cosines
        // cos(angle) = (a^2 + b^2 - c^2) / (2ab)
        float cosAngle = (upperLength * upperLength + lowerLength * lowerLength - 
                         targetDist * targetDist) / (2.0f * upperLength * lowerLength);
        float bendAngle = IKUtils::safeAcos(cosAngle);
        bendAngle = IKUtils::clamp(bendAngle, minBendAngle, maxBendAngle);
        
        // Calculate upper bone angle
        float upperAngle = IKUtils::safeAcos(
            (upperLength * upperLength + targetDist * targetDist - 
             lowerLength * lowerLength) / (2.0f * upperLength * targetDist)
        );
        
        // Determine bend plane normal
        Vec3 bendPlaneNormal;
        if (usePoleTarget) {
            // Use pole target to determine bend direction
            Vec3 poleDir = (poleTarget - rootPos).normalized();
            bendPlaneNormal = targetDir.cross(poleDir).normalized();
            if (bendPlaneNormal.length() < 0.001f) {
                bendPlaneNormal = Vec3(0, 0, 1);  // Fallback
            }
        } else {
            // Use current bend direction
            Vec3 currentMidDir = (midPos - rootPos).normalized();
            bendPlaneNormal = targetDir.cross(currentMidDir).normalized();
            if (bendPlaneNormal.length() < 0.001f) {
                bendPlaneNormal = Vec3(0, 0, 1);  // Fallback
            }
        }
        
        // Calculate new mid position
        Quat upperRotation = Quat::fromAxisAngle(bendPlaneNormal, upperAngle);
        Vec3 upperDir = upperRotation.rotate(targetDir);
        Vec3 newMidPos = rootPos + upperDir * upperLength;
        
        // Calculate rotations for root bone
        Bone* rootBone = const_cast<Bone*>(skeleton.getBone(rootBoneIndex));
        if (rootBone) {
            Vec3 currentUpperDir = (midPos - rootPos).normalized();
            Vec3 desiredUpperDir = (newMidPos - rootPos).normalized();
            Quat deltaRot = IKUtils::rotationBetweenVectors(currentUpperDir, desiredUpperDir);
            
            // Apply weighted rotation
            Quat finalRot = anim::slerp(Quat(), deltaRot, weight);
            rootBone->localRotation = finalRot * rootBone->localRotation;
        }
        
        // Recalculate matrices after root change
        skeleton.computeBoneMatrices(worldMatrices);
        newMidPos = IKUtils::getPosition(worldMatrices[midBoneIndex]);
        
        // Calculate rotation for mid bone
        Bone* midBone = const_cast<Bone*>(skeleton.getBone(midBoneIndex));
        if (midBone) {
            Vec3 currentLowerDir = (endPos - newMidPos).normalized();
            Vec3 desiredLowerDir = (targetPosition - newMidPos).normalized();
            Quat deltaRot = IKUtils::rotationBetweenVectors(currentLowerDir, desiredLowerDir);
            
            Quat finalRot = anim::slerp(Quat(), deltaRot, weight);
            midBone->localRotation = finalRot * midBone->localRotation;
        }
    }
};

// ===== Look-At IK =====
// Rotates a bone to look at a target (e.g., head tracking)
class LookAtIK {
public:
    int boneIndex = -1;
    Vec3 targetPosition;
    float weight = 1.0f;
    
    // Local axis settings
    Vec3 forwardAxis = Vec3(0, 0, 1);  // Which local axis points forward
    Vec3 upAxis = Vec3(0, 1, 0);       // Which local axis points up
    
    // World up reference
    Vec3 worldUp = Vec3(0, 1, 0);
    
    // Constraints (in radians)
    float maxHorizontalAngle = 1.57f;  // ~90 degrees
    float maxVerticalAngle = 0.78f;    // ~45 degrees
    
    void solve(Skeleton& skeleton) const {
        if (weight <= 0.0f || boneIndex < 0) return;
        
        // Get bone world transform
        Mat4 worldMatrices[MAX_BONES];
        skeleton.computeBoneMatrices(worldMatrices);
        
        Vec3 bonePos = IKUtils::getPosition(worldMatrices[boneIndex]);
        Vec3 toTarget = (targetPosition - bonePos).normalized();
        
        // Get current forward direction
        Vec3 currentForward = IKUtils::transformDirection(worldMatrices[boneIndex], forwardAxis);
        
        // Calculate horizontal angle (yaw)
        Vec3 toTargetHorizontal = Vec3(toTarget.x, 0, toTarget.z).normalized();
        Vec3 currentHorizontal = Vec3(currentForward.x, 0, currentForward.z).normalized();
        
        float horizontalAngle = std::acos(IKUtils::clamp(
            currentHorizontal.dot(toTargetHorizontal), -1.0f, 1.0f));
        
        // Calculate vertical angle (pitch)
        float verticalAngle = std::asin(IKUtils::clamp(toTarget.y, -1.0f, 1.0f)) -
                             std::asin(IKUtils::clamp(currentForward.y, -1.0f, 1.0f));
        
        // Apply constraints
        horizontalAngle = IKUtils::clamp(horizontalAngle, -maxHorizontalAngle, maxHorizontalAngle);
        verticalAngle = IKUtils::clamp(verticalAngle, -maxVerticalAngle, maxVerticalAngle);
        
        // Calculate rotation
        Quat deltaRot = IKUtils::rotationBetweenVectors(currentForward, toTarget);
        
        // Apply weighted rotation
        Bone* bone = const_cast<Bone*>(skeleton.getBone(boneIndex));
        if (bone) {
            Quat finalRot = anim::slerp(Quat(), deltaRot, weight);
            bone->localRotation = finalRot * bone->localRotation;
        }
    }
};

// ===== Foot IK =====
// Adapts feet to terrain height
class FootIK {
public:
    // Leg setup
    int hipBoneIndex = -1;
    int kneeBoneIndex = -1;
    int footBoneIndex = -1;
    
    // Foot placement
    Vec3 groundPosition;      // Target foot position on ground
    Vec3 groundNormal;        // Ground surface normal
    float weight = 1.0f;
    
    // Pelvis adjustment
    int pelvisBoneIndex = -1;
    float pelvisOffset = 0.0f;  // How much to lower pelvis
    
    // Foot rotation
    bool alignToGround = true;
    Vec3 footForward = Vec3(0, 0, 1);
    
    void solve(Skeleton& skeleton) const {
        if (weight <= 0.0f) return;
        
        // Apply pelvis offset first
        if (pelvisBoneIndex >= 0 && std::abs(pelvisOffset) > 0.001f) {
            Bone* pelvis = const_cast<Bone*>(skeleton.getBone(pelvisBoneIndex));
            if (pelvis) {
                pelvis->localPosition.y -= pelvisOffset * weight;
            }
        }
        
        // Use Two-Bone IK for leg
        if (hipBoneIndex >= 0 && kneeBoneIndex >= 0 && footBoneIndex >= 0) {
            TwoBoneIK legIK;
            legIK.rootBoneIndex = hipBoneIndex;
            legIK.midBoneIndex = kneeBoneIndex;
            legIK.endBoneIndex = footBoneIndex;
            legIK.targetPosition = groundPosition;
            legIK.weight = weight;
            
            // Pole target in front of knee
            Mat4 worldMatrices[MAX_BONES];
            skeleton.computeBoneMatrices(worldMatrices);
            Vec3 kneePos = IKUtils::getPosition(worldMatrices[kneeBoneIndex]);
            legIK.poleTarget = kneePos + Vec3(0, 0, 1);  // In front
            legIK.usePoleTarget = true;
            
            legIK.solve(skeleton);
        }
        
        // Align foot to ground normal
        if (alignToGround && footBoneIndex >= 0) {
            Bone* foot = const_cast<Bone*>(skeleton.getBone(footBoneIndex));
            if (foot && groundNormal.length() > 0.001f) {
                Vec3 normal = groundNormal.normalized();
                Vec3 up = Vec3(0, 1, 0);
                
                // Calculate rotation to align foot with ground
                Quat alignRot = IKUtils::rotationBetweenVectors(up, normal);
                Quat finalRot = anim::slerp(Quat(), alignRot, weight);
                foot->localRotation = finalRot * foot->localRotation;
            }
        }
    }
};

// ===== FABRIK (Forward And Backward Reaching Inverse Kinematics) =====
// Iterative solver for arbitrary bone chains
class FABRIK {
public:
    std::vector<int> chainBoneIndices;  // From root to end
    Vec3 targetPosition;
    float weight = 1.0f;
    
    int maxIterations = 10;
    float tolerance = 0.001f;  // Stop when end effector is within tolerance
    
    // Optional: constrain root position
    bool constrainRoot = true;
    
    void solve(Skeleton& skeleton) const {
        if (weight <= 0.0f || chainBoneIndices.size() < 2) return;
        
        // Get world positions
        Mat4 worldMatrices[MAX_BONES];
        skeleton.computeBoneMatrices(worldMatrices);
        
        std::vector<Vec3> positions(chainBoneIndices.size());
        std::vector<float> lengths(chainBoneIndices.size() - 1);
        
        for (size_t i = 0; i < chainBoneIndices.size(); i++) {
            positions[i] = IKUtils::getPosition(worldMatrices[chainBoneIndices[i]]);
            if (i > 0) {
                lengths[i - 1] = (positions[i] - positions[i - 1]).length();
            }
        }
        
        Vec3 rootPos = positions[0];
        float totalLength = 0.0f;
        for (float len : lengths) totalLength += len;
        
        // Check if target is reachable
        float targetDist = (targetPosition - rootPos).length();
        if (targetDist > totalLength) {
            // Target is too far - stretch towards it
            Vec3 dir = (targetPosition - rootPos).normalized();
            for (size_t i = 1; i < positions.size(); i++) {
                positions[i] = positions[i - 1] + dir * lengths[i - 1];
            }
        } else {
            // FABRIK iterations
            for (int iter = 0; iter < maxIterations; iter++) {
                // Check convergence
                float error = (positions.back() - targetPosition).length();
                if (error < tolerance) break;
                
                // Forward reaching (from end to root)
                positions.back() = targetPosition;
                for (int i = static_cast<int>(positions.size()) - 2; i >= 0; i--) {
                    Vec3 dir = (positions[i] - positions[i + 1]).normalized();
                    positions[i] = positions[i + 1] + dir * lengths[i];
                }
                
                // Backward reaching (from root to end)
                if (constrainRoot) {
                    positions[0] = rootPos;
                }
                for (size_t i = 1; i < positions.size(); i++) {
                    Vec3 dir = (positions[i] - positions[i - 1]).normalized();
                    positions[i] = positions[i - 1] + dir * lengths[i - 1];
                }
            }
        }
        
        // Apply rotations to bones
        skeleton.computeBoneMatrices(worldMatrices);
        
        for (size_t i = 0; i < chainBoneIndices.size() - 1; i++) {
            int boneIdx = chainBoneIndices[i];
            int nextBoneIdx = chainBoneIndices[i + 1];
            
            Bone* bone = const_cast<Bone*>(skeleton.getBone(boneIdx));
            if (!bone) continue;
            
            // Current direction to next bone
            Vec3 currentNextPos = IKUtils::getPosition(worldMatrices[nextBoneIdx]);
            Vec3 currentPos = IKUtils::getPosition(worldMatrices[boneIdx]);
            Vec3 currentDir = (currentNextPos - currentPos).normalized();
            
            // Desired direction
            Vec3 desiredDir = (positions[i + 1] - positions[i]).normalized();
            
            // Calculate rotation
            Quat deltaRot = IKUtils::rotationBetweenVectors(currentDir, desiredDir);
            Quat finalRot = anim::slerp(Quat(), deltaRot, weight);
            bone->localRotation = finalRot * bone->localRotation;
            
            // Update world matrices for next iteration
            skeleton.computeBoneMatrices(worldMatrices);
        }
    }
};

// ===== IK Manager =====
// Manages multiple IK solvers
class IKManager {
public:
    // Two-Bone IK chains
    std::vector<TwoBoneIK> twoBoneIKs;
    
    // Look-At targets
    std::vector<LookAtIK> lookAtIKs;
    
    // Foot IK
    std::vector<FootIK> footIKs;
    
    // FABRIK chains
    std::vector<FABRIK> fabrikChains;
    
    // Global weight
    float globalWeight = 1.0f;
    bool enabled = true;
    
    // Solve all IK
    void solve(Skeleton& skeleton) {
        if (!enabled || globalWeight <= 0.0f) return;
        
        // Apply global weight
        float origWeight;
        
        // Foot IK first (affects pelvis)
        for (auto& ik : footIKs) {
            origWeight = ik.weight;
            ik.weight *= globalWeight;
            ik.solve(skeleton);
            ik.weight = origWeight;
        }
        
        // Two-Bone IK
        for (auto& ik : twoBoneIKs) {
            origWeight = ik.weight;
            ik.weight *= globalWeight;
            ik.solve(skeleton);
            ik.weight = origWeight;
        }
        
        // FABRIK
        for (auto& ik : fabrikChains) {
            origWeight = ik.weight;
            ik.weight *= globalWeight;
            ik.solve(skeleton);
            ik.weight = origWeight;
        }
        
        // Look-At IK last (after body is positioned)
        for (auto& ik : lookAtIKs) {
            origWeight = ik.weight;
            ik.weight *= globalWeight;
            ik.solve(skeleton);
            ik.weight = origWeight;
        }
    }
    
    // === Convenience setup methods ===
    
    // Setup arm IK (returns index)
    int setupArmIK(int shoulderIdx, int elbowIdx, int handIdx) {
        TwoBoneIK ik;
        ik.rootBoneIndex = shoulderIdx;
        ik.midBoneIndex = elbowIdx;
        ik.endBoneIndex = handIdx;
        twoBoneIKs.push_back(ik);
        return static_cast<int>(twoBoneIKs.size()) - 1;
    }
    
    // Setup leg IK (returns index)
    int setupLegIK(int hipIdx, int kneeIdx, int footIdx, int pelvisIdx = -1) {
        FootIK ik;
        ik.hipBoneIndex = hipIdx;
        ik.kneeBoneIndex = kneeIdx;
        ik.footBoneIndex = footIdx;
        ik.pelvisBoneIndex = pelvisIdx;
        footIKs.push_back(ik);
        return static_cast<int>(footIKs.size()) - 1;
    }
    
    // Setup head look-at (returns index)
    int setupHeadLookAt(int headIdx) {
        LookAtIK ik;
        ik.boneIndex = headIdx;
        lookAtIKs.push_back(ik);
        return static_cast<int>(lookAtIKs.size()) - 1;
    }
    
    // Setup spine chain (returns index)
    int setupSpineChain(const std::vector<int>& spineIndices) {
        FABRIK ik;
        ik.chainBoneIndices = spineIndices;
        fabrikChains.push_back(ik);
        return static_cast<int>(fabrikChains.size()) - 1;
    }
    
    // === Target setters ===
    
    void setHandTarget(int armIndex, const Vec3& target, float weight = 1.0f) {
        if (armIndex >= 0 && armIndex < static_cast<int>(twoBoneIKs.size())) {
            twoBoneIKs[armIndex].targetPosition = target;
            twoBoneIKs[armIndex].weight = weight;
        }
    }
    
    void setFootTarget(int legIndex, const Vec3& groundPos, const Vec3& groundNormal, float weight = 1.0f) {
        if (legIndex >= 0 && legIndex < static_cast<int>(footIKs.size())) {
            footIKs[legIndex].groundPosition = groundPos;
            footIKs[legIndex].groundNormal = groundNormal;
            footIKs[legIndex].weight = weight;
        }
    }
    
    void setLookAtTarget(int lookAtIndex, const Vec3& target, float weight = 1.0f) {
        if (lookAtIndex >= 0 && lookAtIndex < static_cast<int>(lookAtIKs.size())) {
            lookAtIKs[lookAtIndex].targetPosition = target;
            lookAtIKs[lookAtIndex].weight = weight;
        }
    }
};

// ===== IK Rig Helper =====
// Automatically sets up IK for common humanoid skeletons
namespace IKRigHelper {

// Common bone name patterns
inline int findBoneByPattern(const Skeleton& skeleton, const std::vector<std::string>& patterns) {
    for (const auto& pattern : patterns) {
        int idx = skeleton.findBoneByName(pattern);
        if (idx >= 0) return idx;
    }
    return -1;
}

// Setup humanoid IK rig
inline void setupHumanoidRig(IKManager& manager, const Skeleton& skeleton) {
    // Find common bones
    
    // Pelvis/Hips
    int pelvis = findBoneByPattern(skeleton, {"pelvis", "Pelvis", "Hips", "hips", "hip"});
    
    // Left leg
    int leftHip = findBoneByPattern(skeleton, {"thigh_l", "LeftUpLeg", "left_thigh", "L_Thigh"});
    int leftKnee = findBoneByPattern(skeleton, {"calf_l", "LeftLeg", "left_shin", "L_Calf"});
    int leftFoot = findBoneByPattern(skeleton, {"foot_l", "LeftFoot", "left_foot", "L_Foot"});
    
    if (leftHip >= 0 && leftKnee >= 0 && leftFoot >= 0) {
        manager.setupLegIK(leftHip, leftKnee, leftFoot, pelvis);
    }
    
    // Right leg
    int rightHip = findBoneByPattern(skeleton, {"thigh_r", "RightUpLeg", "right_thigh", "R_Thigh"});
    int rightKnee = findBoneByPattern(skeleton, {"calf_r", "RightLeg", "right_shin", "R_Calf"});
    int rightFoot = findBoneByPattern(skeleton, {"foot_r", "RightFoot", "right_foot", "R_Foot"});
    
    if (rightHip >= 0 && rightKnee >= 0 && rightFoot >= 0) {
        manager.setupLegIK(rightHip, rightKnee, rightFoot, pelvis);
    }
    
    // Left arm
    int leftShoulder = findBoneByPattern(skeleton, {"upperarm_l", "LeftArm", "left_upper_arm", "L_UpperArm"});
    int leftElbow = findBoneByPattern(skeleton, {"lowerarm_l", "LeftForeArm", "left_forearm", "L_Forearm"});
    int leftHand = findBoneByPattern(skeleton, {"hand_l", "LeftHand", "left_hand", "L_Hand"});
    
    if (leftShoulder >= 0 && leftElbow >= 0 && leftHand >= 0) {
        manager.setupArmIK(leftShoulder, leftElbow, leftHand);
    }
    
    // Right arm
    int rightShoulder = findBoneByPattern(skeleton, {"upperarm_r", "RightArm", "right_upper_arm", "R_UpperArm"});
    int rightElbow = findBoneByPattern(skeleton, {"lowerarm_r", "RightForeArm", "right_forearm", "R_Forearm"});
    int rightHand = findBoneByPattern(skeleton, {"hand_r", "RightHand", "right_hand", "R_Hand"});
    
    if (rightShoulder >= 0 && rightElbow >= 0 && rightHand >= 0) {
        manager.setupArmIK(rightShoulder, rightElbow, rightHand);
    }
    
    // Head look-at
    int head = findBoneByPattern(skeleton, {"head", "Head", "HEAD"});
    if (head >= 0) {
        manager.setupHeadLookAt(head);
    }
}

}  // namespace IKRigHelper

}  // namespace luma
