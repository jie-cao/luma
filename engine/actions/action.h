// Action is the only state mutation entry.
#pragma once

#include <optional>
#include <string>

#include "engine/foundation/types.h"

namespace luma {

enum class ActionType {
    ApplyLook,
    SetParameter,
    SwitchCamera,
    PlayAnimation,
    SetState,
    SetMaterialVariant
};

struct Action {
    ActionType type;
    // Common fields; actions may use a subset depending on type.
    std::string target;          // e.g., look id, node name, material id
    std::string value;           // e.g., parameter value/state name
    std::optional<int> index;    // e.g., camera index or variant index
    ActionID id{0};
};

}  // namespace luma

