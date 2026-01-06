#include "scene.h"

namespace luma {

Node& Scene::add_node(const Node& node) {
    nodes_.push_back(node);
    return nodes_.back();
}

void Scene::set_active_camera(const AssetID& camera_id) {
    active_camera_ = camera_id;
}

}  // namespace luma

