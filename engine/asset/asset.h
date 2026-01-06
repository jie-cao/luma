// Asset metadata skeleton; deterministic build is enforced upstream.
#pragma once

#include <string>
#include <vector>

#include "engine/foundation/types.h"

namespace luma {

enum class AssetType {
    Mesh,
    Texture,
    Material,
    AnimationClip,
    Look,
    Scene
};

struct AssetMeta {
    AssetID id;
    AssetType type;
    std::vector<AssetID> deps;
};

}  // namespace luma

