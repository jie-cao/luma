// Scene graph skeleton. Scene only references assets by AssetID.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "engine/foundation/types.h"
#include "engine/foundation/math_types.h"

namespace luma {

// Use Vec3 from math_types.h

// Legacy transform struct (separate from entity.h Transform)
struct SceneTransform {
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1.f, 1.f, 1.f};
};

struct Node {
    std::string name;
    AssetID renderable;  // mesh/material bundle reference
    std::optional<AssetID> camera;  // camera asset, if this node is a camera
    SceneTransform transform;
};

class Scene {
public:
    Node& add_node(const Node& node);
    const std::vector<Node>& nodes() const { return nodes_; }

    void set_active_camera(const AssetID& camera_id);
    const std::optional<AssetID>& active_camera() const { return active_camera_; }

private:
    std::vector<Node> nodes_;
    std::optional<AssetID> active_camera_;
};

}  // namespace luma

