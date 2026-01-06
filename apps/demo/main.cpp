#include <iostream>
#include <string>
#include <vector>

#include "engine/actions/action.h"
#include "engine/engine_facade.h"
#include "engine/scene/scene.h"

using namespace luma;

int main() {
    // 构建一个最小场景：一个模型节点与一个相机节点，资产以 AssetID 表示。
    Scene scene;
    scene.add_node(Node{
        .name = "MeshNode",
        .renderable = "asset_mesh_hero",
        .camera = std::nullopt,
        .transform = {}
    });
    scene.add_node(Node{
        .name = "CameraNode",
        .renderable = {},
        .camera = AssetID{"asset_camera_main"},
        .transform = {}
    });

    EngineFacade engine;
    engine.load_scene(scene);

    // 通过 Action 驱动状态：切换 Look、切换相机、播放动画、切换状态。
    std::vector<Action> script = {
        {ActionType::ApplyLook, "look_cinematic", {}, std::nullopt, 1},
        {ActionType::SwitchCamera, "asset_camera_main", {}, std::optional<int>{1}, 2},
        {ActionType::PlayAnimation, "MeshNode", "clip_idle", std::nullopt, 3},
        {ActionType::SetState, "HeroState", "Active", std::nullopt, 4},
    };

    for (const auto& action : script) {
        engine.dispatch_action(action);
    }

    std::cout << "Active camera: "
              << (engine.scene().active_camera().has_value() ? *engine.scene().active_camera() : "<none>")
              << "\n";
    std::cout << "Current look: " << engine.look().id << "\n";
    std::cout << "Timeline time: " << engine.timeline().time() << "\n";
    return 0;
}

