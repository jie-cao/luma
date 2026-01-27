// Navigation Agent
// AI agent that navigates using NavMesh
#pragma once

#include "navmesh.h"
#include <functional>

namespace luma {

// ===== Agent State =====
enum class NavAgentState {
    Idle,
    Moving,
    Waiting,
    Stuck,
    Arrived
};

// ===== Agent Settings =====
struct NavAgentSettings {
    float speed = 5.0f;
    float acceleration = 10.0f;
    float angularSpeed = 360.0f;  // Degrees per second
    float stoppingDistance = 0.1f;
    float radius = 0.5f;
    float height = 2.0f;
    
    // Obstacle avoidance
    bool avoidObstacles = true;
    float avoidanceRadius = 1.0f;
    int avoidancePriority = 50;
    
    // Path following
    float pathUpdateInterval = 0.5f;  // Seconds between path updates
    bool autoRepath = true;
    float repathThreshold = 1.0f;  // Repath if target moves this far
};

// ===== Nav Agent =====
class NavAgent {
public:
    NavAgent() : id_(nextId_++) {}
    
    uint32_t getId() const { return id_; }
    
    // Settings
    void setSettings(const NavAgentSettings& settings) { settings_ = settings; }
    NavAgentSettings& getSettings() { return settings_; }
    const NavAgentSettings& getSettings() const { return settings_; }
    
    // Position
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    void setRotation(float yaw) { rotation_ = yaw; }
    float getRotation() const { return rotation_; }
    
    Vec3 getForward() const {
        float rad = rotation_ * 3.14159f / 180.0f;
        return Vec3(std::sin(rad), 0, std::cos(rad));
    }
    
    Vec3 getVelocity() const { return velocity_; }
    
    // Destination
    bool setDestination(const Vec3& destination);
    Vec3 getDestination() const { return destination_; }
    bool hasPath() const { return currentPath_.valid; }
    
    // State
    NavAgentState getState() const { return state_; }
    bool isMoving() const { return state_ == NavAgentState::Moving; }
    
    // Path
    const NavPath& getCurrentPath() const { return currentPath_; }
    float getRemainingDistance() const;
    
    // Control
    void stop();
    void resume();
    
    // Update (call each frame)
    void update(float dt, const NavMesh& navMesh);
    
    // Callbacks
    using PathCompleteCallback = std::function<void(NavAgent*, bool success)>;
    void setOnPathComplete(PathCompleteCallback callback) { onPathComplete_ = callback; }
    
    // Manual movement
    void move(const Vec3& direction, float dt);
    
    // Debug
    bool showDebugPath = false;
    
private:
    void updateMovement(float dt);
    void updateRotation(float dt);
    bool updatePath(const NavMesh& navMesh);
    Vec3 steerTowards(const Vec3& target, float dt);
    
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
    
    NavAgentSettings settings_;
    
    Vec3 position_ = {0, 0, 0};
    Vec3 velocity_ = {0, 0, 0};
    float rotation_ = 0.0f;
    
    Vec3 destination_ = {0, 0, 0};
    NavPath currentPath_;
    size_t currentPathIndex_ = 0;
    
    NavAgentState state_ = NavAgentState::Idle;
    float pathUpdateTimer_ = 0.0f;
    Vec3 lastDestination_ = {0, 0, 0};
    
    PathCompleteCallback onPathComplete_;
};

// ===== Nav Agent Implementation =====

inline bool NavAgent::setDestination(const Vec3& destination) {
    destination_ = destination;
    lastDestination_ = destination;
    pathUpdateTimer_ = 0.0f;
    
    // Path will be calculated on next update
    state_ = NavAgentState::Moving;
    currentPath_.clear();
    currentPathIndex_ = 0;
    
    return true;
}

inline float NavAgent::getRemainingDistance() const {
    if (!currentPath_.valid || currentPathIndex_ >= currentPath_.points.size()) {
        return 0.0f;
    }
    
    float distance = (currentPath_.points[currentPathIndex_].position - position_).length();
    
    for (size_t i = currentPathIndex_; i < currentPath_.points.size() - 1; i++) {
        distance += (currentPath_.points[i + 1].position - currentPath_.points[i].position).length();
    }
    
    return distance;
}

inline void NavAgent::stop() {
    state_ = NavAgentState::Idle;
    velocity_ = Vec3(0, 0, 0);
}

inline void NavAgent::resume() {
    if (currentPath_.valid) {
        state_ = NavAgentState::Moving;
    }
}

inline void NavAgent::update(float dt, const NavMesh& navMesh) {
    if (state_ == NavAgentState::Idle || state_ == NavAgentState::Arrived) {
        return;
    }
    
    // Update path if needed
    pathUpdateTimer_ += dt;
    if (pathUpdateTimer_ >= settings_.pathUpdateInterval || !currentPath_.valid) {
        if (updatePath(navMesh)) {
            pathUpdateTimer_ = 0.0f;
        }
    }
    
    // Check for auto-repath if destination moved
    if (settings_.autoRepath) {
        float destDist = (destination_ - lastDestination_).length();
        if (destDist > settings_.repathThreshold) {
            currentPath_.clear();
            lastDestination_ = destination_;
        }
    }
    
    if (!currentPath_.valid) {
        state_ = NavAgentState::Stuck;
        return;
    }
    
    // Update movement
    updateMovement(dt);
    updateRotation(dt);
}

inline void NavAgent::updateMovement(float dt) {
    if (currentPathIndex_ >= currentPath_.points.size()) {
        state_ = NavAgentState::Arrived;
        velocity_ = Vec3(0, 0, 0);
        
        if (onPathComplete_) {
            onPathComplete_(this, true);
        }
        return;
    }
    
    Vec3 target = currentPath_.points[currentPathIndex_].position;
    Vec3 toTarget = target - position_;
    toTarget.y = 0;  // Only horizontal movement
    float distance = toTarget.length();
    
    // Check if reached current waypoint
    if (distance < settings_.stoppingDistance) {
        currentPathIndex_++;
        
        if (currentPathIndex_ >= currentPath_.points.size()) {
            state_ = NavAgentState::Arrived;
            velocity_ = Vec3(0, 0, 0);
            
            if (onPathComplete_) {
                onPathComplete_(this, true);
            }
            return;
        }
        
        target = currentPath_.points[currentPathIndex_].position;
        toTarget = target - position_;
        toTarget.y = 0;
        distance = toTarget.length();
    }
    
    // Calculate desired velocity
    Vec3 desiredVelocity = (distance > NAV_EPSILON) ? toTarget * (settings_.speed / distance) : Vec3(0, 0, 0);
    
    // Accelerate towards desired velocity
    Vec3 velocityDiff = desiredVelocity - velocity_;
    float accel = settings_.acceleration * dt;
    
    if (velocityDiff.length() > accel) {
        velocityDiff = velocityDiff.normalized() * accel;
    }
    
    velocity_ = velocity_ + velocityDiff;
    
    // Apply velocity
    position_ = position_ + velocity_ * dt;
}

inline void NavAgent::updateRotation(float dt) {
    if (velocity_.lengthSquared() < NAV_EPSILON) return;
    
    // Calculate target rotation
    float targetRotation = std::atan2(velocity_.x, velocity_.z) * 180.0f / 3.14159f;
    
    // Smoothly rotate towards target
    float diff = targetRotation - rotation_;
    
    // Normalize angle difference
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    
    float maxRotation = settings_.angularSpeed * dt;
    if (std::abs(diff) > maxRotation) {
        diff = (diff > 0) ? maxRotation : -maxRotation;
    }
    
    rotation_ += diff;
    
    // Normalize rotation
    while (rotation_ > 360.0f) rotation_ -= 360.0f;
    while (rotation_ < 0.0f) rotation_ += 360.0f;
}

inline bool NavAgent::updatePath(const NavMesh& navMesh) {
    NavPathfinder pathfinder(&navMesh);
    
    if (!pathfinder.findPath(position_, destination_, currentPath_)) {
        return false;
    }
    
    currentPathIndex_ = 0;
    state_ = NavAgentState::Moving;
    return true;
}

inline Vec3 NavAgent::steerTowards(const Vec3& target, float dt) {
    Vec3 desired = target - position_;
    float distance = desired.length();
    
    if (distance < NAV_EPSILON) return Vec3(0, 0, 0);
    
    desired = desired * (settings_.speed / distance);
    Vec3 steering = desired - velocity_;
    
    float maxSteer = settings_.acceleration * dt;
    if (steering.length() > maxSteer) {
        steering = steering.normalized() * maxSteer;
    }
    
    return steering;
}

inline void NavAgent::move(const Vec3& direction, float dt) {
    Vec3 dir = direction;
    if (dir.lengthSquared() > 1.0f) {
        dir = dir.normalized();
    }
    
    velocity_ = dir * settings_.speed;
    position_ = position_ + velocity_ * dt;
    
    if (dir.lengthSquared() > NAV_EPSILON) {
        rotation_ = std::atan2(dir.x, dir.z) * 180.0f / 3.14159f;
    }
}

// ===== Nav Agent Manager =====
class NavAgentManager {
public:
    NavAgent* createAgent() {
        agents_.push_back(std::make_unique<NavAgent>());
        return agents_.back().get();
    }
    
    void destroyAgent(NavAgent* agent) {
        agents_.erase(
            std::remove_if(agents_.begin(), agents_.end(),
                [agent](const auto& a) { return a.get() == agent; }),
            agents_.end()
        );
    }
    
    void update(float dt, const NavMesh& navMesh) {
        for (auto& agent : agents_) {
            agent->update(dt, navMesh);
        }
    }
    
    const std::vector<std::unique_ptr<NavAgent>>& getAgents() const { return agents_; }
    size_t getAgentCount() const { return agents_.size(); }
    
    NavAgent* getAgentById(uint32_t id) {
        for (auto& agent : agents_) {
            if (agent->getId() == id) return agent.get();
        }
        return nullptr;
    }
    
    void clear() { agents_.clear(); }
    
private:
    std::vector<std::unique_ptr<NavAgent>> agents_;
};

// ===== Global Manager =====
inline NavAgentManager& getNavAgentManager() {
    static NavAgentManager manager;
    return manager;
}

}  // namespace luma
