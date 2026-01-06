#include "engine_facade.h"

namespace luma {

void EngineFacade::load_scene(Scene scene) {
    scene_ = std::move(scene);
    log_info("Scene loaded with " + std::to_string(scene_.nodes().size()) + " nodes");
}

void EngineFacade::advance_time(float dt) {
    timeline_.step(dt);
}

void EngineFacade::dispatch_action(const Action& action) {
    switch (action.type) {
        case ActionType::ApplyLook:
            handle_apply_look(action);
            break;
        case ActionType::SwitchCamera:
            handle_switch_camera(action);
            break;
        case ActionType::PlayAnimation:
            handle_play_animation(action);
            break;
        case ActionType::SetState:
            handle_set_state(action);
            break;
        case ActionType::SetParameter:
            handle_set_parameter(action);
            break;
        case ActionType::SetMaterialVariant:
            handle_set_material_variant(action);
            break;
    }
}

void EngineFacade::handle_apply_look(const Action& action) {
    look_.id = action.target;
    log_info("ApplyLook -> " + look_.id);
}

void EngineFacade::handle_switch_camera(const Action& action) {
    if (action.index && *action.index >= 0 && *action.index < static_cast<int>(scene_.nodes().size())) {
        // In a complete impl we would map index to a camera node. Here we just log.
        log_info("SwitchCamera -> index " + std::to_string(*action.index));
    }
    if (!action.target.empty()) {
        scene_.set_active_camera(action.target);
        log_info("Active camera set to " + action.target);
    }
}

void EngineFacade::handle_play_animation(const Action& action) {
    log_info("PlayAnimation on target " + action.target + " clip " + action.value);
}

void EngineFacade::handle_set_state(const Action& action) {
    log_info("SetState " + action.target + " -> " + action.value);
}

void EngineFacade::handle_set_parameter(const Action& action) {
    // Support material-scoped parameter: "matId/paramName"
    auto pos = action.target.find('/');
    if (pos != std::string::npos) {
        const std::string matId = action.target.substr(0, pos);
        const std::string paramName = action.target.substr(pos + 1);
        set_material_param(matId, paramName, action.value);
        log_info("SetParameter " + matId + "/" + paramName + " = " + action.value);
    } else {
        parameters_[action.target] = action.value;
        log_info("SetParameter " + action.target + " = " + action.value);
    }
}

void EngineFacade::handle_set_material_variant(const Action& action) {
    auto& mat = materials_[action.target];
    mat.variant = action.index.value_or(0);
    log_info("SetMaterialVariant " + action.target + " -> " + std::to_string(mat.variant));
}

void EngineFacade::set_material_param(const std::string& mat_id, const std::string& name, const std::string& value) {
    auto& mat = materials_[mat_id];
    mat.parameters[name] = value;
}

}  // namespace luma

