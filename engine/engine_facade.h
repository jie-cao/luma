// Facade exposed to Creator/Runtime. All state changes go through Action.
#pragma once

#include <string>

#include "engine/actions/action.h"
#include "engine/foundation/log.h"
#include "engine/graph/timeline.h"
#include "engine/material/material.h"
#include "engine/scene/scene.h"

namespace luma {

struct LookState {
    std::string id;
};

class EngineFacade {
public:
    EngineFacade() = default;

    void load_scene(Scene scene);
    void dispatch_action(const Action& action);
    void advance_time(float dt);
    void set_time(float t) { timeline_.set_time(t); }

    const Scene& scene() const { return scene_; }
    const LookState& look() const { return look_; }
    const TimelineLite& timeline() const { return timeline_; }
    const std::unordered_map<std::string, MaterialData>& materials() const { return materials_; }
    const MaterialData* find_material(const std::string& id) const {
        auto it = materials_.find(id);
        return it == materials_.end() ? nullptr : &it->second;
    }
    const std::unordered_map<std::string, std::string>* find_material_params(const std::string& id) const {
        auto it = materials_.find(id);
        return it == materials_.end() ? nullptr : &it->second.parameters;
    }
    std::unordered_map<std::string, std::string> material_params_copy(const std::string& id) const {
        auto it = materials_.find(id);
        if (it == materials_.end()) return {};
        return it->second.parameters;
    }

private:
    Scene scene_;
    LookState look_;
    TimelineLite timeline_;
    std::unordered_map<std::string, MaterialData> materials_;
    std::unordered_map<std::string, std::string> parameters_;

    void handle_apply_look(const Action& action);
    void handle_switch_camera(const Action& action);
    void handle_play_animation(const Action& action);
    void handle_set_state(const Action& action);
    void handle_set_parameter(const Action& action);
    void handle_set_material_variant(const Action& action);

    void set_material_param(const std::string& mat_id, const std::string& name, const std::string& value);
};

}  // namespace luma

